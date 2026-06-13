#!/usr/bin/env python3
"""生成 20 MB 左右的测试数据文件 test_data_raw.txt。"""

import os

import numpy as np

COLUMNS = 100
TARGET_BYTES = 20 * 1024 * 1024  # 20 MiB
TOKEN_BYTES = 10  # 例如 "     0.00,"

# 计算需要的 token 总数（向下取整到完整行）
total_tokens = TARGET_BYTES // TOKEN_BYTES
LINES = total_tokens // COLUMNS
N = LINES * COLUMNS
actual_bytes = N * TOKEN_BYTES

print(f"生成 {LINES} 行 x {COLUMNS} 列 = {N} 个浮点数")
print(f"目标大小: {TARGET_BYTES / 1e6:.2f} MB, 实际大小: {actual_bytes / 1e6:.2f} MB")

np.random.seed(42)
values = np.random.rand(N).astype(np.float32) * 1000.0

# 让 10% 的值为 0，模拟稀疏/边界区域
zero_indices = np.random.choice(N, size=N // 10, replace=False)
values[zero_indices] = 0.0

with open("test_data_raw.txt", "w", encoding="ascii") as f:
    for i in range(0, N, COLUMNS):
        line = "".join(f"{v:9.2f}," for v in values[i : i + COLUMNS])
        f.write(line + "\n")

print(f"已保存到 test_data_raw.txt，实际大小: {os.path.getsize('test_data_raw.txt') / 1e6:.2f} MB")
