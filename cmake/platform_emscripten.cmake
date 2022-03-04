function(set_platform_source_files)
    set(oneValueArgs RESULT)
    cmake_parse_arguments(PLATFORM "" ${oneValueArgs} "" ${ARGN})
    if(NOT DEFINED PLATFORM_RESULT)
        message(FATAL_ERROR set_platform_include_directories needs to be called with RESULT)
    endif()

    if(${CBL_JS_ENV} STREQUAL "Web")
        set(
            ${PLATFORM_RESULT}
            src/WebWebSocket.cc
            PARENT_SCOPE
        )
    elseif(${CBL_JS_ENV} STREQUAL "NodeJS")
        set(
            ${PLATFORM_RESULT}
            PARENT_SCOPE
        )
    else()
        message(FATAL_ERROR "Unknown JS environment ${CBL_JS_ENV}")
    endif()
endfunction()

function(set_platform_include_directories)
    # No-op
endfunction()

function(init_vars)
    string(TOUPPER "CBL_JS_ENV_${CBL_JS_ENV}" CBL_JS_ENV_DEF)
    set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
        ${CBL_JS_ENV_DEF}
    )

    string(
        CONCAT CBL_CXX_FLAGS
        "-Wno-elaborated-enum-base "
        "-pthread "
        "-fwasm-exceptions "
    )
    set(CBL_CXX_FLAGS ${CBL_CXX_FLAGS} CACHE INTERNAL "")

    string(
        CONCAT CBL_C_FLAGS
        "-Wno-elaborated-enum-base "
        "-pthread "
        "-fwasm-exceptions "
    )
    set(CBL_C_FLAGS ${CBL_C_FLAGS} CACHE INTERNAL "")
endfunction()

function(set_dylib_properties)
    add_executable(cblite_js src/CBL_JSAPI.cc)

    target_include_directories(
        cblite_js
        PRIVATE
        "${PROJECT_BINARY_DIR}/generated_headers/private/cbl"
    )

    target_link_libraries(cblite_js PRIVATE ${CBL_LIBRARIES_PRIVATE})

    target_compile_options(
        cblite_js
        PRIVATE
        "-lembind"
    )

    set_property(
        DIRECTORY APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/CBL_JSAPI.js
    )

    target_link_options(
        cblite_js PRIVATE
        "-pthread"
        "-fwasm-exceptions"
        "-lembind"
        "SHELL:--pre-js ${PROJECT_SOURCE_DIR}/src/CBL_JSAPI.js"
        "SHELL:-s WASM_BIGINT=1"
        "SHELL:-s PTHREAD_POOL_SIZE_STRICT=0"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s ALLOW_TABLE_GROWTH=1"
        "SHELL:-s DEMANGLE_SUPPORT=1"
        "SHELL:-s MODULARIZE=1"
        "SHELL:-s EXPORT_NAME=cblite"
        "SHELL:-s EXPORTED_RUNTIME_METHODS=UTF8ToString,stringToUTF8,lengthBytesUTF8"
    )

    if(${CBL_JS_ENV} STREQUAL "Web")
        target_link_options(
            cblite_js PRIVATE
            "SHELL:-s ENVIRONMENT=web,worker"
            "-lwebsocket.js"
        )
    elseif(${CBL_JS_ENV} STREQUAL "NodeJS")
        target_link_options(
            cblite_js PRIVATE
            "SHELL:-s ENVIRONMENT=node"
            "-lnodefs.js"
            "-lnoderawfs.js"
        )
    else()
        message(FATAL_ERROR "Unknown JS environment ${CBL_JS_ENV}")
    endif()
endfunction()
