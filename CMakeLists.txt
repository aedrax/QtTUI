cmake_minimum_required(VERSION 3.16)
project(QtTUI VERSION 0.2.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core REQUIRED)

# Tests, demos and install rules are on by default only when QtTUI is the
# top-level project, not when another project adds it with add_subdirectory()
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(QTTUI_IS_TOP_LEVEL ON)
else()
    set(QTTUI_IS_TOP_LEVEL OFF)
endif()

# The install rules export the QtTUI target. A parent project that adds
# QtTUI with add_subdirectory() usually does not want them.
option(QTTUI_INSTALL "Install the QtTUI library, headers and CMake package" ${QTTUI_IS_TOP_LEVEL})

add_subdirectory(src)

option(QTTUI_BUILD_TESTS "Build QtTUI tests" ${QTTUI_IS_TOP_LEVEL})
if(QTTUI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

option(QTTUI_BUILD_DEMOS "Build QtTUI demos" ${QTTUI_IS_TOP_LEVEL})
if(QTTUI_BUILD_DEMOS)
    add_subdirectory(demos)
endif()
