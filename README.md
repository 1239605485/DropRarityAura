# 掉落物稀有度光柱

适用于 Terraria Android ARM64 1.4.5.8 和 KernelLoader 的原生 C++ 模组。普通地面物品显示白色光柱，高稀有度物品显示相应颜色的更高光柱。

## 代码分层

- `src/mod_entry.cpp`：加载和清理。
- `src/kernel_api.cpp`：定义供 KernelLoader 注入的 API 指针变量，禁止作为未解析符号链接。
- `src/item_draw_hook.cpp`：挂接地面物品绘制并读取位置、稀有度。
- `src/beam_renderer.cpp`：复用 MagicPixel 原版纹理，以 ARM64 原生签名绘制并裁剪屏幕外效果。
- `src/loot_style.cpp`：独立视觉层，柔光环、渐尖光束、扩散地环、横向星芒、上升星点，按稀有度分级；动画使用实际时间。
- 拾取后检查有效状态、物品类型与数量，立即停止所有绘制，不保留场景粒子。
- `src/rarity_palette.cpp`：稀有度颜色。
- `src/diagnostics.cpp`：在 TEFManager 导出目录写入 droprarityaura_runtime.log。

## 编译和安装

```sh
KERNELLOADER_DIR=/path/to/KernelLoader ANDROID_NDK_HOME=/path/to/ndk bash build.sh android-arm64
```

ZIP 根目录包含 Info.json、Manifest.json、mod.json，原生库必须位于 `Resources/lib/libdrop_rarity_aura.android.arm64.so`。安装器把 Resources 的内容复制到私有目录，加载器再从私有目录的 lib/ 读取库。

构建使用严格链接检查，不允许未定义的 PatchLib 指针变量；Android C++ 运行库静态链接。进游戏后的可视效果仍需设备验证，编译成功不代表已经验证游戏绘制。
