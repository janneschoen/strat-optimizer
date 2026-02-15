import json
import time
import logging
from datetime import datetime, timedelta
import argparse
from binance.client import Client
import requests
import pandas as pd


def setupArgs():
    parser = argparse.ArgumentParser(description="Trading Bot Parameters")
    parser.add_argument('--ticker', type=str, help='Asset ticker (BTC-USD, ...)', required=True)
    parser.add_argument('--sma', type=int, help='Length of short moving average', required=True)
    parser.add_argument('--lma', type=int, help='Length of long moving average', required=True)
    parser.add_argument('--tol', type=int, help='Signal tolerance (5, ...)', required=True)

    return parser.parse_args()

def setupLogging():
    logging.basicConfig(
        filename='tradingBot.log',
        level=logging.INFO,
        format='%(asctime)s - %(levelname)s - %(message)s',
    )

def loadKeys(filePath):
    with open(filePath) as f:
        config = json.load(f)
    return config['api_key'], config['api_secret']

def getPrices(ticker, amount):
    now = datetime.now()

    url = "https://api.binance.com/api/v3/klines"
    params = {
        'symbol': ticker,
        'interval': '1d',
        'limit': amount
    }
    response = requests.get(url, params=params)
    data = response.json()

    prices = []
    for candle in data:
        prices.append(float(candle[1]))

    logging.info(f"Downloaded {len(prices)} {ticker} prices.")
    return prices

def getPosition(prices, stratParams):
    smaW = stratParams[0]
    lmaW = stratParams[1]
    sma = sum(prices[-smaW:]) / smaW
    lma = sum(prices[-lmaW:]) / lmaW

    diff = abs((sma-lma)/lma)
    if diff <= stratParams[2]/100:
        position = 0
    elif sma > lma:
        position = 1
    else:
        position = -1
    logging.info(f"SMA: {sma:.2f} | LMA: {lma:.2f} | diff: {diff:.2f} | => position: {position}")
    return position

def takePosition(position):
    pass


def main():
    apiKey, apiSecret = loadKeys('keys.json')
    client = Client(apiKey, apiSecret)    

    args = setupArgs()
    setupLogging()

    ticker = args.ticker
    stratParams = [args.sma, args.lma, args.tol]
    if stratParams[0] < stratParams[1]:
        numPrices = stratParams[1]
    else:
        numPrices = stratParams[0]

    lastDayRan = None

    while True:
        now = datetime.now()
        while now.hour >= 1 and now.day != lastDayRan:
            try:
                prices = getPrices(ticker, numPrices)
                position = getPosition(prices, stratParams)
                takePosition(position)
                lastDayRan = (datetime.now()).day
            except Exception as e:
                print("ERROR:", e)
                logging.error("ERROR:", e)
            time.sleep(10)
        
        time.sleep(90)

if __name__ == "__main__":
    main()
