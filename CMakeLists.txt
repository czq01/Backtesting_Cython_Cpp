cmake_minimum_required(VERSION 3.0.0)
project(teste VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()



include_directories(
    ${PROJECT_SOURCE_DIR}/**
    C:/Users/ziqua/AppData/Local/Programs/Python/Python38/include/**
    C:/Users/ziqua/AppData/Local/Programs/Python/Python38/include
    ./research_cython/include
    ./research_cython

)


add_compile_options(/Ot /std:c++20 /arch:AVX)

link_directories(
    C:/Users/ziqua/AppData/Local/Programs/Python/Python38/libs
)
link_libraries(python38)


add_executable(teste main.cpp)
SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS "/PROFILE")


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
