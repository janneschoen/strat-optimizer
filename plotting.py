import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

def plot(gridIntervals, stratType, stratTypes, stratPath, performances):

    paramCombos = np.loadtxt(stratPath)

    numParams = paramCombos.shape[1]

    numFixed = gridIntervals.count(0)

    visualParams = [p for p in range(numParams) if gridIntervals[p]]

    dimension = numParams + 1 - numFixed

    if dimension == 4:
        data = [paramCombos[:,p] for p in visualParams]

        perfs = performances

        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')

        scatter = ax.scatter(*data, c=perfs, cmap='viridis')

        cbar = plt.colorbar(scatter)
        cbar.set_label("Performance")

        ax.set_title(stratTypes[stratType].name)
        ax.set_xlabel(stratTypes[stratType].paramNames[visualParams[0]])
        ax.set_ylabel(stratTypes[stratType].paramNames[visualParams[1]])
        ax.set_zlabel(stratTypes[stratType].paramNames[visualParams[2]])

    elif dimension == 3:

        data = [paramCombos[:,p] for p in visualParams]

        perfs = performances

        scatter = plt.scatter(*data, c=perfs, cmap='viridis', s=100)

        plt.colorbar(label="Performance")
        plt.title(stratTypes[stratType].name)
        plt.xlabel(stratTypes[stratType].paramNames[visualParams[0]])
        plt.ylabel(stratTypes[stratType].paramNames[visualParams[1]])

    elif dimension == 2:
        data = [paramCombos[:,p] for p in visualParams]

        perf = performances

        plt.scatter(*data, perf, color='black', marker='o')
        plt.title(stratTypes[stratType].name)
        plt.xlabel(stratTypes[stratType].paramNames[visualParams[0]])
        plt.ylabel("Performance")
        plt.grid(True)

        a, b = np.polyfit(data[0], perf, 1)
        lineX = np.linspace(data[0][0], data[0][-1])
        lineY = a * lineX + b
        plt.plot(lineX, lineY, color='blue', label='Regression Line')
    else:
        print("Only 1-3 parameters supported for visualisation.")
        exit(1)


    plt.show()
