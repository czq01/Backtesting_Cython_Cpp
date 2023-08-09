import datetime
from multiprocessing.spawn import get_command_line
import pandas as pd
import talib
import matplotlib.pyplot as plt
import multiprocessing as mp
import numpy as np

SAchange = {  "2020-04-17" : "SA2005", "2020-08-19" : "SA2009", "2020-12-18" : "SA2101",
                "2021-04-07" : "SA2105", "2021-07-29" : "SA2109",
                "2021-12-02" : "SA2201", "2022-03-28": "SA2205", "2022-08-04": "SA2209",
                "2022-12-05" : "SA2301", "2023-03-24": "SA2305", "2023-08-01":"SA2309"}

def get_trading_date(x: str):
    ori_date = datetime.datetime.strptime(x, "%Y-%m-%d %H:%M:%S")
    date = (ori_date+datetime.timedelta(1)) if ori_date.hour>20 else ori_date
    return date.strftime("%Y-%m-%d")

def get_param_data(bars: pd.DataFrame, BNs: list, queue: mp.Queue):
    domain = get_domain_futures()
    for BN in BNs:
        for Bk in (0.5, 1, 1.25, 1.5, 1.75, 2, 2.25, 2.5, 3, 3.25):
            for period in ['1m', '2m', '5m', '10m', '15m','30m', '60m']:
                data = cal_bounds(bars, domain, N=BN, k=Bk, period=period)
                queue.put((data, BN, Bk, period))

def get_bars():
    bars = pd.read_csv('./data/SAbar_data.csv')
    bars["hour"] = bars["datetime"].apply(lambda x: int(x[11:13]))
    bars["minute"] = bars["datetime"].apply(lambda x: int(x[14:16]))
    bars["date"] = bars.datetime.apply(get_trading_date)
    return bars.sort_values(by=["symbol", "datetime"])

def get_domain_futures():
    bars = pd.read_csv("./data/SA9999.csv")
    bars["date"] = bars.datetime.apply(get_trading_date)
    sr = pd.DataFrame(SAchange, index=["symbol"]).T
    bars = bars.merge(sr, how='outer', left_on="date", right_index=True)
    bars.symbol.bfill(inplace=True)
    bars.dropna(inplace=True)
    bars["symbol"] += ".XZCE"
    return bars[["datetime", "symbol"]]

# get_trade:
def get_bolling_bounds(bars: pd.DataFrame, N, k):
    bounds = pd.DataFrame()
    bounds["H"], bounds["M"], bounds["L"] = talib.BBANDS(bars.close,N, k, k, 0)
    return bounds

def get_coef(dt):
    return (dt[-1]-dt[0])/len(dt)

def get_MACD(dt):
    DIF, DEA, MACD = talib.MACD(dt)
    return MACD

def make_VR(df:pd.DataFrame, len: int):
    UVS = DVST = PVS = 0
    result = []
    # for i in range(len):
    #     result.append(pd.NA)
    df.reset_index(drop=True, inplace=True)
    diff = df.close.diff(1)
    diff[0] = 0
    UV = df.volume[diff>0]
    DV = df.volume[diff<0]
    PV = df.volume[diff==0]

    def get_VR(i, l):
        nonlocal UVS, DVST, PVS, UV, DV, PV
        VR = pd.NA

        if diff[i]>0: UVS += UV[i]
        elif diff[i]<0: DVST += DV[i]
        else: PVS += PV[i]

        # UVS = UV.iloc[i-l:i].sum()
        # DVST = DV.iloc[i-l:i].sum()
        # PVS = PV.iloc[i-l:i].sum()

        if i>=l:
            if diff[i-l]>0: UVS -= UV[i-l]
            elif diff[i-l]<0: DVST -= DV[i-l]
            else: PVS -= PV[i-l]

        if i>=l-1:
            VR = (UVS+PVS/2)/(DVST+PVS/2) if (DVST+PVS/2) else 999
        return VR

    for i in df.index:
        print(f'\r {i}/{df.shape[0]}...', end='')
        result.append(get_VR(i, len))
    return result

def get_KLine(bars: pd.DataFrame, period: str):
    length = int(period[:-1])
    interval = period[-1]
    if interval == 'm':
        if length == 1: return bars[['datetime', 'date', 'open_interest', 'close', 'open',
       'high', 'low', 'volume', 'money', 'hour', 'minute', "symbol"]]
        elif length not in [2, 5,10, 15, 30, 60]:
            raise NotImplementedError("non divisible value is not support. Got %d" %length)
        result = pd.DataFrame()
        result["close"] = bars.close .rolling(length).apply(lambda x: x[-1], raw=True)
        result["open"]  = bars.open  .rolling(length).apply(lambda x: x[0], raw=True)
        result["high"]  = bars.high  .rolling(length).max()
        result["low"]   = bars.low   .rolling(length).min()
        result["volume"]= bars.volume.rolling(length).sum()
        result["money"] = bars.money .rolling(length).sum()
        result["hour"]  = bars.hour  .rolling(length).apply(lambda x: x[-1], raw=True)
        result["minute"]= bars.minute.rolling(length).apply(lambda x: x[-1], raw=True)
        result = result[(result.minute%length==0)]
    if interval == 'h':
        # result = pd.DataFrame()
        # for col in ["datetime", "close", "hour", "minute", "ori_date", "date"]:
        #     result[col] = bars[col].groupby(by=['hour', 'minute', 'date']).apply(lambda x: x[-1])
        # result["open"] = bars.open.rolling(length).apply(lambda x:x[0])
        # result["high"] = bars.high.rolling(length).max()
        # result["low"] = bars.low.rolling(length).min()
        # result["volume"] = bars.volume.rolling(length).sum()
        # result["money"] = bars.money.rolling(length).sum()
        raise NotImplementedError("h is not supported as period type")
    result = bars[["datetime", "date", "open_interest", "symbol"]].merge(result, how='right', left_index=True, right_index=True).dropna()
    return result

def cal_bounds(bars: pd.DataFrame, domain:pd.DataFrame, N: int, k: int, period: str='1m'):
    bars = get_KLine(bars.copy(), period)
    data: pd.DataFrame = bars.merge(get_bolling_bounds(bars, N, k), how='left', left_index=True, right_index=True).dropna()
    # data["VR_26"] = -1#make_VR(data,26)
    # data["VR_12"] = -1#make_VR(data,12)
    data["MA_5"] = talib.MA(data.close, 5)
    data["MA_10"] = talib.MA(data.close, 10)
    data["MA_20"] = talib.MA(data.close, 20)
    data["MA_30"] = talib.MA(data.close, 30)
    data["MA_60"] = talib.MA(data.close, 60)
    data['range'] = data.H-data.L
    data['bolling_beta_5'] = data.range.rolling(5).apply(get_coef, raw=True)
    data['bolling_beta_10'] = data.range.rolling(10).apply(get_coef, raw=True)
    data["DIF"], data["DEA"], data['MACD'] = talib.MACD(data.close)
    data["rsi_5"] = talib.RSI(data.close, 5)
    data["cci_5"] = talib.CCI(data.high, data.low, data.close, 5)
    data["cci_5_beta"] = data.cci_5.diff(3)/3
    data["rsi_15"] = talib.RSI(data.close, 15)
    data["cci_15"] = talib.CCI(data.high, data.low, data.close, 15)
    data["cci_15_beta"] = data.cci_15.diff(3)/3
    data = domain.merge(data, 'left', on=["datetime", "symbol"]).dropna()
    data["next_symbol"] = data.symbol.shift(-1)
    data['change'] = data.symbol!=data.next_symbol
    return data.drop(columns=['next_symbol',"symbol"])

def run(data, queue: mp.Queue, res_queue:mp.Queue):
    while (True):
        params, ids = queue.get()
        st = Strategy(params)
        # print(f"\r Currently working on: ({N}, {M}, {k})",end='')
        st.f = open(f"./backtest/bolling_up_5_trasaction_test_{params}.csv", 'w')
        st.f.write("datetime,pos,close,low,high,fee,slip,balance,earn,drawdown,H,L,M,MACD, earn_change\n")
        res = []
        for i in range(data.shape[0]):
            res.append(st.on_bar(data.iloc[i]))
        st.f.close()
        res_queue.put((res, params, ids))


class Strategy:
    def __init__(self, params): ...

    # @classmethod
    def clear(cls): ...

    # @classmethod
    def on_bar(cls, record: pd.Series):...

    # @classmethod
    def manage_pos(cls, record):...

    def get_sig(cls, record):...

    # @classmethod
    def make_order(cls, record):...

    # @classmethod
    def on_trade(cls, target: int, record: pd.Series):...



if __name__ == "__main__":
    import warnings
    warnings.filterwarnings('ignore')
    bars = get_bars()
    data = cal_bounds(bars, N=26, k=2, MACD_prev=3, period='5m')
    params = [[(0.0, 0.8, [[-100,0.1], [-0.1,0.1]], 30), (0, 0, 0, 0)],
              [(0.0, 0, [[0,0], [0,0]], 35), (0, 0, 1, 1)],
              [(0.1, 0, [[0,0], [0,0]], 35), (0, 0, 0, 1)],
              [(0.2, 0, [[0,0], [0,0]], 30), (0, 0, 1, 0)],
              ]
    fig, ax = plt.subplots(2,2, figsize=(15, 10))
    columns = ("datetime", "pos", "fee", "slip", "balance", "earn", "drawdown", "buy_sig",
               'short_sig', 'sell_sig', 'cover_sig', 'pos_high', 'pos_low', "earn_change")
    result = [list(columns)+["transactions", "stop", "macd","beta", "cci"]]
    manager = mp.Manager()
    queue = manager.Queue()
    res_queue = manager.Queue()
    print("starting...")
    pool = [mp.Process(target=run, args=(data, queue,res_queue)) for i in range(12)]
    [p.start() for p in pool]
    for param in params:
        queue.put(param)

    print("finished multi-processing.")
    for i in range(len(params)):
        print(f"\r Progress: {i+1}/{len(params)}...",end="")
        res, param, ids = res_queue.get()
        df = pd.DataFrame(res, columns=columns)
        (id_N, id_M, id_k, id_cci) = ids
        df[(df.pos.diff(1)!=0) & (df.pos==0)].plot(x="datetime", ax=ax[(id_M*6+id_N)*4+id_cci][id_k], title=str(param))
        # df.plot(x="datetime", ax=ax[(id_M*3+id_N)*4+id_cci][id_k], title=str(param))
        outcome = list(res[-1])
        outcome[6] = df.drawdown.max()
        outcome.extend([outcome[3]/20]+ list(param))
        result.append(outcome)
    print("done. result:")
    plt.show()