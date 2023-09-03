#ifndef __CZQ_STRUCTURE__
#define __CZQ_STRUCTURE__
#include <vector>
#include <string>
#include <unordered_map>
#include <Python.h>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <functional>
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
        this->minute = (this->datetime[14]-48)*10 + this->datetime[15]-48;
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


#define ArrMgr_SubSize(n)  (sizeof(int) + sizeof(void*)*(n))

class ArrayManager {
    int pointer;
    double * closes;
    double sum;
    double sum_square;
    void * _subcls_update_func;

    void _update_child() {
        auto func_pointers = (std::function<void()>**)(static_cast<char*>(_subcls_update_func)+sizeof(int));
        int N = *static_cast<int*>(_subcls_update_func);
        while (N) {
            std::function<void()> *func = *(func_pointers+N-1);
            (*func)();
            N--;
        }
    }

public:
    bool is_inited;
    int max_size;
    int size;
    double high;
    double low;
    double open_interest;

    ArrayManager(int size=0):
        max_size(size), closes(new double[size]), pointer(-1), is_inited(false),
        sum(0), sum_square(0), size(0), high(-INFINITY), low(INFINITY),
        open_interest(0), _subcls_update_func(new char[ArrMgr_SubSize(0)]) {
            *(int *)_subcls_update_func = 0;
        }

    ArrayManager(const ArrayManager&) = delete;

    ArrayManager(ArrayManager&& am):
        max_size(am.max_size), pointer(am.pointer), is_inited(am.is_inited),
        sum(am.sum), sum_square(am.sum_square), high(am.high), low(am.low),
        size(am.size), open_interest(am.open_interest) {
        double * _tmp_close = closes;
        closes = am.closes;
        am.closes = _tmp_close;
        void * _tmp_sub_class = _subcls_update_func;
        _subcls_update_func = am._subcls_update_func;
        am._subcls_update_func = _tmp_sub_class;
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

    constexpr double get_val(const int offset) {
        if (pointer>=offset) return closes[pointer-offset];
        else return closes[pointer-offset+max_size];
    }

    void update_bar(const Series_plus& bar) {
        size++; pointer++;
        if (pointer == max_size) [[unlikely]] {pointer=0;}
        if (is_inited) [[likely]] {
            sum-=closes[pointer];
            sum_square -= bar.close*bar.close;
            size--;
        }
        closes[pointer] = bar.close;
        sum += bar.close;
        sum_square += bar.close*bar.close;
        high = std::max(bar.high, high);
        low = std::min(bar.low, low);
        if (pointer==max_size-1) {is_inited=true;}
        this->_update_child();
    };

    constexpr double get_mean() {return sum/max_size;}

    constexpr double get_mean(int N, int offset=0) {
        double _sum = 0;
        int _p = this->pointer;
        int n = N;
        while (offset) {_p--; offset--;}
        while (N) {
            if (_p<0) {_p+= this->max_size;}
            _sum += this->closes[_p];
            N--; _p--;
        }
        return _sum/n;
    }

    double get_std() {return std::sqrt(sum_square/(max_size-1));}

    double get_std(int offset, int N) {
        double _sum_sq = 0;
        int _p = this->pointer;
        int n=N+1;
        while (offset) {_p--;offset--;}
        while (N) {
            if (_p<0) {_p+= this->max_size;}
            _sum_sq += this->closes[_p];
            N--; _p--;
        }
        return sqrt(_sum_sq/n);
    }

    double get_close() {return this->closes[this->pointer];}

    ~ArrayManager() {delete [] closes; delete [] _subcls_update_func;}

    template <class Child> friend class Base_Index_Calculator;
};

template <class Child>
class Base_Index_Calculator {
public:
    ArrayManager* am;
    bool is_inited;
    Base_Index_Calculator(): am(0), is_inited(false) {}
    Base_Index_Calculator(ArrayManager& am):
        am(&am), is_inited(false) {
            int val = *(int *)(am._subcls_update_func);
            void * new_sub_arr;
            if (!(val&0b1)) {
                new_sub_arr = new char[ArrMgr_SubSize(val?val<<1: 2)];
                memcpy(new_sub_arr, am._subcls_update_func, ArrMgr_SubSize(val));
            } else {
                new_sub_arr = am._subcls_update_func;
            }
            val++;
            auto func = new std::function<void()>(std::bind(&Child::update, static_cast<Child*>(this)));
            // set it to func
            int length = ArrMgr_SubSize(val-1);
            *reinterpret_cast<std::function<void()>**>((char *) new_sub_arr + length) = func;
            *(int *)new_sub_arr = val;
            delete [] am._subcls_update_func;
            am._subcls_update_func = new_sub_arr;
    }
};

class MACD_Calculator: public Base_Index_Calculator<MACD_Calculator> {
private:
    int fast;
    int slow;
    int _count;
public:
    double EMA_fast;
    double EMA_slow;
    double MACD;
    double DEA;
    double DIF;

    MACD_Calculator(): fast(0), slow(0) {}

    MACD_Calculator(ArrayManager& am, int fast=12, int slow=26):
        Base_Index_Calculator(am), fast(fast), slow(slow), _count(1), DEA(0),
        EMA_fast(NAN), EMA_slow(NAN), MACD(NAN), DIF(NAN) {}

    MACD_Calculator& operator=(MACD_Calculator&& mc) {
        this->am = mc.am;
        this->fast = mc.fast;
        this->slow = mc.slow;
    }

    // Called at the end of am.update_bar()
    constexpr void update() {
        if (!am->is_inited) {return;}
        if (this->is_inited) [[likely]] {
            EMA_fast = (EMA_fast*(fast-1)+2*am->get_close())/(fast+1);
            EMA_slow = (EMA_slow*(slow-1)+2*am->get_close())/(slow+1);
            DIF = EMA_fast-EMA_slow;
            MACD = (MACD*8 + DIF*2)/10;
            DEA = DIF - MACD;
            return;
        }
        if (std::isnan(EMA_fast)) [[unlikely]] {
            EMA_fast = am->get_mean(fast);
            EMA_slow = am->get_mean(slow);
            DIF = EMA_fast - EMA_slow;
        } else [[likely]] {
            EMA_fast = (EMA_fast*(fast-1)+2*am->get_close())/(fast+1);
            EMA_slow = (EMA_slow*(slow-1)+2*am->get_close())/(slow+1);
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
typedef PyObject*  _Object;

#endif