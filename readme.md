# Backtesting Cython CPP

This is a backtesting framework Python Extension.

## Usage

- compile the extension runing:

```bash

./cython_module/build.sh

```

- extension use in python:

```python3

import cython_module as cm

```

## Functions

1. starting backtesting

```python3

void run(data: DataFrame, params: List[list], res_queue:__Queue, columns: Iterable,
            years: float, get_df: bool=True);

```
