# 主线 Boss 战斗贴图

来自本地《明日方舟》客户端 `refs/arts/enm_art_base_0.ab`（弑君者、W）与 `refs/arts/enm_art_16.ab`（霜星、爱国者），透明逐帧贴图通过客户端 Spine 模型离线渲染生成。

| 文件 | 原始模型 | 行顺序 |
| --- | --- | --- |
| `crownslayer.png` | `enemy_1502_crowns` | Idle、Move、Attack、Disappear、Appear、Die |
| `w.png` | `enemy_1504_cqbw`（敌方 W） | Idle、Move、Attack、Skill、Die |
| `frostnova.png` | `enemy_1510_frstar2`（冬痕） | Idle、Move、Attack、Skill_1、Skill_2、Skill_3、Die |
| `patriot.png` | `enemy_1506_patrt` | Idle_1、Move_1、Attack_1、Idle_2、Move_2、Attack_2、Skill、revive_1、revive_2、revive_3、Die |

标准图集每行 20 帧；弑君者和 W 单格 256 × 256，霜星和爱国者单格 512 × 512。霜星运行时改用 `frostnova_smooth.png/json`，由同一原始骨骼以 30 帧每秒采样，7 段动作共 796 帧；逐帧透明裁切并保留脚底锚点。`manifest.json` 记录原始动作时长、资源包、对象 ID、模型 SHA-256、渲染缩放及公开参考资料。运行时待机可见高度分别为弑君者／W 108、霜星 115、爱国者 175 像素。

角色及素材版权归鹰角网络及相关权利方所有，使用说明见上级目录 `NOTICE.md`。详细玩法和验证见 `docs/主线Boss战.md`。

`effects/` 的 66 张去重原纹理来自 `battle/prefabs/effects/frstar2.ab`、`patrt.ab`、`crowns.ab`、`cqbw.ab` 及依赖，来源与指纹记录于其 manifest.json。`audio/` 含三首原 Boss 战斗曲与两段原霜星技能音效，来源、时长及采样指纹在 sources.json；音乐为原采样转码的 Vorbis。详情见 `docs/霜星与爱国者.md`。
