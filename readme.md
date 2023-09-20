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

Structures are defined in file [structure.hpp](./cython_module/include/structure.hpp)

We have BarData, ArrayManager, MACD_Calculator for strategy use.

### ArrayManager Functions

#### SingleArrayManager

SingleArrayManager is use to store sequence of BarData of one particular contract symbol. It CAN'T be copy.

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

ArrayManager is a structure that can handle multi-symbol. It has all the member functions in its Single version.

// We add a params `symbol` for all the functions appeared in SingleArrayManager. Here are the functions definition:
```cpp
template <class ...Args>
void member_function(const std::string_view& symbol, Args ...args);
```

### Technical_indexes

Same as ArrayManager, we have calculators in both Single-symbol versions and Multi-symbol versions.

#### Calculator_MACD

SingleCalculator_MACD is use to calculate MACD. It is rely on SingleArrayManager; After construction by ArrayManager,
It will be automatically calculating and fetch result in O(1) time.

We also have a Regular Version of Calculator_MACD that hanles multiple symbols.

function usage:

```cpp
// get MACD
struct _MACDType {double MACD, DIF, DEA;};
constexpr _MACDType get_val(const std::string_view&);
```

When use in your strategy:

```cpp
ArrayManager am(N);
Calculator_MACD macd(am);
...
am.update(bar);
...
double MACD = macd.get_val(bar.symbol).MACD;
...
auto&& [MACD,DIF,DEA] = macd.get_val(bar.symbol);

```
