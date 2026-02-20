# Daily Moving Average Crossover Strategy
import yfinance as yf

def generateSignal(bitget, settings):
    params = settings["params"]
    ticker = settings["yfTicker"]

    candles = yf.download(ticker, period=f"{params[1]}d", interval="1d", progress=False)

    openPrices = candles['Open'][ticker].to_list()

    if(params[0] > 1):
        sma = sum(openPrices[-params[0]:]) / params[0]
    else:
        sma = openPrices[-1]

    lma = sum(openPrices[-params[1]:]) / params[1]

    if(sma > lma):
        return 'long'
    else:
        return 'short'

def validate(params):
    if len(params) != 2:
        print("ERROR: strat0 requires exactly 2 parameters.")
        raise ValueError
    if params[0] >= params[1]:
        print("ERROR: SMA length must be smaller than LMA length.")
        raise ValueError
    if params[0] < 1 or params[1] < 1:
        print("ERROR: Moving average lengths must be at least 1.")
        raise ValueError