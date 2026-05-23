import yfinance as yf
from datetime import datetime, timedelta as td
import os
import numpy as np
from config import RunConfig
from strategies import Strategy

def download_prices(run: RunConfig) -> int:

    number_of_prices = run.backtest_length + run.lookback

    if run.asset.is_traded_all_year:
        days_to_download = number_of_prices
    else:
        days_to_download = int(number_of_prices * (5/3))

    day = datetime.now().replace(hour=0,minute=0,second=0,microsecond=0) - td(1)


    try:
        price_data = (yf.download(
            tickers = run.asset.ticker,
            progress = False,
            start = day - td(days_to_download),
            end = day,
            auto_adjust = True))['Open'][run.asset.ticker].to_dict()
    except Exception as e:
        raise RuntimeError(f"Could not download prices for {run.asset.ticker}: {e}")

    dates = list(price_data.keys())

    prices = []
    for date in sorted(price_data.keys()):
        prices.append(price_data[date])

    prices = prices[-number_of_prices:]

    with open(run.prices_path, 'w') as file:
        for price in prices:
            file.write(f"{price}\n")
    
    return number_of_prices
