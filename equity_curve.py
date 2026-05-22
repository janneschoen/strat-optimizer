import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

def show_equity_curve(equity_path, strategy, config):
    with open(equity_path, "r") as file:
        equity_values = [float(equity_value.strip()) for equity_value in file]

    norming_factor = equity_values[0]
    for i in range(len(equity_values)):
        equity_values[i] /= norming_factor

    days = list(range(len(equity_values)))

    plt.plot(days, equity_values, color='green')
    plt.title(f"{strategy.name} | {config["ticker"]} | {config["backtest_length"]} d")
    plt.xlabel("Days")
    plt.ylabel("Equity")
    plt.grid(True)

    a, b = np.polyfit(days, equity_values, 1)
    lineX = np.linspace(days[0], days[-1])
    lineY = a * lineX + b
    plt.plot(lineX, lineY, color='black', label='Regression Line')

    plt.show()
