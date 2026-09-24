"""Plotting for the sweeps
latency (ns/access) vs the swept axis.
fig is the entire canvas, ax is the plot inside it.
x axis is log base 2
y axis is log base 10

CSV col headers
<x>,label,ns_per_access,threads,l1_bytes,l2_bytes,l3_bytes,ram_bytes

The first column is named after whatever the sweep varied -- size_bytes,
stride_bytes or threads -- so it is always read by position, never by name.

    python3 plot.py                     show the size sweep
    python3 plot.py contention.csv      show one sweep
    python3 plot.py --save docs         write every sweep found to docs/*.png
"""

import os
import sys
from collections.abc import Sequence
from itertools import product
from math import ceil, floor, log10
from typing import Any, Callable, cast

import matplotlib

if "--save" in sys.argv:
    matplotlib.use("Agg")

import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.axes import Axes
from matplotlib.figure import Figure
from matplotlib.ticker import FixedLocator, NullLocator, ScalarFormatter

KB = 1024
MB = 1024 * KB
GB = 1024 * MB

Series = Sequence[float] | pd.Series
Curve = tuple[Series, Series, str]
Boundary = tuple[int, str]

DETECTION_CSV = "detection.csv"


def size_label(n: int) -> str:
    if n >= GB:
        return f"{n // GB}G"
    if n >= MB:
        return f"{n // MB}M"
    if n >= KB:
        return f"{n // KB}K"
    return f"{n}B"


def decade_ticks(
    lo: float, hi: float, mantissas: tuple[int, ...] = (1, 2, 5), pad: int = 1
) -> list[float]:
    exponents = range(floor(log10(lo)) - 1, ceil(log10(hi)) + 2)
    ticks = sorted(m * 10**e for e, m in product(exponents, mantissas))
    first = max(i for i, t in enumerate(ticks) if t <= lo) - pad
    last = min(i for i, t in enumerate(ticks) if t >= hi) + pad
    return ticks[max(first, 0) : min(last + 1, len(ticks))]


def hardware_boundaries(row: pd.Series) -> list[Boundary]:
    levels = [
        (row.l1_bytes, "L1d"),
        (row.l2_bytes, "L2"),
        (row.l3_bytes, "L3"),
        (row.ram_bytes, "RAM"),
    ]
    return [(int(b), f"{name} {size_label(int(b))}") for b, name in levels if b > 0]


def detected_boundaries(quantities: tuple[str, ...]) -> list[Boundary]:
    if not os.path.exists(DETECTION_CSV):
        return []
    df = load(DETECTION_CSV)
    out: list[Boundary] = []
    for _, row in df.iterrows():
        if row.quantity in quantities and row.detected_bytes > 0:
            b = int(row.detected_bytes)
            out.append((b, f"detected {row.quantity} {size_label(b)}"))
    return out


def load(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)
    return df.map(lambda x: x.strip() if isinstance(x, str) else x)


def plot_sweep(
    curves: Sequence[Curve],
    title: str,
    xlabel: str,
    ylabel: str = "Latency (ns per access)",
    spec: Sequence[Boundary] = (),
    detected: Sequence[Boundary] = (),
    ylog: bool = True,
    headroom: float = 3,
) -> tuple[Figure, Axes]:
    fig, ax = plt.subplots(figsize=(14, 8), dpi=110)
    for xs, ys, label in curves:
        ax.plot(xs, ys, marker="o", markersize=3, label=label)

    ax.set_title(title, fontsize=15, pad=14)

    xticks = sorted({int(x) for xs, _, _ in curves for x in xs if (int(x) & (int(x) - 1)) == 0})
    ax.set_xscale("log", base=2)
    ax.set_xticks(xticks)
    ax.set_xticklabels([size_label(t) for t in xticks])
    ax.set_xlabel(xlabel)

    positive = [float(y) for _, ys, _ in curves for y in ys if y > 0]
    if ylog and positive:
        lo, hi = min(positive), max(positive)
        yticks = decade_ticks(lo / 2, hi * headroom)
        ax.set_yscale("log")
        ax.set_ylim(yticks[0], yticks[-1])
        ax.yaxis.set_major_locator(FixedLocator(yticks))
        ax.yaxis.set_minor_locator(NullLocator())
        ax.yaxis.set_major_formatter(ScalarFormatter())
    ax.set_ylabel(ylabel)

    for x, name in spec:
        if not xticks[0] <= x <= xticks[-1]:
            continue
        ax.axvline(x, color="0.4", linestyle="--", linewidth=1, zorder=0)
        ax.annotate(
            name,
            xy=(x, 1),
            xycoords=("data", "axes fraction"),
            xytext=(4, -14),
            textcoords="offset points",
            fontsize=9,
            color="0.3",
        )
    for x, name in detected:
        if not xticks[0] <= x <= xticks[-1]:
            continue
        ax.axvline(x, color="tab:red", linestyle=":", linewidth=1.2, zorder=0)
        ax.annotate(
            name,
            xy=(x, 0),
            xycoords=("data", "axes fraction"),
            xytext=(4, 6),
            textcoords="offset points",
            fontsize=9,
            color="tab:red",
        )

    ax.grid(True, alpha=0.8)
    ax.legend()
    fig.tight_layout()
    return fig, ax


def column(df: pd.DataFrame, name: str) -> pd.Series:
    return cast(pd.Series, df[name])


def x_column(df: pd.DataFrame) -> pd.Series:
    return cast(pd.Series, df[df.columns[0]])


AXIS_LABELS = {
    "size_bytes": "Working Set Size (bytes)",
    "stride_bytes": "Stride (bytes)",
    "threads": "Threads",
}


def x_label(df: pd.DataFrame) -> str:
    return AXIS_LABELS.get(str(df.columns[0]), str(df.columns[0]))


def curves_by(
    df: pd.DataFrame, column_name: str, y: str = "ns_per_access", fmt: str = "{}"
) -> list[Curve]:
    return [
        (x_column(g), column(g, y), fmt.format(key))
        for key, g in sorted(df.groupby(column_name), key=lambda kv: cast(Any, kv[0]))
    ]


def read_plot(path: str = "size_detection.csv") -> tuple[Figure, Axes]:
    df = load(path)
    curves = [(x_column(df), df.ns_per_access, "ptr chase")]
    return plot_sweep(
        curves,
        "CPU Cache Latency -> Read",
        x_label(df),
        spec=hardware_boundaries(df.iloc[0]),
        detected=detected_boundaries(("l1d", "l2")),
    )


def line_plot(path: str = "cache_line_size_detection.csv") -> tuple[Figure, Axes]:
    df = load(path)
    curves = [(x_column(df), df.ns_per_access, "ptr chase, 4 x L2 slots")]
    return plot_sweep(curves, "Random chase latency vs stride", x_label(df))


def write_plot(path: str = "write_detection.csv") -> tuple[Figure, Axes]:
    write = load(path)
    curves: list[Curve] = []
    if os.path.exists("size_detection.csv"):
        read = load("size_detection.csv")
        curves.append((x_column(read), read.ns_per_access, "read"))
    curves.append((x_column(write), write.ns_per_access, "write minus read"))
    return plot_sweep(
        curves,
        "CPU Cache Latency, read vs write-subtracted",
        x_label(write),
        spec=hardware_boundaries(write.iloc[0]),
        ylog=False,
    )


def thread_plot(path: str = "contention.csv") -> tuple[Figure, Axes]:
    df = load(path)
    curves = curves_by(df, "threads", fmt="{} threads")
    return plot_sweep(
        curves,
        "CPU Cache Latency under contention (working set per thread)",
        x_label(df),
        spec=hardware_boundaries(df.iloc[0]),
    )


def false_sharing_plot(path: str = "false_sharing.csv") -> tuple[Figure, Axes]:
    df = load(path)
    curves = [(x_column(df), df.ns_per_access, "2 writers")]
    return plot_sweep(
        curves,
        "False sharing: store cost vs separation between writers",
        "Separation between the two written addresses (bytes)",
        ylabel="Latency (ns per store)",
        detected=detected_boundaries(("line_size",)),
    )


PLOTTERS: dict[str, Callable[[str], tuple[Figure, Axes]]] = {
    "size_detection": read_plot,
    "cache_line_size_detection": line_plot,
    "write_detection": write_plot,
    "contention": thread_plot,
    "false_sharing": false_sharing_plot,
}


def main(argv: list[str]) -> None:
    args = argv[1:]
    save_dir = None
    if "--save" in args:
        i = args.index("--save")
        save_dir = args[i + 1]
        del args[i : i + 2]

    paths = args or [f"{stem}.csv" for stem in PLOTTERS if os.path.exists(f"{stem}.csv")]
    if not args and not save_dir:
        paths = ["size_detection.csv"]

    for path in paths:
        stem = os.path.splitext(os.path.basename(path))[0]
        fig, _ = PLOTTERS[stem](path)
        if save_dir:
            os.makedirs(save_dir, exist_ok=True)
            out = os.path.join(save_dir, f"{stem}.png")
            fig.savefig(out)
            print(out)
        else:
            plt.show()


if __name__ == "__main__":
    main(sys.argv)
