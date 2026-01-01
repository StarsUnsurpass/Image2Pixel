# Image2Pixel - 现代版

基于 C++17 和 Qt 6 构建的高性能图片像素化工具。该工具允许用户通过实时控件将普通图片转换为风格化的像素艺术。

## 主要功能

- **实时像素化**：调整滑块时即时预览像素化效果，所见即所得。
- **现代 UI**：基于 Qt Fusion 风格构建的简洁暗色主题界面。
- **高性能**：采用高效的算法直接操作像素数据。
- **交互式控制**：支持缩放和平移，方便查看像素细节。
- **独立部署**：集成 `windeployqt` 自动化支持，生成的程序可在 Windows 上直接运行，无需手动配置 DLL。

## 项目信息

- **作者**: [StarsUnsurpass](https://github.com/StarsUnsurpass)
- **项目主页**: [https://github.com/StarsUnsurpass/Image2Pixel](https://github.com/StarsUnsurpass/Image2Pixel)

## 环境要求

- **Qt 6.x** (本项目在 Qt 6.10.1 MinGW 环境下测试通过，兼容 Qt 5)
- **CMake 3.16+**
- **支持 C++17 的编译器** (MinGW, GCC, Clang, 或 MSVC)

## 安装与构建

### Windows (推荐使用 CLion)

1.  在 CLion 中打开项目文件夹。
2.  **配置 CMake**：
    由于项目移除了硬编码路径，您需要在 CLion 中指定 Qt 的安装位置。
    *   打开 **Settings** (Ctrl+Alt+S) -> **Build, Execution, Deployment** -> **CMake**。
    *   在 **CMake options** 中添加：
        ```text
        -DCMAKE_PREFIX_PATH="E:/DevTools/QT/6.10.1/mingw_64"
        ```
        *(注：请根据您实际的 Qt 安装路径修改上述地址)*
3.  点击 **Reload CMake Project** 刷新项目。
4.  点击 **Run** (绿色播放键)。构建系统会自动检测 `windeployqt` 并部署所需的 DLL 文件。

### 手动构建 (命令行)

如果您使用命令行或其他 IDE：

```bash
mkdir build
cd build
# 请将路径替换为您实际的 Qt 安装路径
cmake -DCMAKE_PREFIX_PATH="……/QT/6.10.1/mingw_64" ..
cmake --build .
```

## 使用说明

1.  点击 **Open Image** 加载本地图片（支持 JPG, PNG, BMP 等格式）。
2.  拖动 **Block Size** 滑块调整马賽克方块的大小。
3.  使用 **Zoom In/Out** 按钮或鼠标滚轮缩放图片。
4.  点击 **Save Image** 将处理后的结果保存到本地。
5.  点击菜单栏的 **Help -> About** 查看项目和作者信息。

## 许可证

本项目开源并遵循 MIT 许可证。