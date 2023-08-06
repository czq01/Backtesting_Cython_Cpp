#include <vector>
#include <string>
#include <unordered_map>
#include <Python.h>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <cmath>

#ifndef __CZQ_STRUCTURE__
#define __CZQ_STRUCTURE__
#define CACHE_LINE_SIZE 64
#define _CACHE_ALIGN alignas(CACHE_LINE_SIZE)

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

#endif