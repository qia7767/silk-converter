# SILK 音频转换器

Windows 单文件转换工具：将 SILK V3 语音转换为 MP3、WAV、OGG 或 FLAC。支持拖入多个文件和添加文件夹（仅处理文件夹内的文件，不递归）。

## 使用

从 [Releases](https://github.com/qia7767/silk-converter/releases/latest) 下载 `SilkConverter.exe`，双击运行，添加音频并选择输出格式，点击「开始转换」。结果保存在原文件旁；已有同名输出不会覆盖。也支持 `SilkConverter.exe "输入文件.silk" mp3`，成功退出码为 0，失败为 1。

仅需一个 EXE；SILK 解码器与 FFmpeg 已嵌入其中，运行转换时会释放到当前用户的临时文件夹并在完成后尝试清理。需要 64 位 Windows 10 或更高版本及 .NET Framework 4.x。程序未签名，安全软件仍有可能误报。

## 重新编译

1. 安装 MinGW GCC；Windows 需要 .NET Framework 4.x 的 C# 编译器。
2. 从 [Gyan FFmpeg Windows builds](https://www.gyan.dev/ffmpeg/builds/) 获取 9.1 Essentials 构建，将其中的 `bin/ffmpeg.exe` 放在项目根目录。构建包 `ffmpeg-9.1-essentials_build.7z` 的 SHA-256 是 `835b9a733445ba7a56a37a26cf7bde84e2f3ce7da2a8ec984276bb2abb1b922c`。
3. 在 PowerShell 运行 `./build.ps1`，生成 `SilkConverter.exe`。

构建脚本先编译 SILK C 解码器，再将解码器和 FFmpeg 压缩嵌入 C# 程序。`ffmpeg.exe`、`decoder.exe` 和中间 `.gz` 文件无需随成品分发。

## 致谢

感谢 [kn007/silk-v3-decoder](https://github.com/kn007/silk-v3-decoder) 的作者及贡献者。本项目基于该项目提供的 SILK 解码源码开发，感谢他们的开源分享。

## 来源与许可

- SILK SDK 源码来自 [kn007/silk-v3-decoder](https://github.com/kn007/silk-v3-decoder)，提交 `507be6bca8ce1fb977a061481f1d79e8c610e309`；保留在 `vendor/silk`，原仓库许可见 `vendor/UPSTREAM-LICENSE.txt`，SDK 本身的许可声明见各源码文件。
- 内嵌 FFmpeg 9.1 Essentials 为 Gyan 构建，按 GPL v3 分发。许可文本见 [FFMPEG-LICENSE.txt](FFMPEG-LICENSE.txt)，构建配置和对应源码链接见 [FFMPEG-README.txt](FFMPEG-README.txt)。
- 本项目新增代码按 GPL v3 分发，见 [LICENSE](LICENSE)。
