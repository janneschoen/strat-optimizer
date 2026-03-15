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
            if p[0] >= p[1] or p[1] < 2:
                return False
        if self.name == "RSI":
            if p[2] < 2 or p[0] < 0 or p[1] > 100 or p[0] > p[1]:
                return False
        return True

strategyTypes = [
    StrategyType(
        name = "SMA Crossover",
        numParams = 2,
        paramNames = ["Fast SMA Length", "Slow SMA Length"],
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

    # Downloading price data
    lookback = config["params"][strategyTypes[config["stratType"]].lookbackParam]
    numPrices = config["btLength"] + lookback
    downloadPrices(numPrices, config)

    # Generating parameter combinations
    if config["singleTest"]:
        paramCombos = [config["params"]]
    else:
        if len(config["gridIntv"]) != len(config["params"]):
            print("Error: please provide exactly one interval per parameter.")
            exit(0)
        paramRanges = []
        for param in config["params"]:
            gridInterval = config["gridIntv"][len(paramRanges)]
            paramRanges.append(range(1, param + gridInterval, gridInterval))
        allCombos = list(itertools.product(*paramRanges))
        paramCombos = []
        for combo in allCombos:
            if strategyTypes[config["stratType"]].isValid(combo):
                paramCombos.append(combo)

    if(len(paramCombos)) == 0:
        print("Error: couldn't generate any valid parameter combinations.")
        exit(0)

    # Saving param combos
    try:
        stratPath = config["tempDir"] + "/" + config["stratFile"]
        with open(stratPath, 'w') as file:
            for combo in paramCombos:
                for param in combo:
                    file.write(str(param)+" ")
                file.write("\n")
    except Exception as e:
        print(f"An error occurred: {e}")
        exit(1)

    numStrats = len(paramCombos)

    # Running the backtesting engine
    subprocess.run(
        ["./compute",
        str(numPrices),
        pricePath,
        str(numStrats),
        stratPath,
        str(strategyTypes[config["stratType"]].numParams),
        str(config["stratType"]),
        str(lookback),
        str(perfPath)])


    # Annualizing profits

    try:
        performances = np.loadtxt(perfPath).tolist()
    except:
        print("Error: backtesting engine generated no performances.\n")
        exit(0)

    yearLength = 365 if config["fullYear"] else 252

    if numStrats > 1:
        for p in range(numStrats):
            performances[p] = (1 + performances[p]) ** (yearLength / config["btLength"]) - 1
    else:
        performances = (1 + performances) ** (yearLength / config["btLength"]) - 1

    # Plotting results

    if numStrats > 1:
        plot(config["stratType"], strategyTypes, stratPath, performances)
    else:
        print(f"Annualized Profit: {performances:.3f}")


if __name__ == "__main__":
    main()
