# TxtJPEG

把 ASCII 浮点文本有损压缩为灰度 JPEG 图像容器。适合“只需要保留主要内容、愿意用精度换体积”的日志/权重/结果文件存档。

## 核心思路

1. 从文本中提取所有浮点数。
2. 按 `[vmin, vmax]` 线性映射到 `uint8` 灰度像素。
3. 将一维数列 reshape 成二维图像，用 JPEG 有损压缩。
4. 把 `vmin/vmax`、原始格式模板、图像数据打包进 `.tj` 自包含容器。
5. 解压时反归一化并按原格式输出近似文本。

## 安装依赖

```bash
pip install numpy pillow
```

## 使用

```bash
# 压缩（默认 quality=85）
python txtjpeg.py compress result.txt result.tj --quality 85

# 解压
python txtjpeg.py decompress result.tj result_approx.txt

# 显式指定二维形状（如果知道原始矩阵维度），格式：行数x列数
python txtjpeg.py compress result.txt result.tj --quality 85 --shape 10000x5789
```

### 测试流程示例

```bash
# 1. 生成 20 MB 测试数据（100 列）
python generate_test_data.py

# 2. 压缩（quality=80）
python txtjpeg.py compress test_data_raw.txt test_data_q80.tj --quality 80

# 3. 解压
python txtjpeg.py decompress test_data_q80.tj test_data_decompress.txt

# 4. 逐行比较并计算精度
python compare.py test_data_raw.txt test_data_decompress.txt
```

## 参数说明

- `--quality`：JPEG 质量，1–100，默认 85。越大体积越大、误差越小。
- `--shape HxW`：显式指定二维图像形状；否则自动推断为接近正方形的 8 的倍数。
- `--subsampling`：JPEG 色度下采样，默认 `4:4:4`（质量最高）。灰度图下该选项影响很小。

## 有损说明

- `vmin` 和 `vmax` 分别是输入文件中所有浮点数的**最小值和最大值**，用于把浮点数线性映射到 `0–255` 的灰度像素。
- `uint8` 归一化本身引入约 `(vmax - vmin) / 255` 的量化误差。
- JPEG DCT 量化会进一步平滑细节，quality 越低越明显。
- 解压后的文本与原文件格式一致（空格、逗号、换行），但数值为近似值。
- **不适合需要精确还原的场景**，适合存档、趋势查看、后续近似分析。

## 文件格式

容器 `.tj` 结构：

```
Magic(8) + Version(1) + Quality(1) + Width(4) + Height(4) +
N(8) + vmin(8) + vmax(8) + columns(4) + fmt_len(2) + fmt_str + jpeg_len(8) + jpeg_data
```

`columns` 用于保存原始文件每行包含的浮点数个数，解压时恢复换行；`0` 表示原始文件无换行。

所有整数均为小端序。`jpeg_data` 是一段标准 JPEG，可单独导出查看。
