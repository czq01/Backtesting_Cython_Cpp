#ifndef __ENTRY_HPP__
#define __ENTRY_HPP__
#include "strategy.hpp"
#include "structure.hpp"
#include <iostream>
#include <thread>
#include <Windows.h>
#undef max
#undef min


std::vector<BarData> barFromDataframe(const DataFrame& cdata) {
    std::vector<BarData> result;
    for (auto&& i: cdata.values()) {
        result.emplace_back(BarData{i.symbol.data(), i.datetime.data()});
        result.back().close = i.get_val("close");
        result.back().high = i.get_val("high");
        result.back().low = i.get_val("low");
        result.back().open = i.get_val("open");
        result.back().hour = (i.datetime[11]-48)*10 + i.datetime[12]-48;
        result.back().minute = (i.datetime[14]-48)*10 + i.datetime[15]-48;
    }
    return result;
}

// Entry: (no df  CPU)
void backtest_threads_no_df(const std::vector<BarData>& cdata, const std::vector<PyObject*>& params,
            std::vector<OutcomeTuple>& outcomes, double years, int start, int end) noexcept {
    double _CACHE_ALIGN ratio_vec[32];
    Strategy st;
    for (int i=start; i<end; i++) {
        st.renew(params[i]);
        double max_drawdown=0;
        int count=0;
        auto iter = cdata.cbegin();
        auto end = cdata.cend();

        // Pre-loading data for technical idnex
        while (iter!=end && !st.loading_data(*iter))
            {++iter;++count;}
        ++iter; ++count;

        // Run backtesting
        const int divide = (cdata.size()-count)>>5;
        int pos = 0;
        count = 0;
        for (;iter!=end;++iter) {
            st.on_bar(*iter);
            max_drawdown = std::max(max_drawdown, st.drawdown);
            count++;
            if (count==divide) {
                ratio_vec[pos]=st.balance;
                count=0; pos++;
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
#if CUDA_ENABLE
    thrust::host_vector<BarData> bar_data = barFromDataframe(cdata)
#else
    constexpr int THREAD_NUM=12;
    std::vector<BarData> bar_data = barFromDataframe(cdata);
    outcomes.resize(params.size());
    std::vector<std::thread> _CACHE_ALIGN ths;
    int divide_len = (params.size())/THREAD_NUM;
    int mod_len = (params.size() - divide_len*THREAD_NUM);
    int start=0;
    divide_len++;
    for (int i=0; i<mod_len; i++) {
        ths.emplace_back(std::thread(backtest_threads_no_df, std::ref(bar_data),
                std::ref(params), std::ref(outcomes), years, start, start+divide_len));
        start += divide_len;
    }
    divide_len--;
    for (int i=mod_len; i<THREAD_NUM; i++) {
        ths.emplace_back(std::thread(backtest_threads_no_df, std::ref(bar_data),
                std::ref(params), std::ref(outcomes), years, start, start+divide_len));
                start += divide_len;
    }

    for (std::thread& i: ths) {
        i.join();
    }

#endif
}





void run_backtest_no_df_cuda(const DataFrame& cdata, const std::vector<PyObject*>& params,
            std::vector<OutcomeTuple>& outcomes, const double& years) {



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