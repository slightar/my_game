#include "enemy_data.h"
#include <stdexcept>

const std::array<EnemyDefinition, 11>& EnemyDefinitions() {
    // Prototype values in this action game's units, not the original tower-defense stats.
    //
    // height / spriteSize are DERIVED FROM THE BAKED ART, not hand-picked. The baker
    // renders every rig at scale = min(174/unionW, 164/unionH) spine-units->px, so a
    // rig's true size is (its ink height in the sheet) / scale, and those units are shared
    // across rigs. Anchoring 士兵 (an ordinary human, 367 units) to 110px - the operator
    // sprite is 112px - fixes C = 0.29976 px per spine unit, so
    //     height     = C * rigHeightInUnits
    //     spriteSize = C * 192 / scale
    // for every kind, which is what keeps the roster in proportion. See
    // output/_tmp_new/size_table.py.
    //
    // `width` is NOT derived: it is a hand-tuned HITBOX width. The rig's bounding box
    // includes outstretched arms and weapons (士兵 measures 398px wide), which would make
    // the shadow ellipse and the guard's block line absurdly broad, so the hitbox is
    // tightened to the torso. Only `reach` and the shadow read it, never the sprite.
    //
    // TODO: Tune per-chapter variants and rewards when real story encounters are authored.
    static const std::array<EnemyDefinition, 11> data{{
        {EnemyKind::Slug, "slug", "源石虫", "低矮近战：短前摇扑咬，跳跃或闪避后反击", 6, 100, 59, 56, 48, .40F, .85F, 95},
        // Ranged corrosion: the acid bolt shortens the operator's post-hit invincibility,
        // which is this game's stand-in for the original's defence-down debuff.
        {EnemyKind::AcidSlug, "acid_slug", "酸液源石虫", "远程腐蚀：酸液命中后削弱防护（无敌帧变短），优先点掉", 11, 84, 66, 70, 400, .95F, 1.10F, 114},
        // Dying detonates with a physical blast - a patient pusher, not a suicidal one.
        {EnemyKind::SlugHigh, "slug_high", "高能源石虫", "结晶腹部：耐打的行走炸弹，死亡必定物理引爆，务必在远处解决", 15, 104, 71, 64, 62, .95F, .45F, 170},
        // Same death blast, but it leaks raw energy: every other enemy gets a burst of
        // speed, mirroring the real "死亡后…使场上敌人获得1点能量".
        {EnemyKind::IrrSlug, "irr_slug", "辐能源石虫", "法术引爆：死亡时泄漏能量，为场上其他敌人供能加速", 18, 74, 62, 73, 66, .60F, .75F, 140},
        {EnemyKind::Soldier, "soldier", "士兵", "近身斩击：看准举刀前摇，绕后反击", 12, 100, 54, 110, 68, .62F, .85F, 196},
        {EnemyKind::Crossbow, "crossbow", "弩手", "远程压制：瞄准线锁定后侧移，剑气可抵消弩箭", 9, 72, 53, 108, 440, .90F, 1.25F, 183},
        // Arts bolt, so it is the answer to the guard's physical-only block. Fragile and
        // slow to fire, which is the window for closing the distance.
        {EnemyKind::Caster, "caster", "术师", "法术远程：弹速慢但无视盾牌物理减伤，趁其前摇贴身", 8, 66, 40, 104, 430, 1.05F, 1.30F, 139},
        {EnemyKind::Hound, "hound", "猎狗", "高速突进：扑咬前摇很短，拉开距离或抢先击退", 8, 168, 68, 77, 58, .34F, .55F, 109},
        // Cloaked by default: player projectiles pass straight through unless the operator
        // is inside kCloakRevealRange. Melee is never affected, so closing in is the answer.
        {EnemyKind::StealthCrossbow, "stealth_crossbow", "隐形弩手", "默认隐形：远程弹道直接穿过，靠近到 220 像素才显形，只有近战能稳定命中", 10, 78, 58, 113, 470, .85F, 1.15F, 184},
        {EnemyKind::Shield, "shield", "重装防御者", "正面物理减伤 80%：绕到背后、用法术，或趁盾击后摇破防", 36, 46, 57, 118, 85, .95F, 1.25F, 138},
        {EnemyKind::Drone, "drone", "妖怪", "空中巡航：保持高度，瞄准后发射能量弹；优先打断其锁定", 16, 70, 119, 64, 520, 1.10F, 1.40F, 175},
    }};
    return data;
}
const EnemyDefinition& EnemyData(EnemyKind kind) {
    const auto i = static_cast<unsigned>(kind);
    if (i >= EnemyDefinitions().size()) throw std::out_of_range("Unknown enemy kind");
    return EnemyDefinitions()[i];
}

std::string EnemyGlyphText() {
    std::string text = "敌方单位战斗设计蓄爆缺少素材格挡隐身供能瞄准锁定腐蚀";
    for (const auto& enemy : EnemyDefinitions()) {
        text += enemy.name;
        text += enemy.tactic;
    }
    return text;
}
