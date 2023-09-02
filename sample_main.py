import multiprocessing as mp
import queue
from py_module.research_func import *
import cython_module as rs

import warnings
warnings.filterwarnings('ignore')
kline_mgr = JQData_KLine_manager('./data/SAbar_data.csv')


params = []
param_names = ["K1", "K2", "stop"]
for k1 in [1.0]:
    for k2 in [0.1, 1.0]:
        for stop in [0.0]:
            params.append([k1, k2, stop])

columns = ("datetime", "pos", "fee", "slip", "balance", "earn", "drawdown", "buy_sig",
                   'short_sig', 'sell_sig', 'cover_sig', 'pos_high', 'pos_low', "earn_change")
result = list(columns) + param_names + ["period"]
print(len(params),'?')

# get_data
def get_data():
    for period in ['1m', '2m']:
        yield period, kline_mgr.cal_bounds()

# run params
length=0; i=0
alive=True; plot = False
def run(params: list, res_queue):
    global length, plot, alive, flag
    for period, data in get_data():
        queue = [p+[period] for p in params.copy()]
        rs.run(data, queue, res_queue, columns, 3.5, plot)
        length += len(params)
    alive=False

if __name__ == "__main__":
    res_queue = queue.Queue()
    # run backtesting
    run(params, res_queue)
    # get result
    for i in range(length):
        result = res_queue.get()
        print(f"\rGet {i+1}/{length}", end='\t')
    print()
