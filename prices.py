import yfinance as yf
from datetime import datetime, timedelta as td
import os
import numpy as np
from config import RunConfig
from strategies import Strategy

DAYS_PER_YEAR = 365

def download_prices(run: RunConfig) -> int:

    number_of_prices = run.backtest_length + run.lookback

    days_to_download = int(number_of_prices * (DAYS_PER_YEAR / run.asset.trading_days))

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
