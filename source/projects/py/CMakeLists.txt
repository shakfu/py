include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-pretarget.cmake)

include(${CMAKE_SOURCE_DIR}/source/scripts/cmake/fn_python3_external.cmake)


option(INCLUDE_NUMPY "include numpy headers if available")
option(INCLUDE_API_MODULE "include api c module" ON)
option(REGEN_API_MODULE "enable cython regen of api.c if api.pyx is modified")

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



include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-sdk-base/script/max-posttarget.cmake)
