#include <util.hpp>
#include <strategy.hpp>
#include <chrono>
#include <entry.hpp>

typedef std::vector<PyObject *> _Params;
typedef std::vector<OutcomeTuple> _Outcomes;


void get_test_data(DataFrame& df, _Params& params, int length ) {
    PyObject * param = PyList_New(3);
    PyList_SET_ITEM(param, 0, PyFloat_FromDouble(1.0));
    PyList_SET_ITEM(param, 1, PyFloat_FromDouble(1.0));
    PyList_SET_ITEM(param, 2, PyFloat_FromDouble(0.0));

    df = DataFrame(std::vector<std::string>{"open","close", "high", "low"});

    for (int p=0; p<length/2; p++) {
        std::string date = "2020-03-01";
        std::string datetime = "2020-03-01 12:12:12";
        df.append(datetime, date, "SA2005.XZCE", DoubleArr{1423, 1234, 2345, 1232});
        df.append(datetime, date, "SA2009.XZCE", DoubleArr{1423, 1236, 2345, 1232});
    }
    for (int p=0; p<3; p++) {
        std::string date = "2020-04-17";
        std::string datetime = "2020-03-01 12:12:12";
        df.append(datetime, date, "SA2005.XZCE", DoubleArr{1423, 1234, 2345, 1232});
        df.append(datetime, date, "SA2009.XZCE", DoubleArr{1423, 1236, 2345, 1232});
    }
    for (int p=0; p<length/2; p++) {
        std::string date = "2020-05-01";
        std::string datetime = "2020-05-01 12:12:12";
        df.append(datetime, date, "SA2009.XZCE", DoubleArr{1423, 1234, 2345, 1232});
        df.append(datetime, date, "SA2101.XZCE", DoubleArr{1423, 1236, 2345, 1232});
    }
    for (int i=0; i<1000; i++) {
        params.push_back(param);
    }
}



int main(int, char**){
    int length=317520;
    Py_Initialize();
    DataFrame t;
    _Params params;
    _Outcomes outcomes; outcomes.resize(20);
    get_test_data(t, params, length);

    // t = __pyx_f_8research_get_data("F:/trading/research/data/SAbar_data.csv");
    long long time = time_test(run_backtest_no_df, t, params, outcomes, 3.5);
    printf("strategy on_bar() time per loop: %lld ns, total: %lld ms\n", time/1000/length, time/1000000);
    Py_Finalize();
    return 0;
}
