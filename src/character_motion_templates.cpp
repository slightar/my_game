#include "character_motion_templates.h"
#include <algorithm>

namespace {
// Attacks already deal damage on their trigger, so the strike starts immediately.
// Subsequent keys describe follow-through and recovery, without delaying damage.
const CharacterActionTemplate melee{"melee", .24F, {{
    {0, -.02F, 0, -10, 1.04F, .97F, -110, 25, 5},
    {.14F, .12F, -.025F, 22, 1.12F, .94F, 65, -40, -10},
    {.38F, .10F, -.015F, 16, 1.06F, .97F, 90, -25, -6},
    {.72F, .035F, 0, 5, .98F, 1.02F, 25, -10, 0},
    {1}
}}};
const CharacterActionTemplate ranged{"ranged", .20F, {{
    {0, -.015F, 0, -4, 1, 1, -88, -65, 0},
    {.12F, -.09F, 0, -13, .92F, 1.04F, -108, -82, 7},
    {.35F, -.06F, 0, -8, .96F, 1.02F, -93, -70, 3},
    {.70F, -.015F, 0, -2, 1.02F, .99F, -55, -35, 0},
    {1}
}}};
const CharacterActionTemplate caster{"caster", .24F, {{
    {0, -.01F, -.015F, -6, .96F, 1.04F, -120, -90, -6},
    {.16F, .055F, -.08F, 9, .94F, 1.10F, -70, -110, -10},
    {.42F, .07F, -.06F, 12, 1.06F, 1.02F, -50, -80, -4},
    {.76F, .025F, -.02F, 3, 1.02F, .98F, -25, -35, 0},
    {1}
}}};
}

CharacterMotionStyle ResolveCharacterMotionStyle(const Character& c) {
    if (c.assets.motionTemplate == "melee") return CharacterMotionStyle::Melee;
    if (c.assets.motionTemplate == "ranged") return CharacterMotionStyle::Ranged;
    if (c.assets.motionTemplate == "caster") return CharacterMotionStyle::Caster;
    if (c.attack.type == EffectType::Melee) return CharacterMotionStyle::Melee;
    return c.attack.damageType == DamageType::Arts || c.attack.type == EffectType::Rain
        ? CharacterMotionStyle::Caster : CharacterMotionStyle::Ranged;
}
const CharacterActionTemplate& GetCharacterActionTemplate(CharacterMotionStyle style) {
    if (style == CharacterMotionStyle::Melee) return melee;
    if (style == CharacterMotionStyle::Caster) return caster;
    return ranged;
}
CharacterAttackKey SampleCharacterAttack(CharacterMotionStyle style, float progress) {
    const auto& keys = GetCharacterActionTemplate(style).attack;
    progress = std::clamp(progress, 0.0F, 1.0F);
    for (std::size_t i = 1; i < keys.size(); ++i) {
        if (progress > keys[i].time) continue;
        const auto& a = keys[i-1]; const auto& b = keys[i];
        float t = (progress-a.time)/(b.time-a.time);
        t = t*t*(3-2*t);
        const auto mix = [t](float x, float y) { return x+(y-x)*t; };
        return {progress, mix(a.shift,b.shift), mix(a.lift,b.lift), mix(a.angle,b.angle),
            mix(a.scaleX,b.scaleX), mix(a.scaleY,b.scaleY), mix(a.armFront,b.armFront),
            mix(a.armBack,b.armBack), mix(a.head,b.head)};
    }
    return keys.back();
}
