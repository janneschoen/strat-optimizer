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
    lines = file.readlines()
    priceLines = lines[-len(networthValues):]
    for price in priceLines:
        prices.append(float(price.strip()))

if(len(networthValues) <= 0):
    exit(1)

normValue = networthValues[0]
for a in range(len(networthValues)):
    networthValues[a] /= normValue

normValue = prices[0]
for a in range(len(prices)):
   prices[a] /= normValue

plt.figure(figsize=(10, 5))

plt.plot(networthValues, linestyle='-', color='green')
plt.plot(prices, linestyle='-', color='black')

plt.title('value over time')
plt.xlabel('time')
plt.ylabel('networth (green), asset (black)')

plt.grid()

system("hyprctl dispatch workspace 9")

plt.show()