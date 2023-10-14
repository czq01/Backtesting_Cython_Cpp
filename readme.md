# Backtesting Cython CPP

This is a backtesting framework Python Extension.

## Backtest Usage

### Add your own strategy

strategy path file at: `./cython_module/include/strategy.hpp`

A typical stratey file defined like this:

```cpp

#ifndef __STRATEGY__
#define __STRATEGY__

#include "structure.hpp"
#include "base_strategy.hpp"

class MyStrategy: BaseStrategy<MyStrategy> {
    ...
};

// you may define several strategy here
// Link the one you want to make backtest on.
typedef MyStrategy Strategy;

#endif

```

### Compile & Run

- compile the extension runing:

```bash

./cython_module/build.sh

```

- extension use in python:

```python3

import cython_module as cm

```

- starting backtesting

```python3

void run(data: DataFrame, params: List[list], res_queue:__Queue, columns: Iterable,
            years: float, get_df: bool=True);

```


## Structure Usage

Structures are defined in file [array_manager.hpp](./cython_module/include/utils/array_manager.hpp)

We have BarData, ArrayManager for strategy use.

### ArrayManager Functions

#### PriceArray

PriceArray is use to store sequence of BarData of one particular contract symbol. It CAN'T be copy.

function usage:

```cpp
// get current/previous prices counted back from latest;
constexpr double get_close(const int offset=0);
// main function to put new close price into sequence;
constexpr void update_bar(const BarData& bar);
// get average price  (O(1) with default param)
constexpr double get_mean(int N=max_size, int offset=0);
// get price stand deviation  (O(1) with default param)
constexpr double get_std(int N=max_size, int offset=0);
```

#### ArrayManager

SingleArrayManager is a structure that inherit from TechnicalIndex Classes and PriceArray. It feeded data from `void update_bar(const BarData&)` function, and calculate TechnicalIndexes automatically in O(1) Time.

ArrayManager is a multi-symbol version of SingleArraymanager. We add a params `symbol` for all the functions appeared in SingleArrayManager except for `get_param()`. Here are the functions definition:

The Similar part of two class:

```cpp
//definition:  Add Any Index you want in template arguments.
// Currently Support MACD, RSI, BBANDS
// array_size should bigger than the indexes' history period.
SingleArrayManager<MACD, RSI, ...> mgr(int array_size, {MACD params...}, {RSI params...}, ...);
ArrayManager<MACD, RSI, ...> mgr(int array_size, {MACD params...}, {RSI params...}, ...);

// feed data:
void SingleArrayManager::update_bar(const BarData&);
void ArrayManager::update_bar(const BarData&);

// check if all Index properly inited and have valid value:
bool SingleArrayManager::is_inited();
bool ArrayManager::is_inited(const std::string& symbol);

// fetch value of _INDEX according to the definition
_INDEX::_ValType SingleArrayManager::get_index<_INDEX>();
_INDEX::_ValType ArrayManager::get_index<_INDEX>(const std::string& symbol);

// check the parameter of _INDEX:
_INDEX::_ParamType SingleArrayManager::get_param<_INDEX>();
_INDEX::_ParamType ArrayManager::get_param<_INDEX>();
```

The SingleArrayManager has extra method to fetch value of indexes:

```cpp
// if you define the SingleArrayManager like this:
SingleArrayManager<MACD, RSI> mgr(int array_size, {MACD params...}, {RSI params...});

// then we can fetch value of INDEX by:  (INDEX is listed in template argument above)
INDEX::_ValType mgr.get_INDEX();

// for example:
RSI::_ValType mgr.get_RSI();  // is ok
MACD::_ValType mgr.get_MACD();  // is ok
BBAND::_ValType mgr.get_BBAND();  // wrong!! BBAND is not presented in mgr definition

```

