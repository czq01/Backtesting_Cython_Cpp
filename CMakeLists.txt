cmake_minimum_required(VERSION 3.0.0)
project(unit_test VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

set(PYTHON_INCLUDE C:/Users/ziqua/AppData/Local/Programs/Python/Python38/include)
set(PYTHON_LIB C:/Users/ziqua/AppData/Local/Programs/Python/Python38/libs)

include_directories(
    ${PROJECT_SOURCE_DIR}/**
    ${PYTHON_INCLUDE}/**
    # C:/Users/ziqua/AppData/Local/Programs/Python/Python38/include
    ${PROJECT_SOURCE_DIR}/cython_module/include
)

link_directories(
    ${PYTHON_LIB}
)



add_subdirectory(cpp_test/whole_test)
add_subdirectory(cpp_test/techinical_index)
add_subdirectory(cpp_test/df_test)



set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
