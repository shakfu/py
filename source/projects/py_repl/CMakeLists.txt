# py_repl: interactive console box for [py] (docs/dev/py_repl-spec.md).
# Runs code in a linked py object through its A_CANT repl methods (py_repl_abi.h);
# links neither Python nor MSP.

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)
enable_language(OBJCXX)

set(PY_CODE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../py_code)
set(PY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../py)

# Headless core, shared by the external and the tests.
file(GLOB PY_REPL_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp)
add_library(py_repl_core STATIC ${PY_REPL_CORE_SOURCES})
target_include_directories(py_repl_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# Editor, Layout, Scroll, Syntax, palette and completion come from py_code.
target_link_libraries(py_repl_core PUBLIC py_code_core)
set_target_properties(py_repl_core PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON)
target_compile_options(py_repl_core PRIVATE -Wall -Wextra -Wno-unused-parameter)

file(GLOB PY_REPL_MAX_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/py_repl*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/*.cpp)
if (PY_REPL_MAX_SOURCES)
    add_library(${PROJECT_NAME} MODULE
        ${PY_REPL_MAX_SOURCES}
        ${PY_CODE_DIR}/platform/clipboard.mm
        # UI boxes need the symbols jpatcher_syms_init() fills.
        "${MAX_SDK_INCLUDES}/common/jpatcher_syms.c")
    target_include_directories(${PROJECT_NAME} PRIVATE "${MAX_SDK_INCLUDES}" ${PY_DIR})
    set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
    # The `(method)` casts of class_addmethod trip -Wcast-function-type-mismatch.
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type-mismatch>)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        py_repl_core
        "-framework AppKit"
        "-framework CoreGraphics"
        # Export only the entry point: vendored symbols stay private.
        "-Wl,-exported_symbol,_ext_main")
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)
endif ()

file(GLOB PY_REPL_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_test.cpp)
add_executable(py_repl_tests tests/main.cpp ${PY_REPL_TESTS})
target_include_directories(py_repl_tests PRIVATE ${PY_CODE_DIR}/tests) # test.h
target_link_libraries(py_repl_tests PRIVATE py_repl_core)
set_target_properties(py_repl_tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_options(py_repl_tests PRIVATE -Wall -Wextra -Wno-unused-parameter)
add_test(NAME py_repl_tests COMMAND py_repl_tests)
