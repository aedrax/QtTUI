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
