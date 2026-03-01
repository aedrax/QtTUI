cmake_minimum_required(VERSION 3.16)
project(QtTUI VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core REQUIRED)

add_subdirectory(src)

option(QTTUI_BUILD_TESTS "Build QtTUI tests" ON)
if(QTTUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(QTTUI_BUILD_DEMOS "Build QtTUI demos" ON)
if(QTTUI_BUILD_DEMOS)
    add_subdirectory(demos)
endif()
