import datetime
from multiprocessing.spawn import get_command_line
import pandas as pd
import talib
import matplotlib.pyplot as plt
import multiprocessing as mp
import numpy as np

class JQData_KLine_manager:
    def __init__(self, data_path_1m: str) -> None:
        self.data_path = data_path_1m
        self.data = self._get_bars()

    def _get_trading_date(self, x: str):
        ori_date = datetime.datetime.strptime(x, "%Y-%m-%d %H:%M:%S")
        date = (ori_date+datetime.timedelta(1)) if ori_date.hour>20 else ori_date
        return date.strftime("%Y-%m-%d")

    def _get_bars(self):
        bars = pd.read_csv(self.data_path)
        bars["trading_day"] = bars.datetime.apply(self._get_trading_date)
        return bars.sort_values(by=["symbol", "datetime"])

    # get_trade:
    def _get_KLine(self, bars: pd.DataFrame, period: str):
        length = int(period[:-1])
        interval = period[-1]
        if interval == 'm':
            if length == 1: return bars[['datetime', 'trading_day', 'open_interest', "symbol", 'close', 'open',
           'high', 'low', 'volume', 'money']]
            elif length not in [2, 5,10, 15, 30, 60]:
                raise NotImplementedError("non divisible value is not support. Got %d" %length)
            result = pd.DataFrame()
            result["close"] = bars.close .rolling(length).apply(lambda x: x[-1], raw=True)
            result["open"]  = bars.open  .rolling(length).apply(lambda x: x[0], raw=True)
            result["high"]  = bars.high  .rolling(length).max()
            result["low"]   = bars.low   .rolling(length).min()
            result["volume"]= bars.volume.rolling(length).sum()
            result["money"] = bars.money .rolling(length).sum()
            result = result[(result.minute%length==0)]
        if interval == 'h':
            # TODO interval in hours
            raise NotImplementedError("h is not supported as period type")
        result = bars[["datetime", "trading_day", "open_interest", "symbol"]].merge(result, how='right', left_index=True, right_index=True).dropna()
        return result

    def cal_bounds(self, period: str='1m'):
        bars = self._get_KLine(self.data, period)
        return bars

