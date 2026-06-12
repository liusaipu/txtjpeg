#!/usr/bin/env python3
"""TxtJPEG: 把 ASCII 浮点文本有损压缩为灰度 JPEG 图像容器。"""

from __future__ import annotations

import argparse
import os
import re
import struct
import sys
import time
from array import array
from pathlib import Path
from typing import Tuple

import numpy as np
from PIL import Image

MAGIC = b"TXTJPEG\0"
VERSION = 1
HEADER_FMT = "<8sB B I I Q d d H"
# 后面依次是 fmt_str（长度由 fmt_len 决定）和 jpeg_len(uint64) + jpeg_data。

CHUNK_SIZE = 1 << 20  # 1 MiB


def _infer_format(path: str) -> str:
    """从文件第一个 token 推断原始浮点格式，例如 '%9.2f,'。"""
    with open(path, "rb") as f:
        data = f.read(4096)
    comma = data.find(b",")
    if comma == -1:
        # 没有逗号时回退到通用格式
        return "%g,"
    token = data[:comma]
    width = len(token)
    dot = token.find(b".")
    if dot == -1:
        decimals = 0
    else:
        decimals = len(token) - dot - 1
    return f"%{width}.{decimals}f,"


def _iter_floats(path: str, chunk_size: int = CHUNK_SIZE):
    """流式迭代文件中的所有浮点数，内存友好。"""
    carry = b""
    with open(path, "rb") as f:
        while True:
            chunk = f.read(chunk_size)
            if not chunk:
                if carry.strip():
                    yield float(carry.strip())
                break
            data = carry + chunk
            last_comma = data.rfind(b",")
            if last_comma == -1:
                carry = data
                continue
            tokens = data[:last_comma].split(b",")
            for tok in tokens:
                if tok:
                    yield float(tok.strip())
            carry = data[last_comma + 1 :]


def _compute_shape(n: int, shape: Tuple[int, int] | None = None) -> Tuple[int, int]:
    """确定二维图像尺寸，宽高均向上对齐到 8（JPEG 宏块）。"""
    if shape is not None:
        h, w = shape
        if h * w < n:
            raise ValueError(
                f"指定的 --shape {h}x{w}={h*w} 小于数据点数 {n}"
            )
        # 仍向上对齐到 8
        h = ((h + 7) // 8) * 8
        w = ((w + 7) // 8) * 8
        return h, w

    w = int(np.ceil(np.sqrt(n)))
    w = max(8, ((w + 7) // 8) * 8)
    h = (n + w - 1) // w
    h = max(8, ((h + 7) // 8) * 8)
    return h, w


def compress(
    input_path: str,
    output_path: str,
    quality: int = 85,
    shape: Tuple[int, int] | None = None,
    subsampling: str = "4:4:4",
) -> dict:
    """压缩文本文件为 .tj 容器。返回统计信息。"""
    t0 = time.time()

    fmt_str = _infer_format(input_path)
    print(f"[info] 检测到原始格式: {fmt_str!r}")

    # 单遍流式读取：收集 float32 数组并统计 min/max
    print("[info] 正在解析浮点数...")
    vals = array("f")
    vmin = float("inf")
    vmax = float("-inf")
    for v in _iter_floats(input_path):
        vals.append(v)
        if v < vmin:
            vmin = v
        if v > vmax:
            vmax = v

    n = len(vals)
    if n == 0:
        raise ValueError("输入文件中没有解析到浮点数")

    values = np.frombuffer(vals, dtype=np.float32).copy()
    del vals  # 释放 array

    print(f"[info] 共 {n} 个浮点数，范围 [{vmin:g}, {vmax:g}]")

    # 归一化到 uint8
    if vmax == vmin:
        pixels = np.zeros(n, dtype=np.uint8)
    else:
        pixels = np.clip(
            (values - vmin) / (vmax - vmin) * 255.0 + 0.5, 0, 255
        ).astype(np.uint8)

    h, w = _compute_shape(n, shape)
    print(f"[info] 图像尺寸: {h}x{w} (数据 {n} 点，填充 {h*w - n} 点)")

    img_arr = np.zeros(h * w, dtype=np.uint8)
    img_arr[:n] = pixels
    img_arr = img_arr.reshape((h, w))

    # 编码 JPEG
    print(f"[info] 正在编码 JPEG (quality={quality}, subsampling={subsampling})...")
    img = Image.fromarray(img_arr, mode="L")
    import io

    jpeg_io = io.BytesIO()
    img.save(
        jpeg_io,
        format="JPEG",
        quality=quality,
        subsampling=subsampling,
        optimize=True,
    )
    jpeg_data = jpeg_io.getvalue()

    # 写入容器
    fmt_bytes = fmt_str.encode("ascii")
    if len(fmt_bytes) > 65535:
        raise ValueError("格式字符串过长")

    header = struct.pack(
        HEADER_FMT,
        MAGIC,
        VERSION,
        quality,
        w,
        h,
        n,
        float(vmin),
        float(vmax),
        len(fmt_bytes),
    )

    with open(output_path, "wb") as f:
        f.write(header)
        f.write(fmt_bytes)
        f.write(struct.pack("<Q", len(jpeg_data)))
        f.write(jpeg_data)

    orig_size = os.path.getsize(input_path)
    comp_size = os.path.getsize(output_path)
    ratio = orig_size / comp_size if comp_size else float("inf")
    elapsed = time.time() - t0

    print(f"[info] 原始大小: {orig_size / 1e6:.2f} MB")
    print(f"[info] 压缩大小: {comp_size / 1e6:.2f} MB")
    print(f"[info] 压缩比:   {ratio:.2f}x")
    print(f"[info] 耗时:     {elapsed:.2f} 秒")

    return {
        "n": n,
        "vmin": vmin,
        "vmax": vmax,
        "width": w,
        "height": h,
        "quality": quality,
        "orig_size": orig_size,
        "comp_size": comp_size,
        "ratio": ratio,
        "elapsed": elapsed,
    }


def decompress(input_path: str, output_path: str) -> dict:
    """从 .tj 容器解压回近似文本文件。"""
    t0 = time.time()

    with open(input_path, "rb") as f:
        header = f.read(struct.calcsize(HEADER_FMT))
        (
            magic,
            version,
            quality,
            w,
            h,
            n,
            vmin,
            vmax,
            fmt_len,
        ) = struct.unpack(HEADER_FMT, header)

        if magic != MAGIC:
            raise ValueError("不是有效的 TxtJPEG 文件")
        if version != VERSION:
            raise ValueError(f"不支持的版本: {version}")

        fmt_str = f.read(fmt_len).decode("ascii")
        (jpeg_len,) = struct.unpack("<Q", f.read(8))
        jpeg_data = f.read(jpeg_len)

    print(f"[info] 容器: {w}x{h}, N={n}, quality={quality}, fmt={fmt_str!r}")

    import io

    img = Image.open(io.BytesIO(jpeg_data)).convert("L")
    arr = np.array(img, dtype=np.uint8).reshape(-1)
    pixels = arr[:n]

    if vmax == vmin:
        values = np.full(n, vmin, dtype=np.float64)
    else:
        values = pixels.astype(np.float64) / 255.0 * (vmax - vmin) + vmin

    print("[info] 正在写回文本...")
    batch = 100000
    with open(output_path, "w", encoding="ascii") as f:
        for i in range(0, n, batch):
            chunk = values[i : i + batch]
            # 按原始格式格式化并连续写入
            f.write("".join(fmt_str % v for v in chunk))

    elapsed = time.time() - t0
    print(f"[info] 解压完成，耗时 {elapsed:.2f} 秒")

    return {
        "n": n,
        "vmin": vmin,
        "vmax": vmax,
        "width": w,
        "height": h,
        "elapsed": elapsed,
    }


def _parse_shape(s: str) -> Tuple[int, int]:
    h, w = s.split("x")
    return int(h), int(w)


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="TxtJPEG：把 ASCII 浮点文本有损压缩为灰度 JPEG 容器。"
    )
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_comp = sub.add_parser("compress", help="压缩文本文件")
    p_comp.add_argument("input", help="输入文本文件")
    p_comp.add_argument("output", help="输出 .tj 容器")
    p_comp.add_argument(
        "--quality",
        type=int,
        default=85,
        help="JPEG 质量 1-100（默认 85）",
    )
    p_comp.add_argument(
        "--shape",
        type=_parse_shape,
        default=None,
        help="显式指定二维形状，格式为 行数x列数，例如 10000x5789（程序会自动对齐到 8 的倍数）",
    )
    p_comp.add_argument(
        "--subsampling",
        choices=["0", "1", "2", "4:4:4", "4:2:2", "4:2:0"],
        default="4:4:4",
        help="JPEG 色度下采样（灰度图影响很小，默认 4:4:4 质量最高）",
    )

    p_decomp = sub.add_parser("decompress", help="解压 .tj 容器")
    p_decomp.add_argument("input", help="输入 .tj 容器")
    p_decomp.add_argument("output", help="输出近似文本文件")

    args = parser.parse_args(argv)

    if args.cmd == "compress":
        if not (1 <= args.quality <= 100):
            parser.error("--quality 必须在 1-100 之间")
        compress(
            args.input,
            args.output,
            quality=args.quality,
            shape=args.shape,
            subsampling=args.subsampling,
        )
    elif args.cmd == "decompress":
        decompress(args.input, args.output)


if __name__ == "__main__":
    main()
