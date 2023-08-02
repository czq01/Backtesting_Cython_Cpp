from distutils.core import setup
from Cython.Build import cythonize

import numpy
import setuptools

ext_modules = [
    setuptools.Extension(
        "research",
        ["research.pyx"],
        extra_compile_args=['/O2'],
        include_dirs=[numpy.get_include()],

    )
]

setup(name='accelerates_func',
      ext_modules=cythonize(
            ext_modules,
            language="c++",
            annotate=True))