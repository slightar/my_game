#pragma once
#include "raylib.h"
#include "character.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <vector>

// raylib's narrow fopen path cannot reliably open UTF-8 filenames on Windows.
// Use the filesystem path overload, then decode image bytes without a filename.
inline Image LoadCharacterImage(const std::filesystem::path& path) {
    std::error_code error;
    const auto size = std::filesystem::file_size(path, error);
    if (error || size == 0 || size > 32 * 1024 * 1024) return {};
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return {};
    std::vector<unsigned char> bytes(static_cast<std::size_t>(size));
    stream.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!stream) return {};
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    Image image = LoadImageFromMemory(extension.c_str(), bytes.data(), static_cast<int>(bytes.size()));
    if (image.data && (image.width > 8192 || image.height > 8192)) { UnloadImage(image); return {}; }
    return image;
}

using CharacterFrameMap = std::vector<std::vector<int>>;
inline CharacterFrameMap FindCharacterFrames(const Image& image, const CharacterAssets& assets) {
    CharacterFrameMap frames(assets.rows);
    if (!image.data || image.width % assets.columns || image.height % assets.rows) return frames;
    const int w = image.width / assets.columns, h = image.height / assets.rows;
    Color* pixels = LoadImageColors(image);
    if (!pixels) return frames;
    for (int row = 0; row < assets.rows; ++row) {
        const int count = row < static_cast<int>(assets.rowFrameCounts.size()) ? assets.rowFrameCounts[row] : assets.columns;
        for (int col = 0; col < count; ++col) {
            bool visible = false;
            for (int y = 0; y < h && !visible; ++y)
                for (int x = 0; x < w; ++x)
                    if (pixels[(row * h + y) * image.width + col * w + x].a > 0) { visible = true; break; }
            if (visible) frames[row].push_back(col);
        }
    }
    UnloadImageColors(pixels);
    return frames;
}

struct CharacterFrame { int row = 0, column = 0; bool valid = false; };
inline CharacterFrame SampleCharacterFrame(const CharacterFrameMap& frames, int row, int idleRow,
                                           float fps, float time, bool loop = true) {
    if (row < 0 || row >= static_cast<int>(frames.size()) || frames[row].empty()) row = idleRow;
    if (row < 0 || row >= static_cast<int>(frames.size()) || frames[row].empty()) return {};
    const auto& sequence = frames[row];
    const double step = std::isfinite(time) ? std::floor(std::max(time, 0.0F) * static_cast<double>(fps)) : 0;
    const auto index = static_cast<std::size_t>(loop ? std::fmod(step, static_cast<double>(sequence.size())) :
                                                std::min(step, static_cast<double>(sequence.size() - 1)));
    return {row, sequence[index], true};
}
