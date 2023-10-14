#include <base.hpp>

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

typedef _Index_Calculator<SingleCalculator_MACD> Calculator_MACD;