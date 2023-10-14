#ifndef __CZQ01_UTIL_TOY_DATAFRAME__
#define __CZQ01_UTIL_TOY_DATAFRAME__

#include "../base.hpp"

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

#endif