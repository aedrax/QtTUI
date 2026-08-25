find_package(Qt6 COMPONENTS Test REQUIRED)

# RapidCheck, fetched via FetchContent if not found locally
find_package(rapidcheck QUIET)
if(NOT rapidcheck_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        rapidcheck
        GIT_REPOSITORY https://github.com/emil-e/rapidcheck.git
        GIT_TAG        master
    )
    set(RC_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(rapidcheck)
endif()

# Helper function to add a QtTUI test
function(qttui_add_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} PRIVATE
        QtTUI::QtTUI
        Qt6::Core
        Qt6::Test
        rapidcheck
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

# Placeholder test to verify the test build works
qttui_add_test(test_placeholder test_placeholder.cpp)

# ScreenBuffer property tests (Properties 5, 8)
qttui_add_test(test_screen_buffer test_screen_buffer.cpp)

# AnsiRenderer unit tests (differential rendering, batching)
qttui_add_test(test_ansi_renderer test_ansi_renderer.cpp)

# MockTerminalBackend (shared test helper, compiled as object library)
add_library(mock_terminal_backend OBJECT MockTerminalBackend.cpp MockTerminalBackend.h)
target_link_libraries(mock_terminal_backend PUBLIC QtTUI::QtTUI Qt6::Core Qt6::Test)
target_include_directories(mock_terminal_backend PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# InputParser property tests (Properties 1-4)
qttui_add_test(test_input_parser test_input_parser.cpp)

# TerminalBackend property tests (Property 35)
add_executable(test_terminal_backend test_terminal_backend.cpp)
target_link_libraries(test_terminal_backend PRIVATE
    QtTUI::QtTUI
    Qt6::Core
    Qt6::Test
    rapidcheck
    mock_terminal_backend
)
add_test(NAME test_terminal_backend COMMAND test_terminal_backend)

# QTuiWidget property tests (Properties 10-13)
qttui_add_test(test_qtui_widget test_qtui_widget.cpp)

# QTuiMainWindow property tests (Properties 14-16)
qttui_add_test(test_qtui_mainwindow test_qtui_mainwindow.cpp)

# QTuiLineEdit property tests (Properties 26-30)
qttui_add_test(test_qtui_lineedit test_qtui_lineedit.cpp)

# QTuiCompleter matching and line edit popup flow
qttui_add_test(test_qtui_completer test_qtui_completer.cpp)

# QTuiValidator classes and QTuiLineEdit maxLength / validator rules
qttui_add_test(test_qtui_validator test_qtui_validator.cpp)

# QTuiTableView property tests (Properties 17-21)
qttui_add_test(test_qtui_tableview test_qtui_tableview.cpp)

# QTuiListView property tests (Properties 22-25)
qttui_add_test(test_qtui_listview test_qtui_listview.cpp)

# QTuiBoxLayout property tests (Properties 31-32)
qttui_add_test(test_qtui_boxlayout test_qtui_boxlayout.cpp)

# Layout base classes unit tests
qttui_add_test(test_qtui_layout_base test_qtui_layout_base.cpp)

# QTuiFormLayout unit tests
qttui_add_test(test_qtui_form_layout test_qtui_form_layout.cpp)

# QTuiGridLayout unit tests
qttui_add_test(test_qtui_grid_layout test_qtui_grid_layout.cpp)

# QTuiStackedLayout unit tests
qttui_add_test(test_qtui_stacked_layout test_qtui_stacked_layout.cpp)

# Theme_Engine unit tests
qttui_add_test(test_theme_engine test_theme_engine.cpp)

# UI_Loader unit tests
qttui_add_test(test_ui_loader test_ui_loader.cpp)

# QTuiLabel unit tests
qttui_add_test(test_qtui_label test_qtui_label.cpp)

# Button and toggle widget tests
qttui_add_test(test_qtui_button_widgets test_qtui_button_widgets.cpp)

# Numeric input widget tests
qttui_add_test(test_qtui_numeric_widgets test_qtui_numeric_widgets.cpp)

# ComboBox and ProgressBar widget tests
qttui_add_test(test_qtui_combobox_progressbar test_qtui_combobox_progressbar.cpp)

# QTuiTextEdit unit tests
qttui_add_test(test_qtui_textedit test_qtui_textedit.cpp)

# QTuiDateTimeEdit, QTuiDateEdit, QTuiTimeEdit, QTuiLCDNumber unit tests
qttui_add_test(test_qtui_datetime_lcd test_qtui_datetime_lcd.cpp)

# QTuiScrollBar and QTuiScrollArea unit tests
qttui_add_test(test_qtui_scroll_widgets test_qtui_scroll_widgets.cpp)

# QTuiGroupBox and QTuiCollapsible unit tests
qttui_add_test(test_qtui_container_widgets test_qtui_container_widgets.cpp)

# QTuiTabBar, QTuiTabWidget, QTuiToolBox unit tests
qttui_add_test(test_qtui_tab_widgets test_qtui_tab_widgets.cpp)

# QTuiStackedWidget, QTuiSplitter, QTuiSplitterHandle unit tests
qttui_add_test(test_qtui_stacked_splitter test_qtui_stacked_splitter.cpp)

# QTuiFocusFrame, QTuiSizeGrip, QTuiCalendarWidget unit tests
qttui_add_test(test_qtui_utility_widgets test_qtui_utility_widgets.cpp)

# QTuiItemDelegate, QTuiStyledItemDelegate, QTuiHeaderView unit tests
qttui_add_test(test_qtui_delegates_headerview test_qtui_delegates_headerview.cpp)

# QTuiTreeView, QTuiColumnView, QTuiUndoView unit tests
qttui_add_test(test_qtui_treeview_columnview_undoview test_qtui_treeview_columnview_undoview.cpp)

# QTuiDataWidgetMapper unit tests
qttui_add_test(test_qtui_datawidgetmapper test_qtui_datawidgetmapper.cpp)

# QTuiListWidget unit tests
qttui_add_test(test_qtui_listwidget test_qtui_listwidget.cpp)

# QTuiTableWidget unit tests
qttui_add_test(test_qtui_tablewidget test_qtui_tablewidget.cpp)

# QTuiTreeWidget unit tests
qttui_add_test(test_qtui_treewidget test_qtui_treewidget.cpp)

# QTuiMenu, QTuiMenuBar unit tests
qttui_add_test(test_qtui_menu_widgets test_qtui_menu_widgets.cpp)

# QTuiToolBar, QTuiStatusBar, QTuiWidgetAction unit tests
qttui_add_test(test_qtui_toolbar_statusbar test_qtui_toolbar_statusbar.cpp)

# QTuiDockWidget unit tests
qttui_add_test(test_qtui_dock_widget test_qtui_dock_widget.cpp)

# QTuiMdiArea, QTuiMdiSubWindow unit tests
qttui_add_test(test_qtui_mdi_widgets test_qtui_mdi_widgets.cpp)

# QTuiMainWindow integration tests (menu bar, toolbars, status bar, dock widgets)
qttui_add_test(test_qtui_mainwindow_integration test_qtui_mainwindow_integration.cpp)

# Widget Gallery demo tests (structure, behavior, signal/slot wiring)
qttui_add_test(test_gallery test_gallery.cpp)
# The gallery test builds the real gallery with the demo's builder
target_sources(test_gallery PRIVATE ${PROJECT_SOURCE_DIR}/demos/GalleryBuilder.cpp
                                    ${PROJECT_SOURCE_DIR}/demos/GalleryBuilder.h)
target_include_directories(test_gallery PRIVATE ${PROJECT_SOURCE_DIR}/demos)

# ScreenBuffer bounds safety property tests (Property 35)
qttui_add_test(test_screenbuffer_bounds test_screenbuffer_bounds.cpp)

# RenderScheduler coalescing property tests (Property 4)
qttui_add_test(test_render_scheduler test_render_scheduler.cpp)

# Focus rendering property tests (Property 28)
qttui_add_test(test_focus_rendering test_focus_rendering.cpp)

# Ellipsis truncation property tests (Property 21)
qttui_add_test(test_ellipsis_truncation test_ellipsis_truncation.cpp)

# Clip-rect enforcement and dirty-only repaint property tests (Properties 3, 5)
qttui_add_test(test_clip_rect test_clip_rect.cpp)

# Widget background clearing property tests (Properties 22, 23, 24)
qttui_add_test(test_widget_clearing test_widget_clearing.cpp)

# Hidden-widget compositing regression tests (content restored under hidden overlays)
qttui_add_test(test_hidden_widget_restore test_hidden_widget_restore.cpp)

# Popup layer tests: menus/dropdowns render topmost above parent clip rects,
# outside-click dismissal, focus restore (popup-overlay-layer)
qttui_add_test(test_popup_rendering test_popup_rendering.cpp)

# Widgets painted by their parent (scroll area content): skipped by the
# render pass and the mouse hit test
qttui_add_test(test_painted_by_parent test_painted_by_parent.cpp)

# Layout invalidation property tests (Properties 6, 7, 8)
qttui_add_test(test_layout_invalidation test_layout_invalidation.cpp)

# Model/View dirty region property tests (Properties 9, 10, 11, 12)
qttui_add_test(test_model_view_dirty test_model_view_dirty.cpp)

# Table rendering property tests (Properties 13, 14, 15)
qttui_add_test(test_tableview_rendering test_tableview_rendering.cpp)

# Model/View DisplayRole rendering and List View scroll property tests (Properties 16, 17)
qttui_add_test(test_model_view_rendering test_model_view_rendering.cpp)

# Tree indentation, indicators, and expand/collapse property tests (Properties 18, 19, 20)
qttui_add_test(test_treeview_rendering test_treeview_rendering.cpp)

# Container rendering property tests (Properties 25, 26, 27)
qttui_add_test(test_container_rendering test_container_rendering.cpp)

# Theme invalidation property tests (Properties 33, 34)
qttui_add_test(test_theme_invalidation test_theme_invalidation.cpp)

# ANSI round-trip property tests (Properties 30, 31, 32)
qttui_add_test(test_ansi_roundtrip test_ansi_roundtrip.cpp)

# Bug condition exploration tests: dirty-tracking bugs (rendering-dirty-tracking-fixes spec)
qttui_add_test(test_dirty_tracking_bugs test_dirty_tracking_bugs.cpp)

# Preservation property tests: dirty-tracking behavior unchanged (rendering-dirty-tracking-fixes spec)
qttui_add_test(test_dirty_tracking_preservation test_dirty_tracking_preservation.cpp)

# Lifetime regression tests: deleting widgets/models still referenced by
# layouts, views, main window caches, or theme overrides must not dangle
qttui_add_test(test_lifetime test_lifetime.cpp)

# QTuiApplication error-handling tests (terminalInitialized(), message
# handler not installed in headless environments)
qttui_add_test(test_qtui_application test_qtui_application.cpp)

# Initial render verification test (tab content renders without interaction)
add_executable(test_initial_render test_initial_render.cpp)
target_link_libraries(test_initial_render PRIVATE QtTUI::QtTUI Qt6::Core)
add_test(NAME test_initial_render COMMAND test_initial_render)

# Global shortcut dispatch tests: QTuiKeySequence parsing, QTuiAction
# registry, main-window shortcut/mnemonic dispatch (backlog #9)
qttui_add_test(test_shortcut_dispatch test_shortcut_dispatch.cpp)

# Enabled state + focus policy tests: disabled widgets are inert, Tab chain
# and mouse focus honor focusPolicy, group box disables children (backlog #12)
qttui_add_test(test_enabled_focuspolicy test_enabled_focuspolicy.cpp)

# Selection tests: selection model, selection modes and behaviors, and
# the selected items of the item widgets
qttui_add_test(test_selection test_selection.cpp)
