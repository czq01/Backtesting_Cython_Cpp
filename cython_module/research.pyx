# cython: language_level=3
# distutils: language=c++
import pandas as pd
from libc.stdio cimport *
from libc.stdlib cimport *
from libcpp.string cimport string
from libcpp.string cimport *
from libcpp.vector cimport vector
from cpython.ref cimport Py_DECREF, Py_INCREF, PyObject
from cython.operator cimport dereference as deref, preincrement as preinc

cdef extern from "include/entry.hpp":

    ctypedef PyObject* _Object

    cdef cppclass BarData:
        string datetime

    cdef struct OutcomeTuple:
        int pos;
        double fee;
        int slip;
        double balance;
        int earn;
        double max_drawdown;
        int earn_change;
        double ratio;

    cdef cppclass DataFrame:
        # DataFrame(const vector[string]& cols)
        DataFrame()
        DataFrame(const vector[string]&& cols)
        void append(const string& datetime, const string& date,
                const string& symbol, vector[double] && params)

    cdef cppclass Strategy:
        Strategy()
        double drawdown, balance, fee
        int pos, slip, earn, earn_change
        bint buy_sig, short_sig, cover_sig, sell_sig
        void on_bar(const BarData&) noexcept
        void renew(PyObject *) noexcept
        void get_next_min_bounds(const BarData&) noexcept
        bint loading_data(const BarData&) noexcept

    vector[BarData] barFromDataframe(const DataFrame& cdata)
    double sharpe_ratio(vector[double]& balance, double risk_free_rate, double years)
    void run_backtest_no_df(const DataFrame& cdata, const vector[_Object]& params,
                    vector[OutcomeTuple]& outcomes, double years) noexcept


cdef void data_to_cdata(object data, DataFrame& cdata) noexcept:
    vals = data.values
    for line in vals:
        cdata.append(line[0].encode('utf-8'),  #datetime
                        line[1].encode('utf-8'),  #trading_date
                        line[2].encode('utf-8'),  #symbol
                        <vector[double]> line[3:])


cdef run_backtest_df(const DataFrame& cdata, const vector[_Object]& param, columns, res_queue):
    cdef Strategy st
    cdef int count
    cdef int i
    cdef vector[double] balanceArr = [0]
    cdef list res, outcome
    cdef int divide
    cdef double max_drawdown
    cdef vector[BarData].const_iterator cdata_iter

    cdef vector[BarData] data = barFromDataframe(cdata)

    for i in range(param.size()):
        res = []
        st.renew(param[i])
        max_drawdown=0
        count = 0
        balanceArr.clear()
        cdata_iter = data.const_begin();
        while (cdata_iter != data.const_end() and
                not st.loading_data(deref(cdata_iter))):
            preinc(cdata_iter); preinc(count);
        preinc(cdata_iter); preinc(count);
        divide = (data.size()-count)>>5
        count = 0;
        while(cdata_iter != data.const_end()):
            st.on_bar(deref(cdata_iter))
            res.append((deref(cdata_iter).datetime, st.pos, st.fee, st.slip, st.balance,
                    st.earn, st.drawdown, st.buy_sig, st.short_sig, st.sell_sig, st.cover_sig,
                    st.earn_change))
            preinc(cdata_iter); preinc(count)
            max_drawdown = max(st.drawdown, max_drawdown)
            if (count == divide):
                balanceArr.push_back(st.balance)
                count = 0
        df = pd.DataFrame(res, columns=columns)
        outcome = list(res[-1])
        sharp = sharpe_ratio(balanceArr, 0.02, 3.5)
        outcome.append(sharp)
        # Py_DECREF(_)
        res_queue.put([df, outcome , <object>(param[i])])


cpdef void run(object data, list params, object res_queue, object columns, double years, bint get_df=True):
    cdef vector[_Object] cparam
    cdef DataFrame cdata
    cdef vector[OutcomeTuple] outcomes

    cols = [line.encode('utf-8') for line in data.columns[3:]]
    cdata = DataFrame(<vector[string]>cols)
    data_to_cdata(data, cdata);
    for param in params:
        cparam.push_back(<_Object>param)
        Py_INCREF(param)

    if get_df:
        run_backtest_df(cdata, cparam, columns, res_queue)
    else:
        run_backtest_no_df(cdata, cparam, outcomes, years)
        set_val(res_queue, outcomes, cparam)

cdef void set_val(res_queue, vector[OutcomeTuple]& outcomes, vector[_Object]& cparam):
    cdef OutcomeTuple _tmp
    cdef int i
    for i in range(outcomes.size()):
        _tmp = outcomes.at(i)
        outcome_tuple = [None, _tmp.pos,
                               _tmp.fee,
                               _tmp.slip,
                               _tmp.balance,
                               _tmp.earn,
                               _tmp.max_drawdown,
                               False, False, False, False,
                               _tmp.earn_change,
                               _tmp.ratio]
        params = <object>cparam[i]
        Py_DECREF(params)
        outcome_tuple.extend(params)
        res_queue.put(outcome_tuple)
