"""
prices.py — Download historical daily prices from Yahoo Finance

Converts between calendar days and trading days so the requested
number of trading days is always available.  For assets that trade
365 days/year (crypto, forex) the conversion is 1:1; for equities
(≈252 trading days/year) we inflate the calendar window accordingly.

Writes the price series as plain-text floats (one per line) to the
temp file that the C engine reads.
"""

import yfinance as yf
from datetime import datetime, timedelta as td
import numpy as np
from config import RunConfig

DAYS_PER_YEAR = 365


def download_prices(run: RunConfig) -> int:
    """
    Download *open* prices for the configured ticker, covering
    (backtest_length + lookback) trading days up to yesterday.

    Returns the number of prices written to disk.
    """

    number_of_prices = run.backtest_length + run.lookback

    # calendar days needed to guarantee enough trading days
    calendar_days = int(
        number_of_prices * (DAYS_PER_YEAR / run.asset.trading_days)
    )

    yesterday = datetime.now().replace(
        hour=0, minute=0, second=0, microsecond=0) - td(days=1)

    try:
        raw = yf.download(
            tickers     = run.asset.ticker,
            progress    = False,
            start       = yesterday - td(days=calendar_days),
            end         = yesterday,
            auto_adjust = True,
        )
        price_dict = raw['Open'][run.asset.ticker].to_dict()
    except Exception as e:
        raise RuntimeError(
            f"Could not download prices for {run.asset.ticker}: {e}"
        )

    # sort by date, extract just the price floats
    prices = [price_dict[d] for d in sorted(price_dict.keys())]

    # trim to exactly the required number
    prices = prices[-number_of_prices:]

    with open(run.prices_path, 'w') as f:
        for price in prices:
            f.write(f"{price}\n")

    return number_of_prices
