#include <util.hpp>
#include <strategy.hpp>
#include <chrono>
#include <entry.hpp>


typedef std::vector<ParamTuple> _Params;
typedef std::vector<OutcomeTuple> _Outcomes;

void get_test_data(DataFrame& df, _Params& params, int length ) {
    PyObject * idx = PyLong_FromLong(0l);
    df = DataFrame(std::vector<std::string>{"open","close", "high", "low", "minute", "hour", "H", "M",
        "L", "MACD", "rsi_5", "bolling_beta_5", "bolling_beta_10"});
    ParamTuple r(0, 0, 100, 0, idx); {
        r.beta[0][0]=0;
        r.beta[0][1]=0;
        r.beta[1][0]=0;
        r.beta[1][1]=0;
    }
    for (int p=0; p<length; p++) {
        std::string date = "2023-12-12";
        std::string datetime = "2023-12-12 12:12:12";
        df.append(date, 0, datetime, DoubleArr{1423, 1234, 2345, 1232, 12, 10, 1234, 1234, 1233, 1.2, 34, 0.2, -0.1});
        df.append(date, 0, datetime, DoubleArr{1423, 1236, 2345, 1232, 12, 10, 1235, 1234, 1233, 1.2, 34, 0.2, -0.1});
    }
    for (int i=0; i<500; i++) {
        params.push_back(r);
        params.push_back(r);
    }
}

int main(int, char**){

    int length=317520;

    Py_Initialize();
    DataFrame t;
    _Params params;
    std::vector<PyObject*> result_list;
    _Outcomes outcomes; outcomes.resize(20);
    PyObject * args = PyLong_FromLong(0l);
    get_test_data(t, params, length);

    long long time = time_test(run_backtest_no_df, t, params, outcomes, result_list, 3.5, args);
    printf("time: %lld ns.\n", time/1000);

    Py_DECREF(args);
    Py_Finalize();
    return 0;
}
