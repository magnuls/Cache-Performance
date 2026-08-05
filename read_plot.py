"""Plotting for  the cache size sweep

latency (ns/access) vs working-set size.

fig is the entire canvas, ax is the plot inside it.

x axis is log base 2
y axis is log base 10

CSV col headers
size_bytes,label,ns_per_access,l1_bytes,l2_bytes,l3_bytes,ram_bytes
"""

from itertools import product
from math import ceil, floor, log10

import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.ticker import FixedLocator, NullLocator, ScalarFormatter

KB = 1024
MB = 1024 * KB
GB = 1024 * MB


def size_label(n):
    if n >= GB:
        return f"{n // GB}G"
    if n >= MB:
        return f"{n // MB}M"
    return f"{n // KB}K"


def decade_ticks(lo, hi, mantissas=(1, 2, 5), pad=1):
    """generates ticks lo <= ticks <= hi, extending pad steps beyond each end"""
    exponents = range(floor(log10(lo)) - 1, ceil(log10(hi)) + 2)
    ticks = sorted(m * 10**e for e, m in product(exponents, mantissas))
    first = max(i for i, t in enumerate(ticks) if t <= lo) - pad
    last = min(i for i, t in enumerate(ticks) if t >= hi) + pad
    return ticks[max(first, 0) : min(last + 1, len(ticks))]


def hardware_boundaries(row):
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


df = pd.read_csv("size_detection.csv")
df = df.map(lambda x: x.strip() if isinstance(x, str) else x)

fig, ax = plt.subplots(figsize=(14, 8), dpi=110)
ax.plot(df["size_bytes"], df["ns_per_access"], label="ptr chase")

ax.set_title("CPU Cache Latency", fontsize=15, pad=14)

# x axis is one tick per measured size, labelled from 4KB ->256MB
xticks = df["size_bytes"]
ax.set_xscale("log", base=2)
ax.set_xticks(xticks)
ax.set_xticklabels([size_label(t) for t in xticks])
ax.set_xlabel("Working Set Size (bytes)")

# y axis is 1-2-5 ticks derived from the data so the plot survives
# new numbers
lo, hi = df.ns_per_access.min(), df.ns_per_access.max()
yticks = decade_ticks(lo / 2, hi * 3)
ax.set_yscale("log")
ax.set_ylim(yticks[0], yticks[-1])
ax.yaxis.set_major_locator(FixedLocator(yticks))
ax.yaxis.set_minor_locator(NullLocator())
ax.yaxis.set_major_formatter(ScalarFormatter())
ax.set_ylabel("Latency (ns per access)")

# Lines drawn directly from system
x_min, x_max = xticks.min(), xticks.max()
for x, name in hardware_boundaries(df.iloc[0]):
    if not x_min <= x <= x_max:
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
# fig.savefig("cache_read_latency.png", dpi=150, bbox_inches="tight")
plt.show()
