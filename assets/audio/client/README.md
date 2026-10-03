# 客户端原版音效

从本机正式《明日方舟》客户端 `Arknights_Data/StreamingAssets/AB/Windows/audio/sound_beta_2` 的 AudioClip 只读导出，共 20 段，原资源属于原游戏权利方。这里保存解码后的原始 PCM WAV，没有裁切、拉伸、变调或重新合成。

`selection.json` 对应原始 AudioClip 名称；`sources.json` 记录来源包、对象 ID、长度、采样参数及源包和导出文件的 SHA-256。枪械、换弹、剑击等选择了客户端的通用战斗素材，映射到本项目的横版操作；不将通用素材标注为某个干员的专属音效。

| 触发事件 | 文件 |
| --- | --- |
| 实际开枪 / 空仓 | gunshot / empty |
| 成功开始换弹 | reload |
| 玩家实际受伤 / 敌人实际命中 | player_hit / enemy_hit |
| 德克萨斯斩击或剑气 | slash |
| 技能成功启动 / 德克萨斯剑雨 | skill / sword_rain |
| 界面确认、选择、返回 | ui_confirm / ui_select / ui_back |
| 成功切换干员 | operator_switch |
| 进入场景 / 首次实际进入隐藏区域 | scene_enter / hidden_enter |
| 完成调查 | investigate |
| 新敌人波次或 Boss 战开始 / 战斗完成 | encounter / clear |
| 暂停或恢复 / PRTS 对话开场 | pause / narration |
| 封闭病房或未清场出口交互失败 | denied |

音量由现有总音量和音效音量共同控制；各事件的增益、最小间隔、并发上限集中在 `src/audio_system.cpp`。射击最多 4 声、敌人命中最多 3 声、剑击最多 2 声，其他提示最多 1 声。静音立即停止仍在播放的尾音。保持 F 不会重复触发调查完成声；靠近隐藏入口不会发出提示，重复进入已发现区域使用普通切换声。

导出工具依赖 Python UnityPy（本次使用 1.25.3），使用示例：

```powershell
python tools/extract_client_audio.py --client '<客户端 AB/Windows 目录>' --output output/client-audio-catalogue
python tools/extract_client_audio.py --client '<客户端 AB/Windows 目录>' --output assets/audio/client --select assets/audio/client/selection.json --catalogue output/client-audio-catalogue/catalogue.json
python tools/validate_client_audio.py
```

音频设备和实际触发回归：`world_map_smoke.exe --audio-audit`。该程序会重置自身程序目录的测试存档，应复制到独立测试目录，并将该目录的 `assets` 指向项目资源后运行。
