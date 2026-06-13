#!/usr/bin/env python3
"""对比不同 JPEG quality 下的压缩体积与解压精度。"""

import os
import subprocess
import sys
import time

from compare import compare_files

RAW = "test_data_raw.txt"
QUALITIES = [50, 60, 70, 80, 90, 100]


def ensure_raw():
    if not os.path.exists(RAW):
        print(f"{RAW} 不存在，先生成测试数据...")
        subprocess.run([sys.executable, "generate_test_data.py"], check=True)


def run_quality(q: int):
    tj = f"test_data_q{q}.tj"
    out = f"test_data_q{q}_out.txt"

    # compress
    t0 = time.time()
    subprocess.run(
        [sys.executable, "txtjpeg.py", "compress", RAW, tj, "--quality", str(q)],
        check=True,
        stdout=subprocess.DEVNULL,
    )
    comp_time = time.time() - t0

    # decompress
    t0 = time.time()
    subprocess.run(
        [sys.executable, "txtjpeg.py", "decompress", tj, out],
        check=True,
        stdout=subprocess.DEVNULL,
    )
    decomp_time = time.time() - t0

    # compare
    stats = compare_files(RAW, out)

    orig_size = os.path.getsize(RAW)
    comp_size = os.path.getsize(tj)
    ratio = orig_size / comp_size

    return {
        "quality": q,
        "orig_size_mb": orig_size / 1e6,
        "comp_size_mb": comp_size / 1e6,
        "ratio": ratio,
        "comp_time": comp_time,
        "decomp_time": decomp_time,
        **stats,
    }


def main():
    ensure_raw()
    print("\n===== TxtJPEG quality 对比测试 =====\n")
    print(f"原始文件: {RAW}, 大小: {os.path.getsize(RAW)/1e6:.2f} MB\n")

    results = []
    for q in QUALITIES:
        print(f"[run] quality={q} ...")
        results.append(run_quality(q))

    print("\n===== 测试结果汇总 =====")
    header = (
        f"{'quality':>8} | {'size(MB)':>10} | {'ratio':>8} | "
        f"{'max_err':>10} | {'MAE':>10} | {'RMSE':>10} | "
        f"{'rel_max%':>9} | {'rel_MAE%':>9} | {'comp_t':>8} | {'decomp_t':>9}"
    )
    print(header)
    print("-" * len(header))
    for r in results:
        print(
            f"{r['quality']:>8} | {r['comp_size_mb']:>10.2f} | {r['ratio']:>8.2f}x | "
            f"{r['max_abs']:>10.4f} | {r['mae']:>10.4f} | {r['rmse']:>10.4f} | "
            f"{r['rel_max']:>9.2f} | {r['rel_mae']:>9.2f} | {r['comp_time']:>8.2f} | {r['decomp_time']:>9.2f}"
        )
    print("========================\n")


if __name__ == "__main__":
    main()
