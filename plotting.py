import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
from config import RunConfig

def plot(run: RunConfig, performances: List[float], parameter_combos: List[Tuple[float]], metric: str):

    parameter_names = [param.name for param in run.strategy.parameters]

    parameter_combos = np.loadtxt(run.parameter_path)

    number_of_parameters = parameter_combos.shape[1]

    number_of_fixed_parameters = run.parameter_steps.count(0)

    visual_parameters = [p for p in range(run.strategy.number_of_parameters) if run.parameter_steps[p]]

    dimension = run.strategy.number_of_parameters + 1 - number_of_fixed_parameters

    plot_title = f"{run.strategy.name} | {run.asset.ticker} | {run.backtest_length} d"

    if dimension == 4:
        data = [parameter_combos[:,p] for p in visual_parameters]

        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')

        scatter = ax.scatter(*data, c=performances, cmap='viridis')

        cbar = plt.colorbar(scatter)
        cbar.set_label(metric)

        ax.set_title(plot_title)
        ax.set_xlabel(parameter_names[visual_parameters[0]])
        ax.set_ylabel(parameter_names[visual_parameters[1]])
        ax.set_zlabel(parameter_names[visual_parameters[2]])

    elif dimension == 3:

        data = [parameter_combos[:,p] for p in visual_parameters]

        scatter = plt.scatter(*data, c=performances, cmap='viridis')

        plt.colorbar(label=metric)
        plt.title(plot_title)
        plt.xlabel(parameter_names[visual_parameters[0]])
        plt.ylabel(parameter_names[visual_parameters[1]])

    elif dimension == 2:
        data = [parameter_combos[:,p] for p in visual_parameters]

        plt.scatter(*data, performances, color='black', marker='o')
        plt.title(plot_title)
        plt.xlabel(parameter_names[visual_parameters[0]])
        plt.ylabel(metric)
        plt.grid(True)

        a, b = np.polyfit(data[0], perf, 1)
        lineX = np.linspace(data[0][0], data[0][-1])
        lineY = a * lineX + b
        plt.plot(lineX, lineY, color='blue', label='Regression Line')
    else:
        print("Only 1-3 parameters supported for visualisation.")
        return

    plt.show()
