"""
equity_curve.py — Plot the equity curve of a single-parameter run

The C engine only saves an equity curve when number_of_combinations==1.
This module reads that curve from the temp file, normalises it so the
initial portfolio value is 1.0, and overlays a linear regression line
to highlight the overall trend.
"""

import matplotlib.pyplot as plt
import numpy as np
from .config import RunConfig


def show_equity_curve(run: RunConfig):
    """
    Read the equity curve written by the C engine and plot it
    against time, normalised to start at 1.0.
    """

    with open(run.equity_path, "r") as f:
        equity = [float(line.strip()) for line in f]

    # normalise to an initial value of 1.0
    norm = equity[0]
    equity = [v / norm for v in equity]

    days = list(range(len(equity)))

    plt.plot(days, equity, color='green', linewidth=1.0)
    plt.title(
        f"{run.strategy.name} | {run.asset.ticker} | "
        f"last {int(run.backtest_length * run.test_size)} d"
    )
    plt.xlabel("Trading days")
    plt.ylabel("Portfolio value (normalised)")
    plt.grid(True, alpha=0.3)

    # linear regression to show the trend
    a, b = np.polyfit(days, equity, 1)
    x_line = np.linspace(days[0], days[-1], 100)
    plt.plot(x_line, a * x_line + b, color='black',
             linewidth=1.5, label='Linear trend')

    plt.legend()
    plt.show()
