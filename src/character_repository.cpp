#include "character_repository.h"
#include "file_path.h"
#include "character_image.h"
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <fstream>
#include <set>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
// Keep Windows' DrawText/CloseWindow declarations away from raylib.
#define DrawText Win32DrawText
#define CloseWindow Win32CloseWindow
#define ShowCursor Win32ShowCursor
#define Rectangle Win32Rectangle
#include <windows.h>
#undef DrawText
#undef CloseWindow
#undef ShowCursor
#undef Rectangle
#undef LoadImage
#endif

namespace fs = std::filesystem;
namespace {
std::string Read(const fs::path& file) {
    if (fs::file_size(file) > 1024 * 1024) throw std::runtime_error("Character JSON exceeds 1 MiB: " + file.string());
    std::ifstream input(file, std::ios::binary);
    if (!input) throw std::runtime_error("Cannot read " + file.string());
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
std::string Token() {
    static std::atomic<unsigned long> counter{0};
    return std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + "-" + std::to_string(counter++);
}
bool Remote(const std::string& ref) { return ref.starts_with("https://"); }
template<class F> void Assets(Character& c, F transform) {
    c.assets.portrait = transform(c.assets.portrait);
    c.assets.sprite = transform(c.assets.sprite);
    for (auto& [slot, part] : c.assets.parts) part.image = transform(part.image);
    for (auto& skill : c.skills) skill.icon = transform(skill.icon);
}
}

LocalCharacterRepository::LocalCharacterRepository(fs::path root) : root_(fs::absolute(std::move(root))) {}

CharacterList LocalCharacterRepository::LoadAll() {
    CharacterList result;
    if (!fs::exists(root_)) return result;
    std::vector<fs::path> paths;
    for (const auto& entry : fs::directory_iterator(root_))
        if (entry.is_regular_file() && entry.path().extension() == ".json") paths.push_back(entry.path());
    std::sort(paths.begin(), paths.end());
    std::set<std::string> ids;
    for (const auto& path : paths) {
        try {
            auto c = Character::FromJson(Read(path));
            if (!ids.insert(c.id).second) throw std::runtime_error("Duplicate character id: " + c.id);
            result.characters.push_back(std::move(c));
        } catch (const std::exception& e) { result.errors.push_back(path.string() + ": " + e.what()); }
    }
    return result;
}

void LocalCharacterRepository::Save(const Character& character) {
    const auto json = character.ToJson(); // Validate before any filesystem mutation.
    fs::create_directories(root_);
    const auto destination = root_ / (character.id + ".json");
    const auto temporary = root_ / (character.id + "." + Token() + ".tmp");
    try {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        output << json; output.flush();
        if (!output) throw std::runtime_error("Cannot write " + temporary.string());
        output.close();
        if (!output) throw std::runtime_error("Cannot close " + temporary.string());
#ifdef _WIN32
        if (!MoveFileExW(temporary.c_str(), destination.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
            throw std::runtime_error("Cannot commit character file (Windows error " + std::to_string(GetLastError()) + ")");
#else
        fs::rename(temporary, destination);
#endif
    } catch (...) { std::error_code ignored; fs::remove(temporary, ignored); throw; }
}

Character LocalCharacterRepository::Import(const fs::path& jsonFile) {
    auto c = Character::FromJson(Read(jsonFile));
    const std::string originalSprite = c.assets.sprite;
    const auto relative = fs::path("media") / c.id / Token();
    const auto directory = root_ / relative;
    int index = 0;
    try {
        Assets(c, [&](const std::string& ref) -> std::string {
            if (ref.empty()) return ref;
            if (ref.find("://") != std::string::npos) throw std::runtime_error("Import requires local assets; download cloud assets through your API adapter first");
            auto source = Utf8Path(ref);
            if (source.is_relative()) source = jsonFile.parent_path() / source;
            if (!fs::is_regular_file(source)) throw std::runtime_error("Missing asset: " + source.string());
            auto ext = source.extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (ext != ".png" && ext != ".jpg" && ext != ".jpeg" && ext != ".bmp")
                throw std::runtime_error("Unsupported artwork format: " + ext);
            if (fs::file_size(source) > 32 * 1024 * 1024) throw std::runtime_error("Asset exceeds 32 MiB");
            Image image = LoadCharacterImage(source);
            if (!image.data) throw std::runtime_error("Cannot decode artwork: " + ref);
            const int width = image.width, height = image.height;
            UnloadImage(image);
            if (width > 8192 || height > 8192) throw std::runtime_error("Artwork dimensions exceed 8192");
            if (ref == originalSprite && (width % c.assets.columns != 0 || height % c.assets.rows != 0))
                throw std::runtime_error("Sprite dimensions must be divisible by columns/rows");
            fs::create_directories(directory);
            const auto filename = std::to_string(index++) + ext;
            fs::copy_file(source, directory / filename);
            return (relative / filename).generic_string();
        });
        Save(c);
    } catch (...) {
        // Verify the resolved destination is still below the explicitly selected repository.
        std::error_code ignored;
        const auto resolvedRoot = fs::weakly_canonical(root_, ignored);
        const auto resolvedDirectory = fs::weakly_canonical(directory, ignored);
        const auto child = resolvedDirectory.lexically_relative(resolvedRoot);
        if (!ignored && !child.empty() && !child.is_absolute() &&
            std::none_of(child.begin(), child.end(), [](const auto& part) { return part == ".."; }))
            fs::remove_all(directory, ignored);
        throw;
    }
    return c;
}

CloudCharacterRepository::CloudCharacterRepository(CharacterCloudApi api, fs::path assetRoot)
    : api_(std::move(api)), assetRoot_(std::move(assetRoot)) {}

CharacterList CloudCharacterRepository::LoadAll() {
    if (!api_.listCharacters) throw std::runtime_error("Cloud list API is not configured");
    CharacterList result; std::set<std::string> ids;
    for (const auto& json : api_.listCharacters()) {
        try {
            auto c = Character::FromJson(json);
            if (!ids.insert(c.id).second) throw std::runtime_error("Duplicate cloud character id: " + c.id);
            result.characters.push_back(std::move(c));
        } catch (const std::exception& e) { result.errors.push_back(e.what()); }
    }
    return result;
}

void CloudCharacterRepository::Save(const Character& character) {
    character.Validate();
    if (!api_.putCharacter) throw std::runtime_error("Cloud save API is not configured");
    auto remote = character;
    // Preflight all files before uploading any artwork.
    Assets(remote, [&](const std::string& ref) {
        if (ref.empty() || Remote(ref)) return ref;
        if (!api_.uploadAsset) throw std::runtime_error("Cloud asset upload API is not configured");
        if (!fs::is_regular_file(assetRoot_ / Utf8Path(ref))) throw std::runtime_error("Missing asset: " + ref);
        return ref;
    });
    Assets(remote, [&](const std::string& ref) {
        if (ref.empty() || Remote(ref)) return ref;
        const auto uri = api_.uploadAsset(assetRoot_ / Utf8Path(ref));
        if (!Remote(uri)) throw std::runtime_error("Asset upload must return an https URL");
        return uri;
    });
    api_.putCharacter(remote.id, remote.ToJson());
}
