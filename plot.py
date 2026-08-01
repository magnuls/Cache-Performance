import matplotlib.pyplot as plt
import pandas as pd

# My implementation
df = pd.read_csv("results.csv")
# Strip white spacing
df = df.map(lambda x: x.strip() if isinstance(x, str) else x)

fig, ax = plt.subplots()
ax.plot(df["size_bytes", "ns_per_access"], label="f(bytes)")
# log_2 of bytes
ax.set_xscale("log", base=2)
ax.grid(True)


"""
# size_bytes, label, ns_per_access
df = pd.read_csv("results.csv", skipinitialspace=True)

print(df)
# y = ns_per_access
# x = scaled logarithmically into size_bytes

"""
