import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

def plot(stratType, stratTypes, stratPath, performances):

    paramCombos = np.loadtxt(stratPath)
    dimension = paramCombos.shape[1] + 1

    if dimension == 4:
        p0 = paramCombos[:, 0]
        p1 = paramCombos[:, 1]
        p2 = paramCombos[:, 2]
        perfs = performances

        fig = plt.figure()
        ax = fig.add_subplot(111, projection='3d')

        scatter = ax.scatter(p0, p1, p2, c=perfs, cmap='viridis')

        cbar = plt.colorbar(scatter)
        cbar.set_label(goal)

        ax.set_title(stratTypes[stratType]["name"])
        ax.set_xlabel(stratTypes[stratType]["paramNames"][0])
        ax.set_ylabel(stratTypes[stratType]["paramNames"][1])
        ax.set_zlabel(stratTypes[stratType]["paramNames"][2])

    elif dimension == 3:

        p0 = paramCombos[:, 0]
        p1 = paramCombos[:, 1]
        perfs = performances

        scatter = plt.scatter(p0, p1, c=perfs, cmap='viridis', s=100)

        plt.colorbar(label="Performance")
        plt.title(stratTypes[stratType]["name"])
        plt.xlabel(stratTypes[stratType]["paramNames"][0])
        plt.ylabel(stratTypes[stratType]["paramNames"][1])

    elif dimension == 2:
        p0 = paramCombos[:, 0]
        perf = performances

        plt.scatter(p0, perf, color='blue', marker='o')
        plt.title(stratTypes[stratType]["name"])
        plt.xlabel(stratTypes[stratType]["paramNames"][0])
        plt.ylabel(goal)
    else:
        print("ERROR: only dimensions 2-4 supported.")
        exit(1)


    plt.show()
