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
