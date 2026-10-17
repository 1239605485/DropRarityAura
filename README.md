# 掉落物稀有度光环

KernelLoader 原生 C++ 模组。稀有掉落物周围会出现与稀有度对应颜色的光点，普通品质不显示。

## 代码分层

- `src/mod_entry.cpp`：KernelLoader 模组入口和生命周期
- `src/item_draw_hook.cpp`：挂接 `Main.DrawItem`，读取掉落物稀有度并节流生成原版尘埃
- `src/rarity_palette.cpp`：独立稀有度 RGB 映射
- `build.sh`：按目标平台编译并生成可导入 ZIP

## 运行方式

模组清单声明 `parentLoader: eternal.future.kernelloader`，二进制放在 `lib/libdrop_rarity_aura.<platform>.<arch>.so`。

依赖 KernelLoader 头文件与目标平台工具链：

```sh
KERNELLOADER_DIR=/path/to/TEFKernel-KernelLoader ./build.sh linux-x64
KERNELLOADER_DIR=/path/to/TEFKernel-KernelLoader ANDROID_NDK_HOME=/path/to/ndk ./build.sh android-arm64
```

产物位于 `dist/`。压缩包内包含 `Info.json`、`Manifest.json`、`mod.json` 和平台动态库，按 KernelLoader 的 Mod 目录结构导入。

## 当前验证范围

Linux x64 动态库已在本地编译。Android arm64 需 Android NDK。代码依据提供的 Terraria 1.4.5.8 反编译源码和 KernelLoader 1.2 API 编写；`Main.DrawItem` 反射查找、游戏内尘埃颜色和 Android IL2CPP hook 行为仍需进游戏实测。
