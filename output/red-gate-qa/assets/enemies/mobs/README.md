# 十一类小怪战斗素材

素材从用户提供的本地《明日方舟》客户端只读提取，由原模型离线渲染。
原始模型与美术归鹰角网络及相关权利方所有；本项目沿用已有同人素材说明。
没有修改客户端文件，没有替换项目已有资源。游戏运行时只加载 PNG 和 JSON。

## 项目 ID → 客户端模型

名称全部按 PRTS 校准到官方中文名。**每个模型名都对应它真正的怪物**，不要凭"看起来像"改名。

| 项目 ID | 官方名 | 本地模型 | 客户端包（refs/arts 下） |
|---|---|---|---|
| slug | 源石虫 | enemy_1007_slime | enm_art_base_0.ab |
| acid_slug | 酸液源石虫 | enemy_1004_mslime | enm_art_1.ab |
| slug_high | 高能源石虫 | enemy_1021_bslime | enm_art_3.ab |
| irr_slug | 辐能源石虫 | enemy_1352_eslime | enm_art_13.ab |
| soldier | 士兵 | enemy_1002_nsabr | enm_art_base_0.ab |
| crossbow | 弩手 | enemy_1003_ncbow | enm_art_1.ab |
| caster | 术师 | enemy_1011_wizard | enm_art_base_0.ab |
| hound | 猎狗 | enemy_1000_gopro | enm_art_base_0.ab |
| stealth_crossbow | 隐形弩手 | enemy_1019_jshoot | enm_art_base_0.ab |
| shield | 重装防御者 | enemy_1006_shield | enm_art_2.ab |
| drone | 妖怪 | enemy_1005_yokai | enm_art_2.ab |

十一套客户端精灵表均为 10 列 × 4 行、单格 192 × 192；依次为待机、移动、攻击、死亡。
无人机使用 drone.png。盾兵使用 shield_v2.png，保留旧 shield.png；新版补齐非循环动作末帧，
记录原始播放时长和各动作朝向。manifest.json 保存原动作名称和落地锚点，避免不同角色脚底悬空。
客户端分离的 alpha 纹理在提取时合并。

## 换素材时踩过的坑（务必先读）

**模型身份不能靠猜。** 上一版把 `enemy_1030_wteeth` 当成猎狗（实际是拾荒者）、把
`enemy_1352_eslime` 当成高能源石虫（实际是辐能源石虫）、把 `enemy_1012_dcross` 当成
隐形弩手（实际是萨卡兹狙击手）。三处都已纠正。另外 **`自爆源石虫` 在原作里并不存在**，
不要为了"听起来合理"而造名。

**命名为 skin 的模型会被烘成全透明图。** `enemy_1011_wizard` 的 `defaultSkin` 是 null、
`skins` 只有一个具名 `Wizard`，不调 `setSkinByName` 的话每个 slot 的 attachment 都是空的，
烘出来是一张 0% 墨迹的空图（症状隐蔽：能存、尺寸对、只是全透明）。
`tools/bake_enemy_sheets.html` 现在会对这类模型自动应用第一个具名 skin。选材时**先量墨迹覆盖率**，
别只看四段动作名齐不齐。同样原因，`enemy_1068_snmage` 也曾在早期被误判。

**catalogue 会把 `_N` skin 后缀折叠。** `enemy_model_catalogue.py` 按去掉变体的模型名归类，
所以它把 `enemy_1019_jshoot` / `enemy_1011_wizard` 报成在 `enm_art_3.ab`（那里面只有 `_2` 变体），
而**基础骨架实际在 `enm_art_base_0.ab`**。取模型时以包内是否真的有 `.atlas` + `.skel` 为准。

## 体型比例

`spriteSize` / `height` 由烘焙产物推导，不是手填。烘焙器统一按
`scale = min(174/unionW, 164/unionH)`（spine 单位 → 像素）出图，因此某模型的真实尺寸是
`(表中墨迹高度) / scale`，且这些 spine 单位在所有模型间通用。把士兵（普通人类，367 单位）
锚定到 110px（玩家立绘 112px）得到 `C = 0.29976 px/单位`，于是

- `height = C × 模型单位高度`
- `spriteSize = C × 192 / scale`

推导结果（`output/_tmp_new/size_table.py`）：

| 项目 ID | 单位高度 | height | spriteSize |
|---|---|---|---|
| 重装防御者 | 392.2 | 118 | 138 |
| 隐形弩手 | 377.0 | 113 | 184 |
| 士兵 | 367.0 | 110 | 196 |
| 弩手 | 359.4 | 108 | 183 |
| 术师 | 347.8 | 104 | 139 |
| 猎狗 | 256.6 | 77 | 109 |
| 辐能源石虫 | 242.6 | 73 | 140 |
| 酸液源石虫 | 233.7 | 70 | 114 |
| 高能源石虫 | 213.1 | 64 | 170 |
| 妖怪（无人机） | 212.8 | 64 | 175 |
| 源石虫 | 187.7 | 56 | 95 |

`enemy_data.cpp` 里的 `width` **不在这张表内**：它是手调的受击盒宽度。模型包围盒含张开的手臂
和武器（士兵实测 398px 宽），照搬会让脚下阴影和盾兵格挡线宽得离谱，所以受击盒收紧到躯干。
`spriteSize` 是"贴图方框"边长而非可见高度——方框有透明留白，可见高度就是 `height`。

## 复现工具

`tools/extract_enemy_models.py`、`tools/enemy_model_catalogue.py`、`tools/bake_enemy_sheets.py`、
`tools/bake_enemy_sheets.html`。
`enemy_model_catalogue.py list [--filter 1030]` 可检索全部 1100 个客户端模型，
`get <模型名> --output <目录>` 直接导出 atlas / skel / 已合并 alpha 的贴图，是换素材时最快的入口。
提取工具复用已安装的 UnityPy；离线烘焙使用隔离的 Edge、Pixi 6.5.10 和 pixi-spine 3.1.2。
这些仅用于素材制作，未增加 C++ 游戏依赖，未打包进游戏。

⚠ 离线 g++ 驱动不会执行 CMake 的 POST_BUILD 素材拷贝。烘焙出新图集后必须同步到
`cmake-build-debug/assets/enemies/mobs/`（`output/_tmp_new/sync_mobs.py`），否则运行时仍读旧资源。
