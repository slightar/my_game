# 干员素材说明

本目录中的能天使与德克萨斯角色立绘、小人、技能图标和特效为《明日方舟》
相关美术资源，仅用于本项目的个人学习、课程作业与非商业同人展示。

- 角色与原始美术版权归鹰角网络（Hypergryph）及相关权利方所有。
- 本项目不以任何形式出售这些素材，也不主张对原始美术拥有权利。
- `exusiai_portrait.png` 来源：Arknights Terra Wiki 的能天使默认立绘。
- `exusiai_chibi.png` 来源：Arknights-Codex-Pets 对
  `build_char_103_angel` 官方基建 Spine 模型的透明逐帧转换；原始模型目录来自
  Ark-Models。
- `exusiai_battle.png` 来源：PRTS 收录的能天使默认战斗正面 Spine 模型
  `char_103_angel`，使用 Arknights-SD-Viewer 的 3.5/3.6 骨骼数据转换器
  离线渲染为透明逐帧精灵表，包含待机、射击与撤退动作。
- `exusiai_skill_2.png` 与 `exusiai_skill_3.png` 来源：ArknightsResource
  收录的能天使“扫射模式”和“过载模式”游戏技能图标。
- `texas_portrait.png`、`texas_skill_e.png` 与 `texas_skill_q.png` 来源：
  ArknightsResource 收录的德克萨斯立绘、缄默德克萨斯二技能图标及
  德克萨斯二技能图标。
- `texas_chibi.png` 来源：Arknights-Codex-Pets 对德克萨斯官方基建
  Spine 模型的透明逐帧转换，用作本项目的移动、跳跃等动作。
- `texas_battle.png` 从本机客户端 `chararts/char_102_texas.ab` 中的正面战斗
  Spine 模型离线渲染，24×3 透明帧表包含 `Idle`、完整的
  `Attack_Start → Attack_Loop → Attack_End` 和 `Die`。
  `texas_battle.json` 记录原资源对象 ID、哈希、动画时长和统一脚底锚点。
  普通攻击和剑气均使用战斗挥剑；挥剑播放压缩到现有 0.32 秒攻击间隔，
  伤害数值与 0.17 秒判定窗口保持原设定。基建 `Wave` 不再用于攻击。
- `texas_skill2_battle.png` 由 ArknightsResource 收录的缄默德克萨斯官方
  战斗 Spine 模型 `char_1028_texas2` 离线渲染，包含二技能
  `Skill_2_Idle`、`Skill_2_Loop` 与 `Skill_2_End` 动画帧。
- `texas_skill2_composite.png` 与 `texas_skill2_hit_composite.png` 由用户提供的
  1080p60 透明 ProRes 4444 素材
  `缄默德克萨斯_纯刀光_原作优化_1080p60_透明.mov` 和
  `缄默德克萨斯_命中迸射_原作优化_1080p60_透明.mov` 转换为两张同步的
  8×9 透明帧表；每层各保留 65 个有效帧，运行时分别绘制主体刀光和命中碎片。
- `texas_skill2_aura.png`、`texas_skill2_slash_a.png`、
  `texas_skill2_slash_b.png`、`texas_skill2_burst.png`、
  `texas_skill2_arc.png` 与 `texas_skill2_impact.png` 提取自用户本机安装的
  《明日方舟》PC 客户端 `battle/prefabs/effects/texas2.ab`，分别来自官方
  `texas2_lt_11$0` 弧形粒子蒙版与 `texas2_lt_11` 刀光爆点；弧形
  蒙版仅做了中性白处理，以便在运行时恢复原效果的蓝白粒子材质着色。
  暗色拖尾来自官方共享资源 `refs/fx/texture/trail.ab` 中由材质
  `texas2_daoguang_an` 引用的 `trail_47_C`，未修改纹理内容。
- `texas_sword.png` 由 `.battle-staging/texas_atlas.png`（德克萨斯官方战斗 Spine
  图集 `char_102_texas`，512x512）中 `F_Weapon` 区域（atlas 记为 108,468 尺寸
  108x42）裁切得到，仅做「按墨迹主轴转正 + 垂直翻转（剑尖朝下）」的几何变换，
  像素内容未修改。用作二技能「剑雨」的落剑贴图（`src/game.cpp` 的
  `BulletKind::FallingSword` 分支）。
- 上述来源不授予本项目对官方素材的再分发或商业使用权。若公开发布项目，需根据
  权利方规则重新确认授权范围，并优先让使用者自行取得素材。

来源链接：

- https://arknights.wiki.gg/wiki/File:Exusiai.png
- https://github.com/lockon-n/Arknights-Codex-Pets/tree/main/pets/exusiai
- https://github.com/isHarryh/Ark-Models/tree/main/models/103_angel
- https://prts.wiki/w/%E8%83%BD%E5%A4%A9%E4%BD%BF/spine
- https://github.com/nuke777/Arknights-SD-Viewer
- https://github.com/fexli/ArknightsResource/tree/main/skills
- https://github.com/fexli/ArknightsResource/tree/main/spine/char_1028_texas2
