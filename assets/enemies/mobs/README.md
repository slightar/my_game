# 六类小怪战斗素材

素材从用户提供的本地《明日方舟》客户端只读提取，由原模型离线渲染。
原始模型与美术归鹰角网络及相关权利方所有；本项目沿用已有同人素材说明。
没有修改客户端文件，没有替换项目已有资源。游戏运行时只加载 PNG 和 JSON。

| 项目 ID | 本地模型 | 客户端包（refs/arts 下） |
|---|---|---|
| slug | enemy_1007_slime | enm_art_base_0.ab |
| soldier | enemy_1002_nsabr | enm_art_base_0.ab |
| crossbow | enemy_1003_ncbow | enm_art_base_0.ab |
| exploder | enemy_1021_bslime | enm_art_3.ab |
| shield | enemy_1006_shield | enm_art_base_0.ab |
| drone | enemy_1005_yokai | enm_art_base_0.ab |

六类客户端精灵表为 10 列 × 4 行、单格 192 × 192；依次为待机、移动、攻击、死亡。
无人机使用 drone.png。盾兵使用 shield_v2.png，保留旧 shield.png；新版补齐非循环动作末帧，记录原始播放时长和各动作朝向。
manifest.json 保存原动作名称和落地锚点，避免不同角色脚底悬空。
客户端分离的 alpha 纹理在提取时合并。

模型身份参考 PRTS 的[源石虫](https://prts.wiki/w/源石虫/spine)、
[士兵](https://prts.wiki/w/士兵/spine)、[弩手](https://prts.wiki/w/弩手/spine)、
[高能源石虫](https://prts.wiki/w/高能源石虫/spine)、[重装防御者](https://prts.wiki/w/重装防御者/spine)。
本次图像实际来源是本地客户端，而非下载上述站点模型。

复现工具：tools/extract_enemy_models.py、tools/bake_enemy_sheets.py、tools/bake_enemy_sheets.html。
提取工具复用已安装的 UnityPy；离线烘焙使用隔离的 Edge、Pixi 6.5.10 和 pixi-spine 3.1.2。
这些仅用于素材制作，未增加 C++ 游戏依赖，未打包进游戏。
