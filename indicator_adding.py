import pandas as pd
import talib

def Bolling(data:pd.Series, timeperiod=20, n_dev_up=2, n_dev_dn=2, matype=0):
    """Bolling Line H,L,M"""
    H_line, M_line, L_line = talib.BBANDS(data, timeperiod,
                                          nbdevup=n_dev_up,
                                          nbdevdn = n_dev_dn,
                                          matype = matype)
    return H_line, M_line, L_line

def SMA(data: pd.Series, timeperiod=20):
    """Simple Moving Average"""
    return talib.SMA(data, timeperiod)

def TRIMA(data: pd.Series, timeperiod=20):
    """Triagular Moving Average"""
    return talib.TRIMA(data, timeperiod)

def WMA(data: pd.Series, timeperiod=20):
    """Weighted Moving Average"""
    return talib.WMA(data, timeperiod)

def MACD(data: pd.Series, speriod=12, lperiod=26, sigperiod=9):
    macd, macdsignal, macdhist = talib.MACD(data, fastperiod=speriod,
                                      slowperiod=lperiod,
                                      signalperiod=sigperiod)
    return macd, macdsignal, macdhist

def Cgaukin_AD_Line(high, low, close, volume):
    return talib.AD(high, low, close, volume)

def Variance(data: pd.Series, timeperiod=5, n_dev = 1):
    return talib.VAR(data, timeperiod, nbdev=n_dev)

def cci(high, low, close, timeperiod=14):
    return talib.CCI(high, low, close, timeperiod)

def rsi(close, timeperiod=14):
    return talib.RSI(close, timeperiod)

def MACDFIX(close, sig_period=9):
    macd, macdsignal, macdhist = talib.MACDFIX(close, sig_period)
    return macd, macdsignal, macdhist