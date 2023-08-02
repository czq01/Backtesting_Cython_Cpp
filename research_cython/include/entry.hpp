#include "strategy.hpp"
#include "util.hpp"
#include <iostream>
#include <thread>
#include <Python.h>
#include <Windows.h>
#ifndef __ENTRY_HPP__
#define __ENTRY_HPP__
#undef max
#undef min


void backtest_threads(const DataFrame& cdata, const std::vector<ParamTuple>& params,
            std::vector<OutcomeTuple>& outcomes, PyObject* args,const double& years, int start, int end ) noexcept {
    std::vector<double> _CACHE_ALIGN ratio_vec; ratio_vec.reserve(2047);
    BollingTrendMiddleClose _CACHE_ALIGN st;
    const int _CACHE_ALIGN divide = (cdata.values().size()>>4);
    for (int i=start; i<end; i++) {
        const ParamTuple& _CACHE_ALIGN param = params[i];
        st.renew(param.val, param.macd, param.rsi, param.crsi, param.beta);
        double max_drawdown=0;
        int count=0;
        ratio_vec.clear();
        for (auto&& _CACHE_ALIGN data: cdata.values()) {
            st.on_bar(data);
            max_drawdown = std::max(max_drawdown, st.drawdown);
            if (count==divide) {ratio_vec.push_back(st.balance); count=0;}
            count++;
        }
        double _CACHE_ALIGN ratio = sharpe_ratio(ratio_vec, 0.02, years);
        outcomes[i] = std::forward<const OutcomeTuple>(
            OutcomeTuple{st.pos, st.fee, st.slip, st.balance, st.earn, max_drawdown,st.pos_high, st.pos_low, st.earn_change, ratio}
        );
    }
}

void new_list (std::vector<PyObject*>& tuple, int list_size, int size) {
    for (int i=0; i<size;i++) {
        PyObject* obj = PyList_New(list_size);
        if (obj) {tuple.push_back(obj);};
    }
}

void run_backtest_no_df(const DataFrame& cdata, const std::vector<ParamTuple>& params, std::vector<OutcomeTuple>& outcomes,
                        std::vector<PyObject*>& result_tuple, const double& years, PyObject* args) noexcept {
    constexpr int THREAD_NUM=8;
    // std::vector<PyObject*> outcome;
    outcomes.resize(params.size());
    new_list(result_tuple, 6, params.size());
    for (int i=0; i<params.size(); i++) {
        Py_INCREF(args); Py_INCREF(params[i].idx);
        Py_INCREF(Py_None);  Py_INCREF(Py_None); Py_INCREF(Py_None);
        PyList_SET_ITEM(result_tuple[i], 0, Py_None); // df
        PyList_SET_ITEM(result_tuple[i], 1, Py_None); // outcome
        PyList_SET_ITEM(result_tuple[i], 2, params[i].to_PyTuple());
        PyList_SET_ITEM(result_tuple[i], 3, args);
        PyList_SET_ITEM(result_tuple[i], 4, params[i].idx);
        PyList_SET_ITEM(result_tuple[i], 5, Py_None);
    }
    // backtest_threads(cdata, params, outcomes, args,3.5, 0, params.size());
    Py_BEGIN_ALLOW_THREADS
    std::vector<std::thread> _CACHE_ALIGN ths;
    int divide_len = (params.size())/THREAD_NUM;
    for (int i=0; i<THREAD_NUM-1; i++) {
        ths.emplace_back(std::thread(backtest_threads, cdata, std::ref(params), std::ref(outcomes), args, years, i*divide_len, (i+1)*divide_len));
    }
    ths.emplace_back(std::thread(backtest_threads, cdata, std::ref(params), std::ref(outcomes), args, years, (THREAD_NUM-1)*divide_len, params.size()));
    for (std::thread& i: ths) {
        i.join();
    }
    Py_END_ALLOW_THREADS
}

#endif