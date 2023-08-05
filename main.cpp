// #include <iostream>
#include <util.hpp>
#include <strategy.hpp>
#include <chrono>
// #include <Windows.h>
#include <entry.hpp>
// #include <research.cpp>
#undef min
#undef max


void test(const DataFrame& df, std::vector<OutcomeTuple>& outcomes,const ParamTuple& t, int i) {
    BaseStrategy * stp = getInstance();
    BaseStrategy& st = *stp;
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
            OutcomeTuple{st.pos, st.fee, (int)st.slip, st.balance, (int)st.earn, max_drawdown,st.pos_high, st.pos_low, (int)st.earn_change, ratio}
    );
    destroyInstance(stp);
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
    // scanf_s("%d %d", &i, &j);
    printf("got: %d", i);
    std::vector<OutcomeTuple> outcomes; outcomes.resize(20);
    // i=10000;
    for (int p=0; p<i; p++) {
        std::string date = "2023-12-12";
        std::string datetime = "2023-12-12 12:12:12";
        t.append(date, 0x0, datetime, DoubleArr{1423, 1234, 2345, 1232, 12, 10, 1234, 1234, 1233, 1.2, 34, 0.2, -0.1});
        t.append(date, 0x0, datetime, DoubleArr{1423, 1236, 2345, 1232, 12, 10, 1235, 1234, 1233, 1.2, 34, 0.2, -0.1});
    }

    printf("start_calculation\n");
    bool flag = true;
    // int i;
    // std::string val{"2021-12-29"};
    // char s[12];
    // scanf_s("%d", &i);
    auto start_time = std::chrono::high_resolution_clock::now();
    ChangeMain cm(SA_change);
    for (; i; i--) {
        // data += sharpe_ratio(balance, 0.02, 3.5);
        // flag &= value_in_arr(vals[i].c_str(), SA_change);
        // flag &= value_in_arr(val.c_str(), SA_change);
        // flag &= (bool)SA_changes.count(val);
        // flag &= cm.is_change(val);

        test(t, outcomes, r, j);
    }
    // flag = (val[3]>(*SA_change)[3]) | (val[5]>(*SA_change)[5]) | (val[6]>(*SA_change)[6])|(val[8]>(*SA_change)[8])|(val[9]>(*SA_change)[9]);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
    printf("time: %d ns. %d %s\n", (int)(duration.count()/200000000), flag, cm.get_next_date().c_str());
    printf("%lf\n", outcomes[0].balance);
    return 0;
}
