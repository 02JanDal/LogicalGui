cmake_minimum_required(VERSION 2.8.9)

project(LogicalGui)

option(DO_COVERAGE "Set to ON to enable coverage reporting when running the unit test" OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

if(DO_COVERAGE)
    include(CodeCoverage)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${CMAKE_C_FLAGS_COVERAGE}")
endif()

find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

add_library(LogicalGui SHARED LogicalGui.h)
qt5_use_modules(LogicalGui Core)

add_library(TestCoreLib STATIC TestCore.h TestCore.cpp)
qt5_use_modules(TestCoreLib Core)
target_link_libraries(TestCoreLib LogicalGui)

add_library(TestGuiLib STATIC TestGui.h TestGui.cpp)
qt5_use_modules(TestGuiLib Widgets)
target_link_libraries(TestGuiLib LogicalGui TestCoreLib)

add_executable(Test test_main.cpp)
qt5_use_modules(Test Core Widgets)
target_link_libraries(Test LogicalGui TestGuiLib)

# Unit tests

add_executable(tst_LogicalGui tst_LogicalGui.cpp)
qt5_use_modules(tst_LogicalGui Core Test)
target_link_libraries(tst_LogicalGui LogicalGui)

enable_testing()
add_test(tst_LogicalGui tst_LogicalGui)

if(DO_COVERAGE)
    setup_target_for_coverage(tst_LogicalGui_coverage tst_LogicalGui coverage)
endif()
