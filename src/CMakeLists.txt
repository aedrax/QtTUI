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
    QTuiPlainTextEdit.cpp
    QTuiPushButton.cpp
    QTuiToolButton.cpp
    QTuiToolTip.cpp
    QTuiCheckBox.cpp
    QTuiClipboard.cpp
    QTuiRadioButton.cpp
    QTuiButtonGroup.cpp
    QTuiSpinBox.cpp
    QTuiDoubleSpinBox.cpp
    QTuiSlider.cpp
    QTuiDial.cpp
    QTuiDialog.cpp
    QTuiComboBox.cpp
    QTuiCompleter.cpp
    QTuiProgressBar.cpp
    QTuiProgressDialog.cpp
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
    QTuiFileDialog.cpp
    QTuiFocusFrame.cpp
    QTuiFrame.cpp
    QTuiSizeGrip.cpp
    QTuiCalendarWidget.cpp
    QTuiInputDialog.cpp
    QTuiItemDelegate.cpp
    QTuiStyledItemDelegate.cpp
    QTuiHeaderView.cpp
    QTuiTreeView.cpp
    QTuiColumnView.cpp
    QTuiUndoStack.cpp
    QTuiUndoView.cpp
    QTuiValidator.cpp
    QTuiWizard.cpp
    QTuiDataWidgetMapper.cpp
    QTuiListWidgetItem.cpp
    QTuiListWidget.cpp
    QTuiTableWidgetItem.cpp
    QTuiTableWidget.cpp
    QTuiTreeWidgetItem.cpp
    QTuiTreeWidgetItemIterator.cpp
    QTuiTreeWidget.cpp
    QTuiAbstractButton.cpp
    QTuiAbstractItemView.cpp
    QTuiAbstractScrollArea.cpp
    QTuiAbstractSpinBox.cpp
    QTuiAbstractSlider.cpp
    QTuiAction.cpp
    QTuiKeySequence.cpp
    QTuiMenu.cpp
    QTuiMenuBar.cpp
    QTuiMessageBox.cpp
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
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QtTUI
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QtTUIGlobal.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QtTuiUnicode.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/ScreenBuffer.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/AnsiRenderer.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/TerminalBackend.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/TuiEvents.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/InputParser.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiWidget.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiMainWindow.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiLineEdit.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTableView.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiListView.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiBoxLayout.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiApplication.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/RenderScheduler.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiSizePolicy.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiLayoutItem.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiLayout.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiLabel.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiFormLayout.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiGridLayout.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiStackedLayout.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/Theme_Engine.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/UI_Loader.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiPlainTextEdit.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiPushButton.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiToolButton.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiToolTip.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiCheckBox.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiClipboard.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiRadioButton.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiButtonGroup.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiSpinBox.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiDoubleSpinBox.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiSlider.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiDial.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiDialog.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiComboBox.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiCompleter.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiProgressBar.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiProgressDialog.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTextEdit.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiDateTimeEdit.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiLCDNumber.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiScrollBar.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiScrollArea.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiGroupBox.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiCollapsible.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTabBar.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTabWidget.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiToolBox.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiStackedWidget.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiSplitter.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiFileDialog.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiFocusFrame.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiFrame.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiSizeGrip.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiCalendarWidget.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiInputDialog.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiItemDelegate.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiStyledItemDelegate.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiHeaderView.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTreeView.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiColumnView.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiUndoStack.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiUndoView.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiValidator.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiWizard.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiDataWidgetMapper.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiListWidgetItem.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiListWidget.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTableWidgetItem.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTableWidget.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTreeWidgetItem.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTreeWidgetItemIterator.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiTreeWidget.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiAbstractButton.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiAbstractItemView.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiAbstractScrollArea.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiAbstractSpinBox.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiAbstractSlider.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiAction.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiKeySequence.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiMenu.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiMenuBar.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiMessageBox.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiToolBar.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiStatusBar.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiWidgetAction.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiDockWidget.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiMdiSubWindow.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/QTuiMdiArea.h
    ${PROJECT_SOURCE_DIR}/include/QtTUI/RenderUtils.h
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
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
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
install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/QtTUI
    DESTINATION include
    FILES_MATCHING PATTERN "*.h"
)

# Install umbrella header (no extension, not matched by *.h glob)
install(FILES ${PROJECT_SOURCE_DIR}/include/QtTUI/QtTUI
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
    ${PROJECT_SOURCE_DIR}/cmake/QtTUIConfig.cmake.in
    ${PROJECT_BINARY_DIR}/QtTUIConfig.cmake
    INSTALL_DESTINATION lib/cmake/QtTUI
)

write_basic_package_version_file(
    ${PROJECT_BINARY_DIR}/QtTUIConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${PROJECT_BINARY_DIR}/QtTUIConfig.cmake
    ${PROJECT_BINARY_DIR}/QtTUIConfigVersion.cmake
    DESTINATION lib/cmake/QtTUI
)
