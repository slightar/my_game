# 客户端 UI 素材

从用户本机安装的《明日方舟》客户端只读提取。`sources.json` 记录原始资源包、Sprite 名称和导出对象 ID；原始 PNG 保留不改，页面文字和排版在 C++ 中实现。

使用位置：主页按钮与罗德岛标识、返回按钮、背景纹理、职业图标、战斗面板、技能阴影、脚下朝向环和箭头。资源版权归原权利人所有。

提取脚本：`tools/extract_client_ui.py`，依赖 UnityPy。只在开发时运行，游戏运行时不需要 Python、客户端路径或网络。客户端的 flag-4 数据块使用低半字节 literal 长度、大端 offset 的 LZ4 变体；解码器校验输出长度和回溯范围。

格式研究参考：https://spottori.com/blog/for-kaltsit-i-reversed-arknights-client/
