"""
prices.py — Download historical daily prices from Yahoo Finance

Converts between calendar days and trading days so the requested
number of trading days is always available.  For assets that trade
365 days/year (crypto, forex) the conversion is 1:1; for equities
(≈252 trading days/year) we inflate the calendar window accordingly.

Returns a numpy float32 array that the C engine reads directly
(no temp files).
"""

import yfinance as yf
from datetime import datetime, timedelta as td
import numpy as np
from .config import RunConfig

DAYS_PER_YEAR = 365


def download_prices(run: RunConfig):
    """
    Download *open* prices for the configured ticker, covering
    (backtest_length + lookback) trading days up to yesterday.

    Returns (number_of_prices, numpy_array_of_floats, first_date, last_date).
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

    # sort by date, extract the prices
    sorted_dates = sorted(price_dict.keys())
    prices = [price_dict[d] for d in sorted_dates]

    # trim to exactly the required number, convert to float32 for C
    prices = np.array(prices[-number_of_prices:], dtype=np.float32)

    # the date range of the trimmed series
    first_date = sorted_dates[-number_of_prices]
    last_date  = sorted_dates[-1]

    return number_of_prices, prices, first_date, last_date
