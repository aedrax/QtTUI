function(qttui_add_demo DEMO_NAME DEMO_SOURCE)
    add_executable(${DEMO_NAME} ${DEMO_SOURCE})
    target_link_libraries(${DEMO_NAME} PRIVATE QtTUI::QtTUI Qt6::Core)
endfunction()

qttui_add_demo(demo_hello hello.cpp)
qttui_add_demo(demo_todo todo.cpp)
qttui_add_demo(demo_table table.cpp)
