import datetime
import pandas as pd
import talib
import matplotlib.pyplot as plt
import multiprocessing as mp
import numpy as np

def get_param_data(bars: pd.DataFrame, BNs: list, queue: mp.Queue):
    for BN in BNs:
        for Bk in (0.5, 1, 1.25, 1.5, 1.75, 2, 2.25, 2.5, 3, 3.25):
            for period in ['1m', '2m', '5m', '10m', '15m','30m', '60m']:
                data = cal_bounds(bars, N=BN, k=Bk, MACD_prev=3, period=period, advanced=False)
                queue.put((data, BN, Bk, period))

def get_bars(nrow = None):
    bars = pd.read_csv("./SA9999.csv", nrows=nrow)
    bars["hour"] = bars["datetime"].apply(lambda x: int(x[11:13]))
    bars["minute"] = bars["datetime"].apply(lambda x: int(x[14:16]))
    bars["ori_date"] = bars["datetime"].apply(lambda x: datetime.datetime.strptime(x[:10], "%Y-%m-%d"))
    bars["date"] = bars.apply(lambda x: (x.ori_date+datetime.timedelta(1))if x.hour>20 else x.ori_date, axis=1)
    bars["date"] = bars.date.apply(lambda x: x.strftime("%Y-%m-%d"))
    return bars

# get_trade:
def get_bounds(bars: pd.DataFrame, N, k):
    bounds = pd.DataFrame()
    # MB = bars.close.rolling(N-1).mean()
    # std = bars.close.rolling(N).std()
    # up = MB + std*k
    # down = MB - std*k
    # bounds['M'] = MB
    # bounds['H'] = up
    # bounds['L'] = down
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
        if length == 1: return bars[['datetime', 'ori_date', 'date', 'open_interest', 'close', 'open',
       'high', 'low', 'volume', 'money', 'hour', 'minute']]
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
    result = bars[["datetime", "ori_date", "date", "open_interest"]].merge(result, how='right', left_index=True, right_index=True).dropna()
    return result

def get_advanced_indexes(test: pd.DataFrame):
    test["MACD_diff"] = test.MACD-test.last_MACD
    test["MACD_multi"] = test.MACD*test.last_MACD
    test["MACD_prev_last"] = test.last_MACD - test.prev_MACD
    test["MACD_prev_curr"] = test.MACD - test.prev_MACD
    test['cci_15_diff'] = test.cci_15.diff(1)
    test['cci_5_diff'] = test.cci_5.diff(1)
    test['cci_30_diff'] = test.cci_30.diff(1)
    test['rsi_15_diff'] = test.rsi_15.diff(1)
    test['rsi_5_diff'] = test.rsi_5.diff(1)
    test['rsi_30_diff'] = test.rsi_30.diff(1)
    test["rsi_cci_5"] = test.rsi_5-test.cci_5
    test["rsi_cci_15"] = test.rsi_15-test.cci_15
    test["rsi_cci_30"] = test.rsi_30-test.cci_30
    test["MA_5_10"] = test.MA_5-test.MA_10
    test["MA_10_20"] = test.MA_10-test.MA_20
    test["MA_20_30"] = test.MA_20-test.MA_30
    test["MA_10_30"] = test.MA_10-test.MA_30
    test["MA_30_60"] = test.MA_30-test.MA_60
    test["MA_20_60"] = test.MA_20-test.MA_60
    test["MA_60_120"] = test.MA_60-test.MA_120
    test["MA_30_120"] = test.MA_30-test.MA_120
    test["MA_10_60"] = test.MA_10-test.MA_60
    test["VR_26_diff"] = test.VR_26.diff(1)
    test["VR_12_diff"] = test.VR_12.diff(1)
    test["VR_26_12"] = test.VR_26-test.VR_12
    test["range_diff"] = test.range.diff(1)
    test["close_high"] = test.close-test.high
    test["close_high_ratio"] = (test.close-test.high)/(test.high-test.low)
    test["close_low"] = test.close-test.low
    test["close_low_ratio"] = (test.close-test.low)/(test.high-test.low)
    test["close_diff"] = test.close.diff(1)
    test["close_diff_range"] = test.close_diff/test.range
    test["close_diff_ratio"] = test.close_diff/(test.high-test.low)
    test["close_diff_last"] = test.close_diff.shift(1)
    test["close_diff_last_range"] = test.close_diff_last/test.range
    test["bolling_beta_5_diff"] = test.bolling_beta_5.diff(1)
    test["bolling_beta_10_diff"] = test.bolling_beta_10.diff(1)

    for i in ['VR_26', 'VR_12', 'MA_5', 'MA_10', "MA_5_10", "MA_10_20", "MA_20_60", "MA_30_60",
              "MA_10_60", "MA_30_120", "MA_60_120", "MA_20_30", "VR_26_diff", "range_diff", "bolling_beta_10_diff",
           'MA_20', 'MA_30', 'MA_60', 'MA_120', 'range', 'bolling_beta_5',"VR_12_diff",
           'bolling_beta_10', 'bolling_beta_20', 'bolling_beta_40', 'DIF', 'DEA',
           'MACD', 'rsi_5', 'cci_5', 'cci_5_beta', 'rsi_15', 'cci_15',
           'cci_15_beta', 'rsi_30', 'cci_30', 'cci_30_beta', 'prev_MACD', 'last_MACD', 'MACD_diff', 'MACD_multi',
           'MACD_prev_last', 'MACD_prev_curr', 'cci_15_diff', 'cci_5_diff',
           'cci_30_diff', 'rsi_15_diff', 'rsi_5_diff', 'rsi_30_diff', 'rsi_cci_5',
           'rsi_cci_15', 'rsi_cci_30',"MACD_diff","MACD_prev_last", "MACD_prev_curr", 'rsi_15_diff',
           "VR_26_12", "bolling_beta_5_diff", "holding_range_15", "holding_range_60", "holding_std_15"
           ]:
        test[f"{i}_delay_1"] = test[i].shift(1)
        test[f"{i}_delay_2"] = test[i].shift(2)
        test[f"{i}_delay_4"] = test[i].shift(4)

def cal_bounds(bars: pd.DataFrame, N: int, k: int, MACD_prev: int=3, period: str='1m', advanced=True):
    bars = get_KLine(bars.copy(), period)
    data: pd.DataFrame = bars.merge(get_bounds(bars, N, k), how='left', left_index=True, right_index=True).dropna()
    data["VR_26"] = -1#make_VR(data,26)
    data["VR_12"] = -1#make_VR(data,12)
    data["MA_5"] = talib.MA(data.close, 5)
    data["MA_10"] = talib.MA(data.close, 10)
    data["MA_20"] = talib.MA(data.close, 20)
    data["MA_30"] = talib.MA(data.close, 30)
    data["MA_60"] = talib.MA(data.close, 60)
    data["MA_120"] = talib.MA(data.close, 120)
    data['range'] = data.H-data.L
    data['bolling_beta_5'] = data.range.rolling(5).apply(get_coef, raw=True)
    data['bolling_beta_10'] = data.range.rolling(10).apply(get_coef, raw=True)
    data['bolling_beta_20'] = data.range.rolling(20).apply(get_coef, raw=True)
    data["DIF"], data["DEA"], data['MACD'] = talib.MACD(data.close)
    data["rsi_5"] = talib.RSI(data.close, 5)
    data["cci_5"] = talib.CCI(data.high, data.low, data.close, 5)
    data["cci_5_beta"] = data.cci_5.diff(3)/3
    data["rsi_15"] = talib.RSI(data.close, 15)
    data["cci_15"] = talib.CCI(data.high, data.low, data.close, 15)
    data["cci_15_beta"] = data.cci_15.diff(3)/3
    data["rsi_30"] = talib.RSI(data.close, 30)
    data["cci_30"] = talib.CCI(data.high, data.low, data.close, 30)
    data["cci_30_beta"] = data.cci_30.diff(3)/3
    if (advanced):
        data["last_close"] = data.close.shift(1)
        data['bolling_beta_40'] = data.range.rolling(40).apply(get_coef, raw=True)
        data["prev_MACD"] = data.MACD.shift(MACD_prev)
        data["last_MACD"] = data.MACD.shift(1)
        data["close_open_range_60"] = data.close-data.open.shift(60)
        data["price_range_60"] = data.high.rolling(60).max() - data.low.rolling(60).min()
        data["ave_holding_past_60"] = data.open_interest.rolling(60).mean()
        data["ave_holding_past_15"] = data.open_interest.rolling(15).mean()
        data["ave_holding_past_5"] = data.open_interest.rolling(5).mean()
        data["holding_range_60"] = data.open_interest.rolling(60).apply(lambda x: max(x)-min(x))
        data["holding_range_15"] = data.open_interest.rolling(15).apply(lambda x: max(x)-min(x))
        data["holding_range_5"] = data.open_interest.rolling(5).apply(lambda x: max(x)-min(x))
        data["holding_std_60"] = data.open_interest.rolling(60).std()
        data["holding_std_15"] = data.open_interest.rolling(15).std()
        data["holding_std_5"] = data.open_interest.rolling(5).std()
        data["holding_diff_60"] = (data.open_interest - data.ave_holding_past_60)
        data["holding_diff_5"] = (data.open_interest - data.ave_holding_past_5)
        data["holding_diff"] = data.open_interest.diff(1)
        get_advanced_indexes(data)
    data.dropna(inplace=True)
    return data


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