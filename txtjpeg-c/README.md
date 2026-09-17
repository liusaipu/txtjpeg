# TxtJPEG-C

TxtJPEG 的 C 语言实现，保持与 Python 版完全相同的 CLI、容器格式（`.tj`）和 JPEG 数据流，支持在 Linux、macOS、Windows（含 VS2013）下编译运行。

## 特性

- 与 `txtjpeg.py` 生成的 `.tj` 文件互相兼容。
- 命令行参数与 Python 版一致。
- 纯 C 实现，依赖 `libjpeg` / `libjpeg-turbo`。
- 代码采用 C89/C90 风格，可在 VS2013 中直接编译。

## 依赖

- C 编译器（gcc、clang 或 MSVC）
- `libjpeg` 或 `libjpeg-turbo`

## 编译

### Linux / macOS / MSYS2（Makefile）

```bash
cd txtjpeg-c
make
```

### CMake（跨平台）

```bash
cd txtjpeg-c
mkdir build && cd build
cmake ..
make
```

### Visual Studio 2013

1. 安装或编译 `libjpeg-turbo`，得到 `jpeg.lib` 和 `jpeg62.dll`。
2. 在 VS2013 中新建空项目，添加所有 `.c` 文件。
3. 配置附加包含目录和附加库目录指向 libjpeg-turbo。
4. 链接器输入中加入 `jpeg.lib`。
5. 确保运行库与 libjpeg-turbo 编译选项一致（`/MD` 或 `/MT`）。
6. 编译生成 `txtjpeg.exe`，运行时把 `jpeg62.dll` 放在同目录。

## 使用

压缩：

```bash
txtjpeg compress input.txt output.tj --quality 85
```

解压：

```bash
txtjpeg decompress input.tj output_approx.txt
```

显式指定形状：

```bash
txtjpeg compress input.txt output.tj --quality 85 --shape 10000x5789
```

## 与 Python 版互操作

```bash
# C 压缩，Python 解压
txtjpeg compress test_data_raw.txt test_c.tj --quality 85
python ../txtjpeg.py decompress test_c.tj test_py_decomp.txt

# Python 压缩，C 解压
python ../txtjpeg.py compress test_data_raw.txt test_py.tj --quality 85
txtjpeg decompress test_py.tj test_c_decomp.txt
```

## 文件说明

| 文件 | 说明 |
|------|------|
| `compat.h` | VS2013 兼容性宏（snprintf、inline、restrict 等） |
| `txtjpeg.h` | 公共头文件、容器结构体、函数声明 |
| `main.c` | 命令行入口 |
| `parser.c` | 流式浮点解析器 |
| `format.c` | 原始格式模板推断 |
| `shape.c` | 二维图像尺寸计算 |
| `normalize.c` | uint8 归一化与反映射 |
| `jpeg_codec.c` | libjpeg 灰度 JPEG 编解码封装 |
| `container.c` | `.tj` 容器读写 |
| `compress.c` | 压缩流程编排 |
| `decompress.c` | 解压流程编排 |
| `方案文档.md` | 方案说明与效果 |
| `设计文档.md` | 架构与模块设计 |

## 许可证

与主项目一致。
