import matplotlib.pyplot as plt
import pandas as pd

"""
fig is the entire canvas
ax is the plot inside the canvas

strip white spacing for df
log_2 of bytes on x axis

log_10 of nanoseconds on y axis


"""
# My implementation
df = pd.read_csv("results.csv")
df = df.map(lambda x: x.strip() if isinstance(x, str) else x)

fig, ax = plt.subplots()
ax.plot(df["size_bytes"], df["ns_per_access"], label="ptr chase")

ax.set_xscale("log", base=2)
ax.set_xlabel("Working Set Size (bytes)")

ax.set_yscale("log")
ax.set_ylabel("Access Latency (ns)")

ax.grid(True)

ax.legend()
plt.show()

#
# size_bytes,label,ns_per_access
# 4096,4K,0.732125
# 8192,8K,0.6730417
# 16384,16K,0.6724375
# 32768,32K,0.6676084
# 65536,64K,0.669075
# 131072,128K,0.6757416
# 262144,256K,5.9182166
# 524288,512K,6.0333875
# 1048576,1M,6.5915167
# 2097152,2M,6.4224917
# 4194304,4M,7.5554792
# 8388608,8M,8.2643875
# 16777216,16M,18.409975
# 33554432,32M,31.6179458
# 67108864,64M,76.1298083
# 134217728,128M,102.818997764587
# 268435456,256M,105.023866891861

"""
# size_bytes, label, ns_per_access
df = pd.read_csv("results.csv", skipinitialspace=True)

print(df)
# y = ns_per_access
# x = scaled logarithmically into size_bytes

"""
