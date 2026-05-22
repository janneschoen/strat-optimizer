import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

def plot(parameter_steps, strategy, parameter_path, performances, config):

    parameter_names = [p["name"] for p in strategy.parameters]

    parameter_combos = np.loadtxt(parameter_path)

    number_of_parameters = parameter_combos.shape[1]

    number_of_fixed_parameters = parameter_steps.count(0)

    visual_parameters = [p for p in range(number_of_parameters) if parameter_steps[p]]

    dimension = number_of_parameters + 1 - number_of_fixed_parameters

    plot_title = f"{strategy.name} | {config["ticker"]} | {config["backtest_length"]} d"

    if dimension == 4:
        data = [parameter_combos[:,p] for p in visual_parameters]

        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')

        scatter = ax.scatter(*data, c=performances, cmap='viridis')

        cbar = plt.colorbar(scatter)
        cbar.set_label("Performance")

        ax.set_title(plot_title)
        ax.set_xlabel(parameter_names[visual_parameters[0]])
        ax.set_zlabel(parameter_names[visual_parameters[2]])
        ax.set_ylabel(parameter_names[visual_parameters[1]])

    elif dimension == 3:

        data = [parameter_combos[:,p] for p in visual_parameters]

        scatter = plt.scatter(*data, c=performances, cmap='viridis', s=100)

        plt.colorbar(label="Performance")
        plt.title(plot_title)
        plt.xlabel(parameter_names[visual_parameters[0]])
        plt.ylabel(parameter_names[visual_parameters[1]])

    elif dimension == 2:
        data = [parameter_combos[:,p] for p in visual_parameters]

        plt.scatter(*data, performances, color='black', marker='o')
        plt.title(plot_title)
        plt.xlabel(parameter_names[visual_parameters[0]])
        plt.ylabel("Performance")
        plt.grid(True)

        a, b = np.polyfit(data[0], perf, 1)
        lineX = np.linspace(data[0][0], data[0][-1])
        lineY = a * lineX + b
        plt.plot(lineX, lineY, color='blue', label='Regression Line')
    else:
        print("Only 1-3 parameters supported for visualisation.")
        return

    plt.show()
