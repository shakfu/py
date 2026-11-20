# py_code: in-patch Python editor (UI only) with tree-sitter highlighting.
# Links neither Python nor MSP: it emits `exec`/`eval` messages for [py].

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)
enable_language(OBJCXX)

set(TS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tree-sitter/lib)
set(TS_PY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tree-sitter-python/src)

# Vendored C sources: compiled once, warnings off (not our code).
add_library(py_code_treesitter STATIC
    ${TS_DIR}/src/lib.c
    ${TS_PY_DIR}/parser.c
    ${TS_PY_DIR}/scanner.c
)
target_include_directories(py_code_treesitter
    PUBLIC ${TS_DIR}/include
    PRIVATE ${TS_DIR}/src ${TS_PY_DIR})
target_compile_options(py_code_treesitter PRIVATE -w)
set_target_properties(py_code_treesitter PROPERTIES
    C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)

# Headless modules shared by the external and the tests.
add_library(py_code_core STATIC
    py_blink.cpp
    py_buffer.cpp
    py_complete.cpp
    py_path.cpp
    py_scroll.cpp
    py_snip.cpp
    py_syntax.cpp
    py_theme.cpp
    py_wrap.cpp
)
target_include_directories(py_code_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(py_code_core PUBLIC py_code_treesitter)
set_target_properties(py_code_core PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON)
target_compile_options(py_code_core PRIVATE -Wall -Wextra -Wno-unused-parameter)

add_library(${PROJECT_NAME} MODULE
    py_code.cpp
    py_clipboard.mm
    py_filedialog.mm
    # UI objects need the symbols jpatcher_syms_init() fills.
    "${MAX_SDK_INCLUDES}/common/jpatcher_syms.c"
)
target_include_directories(${PROJECT_NAME} PRIVATE "${MAX_SDK_INCLUDES}")
# The `(method)` casts of class_addmethod trip -Wcast-function-type-mismatch.
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type-mismatch>)
target_link_libraries(${PROJECT_NAME} PRIVATE
    py_code_core
    "-framework AppKit"
    "-framework UniformTypeIdentifiers"
    # Export only the entry point: the vendored ts_* symbols stay private.
    "-Wl,-exported_symbol,_ext_main")

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)

add_executable(py_code_tests
    tests/main.cpp
    tests/buffer_test.cpp
    tests/complete_test.cpp
    tests/syntax_test.cpp
)
target_link_libraries(py_code_tests PRIVATE py_code_core)
set_target_properties(py_code_tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
add_test(NAME py_code_tests COMMAND py_code_tests)
