"""
plotting.py — Visualise the grid-search results

Dispatches to the appropriate plot based on the number of
free (non-fixed) parameters:

  1 free parameter  → 2-D scatter plot with linear regression
  2 free parameters → 2-D heatmap (colour = performance metric)
  3 free parameters → 3-D scatter / heatmap

Fixed parameters (step = 0) are excluded from the visualisation.
"""

import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D   # noqa: F401 (registers 3-D projection)
import numpy as np
from .config import RunConfig


def plot(run:           RunConfig,
         performances:  list,
         parameter_combos: list,
         metric:        str):
    """
    Render a figure showing 'metric' as a function of the free
    strategy parameters.
    """

    param_names = [p.name for p in run.strategy.parameters]

    # which parameters actually vary in this run?
    free_idx = [i for i in range(run.strategy.number_of_parameters)
                if run.parameter_steps[i] != 0]

    dim = len(free_idx) + 1   # +1 for the performance axis

    title = (f"{run.strategy.name} | {run.asset.ticker} | "
             f"first {int(run.backtest_length * (1.0 - run.test_size))} d")

    # extract data columns for the free parameters
    data = [[combo[i] for combo in parameter_combos] for i in free_idx]

    if dim == 4:
        # ---- 3-D scatter (3 free parameters) -------------------
        fig = plt.figure()
        ax  = fig.add_subplot(111, projection='3d')
        sc  = ax.scatter(*data, c=performances, cmap='viridis')
        cbar = plt.colorbar(sc)
        cbar.set_label(metric)
        ax.set_title(title)
        ax.set_xlabel(param_names[free_idx[0]])
        ax.set_ylabel(param_names[free_idx[1]])
        ax.set_zlabel(param_names[free_idx[2]])

    elif dim == 3:
        # ---- 2-D heatmap (2 free parameters) -------------------
        sc = plt.scatter(*data, c=performances, cmap='viridis')
        plt.colorbar(sc, label=metric)
        plt.title(title)
        plt.xlabel(param_names[free_idx[0]])
        plt.ylabel(param_names[free_idx[1]])

    elif dim == 2:
        # ---- 2-D scatter (1 free parameter) --------------------
        plt.scatter(data[0], performances, color='black', marker='o')
        plt.title(title)
        plt.xlabel(param_names[free_idx[0]])
        plt.ylabel(metric)
        plt.grid(True)

        # linear trend line
        a, b = np.polyfit(data[0], performances, 1)
        x_line = np.linspace(data[0][0], data[0][-1], 100)
        plt.plot(x_line, a * x_line + b, color='blue',
                 label='Linear fit')
        plt.legend()

    else:
        print("Only 1–3 free parameters are supported for "
              "visualisation.")
        return

    plt.show()
