# 客户端红门：2D 正视精灵

只读提取自用户本地明日方舟客户端。原始美术归鹰角网络及相关权利方所有。

## 来源

- 逻辑预制体：`battle/prefabs/[uc]tiles.ab` → `tile_start`，`_effect = tile_start`。
- 效果预制体：`battle/prefabs/effects/tile.ab` → `tile_start`。
- 模型和材质包：`arts/effects/[pack]map.ab`。
- 原网格：`Start_up`、`Start_down`、`Start_back`。
- 原材质：`[opt]start_end_add`、`[opt]start_end_ab`。
- 原贴图：`[opt]merged_textures`，使用网格本身的 UV 选取红门部分。

`source/` 保留原网格 OBJ、材质属性 JSON 和原始贴图，`manifest.json` 记录 Path ID 与包 SHA-256。
入口图案未重画，未用生成图片替代。

## 2D 转换

按效果预制体的朝向，将源网格正面正交投影到透明 PNG，省去透视顶面和侧面。
`entry_front.png` 为 512×512；游戏取中央 496×496 区域，显示为 132×132，底边对齐地面。
`entry_add.png` 保留同一模型投影的 RGB，供原材质的加色层使用；避免红色透明 texel 在普通混合时丢失。
原版材质包含 Unity 专用混合和粒子特效。2D 版本以透明层、加色层及短暂增亮表现入场，
不运行 Unity 着色器和粒子系统。

重新生成：

```powershell
python tools/extract_red_gate.py --client 'C:/Program Files/Arknights bilibili/games/Arknights/Arknights_Data/StreamingAssets/AB/Windows'
```
