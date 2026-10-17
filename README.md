# 掉落物稀有度光环

LuaLoader / TEFKernel 模组原型。给地面稀有物品显示与稀有度对应的彩色环绕光点，普通白色品质不显示。

## 实现方案

1. 在 `Main.DrawItem(WorldItem, int)` 后置钩子里处理单个当前绘制的掉落物，不扫描整个世界物品数组。
2. 从 `WorldItem.inner.rare` 读取稀有度，只做视觉分类，不修改物品、掉落或存档数据。
3. 调用原版 `Dust.NewDust` 生成尘埃，并为每个稀有度设置独立 RGB 颜色；以节流方式让光点分布在椭圆轨迹附近。
4. 代码拆分为入口/绘制回调和独立稀有度映射，后续可在这两层旁添加设置模块。

## 文件结构

- `Info.json`、`Manifest.json`、`luamod.json`：TEFKernel 与 LuaLoader 模组声明
- `Resources/lib/main.lua`：绘制钩子、节流、结构体构造与清理
- `Resources/lib/rarity_palette.lua`：每个稀有度独立 RGB 映射

## 原型边界

Lua 语法、配置 JSON 和包压缩结构已验证。反射对受保护方法的访问、IL2CPP 下 Vector2/Color 参数装箱，以及游戏内尘埃的视觉表现需要在目标设备实测；此环境没有 Terraria 运行实例，因此尚未宣称游戏内效果已通过验证。

## 导入

导入仓库内生成的 `droprarityaura.tefpkg`，或将目录作为 LuaLoader Mod 内容导入 TEFKernel。
