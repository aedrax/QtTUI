set(QTTUI_SOURCES
    placeholder.cpp
    ScreenBuffer.cpp
    AnsiRenderer.cpp
    TerminalBackend.cpp
    TuiEvents.cpp
    InputParser.cpp
)

# Platform-specific backend sources
if(WIN32)
    list(APPEND QTTUI_SOURCES WindowsBackend.cpp)
else()
    list(APPEND QTTUI_SOURCES PosixBackend.cpp)
endif()

set(QTTUI_HEADERS
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QtTUIGlobal.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/ScreenBuffer.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/AnsiRenderer.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/TerminalBackend.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/TuiEvents.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/InputParser.h
)

add_library(QtTUI ${QTTUI_SOURCES} ${QTTUI_HEADERS})
add_library(QtTUI::QtTUI ALIAS QtTUI)

target_include_directories(QtTUI
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(QtTUI PUBLIC Qt6::Core)

# Install library
install(TARGETS QtTUI
    EXPORT QtTUITargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

# Install public headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/QtTUI
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Install CMake config files for find_package(QtTUI REQUIRED)
install(EXPORT QtTUITargets
    FILE QtTUITargets.cmake
    NAMESPACE QtTUI::
    DESTINATION lib/cmake/QtTUI
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/cmake/QtTUIConfig.cmake.in
    ${CMAKE_BINARY_DIR}/QtTUIConfig.cmake
    INSTALL_DESTINATION lib/cmake/QtTUI
)

write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/QtTUIConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_BINARY_DIR}/QtTUIConfig.cmake
    ${CMAKE_BINARY_DIR}/QtTUIConfigVersion.cmake
    DESTINATION lib/cmake/QtTUI
)
