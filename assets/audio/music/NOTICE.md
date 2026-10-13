# 页面背景音乐

这些音乐来自本机已安装的《明日方舟》客户端，只读提取 AudioClip 的原循环段，并转码为 Vorbis q5。未生成或重新创作音乐。

| 文件 | 客户端片段 | 用途 |
| --- | --- | --- |
| home.ogg | m_sys_void_loop | 启动、主页、设置：冷清、悬疑的基调 |
| planning.ogg | m_dia_darkness02_loop | 编队、装备、地图、关卡选择、开发者房间待机：紧张的行动准备氛围 |
| quiet.ogg | m_dia_darkness01_loop | 档案、结局、无敌人的探索场景：废城探索的压抑氛围 |
| battle.ogg | m_bat_normalbattle_loop | 普通战斗、弑君者与 W 战斗 |

来源包路径、原始音频哈希和时长记录在 sources.json。复现工具为 tools/extract_page_music.py。霜星与爱国者继续使用各自已有战斗配乐。

2026-10-05：根据非战斗音乐过于安逸的反馈，替换原 loading、tech、nightoflongmen 三首循环段。上表氛围为选曲意图；保留客户端原循环音频，不以变速、降调制造紧张感。可用 `--tracks home planning quiet` 只重新提取非战斗音乐。
