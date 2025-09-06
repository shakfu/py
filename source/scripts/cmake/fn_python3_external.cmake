unset(CMAKE_OSX_DEPLOYMENT_TARGET)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "requires >= 10.15" FORCE)
set(CMAKE_EXPORT_COMPILE_COMMANDS True)

function(python3_external)
    set(options
        DEBUG
        INCLUDE_COMMONSYMS
        MIN_API
    )
    set(oneValueArgs 
        PROJECT_NAME
        BUILD_VARIANT
    )
    set(multiValueArgs
        PROJECT_SOURCE
        OTHER_SOURCE
        INCLUDE_DIRS
        LINK_LIBS
        LINK_DIRS
        COMPILE_DEFINITIONS
        COMPILE_OPTIONS
        LINK_OPTIONS
    )

    cmake_parse_arguments(
        PY3EXT
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    message(STATUS "PROJECT_NAME: ${PY3EXT_PROJECT_NAME}")
    message(STATUS "BUILD_VARIANT: ${PY3EXT_BUILD_VARIANT}")
    if(DEBUG)
        message(STATUS "PROJECT_SOURCE: ${PY3EXT_PROJECT_SOURCE}")
        message(STATUS "OTHER_SOURCE: ${PY3EXT_OTHER_SOURCE}")
        message(STATUS "PY3EXT_INCLUDE_DIRS: ${PY3EXT_INCLUDE_DIRS}")
        message(STATUS "PY3EXT_COMPILE_DEFINITIONS: ${PY3EXT_COMPILE_DEFINITIONS}")
        message(STATUS "PY3EXT_COMPILE_OPTIONS: ${PY3EXT_COMPILE_OPTIONS}")
        message(STATUS "PY3EXT_LINK_LIBS: ${PY3EXT_LINK_LIBS}")
        message(STATUS "PY3EXT_LINK_DIRS: ${PY3EXT_LINK_DIRS}")
        message(STATUS "PY3EXT_LINK_OPTIONS: ${PY3EXT_LINK_OPTIONS}")
    endif()

    set(variants local shared-ext static-ext framework-ext framework-pkg windows-pkg)
    set(self_contained shared-ext static-ext framework-ext)

    if (NOT ${PY3EXT_BUILD_VARIANT} IN_LIST variants)
        message(FATAL_ERROR 
            "BUILD_VARIANT must be one of local, shared-ext, static-ext, "
            "framework-ext, framework-pkg, windows-pkg" )
    endif()

    if(PY3EXT_BUILD_VARIANT STREQUAL "shared-ext")
        set(BUILD_SHARED_EXT 1)
    elseif(PY3EXT_BUILD_VARIANT STREQUAL "static-ext")
        set(BUILD_STATIC_EXT 1)
    elseif(PY3EXT_BUILD_VARIANT STREQUAL "framework-ext")
        set(BUILD_FRAMEWORK_EXT 1)
    elseif(PY3EXT_BUILD_VARIANT STREQUAL "framework-pkg")
        set(BUILD_FRAMEWORK_PKG 1)
    elseif(PY3EXT_BUILD_VARIANT STREQUAL "windows-pkg")
        set(BUILD_WINDOWS_PKG 1)
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(EXT_SUFFIX "mxo")
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(EXT_SUFFIX "mxe64")
    else()
        message(FATAL_ERROR "platform ${CMAKE_SYSTEM_NAME} not implemented")
    endif()

    set(DEPS_DIR "${CMAKE_SOURCE_DIR}/build/install")
    set(SUPPORT_DIR "${CMAKE_SOURCE_DIR}/support")
    set(EXTERNAL_DIR "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${PY3EXT_PROJECT_NAME}.${EXT_SUFFIX}")
    set(EXTERNAL_RESOURCES_DIR "${EXTERNAL_DIR}/Contents/Resources")

    if(BUILD_SHARED_EXT)
        set(Python3_ROOT_DIR "${DEPS_DIR}/python-shared")
        set(Python3_EXECUTABLE "${Python3_ROOT_DIR}/bin/python3")
    endif()

    if(BUILD_STATIC_EXT)
        set(Python3_ROOT_DIR "${DEPS_DIR}/python-static")
        set(Python3_EXECUTABLE "${Python3_ROOT_DIR}/bin/python3")
    endif()

    if(BUILD_FRAMEWORK_EXT)
        set(Python3_ROOT_DIR "${DEPS_DIR}/Python.framework")
        set(Python3_EXECUTABLE "${Python3_ROOT_DIR}/Versions/Current/bin/python3")
    endif()

    if(BUILD_FRAMEWORK_PKG)
        set(Python3_ROOT_DIR "${SUPPORT_DIR}/Python.framework")
        set(Python3_EXECUTABLE "${Python3_ROOT_DIR}/Versions/Current/bin/python3")
    endif()

    if(BUILD_WINDOWS_PKG)
        set(Python3_ROOT_DIR "${SUPPORT_DIR}")
        set(Python3_EXECUTABLE "${Python3_ROOT_DIR}/python.exe")
        set(Python3_INCLUDE_DIRS "${Python3_ROOT_DIR}/include")
        # message(STATUS "Python3_ROOT_DIR: ${Python3_ROOT_DIR}")
        # message(STATUS "Python3_EXECUTABLE: ${Python3_EXECUTABLE}")
        # message(STATUS "Python3_INCLUDE_DIRS: ${Python3_INCLUDE_DIRS}")
    endif()

    find_package (Python3 COMPONENTS Interpreter Development REQUIRED)

    message(STATUS "Python3_ROOT_DIR: ${Python3_ROOT_DIR}")
    message(STATUS "Python3_EXECUTABLE: ${Python3_EXECUTABLE}")

    if(PY3EXT_PROJECT_SOURCE)
        set(PROJECT_SRC ${PY3EXT_PROJECT_SOURCE})
    else()
        file(GLOB PROJECT_SRC
            "*.h"
            "*.c"
            "*.cpp"
        )
    endif()

    if(PY3EXT_MIN_API)
        include_directories( 
            "${C74_INCLUDES}"
        )
    else()
        include_directories(
            "${MAX_SDK_INCLUDES}"
            "${MAX_SDK_MSP_INCLUDES}"
            "${MAX_SDK_JIT_INCLUDES}"
        )
    endif()

    add_library( 
        ${PY3EXT_PROJECT_NAME} 
        MODULE
        ${PROJECT_SRC}
        ${PY3EXT_OTHER_SOURCE}
        $<$<BOOL:${PY3EXT_INCLUDE_COMMONSYMS}>:${MAX_SDK_INCLUDES}/common/commonsyms.c>
    )

    target_include_directories(
        ${PY3EXT_PROJECT_NAME}
        PRIVATE
        ${Python3_INCLUDE_DIRS}
        ${PY3EXT_INCLUDE_DIRS}
        $<$<BOOL:${BUILD_STATIC_EXT}>:${DEPS_DIR}/bzip2/include>
        $<$<BOOL:${BUILD_STATIC_EXT}>:${DEPS_DIR}/openssl/include>
        $<$<BOOL:${BUILD_STATIC_EXT}>:${DEPS_DIR}/xz/include>
    )

    target_compile_definitions(
        ${PY3EXT_PROJECT_NAME}
        PRIVATE
        ${PY3EXT_COMPILE_DEFINITIONS}
        $<$<CONFIG:Release>:NDEBUG>
        $<$<BOOL:${BUILD_STATIC_EXT}>:-DBUILD_STATIC> # help static find pyhome
        $<$<BOOL:${PY3EXT_INCLUDE_COMMONSYMS}>:-DINCLUDE_COMMONSYMS>
        $<$<BOOL:${BUILD_WINDOWS_PKG}>:-DPY_WINDOWS_PKG>
        # $<IN_LIST:${PY3EXT_BUILD_VARIANT},${self_contained}:-DSELFCONTAINED_EXTERNAL> # special case
    )

    target_compile_options(
        ${PY3EXT_PROJECT_NAME}
        PRIVATE
        ${PY3EXT_COMPILE_OPTIONS}
        $<$<PLATFORM_ID:Darwin>:-Wno-unused-result>
        $<$<PLATFORM_ID:Darwin>:-Wsign-compare>
        $<$<PLATFORM_ID:Darwin>:-Wunreachable-code>
        $<$<PLATFORM_ID:Darwin>:-Wall>  
        $<$<PLATFORM_ID:Darwin>:-g>
        $<$<PLATFORM_ID:Darwin>:-fwrapv>
        $<$<PLATFORM_ID:Darwin>:-O3>
        $<$<PLATFORM_ID:Windows>:/O2>
        $<$<PLATFORM_ID:Windows>:/MD> # api module works with this
    )

    target_link_directories(
        ${PY3EXT_PROJECT_NAME} 
        PRIVATE
        ${Python3_LIBRARY_DIRS}
        ${PY3EXT_LINK_DIRS}
        $<$<BOOL:${BUILD_STATIC_EXT}>:${DEPS_DIR}/bzip2/lib>
        $<$<BOOL:${BUILD_STATIC_EXT}>:${DEPS_DIR}/openssl/lib>
        $<$<BOOL:${BUILD_STATIC_EXT}>:${DEPS_DIR}/xz/lib>
        $<$<BOOL:${BUILD_WINDOWS_PKG}>:${SUPPORT_DIR}/libs>
    )

    set(STATIC_LINK_DEPS
        "$<$<PLATFORM_ID:Darwin>:-framework SystemConfiguration>"
        $<$<PLATFORM_ID:Darwin>:-lssl>
        $<$<PLATFORM_ID:Darwin>:-lbz2>
        $<$<PLATFORM_ID:Darwin>:-llzma>
        $<$<PLATFORM_ID:Darwin>:-lz>
        $<$<PLATFORM_ID:Darwin>:-lcrypto>
        $<$<PLATFORM_ID:Darwin>:-lsqlite3>
    )

    target_link_options(
        ${PY3EXT_PROJECT_NAME}
        PRIVATE
        ${PY3EXT_LINK_OPTIONS}
    )

    target_link_libraries(
        ${PY3EXT_PROJECT_NAME} 
        PRIVATE
        ${Python3_LIBRARIES}
        ${PY3EXT_LINK_LIBS}
        $<$<PLATFORM_ID:Darwin>:-ldl>
        "$<$<PLATFORM_ID:Darwin>:-framework CoreFoundation>"
        $<$<BOOL:${BUILD_STATIC_EXT}>:${STATIC_LINK_DEPS}>
    )

    if(BUILD_SHARED_EXT OR BUILD_STATIC_EXT)
        file(
            COPY "${Python3_ROOT_DIR}/lib" 
            DESTINATION "${EXTERNAL_RESOURCES_DIR}"
        )
    endif()

    if(BUILD_SHARED_EXT)
        ADD_CUSTOM_COMMAND(
            TARGET ${PY3EXT_PROJECT_NAME} POST_BUILD
            COMMAND strip -x "${EXTERNAL_RESOURCES_DIR}/lib/libpython3.*.dylib"
        )   
    endif()

    if(BUILD_STATIC_EXT)
        ADD_CUSTOM_COMMAND(
            TARGET ${PY3EXT_PROJECT_NAME} POST_BUILD
            COMMAND rm -f "${EXTERNAL_RESOURCES_DIR}/lib/libpython3.*.a"
            COMMAND strip -x "${EXTERNAL_DIR}/Contents/MacOS/${PY3EXT_PROJECT_NAME}"
        )
    endif()

    if(BUILD_FRAMEWORK_EXT)
        file(
            COPY "${Python3_ROOT_DIR}"
            DESTINATION "${EXTERNAL_RESOURCES_DIR}"
        )
        ADD_CUSTOM_COMMAND(
            TARGET ${PY3EXT_PROJECT_NAME} POST_BUILD
            COMMAND strip -x "${EXTERNAL_RESOURCES_DIR}/Python.framework/Versions/Current/Python"
        )
    endif()

    if(BUILD_FRAMEWORK_PKG)
        ADD_CUSTOM_COMMAND(
            TARGET ${PY3EXT_PROJECT_NAME} POST_BUILD
            COMMAND strip -x "${SUPPORT_DIR}/Python.framework/Versions/Current/Python"
        )
    endif()

    if(BUILD_WINDOWS_PKG)
        cmake_path(SET SUPPORT_DIR NORMALIZE ${SUPPORT_DIR})

        execute_process(
            OUTPUT_VARIABLE DOCUMENTS_DIR
            COMMAND PowerShell -Command "[Environment]::GetFolderPath('MyDocuments')"
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )

        set(MAX_PKGS_DIR "${DOCUMENTS_DIR}/Max 9/Packages")
        cmake_path(SET MAX_PKGS_DIR NORMALIZE "${MAX_PKGS_DIR}")
        if (NOT EXISTS ${MAX_PKGS_DIR})
            set(MAX_PKGS_DIR "${DOCUMENTS_DIR}/Max 8/Packages")
            cmake_path(SET MAX_PKGS_DIR NORMALIZE "${MAX_PKGS_DIR}")
            if (NOT EXISTS ${MAX_PKGS_DIR})
                message(WARNING "cannot find Max package directory")
                return() # exit this file
            endif()
        endif()

        if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
            set_property(CACHE CMAKE_INSTALL_PREFIX PROPERTY VALUE "${MAX_PKGS_DIR}")
        endif()

        set(INSTALL_DIRS
            docs
            examples
            init
            javascript
            jsextensions
            patchers
        )

        foreach(dir ${INSTALL_DIRS})
            install(DIRECTORY ${CMAKE_SOURCE_DIR}/${dir} DESTINATION py-js)
        endforeach()

        install(DIRECTORY ${CMAKE_SOURCE_DIR}/support DESTINATION py-js
            PATTERN "include" EXCLUDE
            PATTERN "libs" EXCLUDE
        )

        install(FILES ${CMAKE_SOURCE_DIR}/externals/${PY3EXT_PROJECT_NAME}.mxe64 DESTINATION py-js/externals)
        install(FILES ${CMAKE_SOURCE_DIR}/help/${PY3EXT_PROJECT_NAME}.maxhelp DESTINATION py-js/help)
        install(FILES ${CMAKE_SOURCE_DIR}/README.md DESTINATION py-js)
        install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION py-js)
        install(FILES ${CMAKE_SOURCE_DIR}/source/docs/_book/Python3-Externals-for-Max-MSP.pdf DESTINATION py-js)

    endif()

endfunction()
