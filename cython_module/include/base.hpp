#ifndef __CZQ_STRUCTURE__
#define __CZQ_STRUCTURE__
#include <Python.h>

#include <vector>
#include <string>
#include <array>
#include <unordered_map>

#include <stdexcept>
#include <algorithm>
#include <numeric>
#include "utils/math_func.hpp"
#include <initializer_list>
#define CACHE_LINE_SIZE 64
#define _CACHE_ALIGN alignas(CACHE_LINE_SIZE)

#if CUDA_ENABLE
#include <cuda_runtime.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#endif

#include "utils/data.hpp"

class BarData {
public:
    char date[11] {0};
    char datetime[20] {0};
    char symbol[18] {0};
    double open, close;
    double high, low;
    int hour, minute;

    BarData(const Series_plus& sr):
        open(sr.get_val("open")), close(sr.get_val("close")), high(sr.get_val("high")),
        low(sr.get_val("low")),hour(sr.get_val("hour")), minute(sr.get_val("minute")) {
        memcpy(date, sr.date.data(), 10);
        memcpy(datetime, sr.datetime.data(), 19);
        memcpy(symbol, sr.symbol.data(), std::min(sr.symbol.size()+1, 16ull));
    }

    BarData(const char syb[16]) {
        memcpy(symbol, syb, 16);
    }

    BarData(const char syb[16], const char datetime[19]) {
        memcpy(this->symbol, syb, 16);
        memcpy(this->datetime, datetime, 19);
        memcpy(this->date, datetime, 10);
    }

};

struct OutcomeTuple {
    int pos;
    double fee;
    int slip;
    double balance;
    int earn;
    double max_drawdown;
    int earn_change;
    double ratio;
};


// Structures:

/* Fast Find if Main Contract Changed. */
class ChangeMain {
    const char (*change)[11];
public:

    ChangeMain(const char (*SA_change)[11]): change(SA_change) {}

    ChangeMain& operator=(ChangeMain&& obj) {
        this->change = obj.change;
        return *this;
    }

    bool is_change(const std::string& val) {
        bool flag = (val.size()==10)&&(
                (val[3]==(*change)[3])&&(val[5]==(*change)[5])&&(val[6]==(*change)[6])
                        &&(val[8]==(*change)[8])&&(val[9]==(*change)[9]));
        // time is moving at one direction
        change += flag;
        return flag;
    }

    std::string get_next_date() {
        return std::string(*change);
    }

    void get_next_date(char *p) {
        memcpy((void*)*p, (void*)*change, 10);
    }
};

// ---------------------------------------
// **************************************
// *********** Util Classes *************
// **************************************



// ArrayManagers & TechnicalIndex
#include "utils/array_manager.hpp"

using MACD = Calculator_MACD;
using RSI = Calculator_RSI;
using BBAND = Calculator_BollingBand;

template <typename ..._IndexType>
using ArrayManager = MultiSyb_IndexManager<_IndexType...>;


// for Cython Intelligence Lightlight Use
typedef std::vector<double> DoubleArr;
typedef std::vector<std::vector<double>> double_2D_Arr;
typedef PyObject*  _Object;
#endif
