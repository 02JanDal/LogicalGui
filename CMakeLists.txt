cmake_minimum_required(VERSION 2.8.9)

project(LogicalGui)

set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIRECTORY ON)
set(CMAKE_CXX_FLAGS "-std=c++11")

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
