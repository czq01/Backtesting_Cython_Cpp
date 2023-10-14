#ifndef __CZQ01_UTIL_ARRAYMGR__
#define __CZQ01_UTIL_ARRAYMGR_

#include "../base.hpp"

// Constructive Parameter Holder
template <typename ...IndexType>
class ParamsMap;

template <class T, class Type, class ...Others>
auto _get_parent_class() {
    if constexpr(std::is_same_v<T, Type>)
        return ParamsMap<Type, Others...>();
    else return _get_parent_class<T, Others...>();
}

template <typename IndexType, typename ...Rest> requires (
    !(std::is_same_v<IndexType, Rest> || ... ))  // IndexTypes Can't Be Duplicate
class ParamsMap<IndexType, Rest...>: public ParamsMap<Rest...> {
protected:
    using _Current=IndexType;
    using _Parent=ParamsMap<Rest...>;
    IndexType::_ParamType param;
    IndexType::_ParamType _get() {return param;}

public:
    ParamsMap() {}

    ParamsMap(int size, IndexType && index, Rest &&...rest):
        param(index.get_param()), ParamsMap<Rest...>(size, std::forward<Rest>(rest)...) {}

    template <class TypeArg, class ...RestArgs> requires (
        std::is_convertible_v<TypeArg, IndexType>&&(std::is_convertible_v<RestArgs, Rest> && ...))
    ParamsMap(int size, TypeArg&& arg, RestArgs ...args):
        param(arg, args...) {}

    ParamsMap& operator=(ParamsMap&) = delete;

    template <class T> requires (std::is_same_v<T, IndexType>||(std::is_same_v<T, Rest>|| ... ))
    constexpr const T::_ParamType get_val() const {
        using F = decltype(_get_parent_class<T, IndexType, Rest...>());
        if constexpr(std::is_same_v<F, ParamsMap<IndexType, Rest...>>) return param;
        else return F::param;
    }
};

template <>
class ParamsMap<> {
public:
    int size;
    ParamsMap() {}
    ParamsMap(int size): size(size) {}
};

template <typename ..._IndexTypes>
ParamsMap(_IndexTypes...) -> ParamsMap<_IndexTypes...>;

// ---------------------------------------
// **************************************
// ***** Technical Index Structure ******
// **************************************

class PriceArray {
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

    PriceArray(int size=0):
        max_size(size), closes(new double[size]), pointer(-1), array_inited(false),
        sum(0), sum_square(0), size(0), high(-INFINITY), low(INFINITY) {}

    PriceArray(const PriceArray&) = delete;

    PriceArray(PriceArray&& am):
        max_size(am.max_size), pointer(am.pointer), array_inited(am.array_inited),
        sum(am.sum), sum_square(am.sum_square), high(am.high), low(am.low),
        size(am.size) {
        double * _tmp_close = closes;
        closes = am.closes;
        am.closes = _tmp_close;
    }

    PriceArray& operator=(const PriceArray &) = delete;

    PriceArray& operator=(PriceArray && am) {
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

    ~PriceArray() {
        delete [] closes;
    }
};

class Calculator_MACD: public virtual PriceArray {
protected:
    int _fast;
    int _slow;
    int _count;
    double _EMA_fast;
    double _EMA_slow;
    double _MACD;
    double _DEA;
    double _DIF;
public:
    struct _ValType {double macd, dif, dea;};
    struct _ParamType {int fast, slow;};
    bool inited;

    Calculator_MACD(const _ParamType& param):
        _fast(param.fast), _slow(param.slow), _count(1), _DEA(0), inited(false),
        _EMA_fast(NAN), _EMA_slow(NAN), _MACD(NAN), _DIF(NAN) {}

    Calculator_MACD(int fast, int slow):
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

    constexpr _ValType get_MACD() {return {_MACD, _DIF, _DEA};}

    constexpr _ValType get_val() {return get_MACD();}

    constexpr _ParamType get_param() {return {_fast, _slow};}
};

class Calculator_RSI: public virtual PriceArray {
protected:
    int _period;
    int _count;
    double earn;
    double loss;
    double RS;

public:
    struct _ValType {double rsi, rs;};
    struct _ParamType {int period;};
    bool inited;

    Calculator_RSI(const _ParamType& param):
        _period(param.period), earn(0), loss(0), inited(false), _count(0), RS(0) {}

    Calculator_RSI(int period):
        _period(period), earn(0), loss(0), inited(false), _count(0), RS(0) {}

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

    constexpr  _ValType get_RSI() {return {RS, 100.0-100.0/(1+RS)};}

    constexpr  _ValType get_val() {return get_RSI();}

    constexpr _ParamType get_param() {return {_period};}
};

class Calculator_BollingBand: public virtual PriceArray {
    int _N;
    int _count;
    double _K;
    double up, mid, down;

public:
    struct _ValType {double up, mid, down;};
    struct _ParamType {int N; double K;};
    bool inited;

    Calculator_BollingBand(const _ParamType& param):
        _N(param.N), _K(param.K), _count(0), up(NAN), mid(NAN), down(NAN), inited(false) {}

    Calculator_BollingBand(int N, double K):
        _N(N), _K(K), _count(0), up(NAN), mid(NAN), down(NAN), inited(false) {}

    void update() {
        if (!this->inited && array_inited) [[unlikely]] {inited=true;}
        if (!inited) return;
        double std = this->get_std();

        this->mid = this->get_mean();
        this->up = this->mid + this->_K*std;
        this->down = this->mid - this->_K*std;
    }

    constexpr _ValType get_BBand() {return {up, mid, down};}

    constexpr _ValType get_val() {return get_BBand();}

    constexpr _ParamType get_param() {return {_N, _K};}

};

// ---------------------------------------
// **************************************
// ********** Union Structure ***********
// **************************************

// Single Symbol IndexManger
template <class ..._IndexType> requires (std::is_base_of_v<PriceArray,_IndexType> && ...)
class IndexManager: public _IndexType... {

    template <typename _Type, typename ..._Rest>
    void _updateIndexes() {
        if constexpr(sizeof...(_Rest)) this->_updateIndexes<_Rest...>();
        _Type::update();
    }

public:
    IndexManager(int size, _IndexType && ...instances):
        PriceArray(size) ,_IndexType(std::forward<_IndexType>(instances)) ... {}

    template<typename ...T> requires (std::is_convertible_v<T, _IndexType> && ...)
    IndexManager(int size, T ...val):
        PriceArray(size), _IndexType(val) ... {}

    IndexManager(const ParamsMap<_IndexType...>& params):
        PriceArray(params.size), _IndexType(params.get_val<_IndexType>()) ...{}

    IndexManager& operator=(const IndexManager&) = delete;

    void update_bar(const BarData& bar) {
        PriceArray::update_bar(bar);
        this->_updateIndexes<_IndexType...>();
    }

    template <class T> requires (std::is_same_v<T, _IndexType> || ...)
    constexpr auto get_index() {
        return T::get_val();
    }

    template <class T> requires (std::is_same_v<T, _IndexType> || ...)
    constexpr T::_ParamType get_param() {
        return T::get_param();
    }

    constexpr bool is_inited() {
        return PriceArray::array_inited && (_IndexType::inited && ...);
    }

};

template <>
class IndexManager<>: public PriceArray {
public:

    IndexManager() = delete;

    IndexManager(int max_length):
        PriceArray(max_length) {}

    IndexManager(PriceArray && am):
        PriceArray(std::forward<PriceArray>(am)) {}

    constexpr bool is_inited() {return this->array_inited;}
};

template <class ..._IndexType>
IndexManager(_IndexType...) -> IndexManager<_IndexType...>;



// Multi-symbol IndexManager
template <typename ..._IndexTypes> requires (
    std::is_base_of_v<PriceArray, _IndexTypes> && ...)
class MultiSyb_IndexManager {
    using _SybMapType=std::unordered_map<std::string, IndexManager<_IndexTypes...>>;

    _SybMapType _symbol_map;

    ParamsMap<_IndexTypes...> params;

    void new_symbol(const std::string& symbol) {
        _symbol_map.emplace(symbol, params);
    }

public:
    MultiSyb_IndexManager() {}

    MultiSyb_IndexManager(int size, _IndexTypes && ...index):
        params(size, std::forward<_IndexTypes>(index)...) {}

    template<typename ...T> requires (std::is_convertible_v<T, _IndexTypes> && ...)
    MultiSyb_IndexManager(int size, T ...val):
        params(size, val ...) {}

    MultiSyb_IndexManager& operator=(const MultiSyb_IndexManager&) = delete;

    bool is_inited(const std::string& symbol) {
        return this->_symbol_map.at(symbol).is_inited();
    }

    void update_bar(const BarData& bar) {
        if (!this->_symbol_map.count(bar.symbol)) {
            this->new_symbol(bar.symbol);
        }
        this->_symbol_map.at(bar.symbol).update_bar(bar);
    }

    template <typename T>
    T::_ValType get_index(const std::string& syb) {
        return this->_symbol_map.at(syb).get_index<T>();
    }

    template <typename T>
    constexpr T::_ParamType get_param() {
        return this->params.get_val<T>();
    }

};

template <class ..._IndexType>
MultiSyb_IndexManager(_IndexType...) -> MultiSyb_IndexManager<_IndexType...>;


#endif