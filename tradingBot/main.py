import argparse
import ccxt
import json
import time
import logging
from datetime import datetime
import importlib
import inspect

def buy(bitget, symbol, quantity):

    params = {"marginMode": "isolated",}

    bitget.create_order(symbol, type='market', amount=quantity, side='buy', params=params)

def sell(bitget, symbol, quantity):

    params = {"marginMode": "isolated",}

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
            logging.info(f"({side}, {invested}) - maintaining position")
            return

        if (signal == "long" and desiredInv > invested) or (signal == "short" and desiredInv < invested):
            toBuy = (desiredInv - invested) / currentPrice
            buy(bitget, symbol, toBuy)
        else:
            toSell = (invested - desiredInv) / currentPrice
            sell(bitget, symbol, toSell)
    else:
        if signal == "long":
            toBuy = (desiredInv + invested) / currentPrice
            buy(bitget, symbol, toBuy)
        else:
            toSell = (desiredInv + invested) / currentPrice
            sell(bitget, symbol, toSell)

    logging.info(f"({side}, {invested}) -> ({signal}, {desiredInv})")


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
    takePosition(bitget, signal, settings)


def loadConfig():
    with open('config.json', 'r') as file:
        config = json.load(file)
    
    configKeys = ["strategy", "params", "reserves", "asset"]
    for key in configKeys:
        if key not in config:
            print("ERROR: not found in config:", key)
            raise KeyError

    settings = {setting: config[setting] for setting in configKeys}

    if(settings["reserves"] < 0.01 or settings["reserves"] >= 1):
        print("ERROR: 1% to 99% must be reserved. (0.01-0.99)")
        raise ValueError

    try:
        strategy = importlib.import_module(f"strategies.strat{settings["strategy"]}")
    except:
        print("ERROR: couldn't import strategy with stratID", settings["strategy"])
        raise ImportError

    settings["strategy"] = strategy
    strategy.validate(settings["params"])
    
    return settings


def main():

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

    elem = bitget.fetch_canceled_and_closed_orders()

    logging.basicConfig(filename='bot.log', level=logging.INFO, format='%(asctime)s | %(levelname)s | %(message)s')

    lastRun = datetime.now()

    while True:
        now = datetime.now()
        if True or (now.day != lastRun.day and now.hour == 1):
            try:
                settings = loadConfig()

                logging.info("Running strategy")
                runStrategy(bitget, settings)
                lastRun = now

                portf = getPortfolio(bitget)
                logging.info(f"Netw.: {portf[0]} | Inv.: {portf[1]} | Pos.: {portf[2]}")

            except Exception as e:
                print(f'ERROR: {e}')
                logging.error(e)

        time.sleep(60)

if __name__ == "__main__":
    main()
