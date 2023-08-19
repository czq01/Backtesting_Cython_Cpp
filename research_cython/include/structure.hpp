#ifndef __CZQ_STRUCTURE__
#define __CZQ_STRUCTURE__
#include <vector>
#include <string>
#include <unordered_map>
#include <Python.h>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <cmath>
#define CACHE_LINE_SIZE 64
#define _CACHE_ALIGN alignas(CACHE_LINE_SIZE)

class Series_plus {
private:
    typedef std::string _String;
    typedef std::vector<double> _Array;
    typedef std::vector<_String> _KeyArray;
    typedef std::unordered_map<_String, int> _IdxMap;

    _Array _CACHE_ALIGN params;
    _KeyArray *cols;
    _IdxMap* col_mapping;

    void _update_values() {
        this->open = this->_get("open");
        this->close = this->_get("close");
        this->high = this->_get("high");
        this->low = this->_get("low");
        this->hour = (this->datetime[11]-48)*10 + this->datetime[12]-48;
        this->minute = (this->datetime[13]-48)*10 + this->datetime[14]-48;
    }

    double _get(const std::string& key) const {
        if (this->col_mapping->count(key)) {
            return this->params.at(col_mapping->at(key));
        } else {
            throw std::invalid_argument("[get_index] Key not exist in Series.");
        }
    }

    Series_plus(const _String& datetime, const _String& date,
                const _String& symbol, _Array &&params,
                _KeyArray * cols, _IdxMap* col_mapping = 0) :
        col_mapping(col_mapping),
        date(date), datetime(datetime), symbol(symbol),
        cols(cols), params(std::forward<_Array&&>(params)) {
        this->_update_values();
    }

    friend class DataFrame;

public:
    _String date, datetime, symbol;
    double open, close;
    double high, low;
    int hour, minute;
    Series_plus() {}
    Series_plus(const _String& symbol):
        symbol(symbol) {}
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

    void append(const _String& datetime, const _String& date,
            const _String& symbol, _Array && params) {
        series.emplace_back(Series_plus{
            datetime, date, symbol, std::forward<_Array&&>(params),
            &this->cols, &this->col_mapping
        });
    }

    const std::vector<Series_plus>& values() const {
        return series;
    }

    constexpr size_t size() const {return series.size();}
};

class ArrayManager {
    int pointer;
    double * closes;
    double sum;
    double sum_square;

public:
    bool is_inited;
    int max_size;
    double high;
    double low;
    double open_interest;

    ArrayManager(int size=0):
        max_size(size), closes(new double[size]),
        pointer(0), is_inited(false),
        sum(0), sum_square(0) {}

    ArrayManager(const ArrayManager&) = delete;

    ArrayManager(ArrayManager&& am):
        max_size(am.max_size),
        pointer(am.pointer), is_inited(am.is_inited),
        sum(am.sum), sum_square(am.sum_square) {
        double * tmp = closes;
        closes = am.closes;
        am.closes = tmp;
    }

    ArrayManager& operator=(const ArrayManager &) = delete;

    ArrayManager& operator=(ArrayManager && am) {
        max_size = am.max_size;
        pointer = am.pointer;
        is_inited = am.is_inited;
        sum = am.sum;
        sum_square = am.sum_square;
        double * tmp = closes;
        closes = am.closes;
        am.closes = tmp;
        return *this;
    }

    double get_val(const int offset) {
        if (pointer>=offset) return closes[pointer-offset];
        else return closes[pointer-offset+max_size];
    }

    void update_bar(const Series_plus& bar) {
        if (is_inited) {
            sum-=closes[pointer];
            sum_square -= bar.close*bar.close;
        }
        closes[pointer] = bar.close;
        sum += bar.close;
        sum_square += bar.close*bar.close;
        high = std::max(bar.high, high);
        low = std::min(bar.low, low);
        pointer++;
        if (pointer==max_size) {pointer=0; is_inited=true;}
    };

    double get_mean() {return sum/max_size;}

    double get_mean(int N, int offset=0) {
        double _sum = 0;
        int _p = this->pointer; _p--;
        int n = N;
        while (offset) {_p--; offset--;}
        while (N) {
            if (_p<0) {_p+= this->max_size;}
            _sum += this->closes[_p];
            N--;
        }
        return _sum/n;
    }

    double get_std() {return sqrt(sum_square/(max_size-1));}

    double get_std(int offset, int N) {
        double _sum_sq = 0;
        int _p = this->pointer; _p--;
        int n=N+1;
        while (offset) {_p--;offset--;}
        while (N) {
            if (_p<0) {_p+= this->max_size;}
            _sum_sq += this->closes[_p];
            N--;
        }
        return _sum_sq/n;
    }

    ~ArrayManager() {delete [] closes;}

    friend class MACD_Calculator;
};

class MACD_Calculator {
private:
    int fast;
    int slow;
    int _count;
    ArrayManager* am;
public:
    bool is_inited;
    double EMA_fast;
    double EMA_slow;
    double MACD;
    double DEA;
    double DIF;

    MACD_Calculator(): am(0), fast(0), slow(0), is_inited(false) {}

    MACD_Calculator(ArrayManager& am, int fast=12, int slow=26):
        am(&am), fast(fast), slow(slow), _count(1), DEA(0), is_inited(false),
        EMA_fast(NAN), EMA_slow(NAN), MACD(NAN), DIF(NAN) {}

    MACD_Calculator& operator=(MACD_Calculator&& mc) {
        this->am = mc.am;
        this->fast = mc.fast;
        this->slow = mc.slow;
    }

    // Call function exactly after each am.update_bar()
    constexpr void update() {
        if (!am->is_inited) {return;}
        if (is_inited) [[likely]] {
            EMA_fast = (EMA_fast*(fast-1)+2*am->closes[am->pointer])/(fast+1);
            EMA_slow = (EMA_slow*(slow-1)+2*am->closes[am->pointer])/(slow+1);
            DIF = EMA_fast-EMA_slow;
            MACD = (MACD*8 + DIF*2)/10;
            DEA = DIF - MACD;
            return;
        }
        if (EMA_fast == NAN) [[unlikely]] {
            EMA_fast = am->get_mean(fast);
            EMA_slow = am->get_mean(slow);
            DIF = EMA_fast - EMA_slow;
        } else [[likely]] {
            EMA_fast = (EMA_fast*(fast-1)+2*am->closes[am->pointer])/(fast+1);
            EMA_slow = (EMA_slow*(slow-1)+2*am->closes[am->pointer])/(slow+1);
            _count++;
            if (_count < 9) {
                DIF += EMA_fast-EMA_slow;
            } else {
                DIF += EMA_fast-EMA_slow;
                MACD = DIF/9;
                DIF = EMA_fast-EMA_slow;
                DEA = DIF-MACD;
                is_inited = true;
            }
        }
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


typedef std::vector<double> DoubleArr;
typedef std::vector<std::vector<double>> double_2D_Arr;

// for Cython Intelligence Lightlight Use
typedef PyObject*  Object;

#endif