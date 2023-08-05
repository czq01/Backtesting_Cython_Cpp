#include <vector>
#include <string>
#include <unordered_map>
#include <Python.h>
#include <stdexcept>
#include <algorithm>
#include <type_traits>
#include <numeric>
#include <cmath>


#define CACHE_LINE_SIZE 64
#define _CACHE_ALIGN alignas(CACHE_LINE_SIZE)

#ifndef __UTIL_OWN
#define __UTIL_OWN

class ChangeMain {  // main contract change
    const char (*change)[11];
public:

    ChangeMain(const char (*SA_change)[11]): change(SA_change) {}
    bool is_change(const std::string& val) {
        register bool flag = (val.size()==11)&&(
            (val[3]==(*change)[3])&&(val[5]==(*change)[5])&&(val[6]==(*change)[6])
                    &&(val[8]==(*change)[8])&&(val[9]==(*change)[9]));
        // time is moving at one direction
        change += (val[3]>(*change)[3])||(val[5]<=(*change)[5])||(val[6]<=(*change)[6])
                                ||(val[8]>(*change)[8])||(val[9]>(*change)[9]);
        return flag;
    }

    std::string get_next_date() {
        return std::string(*change);
    }
};

class Series_plus {
private:
    typedef std::string _String;
    typedef std::vector<double> _Array;
    typedef std::vector<_String> _KeyArray;
    typedef std::unordered_map<_String, int> _IdxMap;

    PyObject  *ori_date;
    _Array _CACHE_ALIGN params;
    _KeyArray *cols;
    _IdxMap* col_mapping;

    void _update_values() {
        this->open = this->get("open");
        this->close = this->get("close");
        this->high = this->get("high");
        this->low = this->get("low");
        this->hour = this->get("hour");
        this->minute = this->get("minute");
        this->H = this->get("H");
        this->M = this->get("M");
        this->L = this->get("L");
        this->b5 = this->get("bolling_beta_5");
        this->b10 = this->get("bolling_beta_10");
        this->rsi_5 = this->get("rsi_5");
        this->macd = this->get("MACD");
    }

    Series_plus(const _String& datetime, PyObject* ori_date, const _String& date,
            _Array &&params, _KeyArray * cols, _IdxMap* col_mapping = 0) :
        col_mapping(col_mapping), ori_date(ori_date),
        date(date), datetime(datetime),
        cols(cols),
        params(std::forward<_Array&&>(params)) {
        this->_update_values();
    }

    friend class DataFrame;

public:
    _String date;
    _String datetime;
    double open;
    double close;
    double high;
    double low;
    int hour;
    int minute;
    double H;
    double M;
    double L;
    double rsi_5;
    double b5;
    double b10;
    double macd;
    Series_plus() = delete;

    double get(const std::string& key) const {
        if (this->col_mapping->count(key)) {
            return this->params.at(col_mapping->at(key));
        } else {
            throw std::invalid_argument("[get_index] Key not exist in Series.");
        }
    }
};

class DataFrame {
    typedef std::string _String;
    typedef std::vector<double> _Array;
    typedef std::vector<_String> _KeyArray;
    typedef std::unordered_map<_String, int> _IdxMap;

    _IdxMap _CACHE_ALIGN col_mapping;
    _KeyArray _CACHE_ALIGN cols;
    std::vector<Series_plus> _CACHE_ALIGN series;
public:
    DataFrame() {} // Do Not Call in C++ !!! Cython Only

    DataFrame(const _KeyArray& cols): cols(cols) {
        for (unsigned int i=0; i<this->cols.size(); i++) {
            (col_mapping)[this->cols[i]]=i;
        }
    }

    DataFrame(_KeyArray&& cols): cols(cols) {
        for (unsigned int i=0; i<this->cols.size(); i++) {
            (col_mapping)[this->cols[i]]=i;
        }
    }

    void append(const _String& datetime, PyObject* ori_date, const _String& date,
            _Array && params) {
        series.emplace_back(Series_plus{
            datetime, ori_date, date, std::forward<_Array&&>(params),
            &this->cols, &this->col_mapping
        });
    }

    const std::vector<Series_plus>& values() const {
        return series;
    }

    size_t size() const {return series.size();}
};

class ParamTuple {
public:
    double val;
    double macd;
    double rsi;
    double crsi;
    double beta[2][2]{0};
    PyObject * idx;
    ParamTuple(double val, double macd, double rsi, double crsi, PyObject* idx):
        val(val), macd(macd), rsi(rsi), idx(idx) {}

    PyObject* got_PyTuple_beta() const noexcept {
        PyObject* beta_1 = PyTuple_New(2); {
            PyTuple_SET_ITEM(beta_1, 0, PyFloat_FromDouble(beta[0][0]));
            PyTuple_SET_ITEM(beta_1, 1, PyFloat_FromDouble(beta[0][1]));
        }
        PyObject* beta_2 = PyTuple_New(2); {
            PyTuple_SET_ITEM(beta_2, 0, PyFloat_FromDouble(beta[1][0]));
            PyTuple_SET_ITEM(beta_2, 1, PyFloat_FromDouble(beta[1][1]));
        }
        PyObject* beta_tuple = PyTuple_New(2); {
            PyTuple_SET_ITEM(beta_tuple, 0, beta_1);
            PyTuple_SET_ITEM(beta_tuple, 1, beta_2);
        }
        return beta_tuple;
    }

    PyObject * to_PyTuple() const noexcept {
        PyObject* tuple = PyTuple_New(4);{
            PyTuple_SET_ITEM(tuple, 0, PyFloat_FromDouble(val));
            PyTuple_SET_ITEM(tuple, 1, PyFloat_FromDouble(macd));
            PyTuple_SET_ITEM(tuple, 2, PyFloat_FromDouble(rsi));
            PyTuple_SET_ITEM(tuple, 3, got_PyTuple_beta());
        }
        return tuple;
    }

};

class BarReturn{
public:
    int type;
    union {
        PyObject * pyo;
        double dval;
        bool bval;
        int ival;
    };
    char p[24];
    template <typename T, typename std::enable_if_t<
            std::is_same<T, double>::value || std::is_same<T, int>::value
            || std::is_same<T, bool>::value || std::is_same<T, PyObject*>::value, bool> = true>
    BarReturn(T val) {
        if constexpr(std::is_same<T, double>::value) {
            dval = val;
            type = 0;
        } else if constexpr(std::is_same<T, int>::value) {
            ival = val;
            type = 1;
        } else if constexpr(std::is_same<T, bool>::value) {
            bval = val;
            type = 2;
        } else if constexpr(std::is_same<T, PyObject*>::value) {
            pyo = val;
            type = 3;
        }
    }

    void get_val() {
    }

};

struct OutcomeTuple {
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
};

typedef std::vector<double> DoubleArr;
typedef std::vector<std::vector<double>> double_2D_Arr;


inline double mean(const std::vector<double>& v) {
    return std::accumulate(v.begin(), v.end(), 0.0) / v.size();
}

// Calculate the standard deviation of a vector
double stddev(const std::vector<double>& v, double mean) {
    double sum = 0.0;
    for(double x : v) {
        double diff = x - mean;
        sum += diff * diff;
    }
    return std::sqrt(sum / (v.size() - 1));
}

template <size_t N>
constexpr double stddev(const double (&v)[N], double mean) {
    double sum = 0.0;
    for(double x : v) {
        double diff = x - mean;
        sum += diff * diff;
    }
    return std::sqrt(sum / (N-1));
}

// Calculate the Sharpe Ratio
double sharpe_ratio(const std::vector<double>& balance, double risk_free_rate, double period_year_count) {
    // Calculate the returns
    // const int return_size = balance.size()-1;
    const int return_size = 15;
    // std::vector<double> _CACHE_ALIGN returns(return_size, 0);
    double returns[15];
    double adjust_val = static_cast<double>(return_size+1)/period_year_count;
    for(int i = 0; i < return_size; i++) {
        returns[i] = ((balance[i+1] - balance[i]) / (balance[i]+20000));
    }

    // Calculate the mean and standard deviation of returns
    double mean_return = balance[return_size]/return_size/20000;
    double stddev_return = stddev(returns, mean_return);

    // Calculate the Sharpe Ratio
    return (mean_return - risk_free_rate) / stddev_return * sqrt(adjust_val);
}

template <size_t N>
constexpr inline bool is_same_char(const char * val, const char (&arr)[N]) {
    return (val[3]==arr[3]) & (val[5]==arr[5]) & (val[6]==arr[6]) & (val[8]==arr[8]) & (val[9]==arr[9]);
}

template <size_t N, size_t M>
constexpr inline bool value_in_arr(const char * val, const char (&arr)[M][N]) {
    return _value_in_arr<N, M>(val, (const char (*) [N])arr);
}

template <size_t N, size_t M>
constexpr inline bool _value_in_arr(const char * val, const char (*arr) [N]) {
    if constexpr(M==1) return is_same_char(val, *arr);
    else return is_same_char(val, *arr) & _value_in_arr<N, M-1>(val, arr+1);
}
#endif