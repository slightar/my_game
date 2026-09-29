# 分册：客户端素材提取 + UI 界面几何（arknights-go）

读明日方舟客户端素材、复刻 UI 前必读。索引见 `MEMORY.md`。

客户端根目录：`C:\Users\34844\鹰角启动器\Arknights bilibili\games\Arknights\Arknights_Data`；
`StreamingAssets\AB\Windows\{refs\arts, ui}`（31 个 `enm_art_*.ab` + `ui/`）+ `PersistentData\Bundles\`
（补丁）。`extract_client_ui.py` 的 glob **能匹配中文目录**，别改。
⚠ **2026-09-24 起该目录已空**（客户端被卸载/移走）→ 不能重新提取，只能靠既有导出：
`output/_tmp_new/stage-ui-sprites/`（`[uc]stage.ab` 的 55 张）与 `cmake-build-debug/ui-reference/`。

## .ab 读取要点

- .ab 是 **flag-4 (LZHAM)**，UnityPy 直接 load 抛 NotImplementedError，**必须先注册**：
  `from extract_client_ui import decode; CompressionHelper.DECOMPRESSION_MAP[4] = decode`。
- ⚠ **alpha 在单独的 `[alpha]` 图、值存 R 通道**：
  `image.putalpha(objects[m+'[alpha]'].image.convert('RGB').getchannel('R'))`。写成 `.getchannel('A')`
  会得到全不透明图（症状隐蔽：能存、尺寸对、像没抠背景）。

## 敌人模型工具链

- `tools/enemy_model_catalogue.py`：`list [--filter 1006]` 检索 1100 个模型并显示所属包；
  `get <模型名> --output <目录>` 导出 atlas/skel/已合并 alpha 的 png。换素材第一入口。
  ⚠ 它按**去掉 `_N` 变体**的名归类，会把 `enemy_1019_jshoot` / `enemy_1011_wizard` 报成在
  `enm_art_3.ab`，**基础骨架实际在 `enm_art_base_0.ab`**。按包内是否真有 `.atlas`+`.skel` 取。
- 选素材要量**墨迹覆盖率**。`enemy_1011_wizard`（`defaultSkin` 为 null）与 `enemy_1068_snmage`
  都曾因烘出 0% 墨迹被误判"不可用"。修法已进 `tools/bake_enemy_sheets.html`：`defaultSkin` 空且
  有具名 skin 时 `setSkinByName(skins[0].name)` + `setSlotsToSetupPose()`。**别删这段**。

## UI 素材与关卡选择界面（2026-09-23 打通）

- **`ui/[uc]stage.ab` = 关卡选择界面本体**（55 Sprite）；`zonemap_0..3.ab` = 区域地图底图（**另一个
  界面，别混**）；`ui/pages/` = 各子页面。⚠ **字体全客户端没有**（ttf/otf/ttc = 0）→ 只能退系统
  CJK 字体栈。
- 实测几何（照这个摆，别手填）：**节点 `btn_stage_main_normal_adv` 166×82**，其中 `image_bkg`
  166×82 ← `bkg_normal` / `bkg_hilight` / `bkg_normal_branch` / `bkg_training`（四张**同形**：约
  134×70 横向横幅、**向下渐隐到黑**，仅配色不同）；`label_stage_code` 局部 (+36.3,+15.0) 盒
  93.4×41.8 左中对齐（字号约 30）；`panel_rank_common` (+17.8,+16.7) 40×45 三星；`char_container`
  103×100 @(+176.3,−21.0) **挂右侧、溢出节点框外** ⇒ prefab 是可选子件模板。
- **轨道组件是「橙色三件套」**（按颜色分组才看出来，别按名字猜）：
  `line_spst` **4×96**（前 52 行实心，之后长渐隐）**橙 (253,70,0)** = 轨道线段，4px 就是笔触宽度；
  `sprite_track_point_frame` 39×39 **橙 (255,104,1)** 空心菱形 = 当前关卡；
  `sprite_track_point_center` 23×23 橙实心圆 = 已通关；`img_dot_selected` 23×23 灰实心圆 = 未解锁。
  ⚠ **`line` 17×30（17×6 白横条＋下方辉光）是白色、不是轨道**（纯白=运行时 `Image.color` 染色），
  别拿它当折线。`line_spst` 是预先着色的橙色变体，折线用它。
- ⚠ **`line_spst` 不能整条拉伸**：只有前 52 行不透明，后 44 行是渐隐；直接拉伸会把长线段的下半截
  拉成半透明。裁到 alpha≥250 的实心段（4×52）再用。横线段用同一张**旋转 90°**。
- `details_bg` 371×478；`bg` 1024×576，平均 RGB(69,72,77)、亮度 72（**深色背景**，橙色对比够）。
- **滚动条**：`scrollbar` 7×25 不透明白 (255,255,255) + `scrollbar_bg` 7×25 半透明白 (α77)，
  是竖排列表用的；横向用就**旋转 90°**。它的存在说明关卡列表**本来就是可滚动的**。
- ⚠ **屏幕级摆位不在包里**（260 个都是组件 prefab，坐标由代码算）。
  （"蜿蜒路线图"、"竖向直连"、"交错阶梯"三种描述都错过，现在的横向布局同样是几何反推的。
  方向由用户确定：**关卡链条左右走向**。）

## 工具（`output/_tmp_new/`）

- `probe_ui.py` — 列包内对象类型 + sprite/texture 名。
- `dump_ui_tree.py` — dump RectTransform 层级＋真实坐标（参考输出 `_tree_stage.txt`）。
- `export_stage_ui.py` — 干净导出 `ui/[uc]stage.ab` 的 Sprite 到 `stage-ui-sprites/` ＋ ASCII 形状图。
- `dump_pixels.py` — 1:1 逐像素 ASCII ＋ RGB 直方图。
- `build_stage_select_page.py` — 生成自包含 HTML（贴图 base64）。
- `verify_page.js` — Node + 手写 DOM 桩验证页面（npm 被拦，起不了真浏览器）。

产物：`docs/stage-select/index.html`（5 章 × 5 关，可点，state = done/current/locked）。
**横向轨道 + 横向滚动**布局常量（1024×576 设计空间等比缩放）：
章节改为**顶部标签条** `#tabs left=25 top=20 w=975`；关卡视口 `#rail left=25 top=224 585×192`
（`overflow:hidden`），内容 `#rail-inner` 宽 **1054** 用 `translateX(-scroll)` 平移。
瓦片 166×82、水平步进 `PITCH=222`（间隙 56）、**上下交错 80px**（偶 `top=10` / 奇 `top=90`）；
折线用橙色 `line_spst`，出/入横段各 28px、竖段 4×80；节点标记在折线拐点
`x = i===0 ? 194 : 222i-28`。滚动：`MAX_SC=469`、拇指宽 325、行程 260，
支持滚轮 / 拖拇指 / 点轨道 + 选中自动滚入视野。详情面板 `left=629 top=74`。
⚠ **瓦片+面板放不下一行**：5×166 + 371 = 1201 > 1024，只能滚动。
