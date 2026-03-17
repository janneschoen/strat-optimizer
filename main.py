import json
from prices import downloadPrices
from plotting import plot
import subprocess
import itertools, numpy as np
import os

class StrategyType:
    def __init__(self, name, numParams, paramNames, lookbackParam):
        self.name = name
        self.numParams = numParams
        self.paramNames = paramNames
        self.lookbackParam = lookbackParam

    def isValid(self, p):
        if len(p) != self.numParams:
            return False
        if self.name == "SMA Crossover":
            if p[0] >= p[1] or p[1] < 2 or p[2] <= 0 or p[2] > 1:
                return False
        if self.name == "RSI":
            if p[2] < 2 or p[0] < 0 or p[1] > 100 or p[0] > p[1]:
                return False
        return True

strategyTypes = [
    StrategyType(
        name = "SMA Crossover",
        numParams = 3,
        paramNames = ["Fast SMA Length", "Slow SMA Length", "Position Sizing"],
        lookbackParam = 1
    ),
    StrategyType(
        name = "RSI",
        numParams = 3,
        paramNames = ["Buying Threshold", "Selling Threshold", "Window Size"],
        lookbackParam = 2,
    )
]
def main():

    with open('config.json', 'r') as file:
        config = json.load(file)

    os.makedirs(config["tempDir"], exist_ok=True)
    tempDir = config["tempDir"] + "/"
    pricePath = tempDir + config["priceFile"]
    stratPath = tempDir + config["stratFile"]
    perfPath = tempDir + config["resultFile"]

    params = config["params"]
    stratType = config["stratType"]
    btLength = config["btLength"]
    gridIntervals = config["paramSteps"]
    fullYear = config["fullYear"]

    # Downloading price data
    lookback = params[strategyTypes[stratType].lookbackParam]
    lookback = lookback[1] if len(lookback) > 1 else lookback[0]

    numPrices = btLength + lookback
    downloadPrices(numPrices, config)

    # Generating parameter combinations if steps provided
    paramCombos = []

    if gridIntervals.count(0) == len(params):
        paramCombos.append([param[0] for param in params])
    else:
        if len(gridIntervals) != len(params):
            print("Error: please provide exactly one interval per parameter.")
            exit(1)

        paramRanges = []
        for paramRange in params:
            gridInterval = gridIntervals[len(paramRanges)]
            if gridInterval == 0:
                paramRanges.append([paramRange[0]])
            else:
                paramRanges.append(np.arange(paramRange[0], paramRange[1], gridInterval))

        allCombos = list(itertools.product(*paramRanges))

        for combo in allCombos:
            if strategyTypes[stratType].isValid(combo):
                paramCombos.append(combo)

    numStrats = len(paramCombos)

    if(numStrats) == 0:
        print("Error: couldn't generate any valid parameter combinations.")
        exit(1)

    # Saving param combos
    try:
        with open(stratPath, 'w') as file:
            for combo in paramCombos:
                for param in combo:
                    file.write(str(param)+" ")
                file.write("\n")
    except Exception as e:
        print(f"An error occurred: {e}")
        exit(1)


    # Running the backtesting engine
    subprocess.run(
        ["./compute",
        str(numPrices),
        pricePath,
        str(numStrats),
        stratPath,
        str(strategyTypes[stratType].numParams),
        str(stratType),
        str(lookback),
        str(perfPath)])


    # Annualizing profits

    try:
        performances = np.loadtxt(perfPath).tolist()
    except:
        print("Error: backtesting engine generated no performances.\n")
        exit(1)

    yearLength = 365 if fullYear else 252

    if numStrats > 1:
        for p in range(numStrats):
            performances[p] = (1 + performances[p]) ** (yearLength / btLength) - 1
    else:
        performances = (1 + performances) ** (yearLength / btLength) - 1

    # Plotting results

    if numStrats > 1:
        plot(gridIntervals, stratType, strategyTypes, stratPath, performances)
    else:
        print(f"Annualized Profit: {performances:.3f}")


if __name__ == "__main__":
    main()
