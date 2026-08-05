from itertools import product
from math import ceil, floor, log10

import matplotlib.pyplot as plt
import pandas as pd
from matplotlib.ticker import FixedLocator, NullLocator, ScalarFormatter

"""
fig is the entire canvas
ax is the plot inside the canvas

strip white spacing for df
log_2 of bytes on x axis

log_10 of nanoseconds on y axis


"""
# size_bytes,label,ns_per_access,l1_bytes,l2_bytes,l3_bytes,ram_bytes
# My implementation

MB = 1024 * 1024
KB = 1024


def size_label(n):
    return f"{n // MB}M" if n >= MB else f"{n // 1024}K"


def decade_ticks(lo, hi, mantissas=(1, 2, 5), pad=1):
    """1-2-5 ticks bracketing [lo, hi], extending pad steps beyond each end."""
    exponents = range(floor(log10(lo)) - 1, ceil(log10(hi)) + 2)
    ticks = sorted(m * 10**e for e, m in product(exponents, mantissas))
    first = max(i for i, t in enumerate(ticks) if t <= lo) - pad
    last = min(i for i, t in enumerate(ticks) if t >= hi) + pad
    return ticks[max(first, 0) : last + 1]


df = pd.read_csv("size_detection.csv")
df = df.map(lambda x: x.strip() if isinstance(x, str) else x)

fig, ax = plt.subplots()
ax.plot(df["size_bytes"], df["ns_per_access"], label="ptr chase")

ax.set_xscale("log", base=2)
ax.set_xlabel("Working Set Size (bytes)")

xticks = df["size_bytes"].iloc[::]
xlabels = [size_label(t) for t in xticks]

ax.set_xticks(xticks)
ax.set_xticklabels(xlabels)

lo, hi = df.ns_per_access.min(), df.ns_per_access.max()

yticks = decade_ticks(lo / 2, hi * 3)
ax.set_ylim(yticks[0], yticks[-1])
ax.set_yscale("log")
ax.yaxis.set_major_locator(FixedLocator(yticks))
ax.yaxis.set_minor_locator(NullLocator())
ax.yaxis.set_major_formatter(ScalarFormatter())
ax.set_ylabel("Latency (ns per access)")

ax.grid(True)

ax.legend()
plt.show()
