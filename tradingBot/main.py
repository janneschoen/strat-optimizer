import json
import time
import logging
import datetime
import argparse
from binance.client import Client


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
    start = (datetime.utcnow() - timedelta(days=amount)).strftime('%Y-%m-%d')
    prices = client.get_historical_klines(symbol, Client.KLINE_INTERVAL_1DAY, start)

    return [(float(k[0]), float(k[4])) for p in prices]


def getPosition(priceData, stratParams):
    return 0

def takePosition(position):
    pass


def main():
    args = setupArgs()
    setupLogging()

    ticker = args.ticker
    stratParams = [args.sma, args.lma, args.tol]
    if stratParams[0] < stratParams[1]:
        numPrices = stratParams[1]
    else:
        numPrices = stratParams[0]

    client = Client(loadKeys('keys.json'))    

    while True:
        try:
            priceData = getPrices(ticker, numPrices)
            position = getPosition(priceData, stratParams)
            takePosition(position)
            
            time.sleep(60)
        except Exception as e:
            print("ERROR:", e)
            time.sleep(10)

if __name__ == "__main__":
    main()
