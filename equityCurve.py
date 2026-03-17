import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np

def showEquityCurve(equityPath, stratTypes, stratType):
    with open(equityPath, "r") as file:
        equityValues = [float(eqValue.strip()) for eqValue in file]

    days = list(range(len(equityValues)))

    plt.plot(days, equityValues, color='green')
    plt.title(stratTypes[stratType].name)
    plt.xlabel("Days")
    plt.ylabel("Equity")
    plt.grid(True)

    a, b = np.polyfit(days, equityValues, 1)
    lineX = np.linspace(days[0], days[-1])
    lineY = a * lineX + b
    plt.plot(lineX, lineY, color='black', label='Regression Line')

    plt.show()
