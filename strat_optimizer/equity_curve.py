"""
equity_curve.py — Plot the equity curve

Receives the equity curve array directly from the C engine (no file
I/O).  Normalises so the initial portfolio value is 1.0 and overlays
a linear regression trend line.
"""

import matplotlib.pyplot as plt
import numpy as np
from .config import RunConfig


def show_equity_curve(run: RunConfig, equity: np.ndarray):
    """
    Plot the equity curve, normalised to start at 1.0, with a
    linear regression to highlight the overall trend.
    """

    # normalise to an initial value of 1.0
    norm = equity[0]
    equity = equity / norm

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
