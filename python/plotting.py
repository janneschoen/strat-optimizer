import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
import numpy as np
import sys
import os
import argparse

STRATS_FILE = 'temp/strategies.temp'

parser = argparse.ArgumentParser(description='Receive name for purpose of plot.')
parser.add_argument('--goal', type=str, required=False, help='what the plot will show')
args = parser.parse_args()
goal = args.goal

vectors = []

with open(STRATS_FILE, 'r', encoding="utf-8") as file:
    for line in file:
        line = line.strip()
        if line:
            vector = line.split()
            vectors.append(vector)

vectors = (np.array(vectors)).astype(float)
#vectors[id][3] = 100

dimensions = len(vectors[0])

if dimensions == 4:
    x = vectors[:, 0]
    y = vectors[:, 1]
    z = vectors[:, 2]
    colors = vectors[:, 3]

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    scatter = ax.scatter(x, y, z, c=colors, cmap='viridis')

    cbar = plt.colorbar(scatter)
    cbar.set_label('Performance')

    ax.set_title(f"{goal} vs 3 parameters")
    ax.set_xlabel('Parameter 1')
    ax.set_ylabel('Parameter 2')
    ax.set_zlabel('Parameter 3')

elif dimensions == 3:
    x = vectors[:, 0]
    y = vectors[:, 1]
    z = vectors[:, 2]

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    scatter = ax.scatter(x, y, z)

    ax.set_title(f"{goal} vs 2 parameters")
    ax.set_xlabel('Parameter 1')
    ax.set_ylabel('Parameter 2')
    ax.set_zlabel('Results')

elif dimensions == 2:
    x = vectors[:, 0]
    y = vectors[:, 1]
    plt.scatter(x, y, color='blue', marker='o')

    plt.title(f"{goal} vs 1 parameter")
    plt.xlabel('Parameter')
    plt.ylabel('Results')
else:
    print("ERROR: only dimensions 2-4 supported.")
    exit(1)


os.system("hyprctl dispatch workspace 9")
plt.show()
