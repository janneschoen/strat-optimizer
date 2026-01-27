import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import sys
import os

path = 'strategies.txt'

vectors = []

with open(path, 'r', encoding="utf-8") as file:
    for line in file:
        line = line.strip()
        if line:
            vector = line.split()
            vectors.append(vector)

vectors = (np.array(vectors)).astype(float)

dimensions = len(vectors[0])

if dimensions == 4:
    x = vectors[:, 0]
    y = vectors[:, 1]
    z = vectors[:, 3]
    colors = vectors[:, 2]

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    scatter = ax.scatter(x, y, z, c=colors, cmap='viridis')

    cbar = plt.colorbar(scatter)
    cbar.set_label('Parameter 3')

    ax.set_title('Strategy Performance (3 Parameters)')
    ax.set_xlabel('Parameter 1')
    ax.set_ylabel('Parameter 2')
    ax.set_zlabel('Results')

elif dimensions == 3:
    x = vectors[:, 0]
    y = vectors[:, 1]
    z = vectors[:, 2]

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    scatter = ax.scatter(x, y, z)

    ax.set_title('Strategy Performance (2 Parameters)')
    ax.set_xlabel('Parameter 1')
    ax.set_ylabel('Parameter 2')
    ax.set_zlabel('Results')

elif dimensions == 2:
    x = vectors[:, 0]
    y = vectors[:, 1]
    plt.scatter(x, y, color='blue', marker='o')

    plt.title('Strategy Performance (1 Parameter)')
    plt.xlabel('Parameter')
    plt.ylabel('Results')
else:
    print("ERROR: only dimensions 2-4 supported.")
    exit(1)


os.system("hyprctl dispatch workspace 9")
plt.show()
