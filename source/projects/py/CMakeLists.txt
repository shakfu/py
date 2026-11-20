include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)

include(${CMAKE_SOURCE_DIR}/source/scripts/cmake/fn_python3_external.cmake)


option(INCLUDE_NUMPY "include numpy headers if available")
option(INCLUDE_API_MODULE "include api c module" ON)
option(REGEN_API_MODULE "enable cython regen of api.c if api.pyx is modified")
option(RESTRICTED_PYTHON "use the prelude that replaces eval with name lookup")

set(SCRIPTS ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
set(API_PYX_FILE ${CMAKE_CURRENT_SOURCE_DIR}/api.pyx)
set(API_C_FILE ${CMAKE_CURRENT_SOURCE_DIR}/api.c)

set(PROJECT_SOURCE
	${CMAKE_CURRENT_SOURCE_DIR}/py.c
	$<$<BOOL:${INCLUDE_API_MODULE}>:${API_C_FILE}>
)

python3_external(
    PROJECT_NAME ${PROJECT_NAME}
    BUILD_VARIANT ${BUILD_VARIANT}
    PROJECT_SOURCE ${PROJECT_SOURCE}
    COMPILE_DEFINITIONS
    	$<$<BOOL:${RESTRICTED_PYTHON}>:RESTRICTED_PYTHON>
)

if(INCLUDE_NUMPY)
	# optionally add numpy_header
	execute_process(
		COMMAND ${Python3_EXECUTABLE} ${SCRIPTS}/check_numpy.py --include
		OUTPUT_VARIABLE NUMPY_HEADERS
		RESULT_VARIABLE NUMPY_HEADERS_RESULT
	)

	if (NUMPY_HEADERS_RESULT EQUAL 0) # 0 == SUCCESS
		message(STATUS "INCLUDING NUMPY_HEADERS")
		message(STATUS "NUMPY_HEADERS: " ${NUMPY_HEADERS})
		include_directories("${NUMPY_HEADERS}")
		add_compile_definitions(INCLUDE_NUMPY=1)
	else()
		message(STATUS "NOT INCLUDING NUMPY_HEADERS: NUMPY NOT FOUND")
	endif()
endif()


if(INCLUDE_API_MODULE AND REGEN_API_MODULE)

	add_custom_command(
		OUTPUT ${API_C_FILE}
		DEPENDS ${API_PYX_FILE}
		COMMAND cython -3 --timestamps -E INCLUDE_NUMPY=${CYTHON_INCL_NUMPY} ${API_PYX_FILE}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating api.c"
	)

	add_custom_target(api_c
		ALL
		DEPENDS ${API_C_FILE}
	)

	add_dependencies(${PROJECT_NAME} api_c)

endif()


if(INCLUDE_API_MODULE)
	# Type stub for the built-in `api` module: py_code passes it to its language server.
	set(API_STUB_DIR $<TARGET_BUNDLE_DIR:${PROJECT_NAME}>/Contents/Resources/stubs)
	add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory ${API_STUB_DIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_SOURCE_DIR}/api.pyi ${API_STUB_DIR}/api.pyi
	)
endif()

# api.pyi must match api.pyx; skipped (77) without Cython.
find_program(CYTHON_PROGRAM cython)
if(CYTHON_PROGRAM)
	file(STRINGS ${CYTHON_PROGRAM} CYTHON_SHEBANG LIMIT_COUNT 1)
	string(REGEX REPLACE "^#! *" "" CYTHON_PYTHON "${CYTHON_SHEBANG}")
	add_test(NAME py_api_stub
		COMMAND ${CYTHON_PYTHON} ${SCRIPTS}/gen_api_stub.py --check ${API_PYX_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/api.pyi)
	set_tests_properties(py_api_stub PROPERTIES SKIP_RETURN_CODE 77)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)
