import json
from prices import downloadPrices
from plotting import plot
import subprocess
import itertools, numpy as np
import os

def isValidStrat0(paramCombo):
    p0 = paramCombo[0]
    p1 = paramCombo[1]
    if p0 >= p1:
        return False
    if p0 < 1 or p1 < 1:
        return False
    return True

def main():
    with open('config.json', 'r') as file:
        config = json.load(file)

    os.makedirs(config["tempDir"], exist_ok=True)
    tempDir = config["tempDir"] + "/"
    pricePath = tempDir + config["priceFile"]
    stratPath = tempDir + config["stratFile"]
    perfPath = tempDir + config["resultFile"]

    # Defining strategy type attributes
    strategyTypes = [
        {
            "name": "Simple Moving Average Crossover",
            "numParams": 2,
            "paramNames": ["Fast SMA Length", "Slow SMA Length"],
            "lookbackParam": 1,
            "valid": isValidStrat0
        },
    ]

    # Downloading price data
    lookback = config["params"][strategyTypes[config["stratType"]]["lookbackParam"]]
    numPrices = config["btLength"] + lookback
    downloadPrices(numPrices, config)

    # Generating parameter combinations
    if config["singleTest"]:
        paramCombos = [config["params"]]
    else:
        paramRanges = []
        for param in config["params"]:
            gridInterval = config["gridIntv"][len(paramRanges)]
            paramRanges.append(range(1, param + gridInterval, gridInterval))
        allCombos = list(itertools.product(*paramRanges))
        paramCombos = []
        for combo in allCombos:
            if strategyTypes[config["stratType"]]["valid"](combo):
                paramCombos.append(combo)

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
        str(strategyTypes[config["stratType"]]["numParams"]),
        str(config["stratType"]),
        str(lookback),
        str(perfPath)])

    # Annualizing profits
    
    performances = np.loadtxt(perfPath).tolist()
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
