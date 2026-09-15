#pragma once
#include "character.h"
#include <filesystem>
#include <functional>
#include <vector>

struct CharacterList {
    std::vector<Character> characters;
    std::vector<std::string> errors; // Corrupt entries do not hide valid characters.
};

class ICharacterRepository {
public:
    virtual ~ICharacterRepository() = default;
    virtual CharacterList LoadAll() = 0;
    virtual void Save(const Character& character) = 0; // Throws on failure.
};

class LocalCharacterRepository final : public ICharacterRepository {
public:
    explicit LocalCharacterRepository(std::filesystem::path root);
    CharacterList LoadAll() override;
    void Save(const Character& character) override;
    // Copies local artwork into managed storage before committing metadata.
    Character Import(const std::filesystem::path& jsonFile);
    [[nodiscard]] const std::filesystem::path& Root() const { return root_; }
private:
    std::filesystem::path root_;
};

// Implement these operations in your API layer. Authentication stays outside game data.
// putCharacter must throw for HTTP/backend errors and return only after a confirmed write.
struct CharacterCloudApi {
    std::function<std::vector<std::string>()> listCharacters;
    std::function<void(const std::string& id, const std::string& json)> putCharacter;
    std::function<std::string(const std::filesystem::path& localFile)> uploadAsset;
};

class CloudCharacterRepository final : public ICharacterRepository {
public:
    explicit CloudCharacterRepository(CharacterCloudApi api, std::filesystem::path assetRoot = {});
    CharacterList LoadAll() override;
    void Save(const Character& character) override;
private:
    CharacterCloudApi api_;
    std::filesystem::path assetRoot_;
};
