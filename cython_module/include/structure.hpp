#ifndef __CZQ_STRUCTURE__
#define __CZQ_STRUCTURE__
#include <vector>
#include <string>
#include <array>
#include <unordered_map>
#include <Python.h>
#include <stdexcept>
#include <algorithm>
#include <numeric>
#include <functional>
#include "util.hpp"
#include <initializer_list>
#define CACHE_LINE_SIZE 64
#define _CACHE_ALIGN alignas(CACHE_LINE_SIZE)

#if CUDA_ENABLE
#include <cuda_runtime.h>
#include <thrust/host_vector.h>
#include <thrust/device_vector.h>
#endif



// ---------------------------------------
// **************************************
// *********** Util Classes *************
// **************************************


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

class Series_plus {
private:
    typedef std::string _String;
    typedef std::vector<double> _Array;
    typedef std::vector<_String> _KeyArray;
    typedef std::unordered_map<_String, int> _IdxMap;

    _Array _CACHE_ALIGN params;
    _KeyArray *cols;
    _IdxMap* col_mapping;

    Series_plus(const _String& datetime, const _String& date,
                const _String& symbol, _Array &&params,
                _KeyArray * cols, _IdxMap* col_mapping = 0) :
        col_mapping(col_mapping),
        date(date), datetime(datetime), symbol(symbol),
        cols(cols), params(std::forward<_Array&&>(params)) {}

    friend class DataFrame;

public:
    _String datetime, date, symbol;

    Series_plus() {}

    double get_val(const std::string& key) const {
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
    typedef std::vector<Series_plus> _ValArray;

    _IdxMap _CACHE_ALIGN col_mapping;
    _KeyArray _CACHE_ALIGN cols;
    _ValArray _CACHE_ALIGN series;
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


// ---------------------------------------
// **************************************
// *** Data Array Manage Classes *****
// **************************************

#define ArrMgr_SubSize(n)  (sizeof(int) + sizeof(void*)*(n))

/* Single-symbol ArrayManager: manual-managed.*/
class SingleArrayManager {
    int pointer;
    double * closes;
    double sum;
    double sum_square;
    void * _subcls_update_func;

    constexpr void _update_child() {
        auto func_pointers = (std::function<void()>**)(static_cast<char*>(_subcls_update_func)+sizeof(int));
        int N = *static_cast<int*>(_subcls_update_func);
        while (N) {
            std::function<void()> *func = *(func_pointers+N-1);
            (*func)();
            N--;
        }
    }

    void update_class(std::function<void()>* func) {
        int val = *static_cast<int*>(this->_subcls_update_func);
        void * new_sub_arr;
        if (!(val&0b1)) {
            // For cuda use
#if CUDA_ENABLE
            cudaMalloc(&new_sub_arr, ArrMgr_SubSize(val?val<<1: 2));
#else
            new_sub_arr = new char[ArrMgr_SubSize(val?val<<1: 2)];
#endif
            memcpy(new_sub_arr, this->_subcls_update_func, ArrMgr_SubSize(val));
        } else {
            new_sub_arr = this->_subcls_update_func;
        }
        val++;
        // set it to func
        int length = ArrMgr_SubSize(val-1);
        *reinterpret_cast<std::function<void()>**>((char *) new_sub_arr + length) = func;

        *(int *)new_sub_arr = val;
        // for cuda free memory
#if CUDA_ENABLE
        cudaFree(this->_subcls_update_func);
#else
        delete [] this->_subcls_update_func;
#endif
        this->_subcls_update_func = new_sub_arr;
    }

public:
    bool is_inited;
    int max_size;
    int size;
    double high;
    double low;

    SingleArrayManager(int size=0):
        max_size(size), closes(new double[size]), pointer(-1), is_inited(false),
        sum(0), sum_square(0), size(0), high(-INFINITY), low(INFINITY),
         _subcls_update_func(new char[ArrMgr_SubSize(0)]) {
            *(int *)_subcls_update_func = 0;
        }

    SingleArrayManager(const SingleArrayManager&) = delete;

    SingleArrayManager(SingleArrayManager&& am):
        max_size(am.max_size), pointer(am.pointer), is_inited(am.is_inited),
        sum(am.sum), sum_square(am.sum_square), high(am.high), low(am.low),
        size(am.size) {
        double * _tmp_close = closes;
        closes = am.closes;
        am.closes = _tmp_close;
        void * _tmp_sub_class = _subcls_update_func;
        _subcls_update_func = am._subcls_update_func;
        am._subcls_update_func = _tmp_sub_class;
    }

    SingleArrayManager& operator=(const SingleArrayManager &) = delete;

    SingleArrayManager& operator=(SingleArrayManager && am) {
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

    constexpr double get_close() {return this->closes[this->pointer];}

    constexpr double get_close(const int offset) {
        if (pointer>=offset) return closes[pointer-offset];
        else return closes[pointer-offset+max_size];
    }

    constexpr void update_bar(const BarData& bar) {
        size++; pointer++;
        if (pointer == max_size) [[unlikely]] {pointer=0;is_inited=true;}
        if (is_inited) [[likely]] {
            sum-=closes[pointer];
            sum_square -= closes[pointer]*closes[pointer];
            size--;
        }
        closes[pointer] = bar.close;
        sum += bar.close;
        sum_square += bar.close*bar.close;
        high = std::max(bar.high, high);
        low = std::min(bar.low, low);
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

    constexpr double get_std() {
        return sqrt_const((sum_square - get_mean()*sum)/(max_size-1));
    }

    constexpr double get_std(int N, int offset=0) {
        double _sum_sq = 0;
        int _p = this->pointer;
        while (offset) {_p--;offset--;}
        int n = N;
        while (N) {
            if (_p<0) {_p+= this->max_size;}
            _sum_sq += this->closes[_p]*this->closes[_p];
            N--; _p--;
        }
        int mean = get_mean(n, offset);
        return sqrt_const((_sum_sq - mean*mean*n)/(n-1));
    }

    ~SingleArrayManager() {
        delete [] closes;
        int number = *static_cast<int*>(_subcls_update_func);
        auto _p = (std::function<void()>**)(static_cast<int*>(_subcls_update_func)+1);
        while (number) {delete (*_p); _p++;number--;}
        delete [] _subcls_update_func;
    }

    template <class Child> friend class _Base_Index_Calculator;
};

/* Multi-Symbol Array Manager: auto-managed.*/

class ArrayManager {
private:
#if CUDA_ENABLE
    // TODO
#else
    std::unordered_map<std::string, SingleArrayManager> ams;
#endif
    void * _update_func;

    void onNewSymbol(const std::string& symbol) {
        int number = (*static_cast<int*>(_update_func));
        auto funcs = (std::function<void(const std::string&)>**)(static_cast<int*>(_update_func)+1);
        while (number) {
            (**funcs)(symbol);
            funcs++;
            number--;
        }
    }

    void newIndexFunc(std::function<void(const std::string&)>* func) {
        int val = *(int *)(this->_update_func);
        void * new_sub_arr;
        if (!(val&0b1)) {
            new_sub_arr = new char[ArrMgr_SubSize(val?val<<1: 2)];
            memcpy(new_sub_arr, this->_update_func, ArrMgr_SubSize(val));
        } else {
            new_sub_arr = this->_update_func;
        }
        val++;
        // set it to func
        int length = ArrMgr_SubSize(val-1);
        *reinterpret_cast<std::function<void(const std::string&)>**>((char *) new_sub_arr + length) = func;
        *(int *)new_sub_arr = val;
        delete [] this->_update_func;
        this->_update_func = new_sub_arr;
    }

public:
    int max_size;
    ArrayManager(): max_size(0), _update_func(new char[ArrMgr_SubSize(0)]) {}

    ArrayManager(int size): max_size(size), _update_func(new char[ArrMgr_SubSize(0)]) {
        *static_cast<int*>(_update_func) = 0;
    }

    ArrayManager(const ArrayManager &) = delete;

    ArrayManager(ArrayManager &&) = delete;

    ArrayManager& operator=(const ArrayManager&) = delete;

    // use only when new am is empty.
    ArrayManager& operator=(ArrayManager&& am) {
        void * tmp = this->_update_func;
        this->_update_func = am._update_func;
        am._update_func = tmp;
        this->max_size = am.max_size;
        this->ams = std::move(am.ams);
        return *this;
    }

    void update_bar(const BarData& bar) {
        if (!ams.count(bar.symbol)) {
            ams[bar.symbol] = SingleArrayManager(max_size);
            this->onNewSymbol(bar.symbol);
        } else {
            ams.at(bar.symbol).update_bar(bar);
        }
    }

    template <typename ...Args>
    constexpr const double get_mean(const std::string& symbol, Args ...args) {
        return ams.at(symbol).get_mean(std::forward<Args>(args)...);
    }

    template <typename ...Args>
    constexpr double get_std(const std::string& symbol, Args ...args) {
        return ams.at(symbol).get_std(std::forward<Args>(args)...);
    }

    template <typename ...Args>
    constexpr double get_close(const std::string& symbol, Args ...args) {
        return ams.at(symbol).get_close(std::forward<Args>(args)...);
    }

    ~ArrayManager() {
        delete [] _update_func;
    }

    bool is_inited(const std::string& domain) const {
        return this->ams.at(domain).is_inited;
    }

    template <class T> friend class _Index_Calculator;

};

// ---------------------------------------
// **************************************
// *** Technical Indexes Caculators *****
// **************************************


/* Single Calculators definition: */
// Base Calculator (for inherit use only. Cannot create instances.)
template <class Child>
class _Base_Index_Calculator {
protected:
    SingleArrayManager * _am;
    _Base_Index_Calculator(): _am(0), is_inited(false) {}
    _Base_Index_Calculator(SingleArrayManager& am):
        _am(&am), is_inited(false) {
        auto func = new std::function<void()>(std::bind(&Child::update, static_cast<Child*>(this)));
        _am->update_class(func);
    }

    _Base_Index_Calculator& operator=(const _Base_Index_Calculator&) = delete;

    _Base_Index_Calculator& operator=(_Base_Index_Calculator&& obj) = delete;


public:
    bool is_inited;
};

// MACD Calculators
class SingleCalculator_MACD: public _Base_Index_Calculator<SingleCalculator_MACD> {
private:
    struct _MACDType {double MACD,DIF,DEA;};
    int _fast;
    int _slow;
    int _count;
    double _EMA_fast;
    double _EMA_slow;
    double _MACD;
    double _DEA;
    double _DIF;
public:
    typedef _MACDType _IndexReturnType;

    SingleCalculator_MACD(): _fast(0), _slow(0) {}

    SingleCalculator_MACD(SingleArrayManager& am, int fast=12, int slow=26):
        _Base_Index_Calculator(am), _fast(fast), _slow(slow), _count(1), _DEA(0),
        _EMA_fast(NAN), _EMA_slow(NAN), _MACD(NAN), _DIF(NAN) {}

    constexpr void update() {
        if (!_am->is_inited) {return;}
        if (this->is_inited) [[likely]] {
            _EMA_fast = (_EMA_fast*(_fast-1)+2*_am->get_close())/(_fast+1);
            _EMA_slow = (_EMA_slow*(_slow-1)+2*_am->get_close())/(_slow+1);
            _DIF = _EMA_fast-_EMA_slow;
            _MACD = (_MACD*8 + _DIF*2)/10;
            _DEA = _DIF - _MACD;
            return;
        }
        if (std::isnan(_EMA_fast)) [[unlikely]] {
            _EMA_fast = _am->get_mean(_fast);
            _EMA_slow = _am->get_mean(_slow);
            _DIF = _EMA_fast - _EMA_slow;
        } else [[likely]] {
            _EMA_fast = (_EMA_fast*(_fast-1)+2*_am->get_close())/(_fast+1);
            _EMA_slow = (_EMA_slow*(_slow-1)+2*_am->get_close())/(_slow+1);
            _count++;
            if (_count < 9) {
                _DIF += _EMA_fast-_EMA_slow;
            } else {
                _DIF += _EMA_fast-_EMA_slow;
                _MACD = _DIF/9;
                _DIF = _EMA_fast-_EMA_slow;
                _DEA = _DIF-_MACD;
                is_inited = true;
            }
        }
    }

    constexpr _MACDType get_val() {return {_MACD, _DIF, _DEA};}
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


// Multi-Technical Index Calculator
template <class _IndexClassType>
class _Index_Calculator {
public:
    static_assert(std::is_base_of_v<_Base_Index_Calculator<_IndexClassType>, _IndexClassType>,
                    "Must be Inherited from Base_Index_Calculator.");

    std::unordered_map<std::string, _IndexClassType> _syb_mapper;

    ArrayManager * _amp;

    constexpr void onNewSymbol(const std::string& symbol) {
        _syb_mapper.emplace(symbol, (_amp->ams.at(symbol)));
    }

public:

    _Index_Calculator(ArrayManager& am): _amp(&am)  {
        auto func = new std::function<void(const std::string&)>(
            std::bind(&_Index_Calculator::onNewSymbol, this, std::placeholders::_1));
        _amp->newIndexFunc(func);
    }

    constexpr bool is_inited(const std::string& symbol) {
        return _syb_mapper.at(symbol).is_inited;
    }

    constexpr _IndexClassType::_IndexReturnType get_val(const std::string& symbol) {
        return _syb_mapper.at(symbol).get_val();
    }
};

// ---------------------------------------
// **************************************
// ********** Single Structure ***********
// **************************************

class SingleArrayManager_t {
    int pointer;
    double * closes;
    double sum;
    double sum_square;
public:
    bool array_inited;
    int max_size;
    int size;
    double high;
    double low;

    SingleArrayManager_t(int size=0):
        max_size(size), closes(new double[size]), pointer(-1), array_inited(false),
        sum(0), sum_square(0), size(0), high(-INFINITY), low(INFINITY) {}

    SingleArrayManager_t(const SingleArrayManager_t&) = delete;

    SingleArrayManager_t(SingleArrayManager_t&& am):
        max_size(am.max_size), pointer(am.pointer), array_inited(am.array_inited),
        sum(am.sum), sum_square(am.sum_square), high(am.high), low(am.low),
        size(am.size) {
        double * _tmp_close = closes;
        closes = am.closes;
        am.closes = _tmp_close;
    }

    SingleArrayManager_t& operator=(const SingleArrayManager_t &) = delete;

    SingleArrayManager_t& operator=(SingleArrayManager_t && am) {
        max_size = am.max_size;
        pointer = am.pointer;
        array_inited = am.array_inited;
        sum = am.sum;
        sum_square = am.sum_square;
        double * tmp = closes;
        closes = am.closes;
        am.closes = tmp;
        return *this;
    }

    constexpr double get_close() {return this->closes[this->pointer];}

    constexpr double get_close(const int offset) {
        if (pointer>=offset) return closes[pointer-offset];
        else return closes[pointer-offset+max_size];
    }

    constexpr void update_bar(const BarData& bar) {
        size++; pointer++;
        if (pointer == max_size) [[unlikely]] {pointer=0;array_inited=true;}
        if (array_inited) [[likely]] {
            sum-=closes[pointer];
            sum_square -= closes[pointer]*closes[pointer];
            size--;
        }
        closes[pointer] = bar.close;
        sum += bar.close;
        sum_square += bar.close*bar.close;
        high = std::max(bar.high, high);
        low = std::min(bar.low, low);
    };

    constexpr double get_mean() {return sum/size;}

    constexpr double get_mean(int N, int offset=0) {
        double _sum = 0;
        int _p = this->pointer;
        int n = std::min(N, size-offset);
        N = n;
        while (offset) {_p--; offset--;}
        while (N) {
            if (_p<0) {_p+= this->max_size;}
            _sum += this->closes[_p];
            N--; _p--;
        }
        return _sum/n;
    }

    constexpr double get_std() {
        return sqrt_const((sum_square - get_mean()*sum)/(size-1));
    }

    constexpr double get_std(int N, int offset=0) {
        double _sum_sq = 0;
        int _p = this->pointer;
        int n = std::min(N, size-offset);
        N = n;
        while (offset) {_p--;offset--;}
        while (N) {
            if (_p<0) {_p+= this->max_size;}
            _sum_sq += this->closes[_p]*this->closes[_p];
            N--; _p--;
        }
        int mean = get_mean(n, offset);
        return sqrt_const((_sum_sq - mean*mean*n)/(n-1));
    }

    ~SingleArrayManager_t() {
        delete [] closes;
    }
};

class Calculator_MACD_t: public virtual SingleArrayManager_t {
protected:
    struct _MACDType {double MACD,DIF,DEA;};
    int _fast;
    int _slow;
    int _count;
    double _EMA_fast;
    double _EMA_slow;
    double _MACD;
    double _DEA;
    double _DIF;
public:
    bool inited;

    Calculator_MACD_t(int fast, int slow):
        _fast(fast), _slow(slow), _count(1), _DEA(0), inited(false),
        _EMA_fast(NAN), _EMA_slow(NAN), _MACD(NAN), _DIF(NAN) {}

    void update() {
        if (this->inited) [[likely]] {
            _EMA_fast = (_EMA_fast*(_fast-1)+2*this->get_close())/(_fast+1);
            _EMA_slow = (_EMA_slow*(_slow-1)+2*this->get_close())/(_slow+1);
            _DIF = _EMA_fast-_EMA_slow;
            _MACD = (_MACD*8 + _DIF*2)/10;
            _DEA = _DIF - _MACD;
            return;
        }
        if (std::isnan(_EMA_fast)) [[unlikely]] {
            _EMA_fast = this->get_mean(_fast);
            _EMA_slow = this->get_mean(_slow);
            _DIF = _EMA_fast - _EMA_slow;
        } else [[likely]] {
            _EMA_fast = (_EMA_fast*(_fast-1)+2*this->get_close())/(_fast+1);
            _EMA_slow = (_EMA_slow*(_slow-1)+2*this->get_close())/(_slow+1);
            _count++;
            if (_count < 9) {
                _DIF += _EMA_fast-_EMA_slow;
            } else {
                _DIF += _EMA_fast-_EMA_slow;
                _MACD = _DIF/9;
                _DIF = _EMA_fast-_EMA_slow;
                _DEA = _DIF-_MACD;
                inited = true;
            }
        }
    }

    constexpr _MACDType get_MACD() {return {_MACD, _DIF, _DEA};}
};

class Calculator_RSI: public virtual SingleArrayManager_t {
protected:
    struct _RSIType {double RSI, RS;};
    int _period;
    int _count;
    double earn;
    double loss;
    double RS;

public:
    bool inited;

    Calculator_RSI(int period):
        _period(period), earn(0), loss(0), inited(false), _count(0), RS(0) {
            assert(period>=max_size && "period should strictly less than max_size");
        }

    void update() {
        if (this->size==1) [[unlikely]] {return;}
        _count++;
        double diff = this->get_close() - this->get_close(1);
        earn += std::max(0.0, diff);
        loss -= std::min(0.0, diff);
        if (inited) [[likely]] {
            diff = this->get_close(_period) - this->get_close(_period+1);
            earn -= std::max(0.0, diff);
            loss += std::min(0.0, diff);
            _count--;
        }
        RS = earn/loss;
        if (_count==_period) {inited = true;}
    }

    constexpr _RSIType get_RSI() {return {RS, 100.0-100.0/(1+RS)};}

};


// ---------------------------------------
// **************************************
// ********** Union Structure ***********
// **************************************


template <class ..._IndexType> requires (std::is_base_of_v<SingleArrayManager_t,_IndexType> && ...)
class IndexManager: public _IndexType... {

    template <typename _Type, typename ..._Rest>
    void _updateIndexes() {
        if constexpr(sizeof...(_Rest)) this->_updateIndexes<_Rest...>();
        _Type::update();
    }

public:

    IndexManager(int size, _IndexType && ...instances):
        SingleArrayManager_t(size) ,_IndexType(std::forward<_IndexType>(instances)) ... {}

    template<typename ...T> requires (std::is_convertible_v<T, _IndexType> && ...)
    IndexManager(int size, T ...val):
        SingleArrayManager_t(size), _IndexType(val) ... {}

    void update_bar(const BarData& bar) {
        SingleArrayManager_t::update_bar(bar);
        this->_updateIndexes<_IndexType...>();
    }

    constexpr bool is_inited() {
        return SingleArrayManager_t::array_inited && (_IndexType::inited && ...);
    }
};



template <>
class IndexManager<>: public SingleArrayManager_t {
public:

    IndexManager() = delete;

    IndexManager(int max_length):
        SingleArrayManager_t(max_length) {}

    IndexManager(SingleArrayManager_t && am):
        SingleArrayManager_t(std::forward<SingleArrayManager_t>(am)) {}

    constexpr bool is_inited() {return this->array_inited;}
};

template <typename ...IndexTypes> requires (
    std::is_base_of_v<SingleArrayManager_t, IndexTypes> && ...)
class MultiSyb_IndexManager {
    using _SybMapType=std::unordered_map<std::string, IndexManager<IndexTypes...>>;

    _SybMapType _symbol_map;

    //TODO construction parameters.

    void new_symbol(const std::string& symbol) {
        _symbol_map.emplace(symbol);
    }

public:

    MultiSyb_IndexManager() {}



};

// template <typename ..._IndexTypes>
// MultiSyb_IndexManager()

// typedef SingleIndexManager<> ArrayMgr;


typedef _Index_Calculator<SingleCalculator_MACD> Calculator_MACD;


typedef std::vector<double> DoubleArr;
typedef std::vector<std::vector<double>> double_2D_Arr;

// for Cython Intelligence Lightlight Use
typedef PyObject*  _Object;

#endif