import yfinance as yf
import sys
from datetime import datetime
from datetime import timedelta as td
import os

FILENAME = "temp/prices.temp"

ticker = sys.argv[1]
priceAmount = int(sys.argv[2])

day = datetime.now().replace(hour=0,minute=0,second=0,microsecond=0) - td(1)

try:
    priceData = (yf.download(ticker,
        progress = False,
        start = day - td(priceAmount - 1),
        end = day,
        auto_adjust = True))['Close'][ticker].to_dict()
except:
    print("WARNING: Couldn't download prices, check internet connection\n")
    
pricesDownloaded = len(priceData.values())
#print(f"Got {pricesDownloaded} / {priceAmount} prices for: {ticker}")

dates = list(priceData.keys())

for i in range(priceAmount):
    if not(day-td(i) in dates):
        priceData[day-td(i)] = 0

prices = []
for date in sorted(priceData.keys()):
    prices.append(priceData[date])

with open(FILENAME, 'w') as file:
    for price in prices:
        file.write(f"{price}\n")