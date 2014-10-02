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

################# Main lib #################

add_library(LogicalGui SHARED src/LogicalGui.h)
qt5_use_modules(LogicalGui Core)

# for example and unit tests
include_directories(src)

################# Example #################

add_library(TestCoreLib STATIC example/Core.h example/Core.cpp)
qt5_use_modules(TestCoreLib Core)
target_link_libraries(TestCoreLib LogicalGui)

add_library(TestGuiLib STATIC example/Gui.h example/Gui.cpp)
qt5_use_modules(TestGuiLib Widgets)
target_link_libraries(TestGuiLib LogicalGui TestCoreLib)

add_executable(Test example/main.cpp)
qt5_use_modules(Test Core Widgets)
target_link_libraries(Test LogicalGui TestGuiLib)

################# Unit tests #################

add_executable(tst_LogicalGui test/tst_LogicalGui.cpp)
qt5_use_modules(tst_LogicalGui Core Test)
target_link_libraries(tst_LogicalGui LogicalGui)

enable_testing()
add_test(tst_LogicalGui tst_LogicalGui)

if(DO_COVERAGE)
    setup_target_for_coverage(tst_LogicalGui_coverage tst_LogicalGui coverage)
endif()

################# Documentation #################

find_package(Doxygen)
if(DOXYGEN_FOUND)
    execute_process(
	COMMAND Qt5::qmake -query QT_INSTALL_DOCS
	OUTPUT_VARIABLE QT_INSTALL_DOCS
	OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    if(QT_INSTALL_DOCS)
    	set(TAGFILES "${QT_INSTALL_DOCS}/qtcore/qtcore.tags=http://qt-project.org/doc/qt-5/")
    else()
        set(TAGFILES)
    endif()
    configure_file(${CMAKE_SOURCE_DIR}/docs/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    add_custom_target(doc
	COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
	COMMENT "Generating API documentation with Doxygen" VERBATIM
    )
endif()
