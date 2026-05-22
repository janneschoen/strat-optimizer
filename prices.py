import yfinance as yf
from datetime import datetime, timedelta as td
import os
import numpy as np

def download_prices(amount, config):
    ticker = config["ticker"]
    is_traded_all_year = config["is_traded_all_year"]

    prices_path = config["temp_directory"] + "/" + config["prices_file"]

    if is_traded_all_year:
        days_to_download = amount
    else:
        days_to_download = int(amount * (5/3))

    day = datetime.now().replace(hour=0,minute=0,second=0,microsecond=0) - td(1)

    try:
        price_data = (yf.download(ticker,
            progress = False,
            start = day - td(days_to_download),
            end = day,
            auto_adjust = True))['Open'][ticker].to_dict()
    except Exception as e:
        raise RuntimeError(f"Could not download price data: {e}")

    dates = list(price_data.keys())

    prices = []
    for date in sorted(price_data.keys()):
        prices.append(price_data[date])

    prices = prices[-amount:]

    with open(prices_path, 'w') as file:
        for price in prices:
            file.write(f"{price}\n")
