import matplotlib.pyplot as plt
import numpy as np
from os import system

networthPath = 'temp/networth.temp'
pricePath = 'temp/prices.temp'

networthValues = []
prices = []

with open(networthPath, 'r', encoding="utf-8") as file:
    for line in file:
        networthValues.append(float(line.strip()))

with open(pricePath, 'r', encoding="utf-8") as file:
    lines = (file.readlines())[-len(networthValues):]
    for price in lines:
        prices.append(float(price.strip()))

normFactor = prices[0] / 100

for a in range(len(prices)):
   prices[a] /= normFactor

plt.figure(figsize=(10, 5))

plt.plot(networthValues, linestyle='-', color='green')
plt.plot(prices, linestyle='-', color='black')

plt.title('networth and normed price / time')
plt.xlabel('time')
plt.ylabel('networth')

plt.grid()

system("hyprctl dispatch workspace 9")

plt.show()
