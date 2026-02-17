# Daily Moving Average Crossover Strategy

def generateSignal(bitget, params, symbol):
    candles = bitget.fetch_ohlcv(symbol, '1d', limit=params[1])
    print("Timestamp of newest price:", candles[-1][0])
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

def valid(params):
    if params[0] >= params[1]:
        print("ERROR: SMA length must be smaller than LMA length.")
        return 0
    if params[0] < 1 or params[1] < 1:
        print("ERROR: Moving average lengths must be at least 1.")
        return 0
    return 1