#!/usr/bin/env python3
"""逐行比较原始文件与解压文件，输出有损压缩精度指标。"""

import re
import sys
from itertools import zip_longest

import numpy as np


def parse_line(line: str) -> np.ndarray:
    """从一行文本中提取所有浮点数。"""
    nums = [float(x) for x in re.findall(r"-?\d+\.\d+", line)]
    return np.array(nums, dtype=np.float64)


def main(raw_path: str, decompressed_path: str):
    total_count = 0
    line_count = 0
    mismatch_shape_lines = 0
    sum_abs = 0.0
    sum_sq = 0.0
    max_abs = 0.0
    max_rel = 0.0
    max_line = 0
    raw_vmax = 0.0

    with open(raw_path, "r", encoding="ascii") as fr, open(
        decompressed_path, "r", encoding="ascii"
    ) as fd:
        for li, (lr, ld) in enumerate(zip_longest(fr, fd), start=1):
            if lr is None or ld is None:
                print(f"[warn] 行数不一致：第 {li} 行仅存在于一侧")
                break

            a = parse_line(lr)
            b = parse_line(ld)

            # 更新原始最大值
            if a.size:
                raw_vmax = max(raw_vmax, np.max(a))

            if a.size != b.size:
                mismatch_shape_lines += 1
                print(f"[warn] 第 {li} 行列数不一致: {a.size} vs {b.size}")
                continue

            if a.size == 0:
                continue

            err = np.abs(a - b)
            line_max = float(err.max())
            if line_max > max_abs:
                max_abs = line_max
                max_line = li

            sum_abs += float(err.sum())
            sum_sq += float((err ** 2).sum())
            total_count += a.size
            line_count += 1

    if total_count == 0:
        print("未解析到可比较的浮点数")
        return

    mae = sum_abs / total_count
    rmse = np.sqrt(sum_sq / total_count)
    rel_max = (max_abs / raw_vmax * 100.0) if raw_vmax else 0.0
    rel_mae = (mae / raw_vmax * 100.0) if raw_vmax else 0.0

    print("\n===== 逐行比较结果 =====")
    print(f"比较行数:          {line_count}")
    print(f"比较浮点数总数:    {total_count}")
    print(f"列数不一致行数:    {mismatch_shape_lines}")
    print(f"原始数值最大值:    {raw_vmax:.4f}")
    print(f"最大绝对误差:      {max_abs:.4f} (出现在第 {max_line} 行)")
    print(f"平均绝对误差 MAE:  {mae:.4f}")
    print(f"均方根误差 RMSE:   {rmse:.4f}")
    print(f"最大相对误差:      {rel_max:.2f}%")
    print(f"平均相对误差:      {rel_mae:.2f}%")
    print("========================")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"用法: python {sys.argv[0]} <raw.txt> <decompressed.txt>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
