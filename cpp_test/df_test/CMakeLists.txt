cmake_minimum_required(VERSION 3.0.0)
project(df_test VERSION 0.1.0 LANGUAGES C CXX)
include(CTest)
enable_testing()

include_directories(
    ${PROJECT_SOURCE_DIR}/**
    ${PYTHON_INCLUDE}/**
    ${PYTHON_INCLUDE}
    ./cython_module/include
)

link_directories(
    ${PYTHON_LIB}
)

add_compile_options(/Ot /std:c++20 /arch:AVX)
link_libraries(python38)

add_executable(df_test main.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)