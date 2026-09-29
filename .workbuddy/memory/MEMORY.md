# 项目长期笔记 — arknights-go (D:\my_game)

raylib 5.5 + C++20 横版动作游戏，CMake + MinGW。源码 `src/`、测试 `tests/`、美术 `assets/`、
QA 产物 `output/`（已 gitignore）。逐次修订见 `memory/YYYY-MM-DD.md`。
**本文件只做索引 + 高频铁律；专题细节在同目录分册，改相关代码前先读它：**
`ref-enemy-system.md`（敌人系统）｜`ref-client-assets.md`（客户端素材提取 + UI 界面几何）。

## 构建 / 验证（本机无 cmake / ninja / make）

`python output/_tmp_new/build.py tests|game|smoke|gamesmoke|all`；另 `run_gamesmoke.py`
（gamesmoke **必须在隔离目录**跑）、`sync_mobs.py`（改素材后**必跑**，assets→cmake-build-debug）、
`verify_roster.py`。

- 编译器 `C:\msys64\ucrt64\bin\g++.exe`；raylib 头/库在 `cmake-build-debug/_deps/`。需
  `-Isrc -Ithird_party -DGRAPHICS_API_OPENGL_33 -DPLATFORM_DESKTOP`，链接
  `-lopengl32 -lglu32 -lwinmm -luser32 -lgdi32 -lwinspool -lshell32 -lole32 -loleaut32 -luuid
  -lcomdlg32 -ladvapi32 -limm32 -lkernel32` + `-static-libgcc -static-libstdc++`。
- exe 必须输出到 `cmake-build-debug/`（`GetApplicationDirectory()` 取 exe 目录）。
- ⚠ **直接编 exe 不拷 assets**（CMake POST_BUILD 才拷）→ 改素材后必须手动同步。
- ⚠ `Game`/`MainMenu` 把 save+settings 写在 **exe 同级**，`game_enemy_smoke` 只能在拷贝出的目录跑。
- `enemy_tests` 只链 `enemy_data.cpp` + `enemy_system.cpp` + raylib，别挂 character_art。

## 环境坑

- 内置 Git Bash **没有 coreutils**（`ls/head/tail/sed/mkdir/rm/cp/mv/find/wc` 全缺）。文件操作用
  Read/Write/Glob/Grep，批量/统计用 Python。
- Python venv（Pillow + UnityPy，无 numpy）：
  `C:\Users\34844\.workbuddy\binaries\python\envs\default\Scripts\python.exe`。
- ⚠ **`npm` 被安全策略拦**（它去调黑名单 `wsl.exe`）→ 装不了/起不了真浏览器；要"渲染验证"改用
  Node + 手写 DOM 桩。
- ⚠ **图像 Read 返回 "model does not support images"**，看不到像素。判素材/渲染只能走数值、
  ASCII 密度图或原始骨架重渲。**别用"我看了一眼"下结论。**
- ⚠ **Edit 不要对同一文件并行发多条**（并发 Edit 互相覆盖，症状是"报成功却没生效"）。逐条串行。

## UI 输入模型

- `src/ui_input.h`：`ReadUiPointer()` → `UiPointer{position, pressed, released, down, valid}`
  （声明顺序 pressed, released, down；按名字赋值）。
- `UiPointer::Clicked(rect) = released && Hit(rect)` → **动作在松手时提交**。
- 按下视觉：`TerminalUi::SetPointerState(pos,down,valid)` + `Pressed()`/`PressedRect()`（下移 5px）
  /`PressedVeil()`（统一灰蒙版）。新的可点击控件画完必须调 `PressedVeil(rect)`。

## 德克萨斯技能（铁律）

- 键位 `kDefaultKeys={A,D,W,S,J,R,KEY_Q,KEY_E,ONE,TWO,ESC}` → `skillOne=Q`、`skillTwo=E`；
  德州 **Q = 剑雨、E = 阵雨连绵（模式开关）**。
- ⚠ **剑雨 = 持续 1.35s 的落剑雨**（`kSwordRainDuration`，冷却 14s）：每
  `kSwordRainSpawnInterval=0.12s` 从敌人上方 ±130px 生成 `FallingSword`（速度 920 向下），
  每把 `kTexasSwordRainDamage=1.55` 法术伤害 + `kSwordRainStunDuration=0.42s` 晕眩。
  **2026-09-23 用户明确要求回退到这一版**，别复活「瞬发／以德州为中心／半径 300 圆」的爆发
  （`BulletKind::SwordRain`、`radial`、`DrawTexasSwordRainBurst`、`kSwordRainRadius` 均已不存在）。
- `ResolveBullet`：`melee=(kind==MeleeSlash)` ⇒ 仅近战圆碰撞、**一次多目标**、**不过滤隐身**；
  其余（含 `FallingSword`）走 `SegmentHit`、**命中即 `lifetime=0`**、**被隐身过滤**。
- 落剑美术：`assets/operators/texas_sword.png`（19×118、金刃深柄、**尖朝下**），取自官方
  `char_102_texas` 图集 `F_Weapon` 区 `(108,468,108,42)`；绘制用
  `CharacterArt::DrawTexasSword(center, length, rotDeg, tint)`（`length`=目标像素长），
  调用点在 `src/game.cpp` 的 `FallingSword` 分支（`kBladeLength=88.0F`），外套暖白辉光
  `{255,243,214}`。⚠ 别改用 S2 特效包的 `texas2_01`。
- ⚠ **`BulletKind::FallingSword` 不能删**：模组角色 `EffectType::Rain` 仍在用，是**另一套东西**。
- ⚠ 施放动作**别用 `texasRainBurstTimer_`**：它驱动 S2 战斗立绘 `ending` 分支，帧号按
  `0.18 - timer` 算会取到负帧号 → 源矩形越界。

## 敌人系统（铁律速查）

名单 11 类官方名，对照表在 `assets/enemies/mobs/README.md`。细节见 `ref-enemy-system.md`。

- ⚠ **身份必须查 PRTS，不能凭"看起来像"取模型**（曾把拾荒者当猎狗、辐能源石虫当高能源石虫、
  萨卡兹狙击手当隐形弩手；`自爆源石虫` 原作不存在）。
- ⚠ **判朝向禁用 alpha 质心／分带测量**（盾兵 Idle 近正面，两次带偏）。
- ⚠ `kShieldAttackNativeRight{false×4,true×5,false}`、`EnemyUnit::facingTime`、
  `enemy_tests` 比例断言：**别删**。`EnemyUnit::turning` 已删，别再引用。
- ⚠ **别把 Drone 加进 `KeepsDistance()`**（reach 520，加了永远后退）。敌人数一律取
  `EnemyDefinitions().size()`；`ResetTrial()` 刷 11 只。

## 客户端素材（铁律速查）

细节与工具链见 `ref-client-assets.md`。

- ⚠ .ab 是 **flag-4 (LZHAM)**，必须先注册
  `CompressionHelper.DECOMPRESSION_MAP[4] = decode`（`from extract_client_ui import decode`）。
- ⚠ **alpha 在单独的 `[alpha]` 图、值存 R 通道**（`.getchannel('R')`，不是 `'A'`）。
- `ui/[uc]stage.ab` = **关卡选择界面本体**（节点 166×82；**关卡链条左右走向**，折线用
  **橙色 `line_spst` 4×96**，⚠ `line` 17×30 是白横条、不是轨道）；⚠ 字体全客户端没有
  → 只能退系统 CJK 字体栈。原型产物 `docs/stage-select/index.html`（5 章×5 关，
  横向轨道 + 横向滚动，细节见分册）。
- ⚠ 客户端目录 `C:\Users\34844\鹰角启动器\` **自 2026-09-24 起已空**（被卸载/移走），
  无法重新提取素材，只能用既有导出。

## 排查「改了没生效」

源码改动 → CMakeLists 真编了这个文件 → `.obj` 比源码新（比 exe 日期可靠）→ 运行时素材一致
（比 sha1）→ 都成立再让用户确认跑的是这份构建。
**最常见真因是最后一步：只编了 exe 没同步 `cmake-build-debug/assets/`。**

## 测试现状（2026-09-23 全绿）

- `enemy_tests` → "Enemy combat rules passed"；`game_enemy_smoke`（隔离目录）→ "…pause and boss
  return passed"（入口校验已从写死 `!= 6` 改成对比 `EnemyDefinitions().size()`）。
- `character_render_smoke` → "Character render smoke test passed"，截图产物在 `character-qa/`。
  历史卡在第 36 行，**根因是两个真 bug 而非"陈旧断言"**：① `menu_render_checks.h` 的 `click()`
  是 `Down → Update → Up`，**松手后没再跑一帧**，而菜单在**松手**时提交 ⇒ 每次点击都是空操作
  （已修；**改这个 lambda 前先想清楚边沿**）；② 「走到出口自动旅行」断言里的
  `MainMenu::MoveExplorer` **有声明有实现但无人调用**，该断言不可能通过，已删除（不是伪造通过）。
