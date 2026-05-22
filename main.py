import json
from prices import downloadPrices
from plotting import plot
from equityCurve import showEquityCurve
from strategyTypes import StrategyType
import subprocess
import itertools, numpy as np
import os, sys

def main():
    args = sys.argv
    if len(args) == 1:
        configFile = 'config.json'
    else:
        configFile = args[1]

    print("Reading configuration from", configFile)

    with open(configFile, 'r') as file:
        config = json.load(file)

    os.makedirs(config["tempDir"], exist_ok=True)
    tempDir = config["tempDir"] + "/"
    pricePath = tempDir + config["priceFile"]
    stratPath = tempDir + config["stratFile"]
    perfPath = tempDir + config["resultFile"]
    equityPath = tempDir + config["equityFile"]

    params = config["params"]
    stratTypeName = config["stratType"]
    btLength = config["backtestLength"]
    paramSteps = config["paramSteps"]
    fullYear = config["fullYear"]


    with open(config["stratTypesFile"], 'r') as file:
        strategyTypes = json.load(file)
    
    stratTypeNames = [st["name"] for st in strategyTypes]

    if stratTypeName not in stratTypeNames:
        raise KeyError(f"Strategy type '{stratTypeName}' not defined.")
    
    for stratType in strategyTypes:
        if stratType["name"] == stratTypeName:
            strategyType = StrategyType(
                stratType["name"],
                stratType["parameters"]
            )
            break

    print("Strategy type:", strategyType.name)

    # Downloading price data
    for i, param in enumerate(strategyType.parameters):
        if "defines_lookback" in param:
            lookback = params[i]
            lookback = lookback[1] if len(lookback) > 1 else lookback

    numPrices = btLength + lookback
    
    downloadPrices(numPrices, config)

    # Generating parameter combinations if steps provided
    paramCombos = []

    if paramSteps.count(0) == len(params):
        paramCombos.append([param[0] for param in params])
    else:
        if len(paramSteps) != len(params):
            raise ValueError(f"Got {len(params)} parameters but {len(paramSteps)} intervals.")

        paramRanges = []
        for paramRange in params:
            paramStep = paramSteps[len(paramRanges)]
            if paramStep == 0:
                paramRanges.append([paramRange[0]])
            else:
                paramRanges.append(np.arange(paramRange[0], paramRange[1], paramStep))

        allCombos = list(itertools.product(*paramRanges))

        for combo in allCombos:
            if strategyType.isValid(combo):
                paramCombos.append(combo)

    numStrats = len(paramCombos)

    if(numStrats) == 0:
        raise ValueError("Could not generate any valid parameter combinations.")

    # Saving param combos

    with open(stratPath, 'w') as file:
        for combo in paramCombos:
            for param in combo:
                file.write(str(param)+" ")
            file.write("\n")

    # Running the backtesting engine

    arguments = [
        numPrices,
        pricePath,
        numStrats,
        stratPath,
        strategyType.numParams,
        stratType,
        lookback,
        perfPath,
        equityPath
    ]

    subprocess.run(["./compute"] + [str(a) for a in arguments])

    # Annualizing profits

    try:
        performances = np.loadtxt(perfPath).tolist()
    except:
        raise RuntimeError("Got no performances from backtesting engine.")

    yearLength = 365 if fullYear else 252

    if numStrats > 1:
        for p in range(numStrats):
            performances[p] = (1 + performances[p]) ** (yearLength / btLength) - 1
    else:
        performances = (1 + performances) ** (yearLength / btLength) - 1

    # Plotting results

    if numStrats > 1:
        plot(paramSteps, strategyType, stratPath, performances, config)
    else:
        print(f"Annualized Profit: {performances:.3f}")
        showEquityCurve(equityPath, strategyType, config)


if __name__ == "__main__":
    main()
