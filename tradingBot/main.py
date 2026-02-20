import ccxt
import json
import time
import logging
from datetime import datetime
import importlib

def buy(bitget, symbol, quantity):

    params = {
        "marginMode": "isolated",
        "leverage": "1",
    }

    bitget.create_order(symbol, type='market', amount=quantity, side='buy', params=params)

def sell(bitget, symbol, quantity):

    params = {
        "marginMode": "isolated",
        "leverage": "1",
    }

    bitget.create_order(symbol, type='market', amount=quantity, side='sell', params=params)

def takePosition(bitget, signal, settings):
    symbol = settings["asset"]

    ticker = bitget.fetch_ticker(symbol)
    currentPrice = ticker['last']

    portfolio = getPortfolio(bitget)
    networth = portfolio[0]
    invested = portfolio[1]
    side = portfolio[2]

    desiredInv = networth * (1 - settings["reserves"])

    markets = bitget.load_markets()

    if signal == side:
        if abs(desiredInv - invested) / currentPrice <= 0.0001:
            logging.info(f"{side} - maintaining position")
            return

        if (signal == "long" and desiredInv > invested) or (signal == "short" and desiredInv < invested):
            toBuy = abs(desiredInv - invested) / currentPrice
            buy(bitget, symbol, toBuy)
        else:
            toSell = abs(invested - desiredInv) / currentPrice
            sell(bitget, symbol, toSell)
    else:
        if signal == "long":
            toBuy = (desiredInv + invested) / currentPrice
            buy(bitget, symbol, toBuy)
        else:
            toSell = (desiredInv + invested) / currentPrice
            sell(bitget, symbol, toSell)

    logging.info(f"{side} -> {signal}")


def getPortfolio(bitget):
    balance = bitget.fetch_balance()
    positions = bitget.fetch_positions()

    networth = balance["total"]["USDT"]
    invested = balance["used"]["USDT"]

    if(len(positions) > 0):
        position = positions[0]["side"]
    else:
        position = None

    return networth, invested, position

def runStrategy(bitget, settings):

    signal = settings["strategy"].generateSignal(bitget, settings)
    positionTaken = False
    increasedReserves = 0
    while not positionTaken:
        try:
            takePosition(bitget, signal, settings)
            positionTaken = True
        except Exception as e:
            if settings["reserves"] >= 0.03:
                logging.warning("Order could not be executed at >=3% reserves.")
                logging.error("ERROR:", e)
                exit(1)
            settings["reserves"] += (5/1000)
            increasedReserves += (5/1000)
            logging.warning(f"Order invalid, increased reserves -> {settings['reserves']}")

def loadConfig():

    with open('config.json', 'r') as file:
        config = json.load(file)

    configKeys = ["strategy", "params", "reserves", "asset", "yfTicker"]
    for key in configKeys:
        if key not in config:
            print("ERROR: not found in config:", key)
            raise KeyError

    settings = {setting: config[setting] for setting in configKeys}

    if(settings["reserves"] < 0.01 or settings["reserves"] >= 1):
        print("ERROR: 1% - 100% must be reserved. (0.01 - 1.00)")
        raise ValueError

    try:
        strategy = importlib.import_module(f"strategies.strat{settings['strategy']}")
    except:
        print("ERROR: couldn't import strategy with stratID", settings["strategy"])
        raise ImportError

    settings["strategy"] = strategy
    strategy.validate(settings["params"])

    return settings

def ranToday():

    with open('bot.log', 'r') as file:
        logs = file.readlines()
        for log in logs:
            if str((datetime.now()).date()) in log:
                return True
    return False

def main():
    logging.basicConfig(filename='bot.log', level=logging.INFO, format='%(asctime)s | %(levelname)s | %(message)s')

    if(ranToday()):
        logging.warning("Tried running again")
        exit(1)

    with open('keys.json', 'r') as file:
        keys = json.load(file)

    apiKey = keys["apikey"]
    secret = keys["secret"]
    password = keys["password"]

    bitget = ccxt.bitget({
        'apiKey': apiKey,
        'secret': secret,
        'password': password,
        'headers': {
            'paptrading': '1',
        },
        'options': {
            'defaultType': 'future',
        },
    })

    try:
        settings = loadConfig()

        logging.info("Running strategy")
        runStrategy(bitget, settings)

        portf = getPortfolio(bitget)

        networth = f"{portf[0]:.2f}"
        invested = f"{portf[1]:.2f}"
        cash = f"{(portf[0]-portf[1]):.2f}"
        reserves = settings["reserves"]

        logging.info(f"Networth: {networth} | Invested: {invested} | Cash: {cash} | Reserves: {reserves}")
        print(datetime.now(), "Success")

    except Exception as e:
        print(datetime.now(), "ERROR:", e)
        logging.error(e)


if __name__ == "__main__":
    main()
