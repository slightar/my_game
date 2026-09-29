# 分册：敌人系统（arknights-go）

改 `enemy_*` / `EnemySystem` / 敌人渲染前必读。索引见 `MEMORY.md`。

## 名单与身份（11 类，官方名）

`EnemyKind`：Slug / AcidSlug / SlugHigh / IrrSlug / Soldier / Crossbow / Caster / Hound /
StealthCrossbow / Shield / Drone + `Count`。官方名 + 模型对照表在 `assets/enemies/mobs/README.md`。

- ⚠ **身份必须查 PRTS，不能凭"看起来像"取模型**：曾把 `enemy_1030_wteeth`（拾荒者）当猎狗、
  `enemy_1352_eslime`（辐能源石虫）当高能源石虫、`enemy_1012_dcross`（萨卡兹狙击手）当隐形弩手。
  名字 `自爆源石虫` 原作不存在，别再造名。隐形弩手 = `enemy_1019_jshoot`、术师 = `enemy_1011_wizard`。
- 名字与数值要自洽（曾给移速 104 的高能源石虫写「慢行」）。

## 体型比例（算出来的，别手填）

烘焙器统一 `scale = min(174/unionW, 164/unionH)` ⇒ **真实尺寸 = 表中墨迹高度 / scale**（spine 单位
跨模型通用）；锚定士兵（367 单位）= 110px → `C = 0.29976 px/单位`，`size_table.py` 出全表。

- ⚠ `spriteSize` 是**贴图方框边长**（含透明留白），不是可见高度。
- ⚠ `width` 是手调的**受击盒**宽度（模型包围盒含手臂/武器，士兵实测 398px 宽）；只有
  `reach`/阴影/格挡线读它。
- `enemy_tests` 有比例回归断言，别删。

## 朝向

- 图集 1920×768，10 列 × 4 行，格 192px；行 = Idle / Move(_Loop) / Attack / Die。
- `manifest.json` 的 `facing` 是烘焙器**写死的猜测**（`key==='shield'?[-1,1,-1,-1]:[1,1,1,1]`）；
  +X = 原生素材朝右，缺字段按 `{1,1,1,1}`。
- ⚠ `enemy_renderer.cpp` 的 `kShieldAttackNativeRight{false×4,true×5,false}` **必需别删**：
  `shield_v2.png` 的 Attack 行 f0-f3/f9 与 f4-f8 原生朝向相反，逐行 facing 表达不了。结论用
  **原始客户端骨架**（`cmake-build-debug/enemy-reference/spine/shield/`）无头重渲、与「原始渲染／
  其镜像」比 alpha 差得出（非循环论证）。
- ⚠ **判朝向禁用 alpha 质心／分带测量**：盾兵 Idle 近正面（同帧腿带 −5、头盔带 +4），09-20、
  09-22 两次被它带偏（一次误删覆盖表）。
- `EnemySystem` 只在 Approach / Recover 结束时改 `facing`，**攻击过程绝不翻转**。

## 大盾兵格挡

- `Damage()`：`exposed = !front && kind==Shield && facingTime>=kSpawnGrace(.4s)`；
  `blocked = Guarding() && !exposed && 物理 && 高度够` → 减免 ×0.2。
- `Guarding()` = 盾兵 && 未眩晕 && `state!=Recover` && 未死 ⇒ **后摇(1.25s)正面也吃满**。
- `EnemyUnit::facingTime` 是**刷怪保护**（0.4s 内背后不额外吃伤害），别删。
- 盾兵「转身延迟」已移除，`EnemyUnit::turning` 字段已删，别再引用。

## 隐身弩手

- `EnemySystem::kCloakRevealRange = 220`（公开静态，测试/渲染共用）。每 Tick
  `cloaked = !(Windup||Strike) && distance > 220` ⇒ **前摇/出手强制显形**，走远复隐。
- `ResolveBullet` 核心一行 `if(!melee && !u.InterceptsProjectiles()) continue;` ⇒ 远程子弹
  **穿过、不消耗**；近战无过滤。`InterceptsProjectiles() = !cloaked`。
- 渲染为**半透明剪影**（`Fade(Color{150,168,190,255},.20F)`）+ 淡圈 +「隐身」标签。
  **别改成全透明**（会被当渲染 bug）。

## 腐蚀与供能

- **酸液源石虫腐蚀**：角色无防御力属性，故「防御下降」做成**受击无敌帧减半**
  （`kCorrodedInvincibilityScale=0.5`，`kCorrosionDuration=5s`）。链路 `EnemyBolt::corrosive` →
  `AttackHits(...,bool*)` 输出参数 → `Player::TakeDamage(src,bool)`，不污染通用命中判定。
  `Player::IsCorroded()` 驱动 HUD「腐蚀 // CORRODED」徽章（`{1008,49,258,74}`，**x 起 1008 是为
  避开试炼横幅 340..990**）。
- **引爆系源石虫**（`DetonatesOnDeath = SlugHigh || IrrSlug`）：击杀进 `Fuse`（高能 1.00s 红圈物理／
  辐能 1.10s 紫圈法术），并给其他所有存活敌人挂 `haste=3.5s`、移速 ×1.5。渲染要有「供能」标签 + 紫圈。

## 其它

- `KeepsDistance()` 只含 Crossbow / Caster / StealthCrossbow / AcidSlug。**不要把 Drone 加进去**
  （reach 520，加了就永远后退，报 "Drone lock-on phase missing"）。
- `EnemyBolt::arts` = 法术弹（术师 290 px/s，无视物理减伤）；酸液弹 290；普通弩箭 410。
- `ResetTrial()` 刷 11 只，按威胁从左到右排，`Remaining()==11`。
- HUD / 横幅里的敌人数**一律从 `EnemyDefinitions().size()` 取**，别再写死。
