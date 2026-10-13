# 敌方角色素材说明

本目录中的 `crownslayer_battle.png` 为《明日方舟》敌方角色“弑君者”的
战斗美术资源，仅用于本项目的个人学习、课程作业与非商业同人展示。

- 角色与原始美术版权归鹰角网络（Hypergryph）及相关权利方所有。
- 本项目不以任何形式出售这些素材，也不主张对原始美术拥有权利。
- 素材来源：PRTS 收录的敌方弑君者 Spine 模型
  `enemy_1502_crowns`。
- 项目使用兼容 Spine 3.8 的 Pixi Spine 运行时，将原始模型的 `Idle`、
  `Move`、`Attack`、`Disappear`、`Appear` 与 `Die` 动作离线渲染为
  透明逐帧精灵表。
- 上述来源不授予本项目对官方素材的再分发或商业使用权。若公开发布项目，
  需根据权利方规则重新确认授权范围，并优先让使用者自行取得素材。

来源链接：

- https://prts.wiki/w/%E5%BC%91%E5%90%9B%E8%80%85%28%E6%95%8C%E6%96%B9%29/spine
- https://github.com/pixijs-userland/spine

## 主线 Boss 客户端资源

`bosses/crownslayer.png` 与 `bosses/w.png` 来自本地客户端的
`refs/arts/enm_art_base_0.ab`，原始模型分别为 `enemy_1502_crowns` 和
`enemy_1504_cqbw`（敌方 W）。使用 Pixi 6 / Spine 3.8 兼容运行时离线
渲染为透明逐帧精灵表；模型、动作时长、对象 ID 和模型指纹记录在
`bosses/manifest.json`。上述版权和使用范围说明同样适用。

公开技能资料：

- [弑君者（敌方）](https://prts.wiki/w/%E5%BC%91%E5%90%9B%E8%80%85%28%E6%95%8C%E6%96%B9%29)
- [W（敌方）](https://prts.wiki/w/W%28%E6%95%8C%E6%96%B9%29)

## 霜星与爱国者客户端资源

`bosses/frostnova.png`、`bosses/patriot.png` 来自 `refs/arts/enm_art_16.ab` 中的 `enemy_1510_frstar2` 与 `enemy_1506_patrt` 原 Spine 模型。`bosses/effects/` 为 `battle/prefabs/effects/frstar2.ab`、`patrt.ab` 及其依赖的原特效纹理；`bosses/audio/` 为原霜星、霜星进化、爱国者战斗曲和霜星冰环／黑冰技能音效。资源包、对象 ID 和指纹在相应 manifest.json 或 sources.json 中。上述版权归属与个人学习使用范围同样适用。

技能公开资料：[霜星，“冬痕”](https://prts.wiki/w/%E9%9C%9C%E6%98%9F%EF%BC%8C%E2%80%9C%E5%86%AC%E7%97%95%E2%80%9D)、[爱国者](https://prts.wiki/w/%E7%88%B1%E5%9B%BD%E8%80%85)。

## W 的枪口与飞行特效

`bosses/effects/w_mine.png` 是本地客户端 `skinpack/token_10008_cqbw_box.ab` 中 `token_10008_cqbw_box_epoque#7` 地雷模型的完整 Idle 姿态，由原 Spine 模型、图集和分离透明通道渲染得到。使用完整 W 地雷本体，替代此前误用的爆炸碎片展开图 `cqbw_04.png`。原模型指纹、对象 ID、姿态和落地锚点记录在 `effects/manifest.json` 的 `w_mine_idle` 项；可通过 `extract_w_mine.py`、`bake_w_mine.html` 和 `install_w_mine.py` 复现。上述版权归属与个人学习使用范围同样适用。

`bosses/effects/w_client/` 来自本地客户端 `battle/prefabs/effects/cqbw.ab` 及其纹理依赖，提取 `cqbw_attack_01_start`、`cqbw_attack_01_start_02` 和 `cqbw_attack_01_trail_01` 的 6 张原始纹理、10 层粒子曲线。导出入口为 `tools/extract_w_combat_fx.py`。原图像素、透明通道和 60 Hz 采样曲线保留，来源及指纹在 `manifest.json`。

爆炸继续使用 `effects/` 中已有的 W 火焰、烟尘、冲击环、碎片、骷髅与电光。播放方式按横版投影编排，火焰采用原素材的透明混合，发光层采用叠加混合；不代表完整还原 Unity 发射器和着色器。上述版权归属与个人学习使用范围同样适用。

## 弑君者连斩的客户端特效素材

`bosses/effects/crown_client/` 从本地客户端的 `battle/prefabs/effects/red.ab`、`texas2.ab` 及其 `refs/fx/texture/` 依赖提取，包含 19 张原始纹理与 34 层原粒子参数。刀光使用弑君者与红共用的 `daoguang_09` / `daoguang_03`；闪现烟雾、落地破碎和碎光使用原 `smoke_26` / `dilie_12` / `star_54` 等。未使用其中的狼头形象或蓝色花瓣。

贴图保持原始像素和透明通道，没有新绘制图片。来源包指纹、对象 ID、原始纹理名和 PNG 指纹记录在该目录的 `manifest.json`；颜色、尺寸、透明度和图集帧曲线按 60 Hz 记录在 `timelines.json`。播放时适配横版场景的位置、方向、亮度和起手/命中时机，属于使用原素材的二次编排，不代表逐帧复刻完整原技能。上述版权归属与个人学习使用范围同样适用。
