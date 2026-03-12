set(QTTUI_SOURCES
    placeholder.cpp
    ScreenBuffer.cpp
    AnsiRenderer.cpp
    TerminalBackend.cpp
    TuiEvents.cpp
    InputParser.cpp
    QTuiWidget.cpp
    QTuiMainWindow.cpp
    QTuiLineEdit.cpp
    QTuiTableView.cpp
    QTuiListView.cpp
    QTuiBoxLayout.cpp
    QTuiApplication.cpp
    QTuiSizePolicy.cpp
    QTuiLayoutItem.cpp
    QTuiLayout.cpp
    QTuiLabel.cpp
    QTuiFormLayout.cpp
    QTuiGridLayout.cpp
    QTuiStackedLayout.cpp
    Theme_Engine.cpp
    UI_Loader.cpp
    QTuiPushButton.cpp
    QTuiToolButton.cpp
    QTuiCheckBox.cpp
    QTuiRadioButton.cpp
    QTuiButtonGroup.cpp
    QTuiSpinBox.cpp
    QTuiDoubleSpinBox.cpp
    QTuiSlider.cpp
    QTuiDial.cpp
)

# Platform-specific backend sources
if(WIN32)
    list(APPEND QTTUI_SOURCES WindowsBackend.cpp)
else()
    list(APPEND QTTUI_SOURCES PosixBackend.cpp)
endif()

set(QTTUI_HEADERS
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QtTUI
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QtTUIGlobal.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/ScreenBuffer.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/AnsiRenderer.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/TerminalBackend.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/TuiEvents.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/InputParser.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiWidget.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiMainWindow.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiLineEdit.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTableView.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiListView.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiBoxLayout.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiApplication.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiSizePolicy.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiLayoutItem.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiLayout.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiLabel.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiFormLayout.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiGridLayout.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiStackedLayout.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/Theme_Engine.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/UI_Loader.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiPushButton.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiToolButton.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiCheckBox.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiRadioButton.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiButtonGroup.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiSpinBox.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiDoubleSpinBox.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiSlider.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiDial.h
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

# Install public headers (*.h files)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/QtTUI
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Install umbrella header (no extension, not matched by *.h glob)
install(FILES ${CMAKE_SOURCE_DIR}/include/QtTUI/QtTUI
    DESTINATION include/QtTUI
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
