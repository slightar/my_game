#include "enemy_data.h"
#include <stdexcept>

const std::array<EnemyDefinition, kEnemyKindCount>& EnemyDefinitions() {
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
    static const std::array<EnemyDefinition, kEnemyKindCount> data{{
        {EnemyKind::Slug, "slug", "源石虫", "低矮近战：短前摇扑咬，跳跃或闪避后反击", 6, 100, 59, 56, 48, .40F, .85F, 95},
        // Ranged corrosion: the acid bolt shortens the operator's post-hit invincibility,
        // which is this game's stand-in for the original's defence-down debuff.
        {EnemyKind::AcidSlug, "acid_slug", "酸液源石虫", "远程腐蚀：酸液命中后削弱防护（无敌帧变短），优先点掉", 11, 84, 66, 70, 400, .95F, 1.10F, 114, EnemyAttack::AcidBolt},
        // Dying detonates with a physical blast - a patient pusher, not a suicidal one.
        {EnemyKind::SlugHigh, "slug_high", "高能源石虫", "结晶腹部：耐打的行走炸弹，死亡必定物理引爆，务必在远处解决", 15, 104, 71, 64, 62, .95F, .45F, 170},
        // Same death blast, but it leaks raw energy: every other enemy gets a burst of
        // speed, mirroring the real "死亡后…使场上敌人获得1点能量".
        {EnemyKind::IrrSlug, "irr_slug", "辐能源石虫", "法术引爆：死亡时泄漏能量，为场上其他敌人供能加速", 18, 74, 62, 73, 66, .60F, .75F, 140},
        {EnemyKind::Soldier, "soldier", "士兵", "近身斩击：看准举刀前摇，绕后反击", 12, 100, 54, 110, 68, .62F, .85F, 196},
        {EnemyKind::Crossbow, "crossbow", "弩手", "远程压制：瞄准线锁定后侧移，剑气可抵消弩箭", 9, 72, 53, 108, 440, .90F, 1.25F, 183, EnemyAttack::PhysicalBolt},
        // Arts bolt, so it is the answer to the guard's physical-only block. Fragile and
        // slow to fire, which is the window for closing the distance.
        {EnemyKind::Caster, "caster", "术师", "法术远程：弹速慢但无视盾牌物理减伤，趁其前摇贴身", 8, 66, 40, 104, 430, 1.05F, 1.30F, 139, EnemyAttack::ArtsBolt},
        {EnemyKind::Hound, "hound", "猎狗", "高速突进：扑咬前摇很短，拉开距离或抢先击退", 8, 168, 68, 77, 58, .34F, .55F, 109},
        // Cloaked by default: player projectiles pass straight through unless the operator
        // is inside kCloakRevealRange. Melee is never affected, so closing in is the answer.
        {EnemyKind::StealthCrossbow, "stealth_crossbow", "隐形弩手", "默认隐形：远程弹道直接穿过，靠近到 220 像素才显形，只有近战能稳定命中", 10, 78, 58, 113, 470, .85F, 1.15F, 184, EnemyAttack::PhysicalBolt},
        {EnemyKind::Shield, "shield", "重装防御者", "正面物理减伤 80%：绕到背后、用法术，或趁盾击后摇破防", 36, 46, 57, 118, 85, .95F, 1.25F, 138, EnemyAttack::Melee, .80F, true},
        {EnemyKind::Drone, "drone", "妖怪", "空中巡航：保持高度，瞄准后发射能量弹；优先打断其锁定", 16, 70, 119, 64, 520, 1.10F, 1.40F, 175, EnemyAttack::PhysicalBolt},
        // Exact variant rigs and chapter membership are retained in chapter_sources.json
        // and chapter_rosters.json; health/timing are adaptations for this action game.
        {EnemyKind::SlugAlpha,"slug_alpha","源石虫·α","强化扑咬：耐久高于源石虫，跃过后反击",9,105,59,55.8F,50,.40F,.80F,94.8F},
        {EnemyKind::MobileShield,"mobile_shield","机动盾兵","机动防御：正面物理减伤 55%，绕后或法术破盾",20,85,56,109.5F,72,.75F,1.05F,194.6F,EnemyAttack::Melee,.55F,true},
        {EnemyKind::HoundPro,"hound_pro","猎狗pro","强化猎狗：高速接近，抢先击退或闪避扑咬",10,180,68,76.9F,60,.32F,.55F,109.4F},
        {EnemyKind::DualSwordsman,"dual_swordsman","双持剑士","快速连斩：短前摇，贴身时保持移动",14,118,52,111.4F,74,.38F,.55F,173.9F},
        {EnemyKind::Molotov,"molotov","燃烧瓶投掷者","抛物线投掷：锁定落点后范围爆燃，及时离开落点",12,68,48,109.5F,540,1.05F,1.35F,143,EnemyAttack::Lob},
        {EnemyKind::Scavenger,"scavenger","拾荒者","轻装近战：与双持剑士协同压近，优先阻断包围",10,100,52,108.2F,64,.60F,.85F,221},
        {EnemyKind::SnowSoldier,"snow_soldier","雪怪小队","冻结追击：攻击冻结目标时伤害提高",15,90,54,110.3F,70,.65F,.90F,194.2F,EnemyAttack::Melee,0,false,false,true},
        {EnemyKind::SnowSniper,"snow_sniper","雪怪狙击手","远程追击：冻结时弩箭伤害提高，避开术师锁定",12,65,52,111.2F,470,.90F,1.15F,197.7F,EnemyAttack::PhysicalBolt,0,false,false,true},
        {EnemyKind::IceSlug,"ice_slug","冰爆源石虫","死亡冰爆：范围寒冷；寒冷再次叠加会短暂冻结",12,82,66,69.6F,58,.70F,.85F,171.4F,EnemyAttack::Melee,0,false,true},
        {EnemyKind::Icebreaker,"icebreaker","雪怪小队凿冰人","凿冰重击：耐久较高，对冻结目标造成三倍伤害",30,54,64,105.7F,96,1.10F,1.30F,153.8F,EnemyAttack::Melee,.20F,false,false,true},
        {EnemyKind::SnowCasterLeader,"snow_caster_leader","雪怪术师组长","寒冷法术：每第三次施法附加寒冷，叠加会冻结",18,60,44,111.6F,470,1.05F,1.30F,141.8F,EnemyAttack::ColdBolt},
        {EnemyKind::GuerrillaHound,"guerrilla_hound","游击队猎犬","高速先锋：率先冲入近身范围，避免被后排夹击",12,188,70,75.9F,62,.32F,.55F,153.5F},
        {EnemyKind::GuerrillaFighter,"guerrilla_fighter","游击队战士","近身推进：耐久高于普通士兵，前后排交错推进",18,108,54,109.8F,74,.55F,.80F,158.5F},
        {EnemyKind::GuerrillaSniper,"guerrilla_sniper","游击队狙击手","远程压制：锁定后发射弩箭，先处理远程后排",14,72,54,109.7F,500,.80F,1.10F,172.7F,EnemyAttack::PhysicalBolt},
        {EnemyKind::GuerrillaMortar,"guerrilla_mortar","游击队迫击炮兵","远距炮击：提前标记落点，范围爆炸；不要原地站桩",22,45,62,114.5F,760,1.30F,1.65F,183.2F,EnemyAttack::Lob,.25F},
        {EnemyKind::GuerrillaRaider,"guerrilla_raider","游击队突袭战士","装甲突袭：高速近战，物理减伤 20%，法术穿透",24,140,56,109.7F,82,.48F,.75F,188.1F,EnemyAttack::Melee,.20F},
        {EnemyKind::GuerrillaSarkaz,"guerrilla_sarkaz","游击队萨卡兹战士","萨卡兹重兵：耐久高、重击前摇长，闪避后反击",28,80,60,113.8F,88,.95F,1.10F,177.6F},
        {EnemyKind::UrsusBeast,"ursus_beast","乌萨斯裂兽","高速裂兽：压缩闪避时间，防止术师与近战合围",14,180,72,83.6F,66,.30F,.55F,115.5F},
        {EnemyKind::UrsusCaster,"ursus_caster","乌萨斯着铠术师","着铠施法：物理减伤 35%，远距法术压制",26,58,48,105,520,1.00F,1.20F,146.1F,EnemyAttack::ArtsBolt,.35F},
        {EnemyKind::UrsusAssault,"ursus_assault","乌萨斯突击者","突击推进：物理减伤 30%，抢占近身区域",28,125,62,117.4F,92,.55F,.85F,201.3F,EnemyAttack::Melee,.30F},
    }};
    return data;
}
const EnemyDefinition& EnemyData(EnemyKind kind) {
    const auto i = static_cast<unsigned>(kind);
    if (i >= EnemyDefinitions().size()) throw std::out_of_range("Unknown enemy kind");
    return EnemyDefinitions()[i];
}

std::string EnemyGlyphText() {
    std::string text = "敌方单位战斗设计蓄爆缺少素材格挡隐身供能瞄准锁定腐蚀寒冷冻结";
    for (const auto& enemy : EnemyDefinitions()) {
        text += enemy.name;
        text += enemy.tactic;
    }
    return text;
}
