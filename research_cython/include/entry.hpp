#include "strategy.hpp"
#include "util.hpp"
#include <iostream>
#include <thread>
#include <Windows.h>
#ifndef __ENTRY_HPP__
#define __ENTRY_HPP__
#undef max
#undef min


// Entry: (no df)
void backtest_threads_no_df(const DataFrame& cdata, const std::vector<PyObject*>& params,
            std::vector<OutcomeTuple>& outcomes, double years, int start, int end) noexcept {
    std::vector<double> _CACHE_ALIGN ratio_vec; ratio_vec.reserve(20);
    Strategy st;

    for (int i=start; i<end; i++) {
        st.renew(params[i]);
        double max_drawdown=0;
        int count=0;
        ratio_vec.clear();
        auto iter = cdata.values().cbegin();
        auto end = cdata.values().cend();
        while (iter!=end && !st.loading_data(*iter))
            {++iter;++count;}
        if (iter != end) {
            st.get_next_min_bounds(*iter);
            ++iter;++count;
        }
        const int divide = (cdata.size()-count)>>4;
        count = 0;
        for (;iter!=end;++iter) {
            st.on_bar(*iter);
            max_drawdown = std::max(max_drawdown, st.drawdown);
            count++;
            if (count==divide) {
                ratio_vec.push_back(st.balance); count=0;
            }
        }
        double ratio = sharpe_ratio(ratio_vec, 0.02, years);
        outcomes[i] = std::forward<OutcomeTuple>(
            OutcomeTuple{st.pos, st.fee, (int)st.slip, st.balance, (int)st.earn, max_drawdown, (int)st.earn_change, ratio}
        );
    }
}

void run_backtest_no_df(const DataFrame& cdata, const std::vector<PyObject*>& params,
            std::vector<OutcomeTuple>& outcomes, const double& years) noexcept {
    constexpr int THREAD_NUM=8;
    outcomes.resize(params.size());
    // Py_BEGIN_ALLOW_THREADS
    std::vector<std::thread> _CACHE_ALIGN ths;
    int divide_len = (params.size())/THREAD_NUM;
    for (int i=0; i<THREAD_NUM-1; i++) {
        ths.emplace_back(std::thread(backtest_threads_no_df, std::ref(cdata),
                std::ref(params), std::ref(outcomes), years, i*divide_len, (i+1)*divide_len));
    }
    ths.emplace_back(std::thread(backtest_threads_no_df, std::ref(cdata),
                std::ref(params), std::ref(outcomes), years, (THREAD_NUM-1)*divide_len, params.size()));
    // backtest_threads_no_df(cdata, params, outcomes, years, 0, params.size());
    for (std::thread& i: ths) {
        i.join();
    }
    // Py_END_ALLOW_THREADS
    // printf("out backtest\n");
}





// Entry: (with DF)
// void get_PyList_df(std::vector<PyObject*> df, int size) {
//     new_list(df, 14, size);
// }

// void backtest_threads_with_df(const DataFrame& cdata, const std::vector<PyObject*>& params,
//             std::vector<OutcomeTuple>& outcomes, PyObject* args,const double& years, int start, int end ) noexcept {
//     std::vector<double> _CACHE_ALIGN ratio_vec; ratio_vec.reserve(20);
//     Strategy st;
//     std::vector<PyObject*> size;
//     const int divide = (cdata.values().size()>>4);
//     for (int i=start; i<end; i++) {
//         st.renew(params[i]);
//         double max_drawdown=0;
//         int count=0;
//         ratio_vec.clear();
//         for (auto&& data: cdata.values()) {
//             st.on_bar(data);
//             max_drawdown = std::max(max_drawdown, st.drawdown);
//             count++;
//             if (count==divide) {ratio_vec.push_back(st.balance); count=0;}
//         }
//         double ratio = sharpe_ratio(ratio_vec, 0.02, years);
//         outcomes[i] = std::forward<const OutcomeTuple>(
//             OutcomeTuple{st.pos, st.fee, (int)st.slip, st.balance, (int)st.earn, max_drawdown,st.pos_high, st.pos_low, (int)st.earn_change, ratio}
//         );
//     }
// }

// void run_backtest_df(const DataFrame& cdata, const std::vector<ParamTuple>& params, std::vector<OutcomeTuple>& outcomes,
//                         std::vector<PyObject*>& result_list, const double& years, PyObject* args) noexcept {
//     // std::vector<PyObject*> outcome;
//     outcomes.resize(params.size());
//     new_list(result_list, 6, params.size());
//     set_list(params, result_list, args);
//     backtest_threads_with_df(cdata, params, outcomes, args, years, 0, params.size());
// }




#endif