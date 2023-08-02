// #include <iostream>
#include <util.hpp>
#include <strategy.hpp>
#include <chrono>
// #include <Windows.h>
// #include <entry.hpp>
// #include <research.cpp>
#undef min
#undef max


void test(const DataFrame& df, std::vector<OutcomeTuple>& outcomes,const ParamTuple& t, int i) {
    Bolling_Trend_upsert st;
    st.renew(t.val, t.macd, t.rsi, t.crsi, t.beta);
    int count=0;
    double max_drawdown=0;
    const int divide = (df.values().size()>>4);
    std::vector<double> ratio_vec; ratio_vec.reserve(2047);
    for (auto&& data: df.values()) {
            st.on_bar(data);
            max_drawdown = std::max(max_drawdown, st.drawdown);
            count++;
            if (count==divide) {ratio_vec.push_back(st.balance); count=0;}
    }
    double ratio = sharpe_ratio(ratio_vec, 0.02, 3.5);
    outcomes[i] = std::forward<const OutcomeTuple>(
            OutcomeTuple{st.pos, st.fee, st.slip, st.balance, st.earn, max_drawdown,st.pos_high, st.pos_low, st.earn_change, ratio}
    );
}


int main(int, char**){
    ParamTuple r(0,0,100,0, 0); {
        r.beta[0][0]=0;
        r.beta[0][1]=0;
        r.beta[1][0]=0;
        r.beta[1][1]=0;
    }

    std::vector<std::string> cols{"open","close", "high", "low", "minute", "hour", "H", "M",
        "L", "MACD", "rsi_5", "bolling_beta_5", "bolling_beta_10"};
    DataFrame t(cols);
    int i=10000;
    int j=0;
    scanf_s("%d %d", &i, &j);
    printf("got: %d", i);
    std::vector<OutcomeTuple> outcomes; outcomes.resize(20);
    // i=10000;
    for (int p=0; p<i; p++) {
        std::string date = "2023-12-12";
        std::string datetime = "2023-12-12 12:12:12";
        t.append(date, 0x0, datetime, DoubleArr{1423, 1234, 2345,1232, 12, 10, 1234, 2345, 3245, 1.2, 34, 0.2, -0.1});
    }

    printf("start_calculation\n");
    bool flag = true;
    auto start_time = std::chrono::high_resolution_clock::now();
    for (; i; i--) {
        // data += sharpe_ratio(balance, 0.02, 3.5);
        // flag &= value_in_arr(vals[i].c_str(), SA_change);
        // flag &= value_in_arr(val.c_str(), SA_change);
        // flag &= (bool)SA_changes.count(val);
        test(t, outcomes, r, j);
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    printf("time: %d ns. %d\n", (int)(duration.count()/100000000), flag);
    return 0;
}
