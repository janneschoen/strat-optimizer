import argparse
import ccxt
import json
import time
import logging
from datetime import datetime
import importlib
import inspect

def buy(bitget, symbol, quantity):

    params = {
        "marginMode": "isolated",
    }

    bitget.create_order(f"{symbol}:USDT", type='market', amount=quantity, side='buy', params=params)


def sell(bitget, symbol, quantity):

    params = {
        "marginMode": "isolated",
    }

    bitget.create_order(f"{symbol}:USDT", type='market', amount=quantity, side='sell', params=params)


def takePosition(bitget, signal, symbol, reserves):
        
    ticker = bitget.fetch_ticker(symbol)
    currentPrice = ticker['last']

    portfolio = getPortfolio(bitget)
    networth = portfolio[0]
    invested = portfolio[1]
    side = portfolio[2]

    desiredInv = networth * (1 - reserves)

    markets = bitget.load_markets()

    if signal == side:
        if abs(desiredInv - invested) / currentPrice <= 0.0001:
            logging.info(f"({side}, {invested}) - maintaining position")
            print(f"({side}, {invested}) - maintaining position")
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
    print(f"({side}, {invested}) -> ({signal}, {desiredInv})")


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

def runStrategy(bitget, strategy, symbol, params, reserves):

    signal = strategy.generateSignal(bitget, params, symbol)

    takePosition(bitget, signal, symbol, reserves)


def main():
    parser = argparse.ArgumentParser(description='Run trading strategy with specific parameters.')
    parser.add_argument('-s', required=True, type=int, help='strategy ID')
    parser.add_argument('-p', nargs='+', type=int, help='strategy parameters')
    parser.add_argument('-r', required=True, type=float, help='reserves size (%% of netwoth not invested)')
    parser.add_argument('-a', required=True, type=str, help='asset ticker')

    args = parser.parse_args()
    symbol = args.a
    stratID = args.s
    params = args.p
    reserves = args.r

    try:
        strategy = importlib.import_module(f"strategies.strat{stratID}")
    except:
        print("Error importing strategy.")
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

    elem = bitget.fetch_canceled_and_closed_orders()
    

    logging.basicConfig(filename='bot.log', level=logging.INFO, format='%(asctime)s | %(levelname)s | %(message)s')


    while True:
        try:
            runStrategy(bitget, strategy, symbol, params, reserves)
            
        except Exception as e:
            print(f'ERROR: {e}')
            logging.error(e)

        time.sleep(60)

if __name__ == "__main__":
    main()
