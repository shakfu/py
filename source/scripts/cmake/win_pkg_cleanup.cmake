# delete the `libs` and `include` folder in `py-js/support`
#
# to be run as: cmake -P source/scripts/win_pkg_cleanup.cmake

set(SUPPORT_DIR ${CMAKE_SOURCE_DIR}/support)
set(lib_dir ${SUPPORT_DIR}/libs)
set(include_dir ${SUPPORT_DIR}/include)

cmake_path(NORMAL_PATH lib_dir OUTPUT_VARIABLE lib_dir)
cmake_path(NORMAL_PATH include_dir OUTPUT_VARIABLE include_dir)

if(EXISTS ${lib_dir})
	file(REMOVE_RECURSE ${lib_dir})
endif()

if(EXISTS ${include_dir})
	file(REMOVE_RECURSE ${include_dir})
endif()
