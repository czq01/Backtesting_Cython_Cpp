from typing import Iterable, overload, List
from pandas import DataFrame as __DataFrame
from queue import Queue as __Queue


# add overloaded hint for pyd functions.
@overload
def run(data: __DataFrame, params: List[list], res_queue:__Queue, columns: Iterable,
            years: float, get_df: bool=True): ...


# include implemented function
from .package.research import *