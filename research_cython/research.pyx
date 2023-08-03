# cython: language_level=3
# distutils: language=c++
import pandas as pd
from libc.stdio cimport *
from libc.stdlib cimport *
from libcpp.string cimport string
from libcpp.string cimport *
from libcpp.vector cimport vector
from cpython.ref cimport Py_DECREF, PyObject
from cython.operator cimport dereference as deref, preincrement as preinc


cdef extern from 'include/util.hpp':
    cdef cppclass Series_plus:
        string datetime
        double get(string& key) noexcept

    cdef cppclass ParamTuple:
        double val
        double macd
        double rsi
        double crsi
        double beta[2][2]
        PyObject* idx
        ParamTuple(double val, double macd, double rsi, double crsi, PyObject* idx)

    cdef struct OutcomeTuple:
        int pos;
        double fee;
        int slip;
        double balance;
        int earn;
        double max_drawdown;
        double pos_high;
        double pos_low;
        int earn_change;
        double ratio;

    cdef cppclass DataFrame:
        # DataFrame(const vector[string]& cols)
        DataFrame()
        DataFrame(const vector[string]&& cols)
        void append(const string& datetime, PyObject* ori_date, const string& date,
                    vector[double] && params)
        const vector[Series_plus]& values() const
        const size_t size() const

    double sharpe_ratio(vector[double]& balance, double risk_free_rate, double years)

cdef extern from "include/strategy.hpp":
    cdef cppclass BollingTrendMiddleClose:
        BollingTrendMiddleClose()
        double drawdown, val, macd_shreshod, rsi_shreshod, balance, fee, pos_high, pos_low
        int pos, slip, earn, earn_change
        double beta_shreshod[2][2]
        bint buy_sig, short_sig, cover_sig, sell_sig
        void on_bar(const Series_plus&) noexcept
        void renew(double val, double macd, double rsi, double crsi, const double beta[2][2]) noexcept

    cdef cppclass Bolling_Trend_upsert:
        Bolling_Trend_upsert()
        double drawdown, val, macd_shreshod, rsi_shreshod, balance, fee, pos_high, pos_low
        int pos, slip, earn, earn_change
        double beta_shreshod[2][2]
        bint buy_sig, short_sig, cover_sig, sell_sig
        void on_bar(const Series_plus&) noexcept
        void renew(double val, double macd, double rsi, double crsi, const double beta[2][2]) noexcept


cdef extern from "include/entry.hpp":
    void run_backtest_no_df(const DataFrame& cdata, const vector[ParamTuple]& params, vector[OutcomeTuple]& outcomes,
                    vector[PyObject*]& result, double years, PyObject* args) noexcept


cdef void data_to_cdata(object data, DataFrame& cdata) noexcept:
    vals = data.values
    for line in vals:
        cdata.append(line[0].encode('utf-8'),
                        <PyObject*>line[1],
                        line[2].encode('utf-8'),
                        <vector[double]> line[3:])

cdef void param_to_cparam(object param, vector[ParamTuple]& cparam):

    return


cpdef void run(object data, list queue, res_queue, columns, args, double years, bint get_df=True):
    cdef Bolling_Trend_upsert st
    cdef double max_drawdown, val, macd, rsi
    cdef int count
    cdef int i
    cdef vector[double] balanceArr = [0]
    cdef list res, outcome
    cdef vector[ParamTuple] param_queue
    cdef vector[OutcomeTuple] outcomes
    cdef vector[PyObject*] result
    cdef PyObject* _tmp_rr
    cdef PyObject*_tmp2
    cdef DataFrame cdata
    cdef vector[Series_plus].const_iterator cdata_iter
    cdef int divide;

    cols = [line.encode('utf-8') for line in data.columns[3:]]
    cdata = DataFrame(<vector[string]>cols)
    data_to_cdata(data, cdata);
    data.drop(data.index, inplace=True);


    for params, ids in queue:
        val, macd, beta, rsi = params
        param_queue.push_back(ParamTuple(val, macd, rsi, 0.0, <PyObject*>ids))
        param_queue.back().beta[0][0] = beta[0][0]
        param_queue.back().beta[0][1] = beta[0][1]
        param_queue.back().beta[1][1] = beta[1][1]
        param_queue.back().beta[1][0] = beta[1][0]

    if get_df:
        st = Bolling_Trend_upsert()
        for i in range(param_queue.size()):
            res = []
            st.renew(param_queue[i].val, param_queue[i].macd,
                        param_queue[i].rsi, param_queue[i].crsi , param_queue[i].beta)
            cdata_iter = cdata.values().const_begin();
            max_drawdown=0
            count = 0
            divide = cdata.size()>>4
            balanceArr.clear()
            balanceArr.reserve(20)
            while(cdata_iter != cdata.values().const_end()):
                st.on_bar(deref(cdata_iter))
                res.append((deref(cdata_iter).datetime, st.pos, st.fee, st.slip, st.balance,
                        st.earn, st.drawdown, st.buy_sig, st.short_sig, st.sell_sig, st.cover_sig,
                        st.pos_high, st.pos_low, st.earn_change))
                preinc(cdata_iter)
                max_drawdown = max(st.drawdown, max_drawdown)
                preinc(count)
                if (count == divide):
                    balanceArr.push_back(st.balance)
                    count = 0
            df = pd.DataFrame(res, columns=columns)
            outcome = list(res[-1])
            sharp = sharpe_ratio(balanceArr, 0.02, 3.5)
            res_queue.put((df, outcome, (st.val, st.macd_shreshod, st.rsi_shreshod,
                            st.beta_shreshod), args, <object>param_queue[i].idx, sharp))
        pass
    else:
        _tmp2 = <PyObject*>args
        run_backtest_no_df(cdata, param_queue, outcomes, result, years, _tmp2)
        set_val(res_queue, outcomes, result)



cdef void set_val(res_queue,vector[OutcomeTuple]& outcomes, vector[PyObject*]& result,):
    cdef PyObject * result_list;
    cdef OutcomeTuple _tmp
    cdef int i;
    for i in range(outcomes.size()):
        result_list = result.at(i)
        result_obj = <object>result_list
        Py_DECREF(result_obj)
        _tmp = outcomes.at(i)
        outcome_tuple = (None, _tmp.pos,
                               _tmp.fee,
                               _tmp.slip,
                               _tmp.balance,
                               _tmp.earn,
                               _tmp.max_drawdown,
                               False, False, False, False,
                               _tmp.pos_high,
                               _tmp.pos_low,
                               _tmp.earn_change)
        result_obj[1] = outcome_tuple
        result_obj[5] = _tmp.ratio
        res_queue.put(result_obj)
