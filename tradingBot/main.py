import argparse
import ccxt
import json
import time
import logging
from datetime import datetime
import importlib

parser = argparse.ArgumentParser(description='Run trading strategy with specific parameters.')
parser.add_argument("-s", required=True, type=int, help='strategy ID')
parser.add_argument('-p', nargs='+', type=int, help='strategy parameters')
parser.add_argument('-a', required=True, type=str, help='asset ticker')

args = parser.parse_args()
symbol = args.a
stratID = args.s
params = args.p

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

logging.basicConfig(filename='bot.log', level=logging.INFO, format='%(asctime)s | %(levelname)s | %(message)s')


while True:
    try:
        now = datetime.now()

        balance = bitget.fetch_balance()
        positions = bitget.fetch_positions()

        networth = balance["total"]["USDT"]
        cash = balance["free"]["USDT"]

        if(len(positions) > 0):
            position = positions[0]["side"]
        else:
            position = None
        
        logging.info(f"Net: {networth} | Cash: {cash} | Pos: {position}")

        signal = strategy.generateSignal(bitget, params, symbol)

        if signal != position:
            bitget.load_markets()

            asset = bitget.fetch_ticker(symbol)
            currentPrice = asset['last']

            investment = (networth / 10) / currentPrice

            if signal == 'long':
                #close short pos
                #open long pos
                pass
            elif signal == 'short':
                #close long pos
                #open short pos
                pass
        
    except Exception as e:
        print(f'ERROR: {e}')
        logging.error(e)

    time.sleep(120)
