"""Plotting for  the cache size sweep
latency (ns/access) vs working-set size.
fig is the entire canvas, ax is the plot inside it.
x axis is log base 2
y axis is log base 10

CSV col headers
size_bytes,label,ns_per_access,l1_bytes,l2_bytes,l3_bytes,ram_bytes
"""

from collections.abc import Sequence
from itertools import product
from math import ceil, floor, log10
from typing import Any, cast

import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.axes import Axes
from matplotlib.figure import Figure
from matplotlib.ticker import FixedLocator, NullLocator, ScalarFormatter

KB = 1024
MB = 1024 * KB
GB = 1024 * MB

# pandas hands back Series, not list, so the curve endpoints stay deliberately wide
Series = Sequence[float] | pd.Series
Curve = tuple[Series, Series, str]


def size_label(n: int) -> str:
    if n >= GB:
        return f"{n // GB}G"
    if n >= MB:
        return f"{n // MB}M"
    return f"{n // KB}K"


def decade_ticks(
    lo: float, hi: float, mantissas: tuple[int, ...] = (1, 2, 5), pad: int = 1
) -> list[float]:
    """generates ticks lo <= ticks <= hi, extending pad steps beyond each end"""
    exponents = range(floor(log10(lo)) - 1, ceil(log10(hi)) + 2)
    ticks = sorted(m * 10**e for e, m in product(exponents, mantissas))
    first = max(i for i, t in enumerate(ticks) if t <= lo) - pad
    last = min(i for i, t in enumerate(ticks) if t >= hi) + pad
    return ticks[max(first, 0) : min(last + 1, len(ticks))]


def hardware_boundaries(row: pd.Series) -> list[tuple[int, str]]:
    """(bytes, name) for every cache level derived from machine
    l3_bytes is -1 on Apple Silicon (no true L3, instead we have a SLC which is
    not exposed from sysctl), so it drops out here rather than being special case
    """
    levels = [
        (row.l1_bytes, "L1d"),
        (row.l2_bytes, "L2"),
        (row.l3_bytes, "L3"),
        (row.ram_bytes, "RAM"),
    ]
    return [(int(b), f"{name} {size_label(int(b))}") for b, name in levels if b > 0]


def load(path: str) -> pd.DataFrame:
    df = pd.read_csv(path)
    return df.map(lambda x: x.strip() if isinstance(x, str) else x)


def plot_sweep(
    curves: Sequence[Curve],
    hw_row: pd.Series,
    title: str,
    ylabel: str = "Latency (ns per access)",
    headroom: float = 3,
) -> tuple[Figure, Axes]:
    """Draw one or more latency curves on a shared log/log axis.

    curves  -> list of (xs, ys, label); every curve shares the x axis
    hw_row  -> any row of the frame, used for the l1/l2/l3/ram columns
    headroom-> multiplier on the largest y value, sets the top of the axis

    Returns (fig, ax) so the caller can save, tweak, or show it.
    """
    fig, ax = plt.subplots(figsize=(14, 8), dpi=110)
    for xs, ys, label in curves:
        ax.plot(xs, ys, label=label)

    ax.set_title(title, fontsize=15, pad=14)

    # x axis is one tick per measured size which is labelled from 4KB -> 256MB
    xticks = sorted({int(x) for xs, _, _ in curves for x in xs})
    ax.set_xscale("log", base=2)
    ax.set_xticks(xticks)
    ax.set_xticklabels([size_label(t) for t in xticks])
    ax.set_xlabel("Working Set Size (bytes)")

    # y axis is 1-2-5 ticks derived from the data so the plot survives
    # new numbers
    lo = min(min(ys) for _, ys, _ in curves)
    hi = max(max(ys) for _, ys, _ in curves)
    yticks = decade_ticks(lo / 2, hi * headroom)
    ax.set_yscale("log")
    ax.set_ylim(yticks[0], yticks[-1])
    ax.yaxis.set_major_locator(FixedLocator(yticks))
    ax.yaxis.set_minor_locator(NullLocator())
    ax.yaxis.set_major_formatter(ScalarFormatter())
    ax.set_ylabel(ylabel)

    # Lines drawn directly from hardware specs
    for x, name in hardware_boundaries(hw_row):
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

    ax.grid(True, alpha=0.8)
    ax.legend()
    fig.tight_layout()
    return fig, ax


def column(df: pd.DataFrame, name: str) -> pd.Series:
    """df[name] types as Series | DataFrame; a str key always gives a Series."""
    return cast(pd.Series, df[name])


def curves_by(
    df: pd.DataFrame, column_name: str, y: str = "ns_per_access", fmt: str = "{}"
) -> list[Curve]:
    """One curve per distinct value in `column`, sorted."""
    return [
        (column(g, "size_bytes"), column(g, y), fmt.format(key))
        for key, g in sorted(df.groupby(column_name), key=lambda kv: cast(Any, kv[0]))
    ]


def read_plot(path: str = "size_detection.csv") -> tuple[Figure, Axes]:
    df = load(path)
    curves = [(df.size_bytes, df.ns_per_access, "ptr chase")]
    return plot_sweep(curves, df.iloc[0], "CPU Cache Latency -> Read")


def write_plot(path: str = "size_detection.csv") -> tuple[Figure, Axes]:
    """Both modes on one axis
    We expect the write curve to sit above read past L1."""
    df = load(path)
    curves = curves_by(df, "mode", fmt="{}")
    return plot_sweep(curves, df.iloc[0], "CPU Cache Latency, read vs write")


def thread_plot(
    path: str = "results_mt.csv", y: str = "ns_mean"
) -> tuple[Figure, Axes]:
    """One curve per thread count. Curves overlap in private L1, fan out in shared L2."""
    df = load(path)
    curves = curves_by(df, "threads", y=y, fmt="{} threads")
    return plot_sweep(curves, df.iloc[0], "CPU Cache Latency under contention")


def main() -> None:
    read_plot()
    plt.show()


if __name__ == "__main__":
    main()
