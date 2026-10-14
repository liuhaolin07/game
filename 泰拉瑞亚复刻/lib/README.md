# lib/ —— 手动放置 raylib 的地方

工程默认走 **vcpkg 清单模式**（根目录的 `vcpkg.json`），VS 第一次生成工程时会自动把 raylib
装到 `vcpkg_installed/`。如果这一步失败（没装 vcp��� / 没网 / 编译不过），可以改用手动方式：

## 方式一：跑一下根目录的脚本

双击项目根目录的 **`setup_raylib.bat`**，它会自动下载并初始化 vcpkg，然后安装 raylib。

## 方式二：手动放二进制（不想装 vcpkg 就用这个）

1. 去 <https://github.com/raysan5/raylib/releases> 下载 `raylib-5.5_win64_msvc16.zip`
2. 解压后把这两个文件夹复制到本目录：

```
lib/
├── include/
│   ├── raylib.h
│   ├── rlgl.h
│   └── ...
└── lib/
    ├── raylib.lib
    └── raylib.dll        <- 运行游戏时需要, 放在 exe 旁边或加到 PATH
```

工程文件里已经同时写了两条搜索路径（`$(SolutionDir)lib` 和 `$(SolutionDir)vcpkg_installed\x64-windows`），
放了哪个都能找到，不需要改工程。

## 常见报错对照

| 报错 | 原因 | 解决 |
|---|---|---|
| `无法打开包括文件: "raylib.h"` | raylib 没装上 | 跑 `setup_raylib.bat` 或按上面方式二手动放 |
| `无法打开输入文件: "raylib.lib"` | 同上，或者平台选成了 Win32 | 确认平台选的是 **x64** |
| `LNK1104` / 缺少 `raylib.dll` | 运行时找不到动态库 | 把 `lib\lib\raylib.dll` 复制到 exe 同目录 |
| `vcpkg` 清单还原失败 | 机器没装 vcpkg | 见方式一 / 二 |
