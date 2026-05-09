set(QTTUI_SOURCES
    QtTuiUnicode.cpp
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
    RenderScheduler.cpp
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
    QTuiComboBox.cpp
    QTuiProgressBar.cpp
    QTuiTextEdit.cpp
    QTuiDateTimeEdit.cpp
    QTuiLCDNumber.cpp
    QTuiScrollBar.cpp
    QTuiScrollArea.cpp
    QTuiGroupBox.cpp
    QTuiCollapsible.cpp
    QTuiTabBar.cpp
    QTuiTabWidget.cpp
    QTuiToolBox.cpp
    QTuiStackedWidget.cpp
    QTuiSplitter.cpp
    QTuiFocusFrame.cpp
    QTuiSizeGrip.cpp
    QTuiCalendarWidget.cpp
    QTuiItemDelegate.cpp
    QTuiStyledItemDelegate.cpp
    QTuiHeaderView.cpp
    QTuiTreeView.cpp
    QTuiColumnView.cpp
    QTuiUndoStack.cpp
    QTuiUndoView.cpp
    QTuiDataWidgetMapper.cpp
    QTuiListWidgetItem.cpp
    QTuiListWidget.cpp
    QTuiTableWidgetItem.cpp
    QTuiTableWidget.cpp
    QTuiTreeWidgetItem.cpp
    QTuiTreeWidgetItemIterator.cpp
    QTuiTreeWidget.cpp
    QTuiAction.cpp
    QTuiMenu.cpp
    QTuiMenuBar.cpp
    QTuiToolBar.cpp
    QTuiStatusBar.cpp
    QTuiWidgetAction.cpp
    QTuiDockWidget.cpp
    QTuiMdiSubWindow.cpp
    QTuiMdiArea.cpp
    RenderUtils.cpp
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
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QtTuiUnicode.h
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
    ${CMAKE_SOURCE_DIR}/include/QtTUI/RenderScheduler.h
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
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiComboBox.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiProgressBar.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTextEdit.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiDateTimeEdit.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiLCDNumber.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiScrollBar.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiScrollArea.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiGroupBox.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiCollapsible.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTabBar.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTabWidget.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiToolBox.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiStackedWidget.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiSplitter.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiFocusFrame.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiSizeGrip.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiCalendarWidget.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiItemDelegate.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiStyledItemDelegate.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiHeaderView.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTreeView.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiColumnView.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiUndoStack.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiUndoView.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiDataWidgetMapper.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiListWidgetItem.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiListWidget.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTableWidgetItem.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTableWidget.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTreeWidgetItem.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTreeWidgetItemIterator.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiTreeWidget.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiAction.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiMenu.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiMenuBar.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiToolBar.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiStatusBar.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiWidgetAction.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiDockWidget.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiMdiSubWindow.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/QTuiMdiArea.h
    ${CMAKE_SOURCE_DIR}/include/QtTUI/RenderUtils.h
)

# QtTUI is deliberately a STATIC library. The public headers carry no export
# macro (no QTTUI_EXPORT decorations), so BUILD_SHARED_LIBS=ON would silently
# produce a shared library with no exported symbols on some platforms. If a
# shared build is ever wanted, do an export-macro pass across the public
# headers first (see include/QtTUI/QtTUIGlobal.h), then revisit this keyword.
add_library(QtTUI STATIC ${QTTUI_SOURCES} ${QTTUI_HEADERS})
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
