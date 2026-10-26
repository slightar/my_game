#pragma once
#include "character.h"
#include <array>

enum class CharacterMotionStyle { Auto, Melee, Ranged, Caster };

// Height-relative root motion and joint angles. Shared by single sprites and rigs.
struct CharacterAttackKey {
    float time = 0, shift = 0, lift = 0, angle = 0;
    float scaleX = 1, scaleY = 1, armFront = 0, armBack = 0, head = 0;
};
struct CharacterActionTemplate {
    const char* id;
    float duration;
    std::array<CharacterAttackKey, 5> attack;
};
CharacterMotionStyle ResolveCharacterMotionStyle(const Character& character);
const CharacterActionTemplate& GetCharacterActionTemplate(CharacterMotionStyle style);
CharacterAttackKey SampleCharacterAttack(CharacterMotionStyle style, float progress);
