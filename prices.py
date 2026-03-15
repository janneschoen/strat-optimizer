import yfinance as yf
from datetime import datetime, timedelta as td
import os
import numpy as np


def downloadPrices(amount, config):
    ticker = config["ticker"]
    fullYear = config["fullYear"]

    pricesPath = config["tempDir"] + "/" + config["priceFile"]

    if fullYear:
        daysToDownload = amount
    else:
        daysToDownload = int(amount * (5/3))

    day = datetime.now().replace(hour=0,minute=0,second=0,microsecond=0) - td(1)

    try:
        priceData = (yf.download(ticker,
            progress = False,
            start = day - td(daysToDownload),
            end = day,
            auto_adjust = True))['Open'][ticker].to_dict()
    except:
        with open(pricesPath, 'w') as file:
            file.write(f"ERROR\n")
        exit(1)

    dates = list(priceData.keys())

    prices = []
    for date in sorted(priceData.keys()):
        prices.append(priceData[date])

    prices = prices[-amount:]

    try:
        with open(pricesPath, 'w') as file:
            for price in prices:
                file.write(f"{price}\n")
    except Exception as e:
        print(f"An error occurred: {e}")
        exit(1)