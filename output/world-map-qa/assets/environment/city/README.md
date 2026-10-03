# 切城首版场景资源

原始背景及地块图集只读提取自用户安装的明日方舟客户端，版权归鹰角网络及相关权利方。`sources.json` 记录原资源包、对象 ID、尺寸和包哈希。没有用生成图替换客户端美术。

- `bg_cher*`：切尔诺伯格城区、地下及核心区剧情背景。
- `bg_infirmary`：医疗室背景，用于诊疗所与病房的初版氛围。
- `bg_under*`：地下工业设施背景。
- `bg_towerinside`、`bg_top*`：塔内与核心塔背景。
- `TX_Qcity_Common_A`：切城原始地块图集。运行时取混凝土板、警示条、医疗标识及金属门板区域，适配横向场景。

背景作为远景使用，未宣称对应剧情地点的精确复原。近景结构及平台由项目绘制，原图与新增结构的区别保留在代码中。

重新导出：运行 `tools/extract_world_art.py --client <StreamingAssets/AB/Windows> --output assets/environment/city`。

场景方向参考 PRTS 的[切尔诺伯格 6区废墟](https://prts.wiki/w/切尔诺伯格_6区废墟)与[主题曲关卡](https://prts.wiki/w/关卡一览/主题曲)，实际采用的图像均来自本地客户端。
