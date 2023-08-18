#include <util.hpp>
#include <strategy.hpp>
#include <chrono>
#include <entry.hpp>

typedef std::vector<PyObject *> _Params;
typedef std::vector<OutcomeTuple> _Outcomes;

void get_test_data(DataFrame& df, _Params& params, int length ) {
    PyObject * param = PyList_New(6);
    PyList_SET_ITEM(param, 0,PyFloat_FromDouble(0.1));
    PyList_SET_ITEM(param, 1,PyFloat_FromDouble(0.1));
    PyList_SET_ITEM(param, 2,PyFloat_FromDouble(0.1));
    PyObject * l3 = PyTuple_New(2);{
        PyObject * l1 = PyTuple_New(2); {
            PyTuple_SET_ITEM(l1, 0, PyFloat_FromDouble(0.0));
            PyTuple_SET_ITEM(l1, 1, PyFloat_FromDouble(0.0));
        }
        PyObject * l2 = PyTuple_New(2);{
            PyTuple_SET_ITEM(l2, 0, PyFloat_FromDouble(0.0));
            PyTuple_SET_ITEM(l2, 1, PyFloat_FromDouble(0.0));
        }
        PyTuple_SET_ITEM(l3, 0 , l1);
        PyTuple_SET_ITEM(l3, 1 , l2);
    }
    PyList_SET_ITEM(param, 3, l3);
    PyList_SET_ITEM(param, 4, PyLong_FromLong(10l));
    PyList_SET_ITEM(param, 5, PyFloat_FromDouble(2.0));

    df = DataFrame(std::vector<std::string>{"open","close", "high", "low"});
    for (int p=0; p<length; p++) {
        std::string date = "2023-12-12";
        std::string datetime = "2023-12-12 12:12:12";
        df.append(datetime, date, DoubleArr{1423, 1234, 2345, 1232});
        df.append(datetime, date, DoubleArr{1423, 1236, 2345, 1232});
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

    long long time = time_test(run_backtest_no_df, t, params, outcomes, 3.5);
    printf("strategy on_bar() time per loop: %lld ns, total: %lld ms\n", time/1000/length, time/1000000);

    Py_Finalize();
    return 0;
}
