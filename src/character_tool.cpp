#include "character_repository.h"
#include "file_path.h"
#include <fstream>
#include <iostream>

#ifdef _WIN32
int wmain(int argc, wchar_t** argv) {
#else
int main(int argc, char** argv) {
#endif
    try {
        if (argc < 3) {
            std::cerr << "Usage: character_tool validate <character.json>\n"
                         "       character_tool import <character.json> [repository-directory]\n";
            return 2;
        }
        const std::filesystem::path input(argv[2]);
        const std::string command = std::filesystem::path(argv[1]).string();
        if (command == "validate") {
            if (std::filesystem::file_size(input) > 1024 * 1024) throw std::runtime_error("JSON exceeds 1 MiB");
            std::ifstream stream(input, std::ios::binary);
            if (!stream) throw std::runtime_error("Cannot open JSON");
            auto character = Character::FromJson({std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()});
            std::cout << "Valid character: " << character.id << '\n';
        } else if (command == "import") {
            auto root = argc > 3 ? std::filesystem::path(argv[3]) : Utf8Path(GetApplicationDirectory()) / "characters";
            LocalCharacterRepository repository(root);
            const auto character = repository.Import(input);
            std::cout << "Saved locally: " << (repository.Root() / (character.id + ".json")) << '\n';
        } else { throw std::runtime_error("Unknown command"); }
        return 0;
    } catch (const std::exception& e) { std::cerr << "Error: " << e.what() << '\n'; return 1; }
}
