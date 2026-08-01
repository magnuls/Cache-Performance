import matplotlib.pyplot as plt
import numpy as np

x = np.linspace(0, 10, 100)
y = np.sin(x)

fig, ax = plt.subplots()  # one figure, one axes
ax.plot(x, y, label="sin(x)")
ax.set_xlabel("time (s)")
ax.set_ylabel("amplitude")
ax.set_title("Damped signal")
ax.legend()
ax.grid(True)
plt.show()  # or fig.savefig("plot.png", dpi=150)
