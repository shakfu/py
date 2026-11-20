# py_code: in-patch Python editor for [py], tree-sitter highlighting, LSP client.
# Links neither Python nor MSP: it emits `exec`/`eval` messages.

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)
enable_language(OBJCXX)

set(TS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tree-sitter/lib)
set(TS_PY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tree-sitter-python/src)

# Vendored C sources, warnings off.
add_library(py_code_treesitter STATIC
    ${TS_DIR}/src/lib.c
    ${TS_PY_DIR}/parser.c
    ${TS_PY_DIR}/scanner.c
)
target_include_directories(py_code_treesitter
    PUBLIC ${TS_DIR}/include
    PRIVATE ${TS_DIR}/src ${TS_PY_DIR})
target_compile_options(py_code_treesitter PRIVATE -w)
set_target_properties(py_code_treesitter PROPERTIES C_STANDARD 11 POSITION_INDEPENDENT_CODE ON)

# Headless core and LSP client, shared by the external and the tests.
file(GLOB PY_CODE_CORE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/core/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lsp/*.cpp)
add_library(py_code_core STATIC py_syntax.cpp ${PY_CODE_CORE_SOURCES})
target_include_directories(py_code_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/nlohmann/include)
target_link_libraries(py_code_core PUBLIC py_code_treesitter)
set_target_properties(py_code_core PROPERTIES
    CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON POSITION_INDEPENDENT_CODE ON)
target_compile_options(py_code_core PRIVATE -Wall -Wextra -Wno-unused-parameter)

file(GLOB PY_CODE_MAX_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/py_code*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/*.mm)
if (PY_CODE_MAX_SOURCES)
    add_library(${PROJECT_NAME} MODULE
        ${PY_CODE_MAX_SOURCES}
        # UI boxes need the symbols jpatcher_syms_init() fills.
        "${MAX_SDK_INCLUDES}/common/jpatcher_syms.c")
    target_include_directories(${PROJECT_NAME} PRIVATE "${MAX_SDK_INCLUDES}")
    # The `(method)` casts of class_addmethod trip -Wcast-function-type-mismatch.
    target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX,OBJCXX>:-Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type-mismatch>)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        py_code_core
        "-framework AppKit"
        "-weak_framework UniformTypeIdentifiers"
        # Export only the entry point: vendored symbols stay private.
        "-Wl,-exported_symbol,_ext_main")
    include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)
endif ()

file(GLOB PY_CODE_TESTS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tests/*_test.cpp)
list(FILTER PY_CODE_TESTS EXCLUDE REGEX "integration_test\\.cpp$")
add_executable(py_code_tests tests/main.cpp ${PY_CODE_TESTS})
target_link_libraries(py_code_tests PRIVATE py_code_core)
set_target_properties(py_code_tests PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
target_compile_definitions(py_code_tests PRIVATE
    PY_CODE_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests")
add_test(NAME py_code_tests COMMAND py_code_tests)

# Real-server integration (pylsp through the login shell); exits 77 when absent.
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tests/lsp_integration_test.cpp)
    add_executable(py_code_lsp_integration tests/lsp_integration_test.cpp)
    target_link_libraries(py_code_lsp_integration PRIVATE py_code_core)
    set_target_properties(py_code_lsp_integration PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)
    target_compile_definitions(py_code_lsp_integration PRIVATE
        PY_API_STUB="${CMAKE_CURRENT_SOURCE_DIR}/../py/api.pyi")
    add_test(NAME py_code_lsp_integration COMMAND py_code_lsp_integration)
    set_tests_properties(py_code_lsp_integration PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endif ()
