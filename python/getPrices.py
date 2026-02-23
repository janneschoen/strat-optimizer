import yfinance as yf
import sys, os
from datetime import datetime
from datetime import timedelta as td
import json

PRICES_FILE = "temp/prices.temp"

ticker = sys.argv[1]
priceAmount = int(sys.argv[2])
fullYear = int(sys.argv[3])

if fullYear:
    daysToDownload = priceAmount
else:
    daysToDownload = int(priceAmount * (5/3))

day = datetime.now().replace(hour=0,minute=0,second=0,microsecond=0) - td(1)


try:
    priceData = (yf.download(ticker,
        progress = False,
        start = day - td(daysToDownload),
        end = day,
        auto_adjust = True))['Open'][ticker].to_dict()
except:
    with open(PRICES_FILE, 'w') as file:
        file.write(f"ERROR\n")
    exit(1)

#pricesDownloaded = len(priceData.values())
#print(f"Got {pricesDownloaded} / {priceAmount} prices for: {ticker}")

dates = list(priceData.keys())

prices = []
for date in sorted(priceData.keys()):
    prices.append(priceData[date])

prices = prices[-priceAmount:]

with open(PRICES_FILE, 'w') as file:
    for price in prices:
        file.write(f"{price}\n")