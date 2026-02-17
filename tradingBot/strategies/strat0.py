
def generateSignal(bitget, params, symbol):
    candles = bitget.fetch_ohlcv(symbol, '1h', limit=params[1])
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
