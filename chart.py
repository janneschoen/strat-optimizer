import matplotlib.pyplot as plt
import numpy as np
from os import system

path = 'temp/networth.temp'

networthValues = []

with open(path, 'r', encoding="utf-8") as file:
    for line in file:
        networthValues.append(float(line.strip()))


plt.figure(figsize=(10, 5))

plt.plot(networthValues, marker='o', linestyle='-', color='green')

plt.title('Networth / Time')
plt.xlabel('Time')
plt.ylabel('Networth')

plt.grid()

system("hyprctl dispatch workspace 9")

plt.show()
