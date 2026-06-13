#!/usr/bin/env python3
"""逐行比较原始文件与解压文件，输出有损压缩精度指标。"""

import re
import sys
from itertools import zip_longest
from typing import Dict

import numpy as np


def parse_line(line: str) -> np.ndarray:
    """从一行文本中提取所有浮点数。"""
    nums = [float(x) for x in re.findall(r"-?\d+\.\d+", line)]
    return np.array(nums, dtype=np.float64)


def compare_files(raw_path: str, decompressed_path: str) -> Dict:
    """逐行比较两个文件，返回各项精度指标。"""
    total_count = 0
    line_count = 0
    mismatch_shape_lines = 0
    sum_abs = 0.0
    sum_sq = 0.0
    max_abs = 0.0
    max_line = 0
    raw_vmax = 0.0

    with open(raw_path, "r", encoding="ascii") as fr, open(
        decompressed_path, "r", encoding="ascii"
    ) as fd:
        for li, (lr, ld) in enumerate(zip_longest(fr, fd), start=1):
            if lr is None or ld is None:
                mismatch_shape_lines += 1
                continue

            a = parse_line(lr)
            b = parse_line(ld)

            if a.size:
                raw_vmax = max(raw_vmax, np.max(a))

            if a.size != b.size:
                mismatch_shape_lines += 1
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
        return {
            "line_count": 0,
            "total_count": 0,
            "mismatch_shape_lines": mismatch_shape_lines,
            "max_abs": 0.0,
            "mae": 0.0,
            "rmse": 0.0,
            "rel_max": 0.0,
            "rel_mae": 0.0,
        }

    mae = sum_abs / total_count
    rmse = np.sqrt(sum_sq / total_count)
    rel_max = (max_abs / raw_vmax * 100.0) if raw_vmax else 0.0
    rel_mae = (mae / raw_vmax * 100.0) if raw_vmax else 0.0

    return {
        "line_count": line_count,
        "total_count": total_count,
        "mismatch_shape_lines": mismatch_shape_lines,
        "max_abs": max_abs,
        "max_line": max_line,
        "mae": mae,
        "rmse": rmse,
        "rel_max": rel_max,
        "rel_mae": rel_mae,
    }


def print_results(stats: Dict) -> None:
    print("\n===== 逐行比较结果 =====")
    print(f"比较行数:          {stats['line_count']}")
    print(f"比较浮点数总数:    {stats['total_count']}")
    print(f"列数不一致行数:    {stats['mismatch_shape_lines']}")
    print(f"最大绝对误差:      {stats['max_abs']:.4f} (出现在第 {stats['max_line']} 行)")
    print(f"平均绝对误差 MAE:  {stats['mae']:.4f}")
    print(f"均方根误差 RMSE:   {stats['rmse']:.4f}")
    print(f"最大相对误差:      {stats['rel_max']:.2f}%")
    print(f"平均相对误差:      {stats['rel_mae']:.2f}%")
    print("========================")


def main(raw_path: str, decompressed_path: str):
    stats = compare_files(raw_path, decompressed_path)
    print_results(stats)


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"用法: python {sys.argv[0]} <raw.txt> <decompressed.txt>")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2])
