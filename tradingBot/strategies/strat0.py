# Daily Moving Average Crossover Strategy

def generateSignal(bitget, settings):
    params = settings["params"]

    candles = bitget.fetch_ohlcv(settings["asset"], '1d', limit=params[1])
    openPrices = []

    for candle in candles:
        openPrices.append(candle[1])

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