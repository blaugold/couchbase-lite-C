cmake_minimum_required (VERSION 3.1)
cmake_policy(VERSION 3.1)
project (CBL_C_Tests)

set(TOP ${PROJECT_SOURCE_DIR}/../)

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()
    
add_definitions(
    -DCBL_TESTS
    -DCATCH_CONFIG_CPP11_STREAM_INSERTABLE_CHECK
    -DNOMINMAX # Get rid of min/max macros that interfere with std::min/std::max
)

include_directories(
    ${TOP}test/
    ${TOP}include/
    ${TOP}/src/        # Just for CBLPrivate.h
    ${TOP}vendor/couchbase-lite-core/vendor/fleece/API/
    ${TOP}vendor/couchbase-lite-core/vendor/fleece/vendor/catch/
    ${TOP}vendor/couchbase-lite-core/vendor/fleece/vendor/libb64/
    ${PROJECT_BINARY_DIR}/../generated_headers/private/cbl/ # Real independent projects won't need this
    ${PROJECT_BINARY_DIR}/../generated_headers/public/cbl/ # or this
)

set(TEST_SRC
    BlobTest.cc
    BlobTest_Cpp.cc
    CBLTest.c
    CBLTest.cc
    CBLTestsMain.cpp
    DatabaseTest.cc
    DatabaseTest_Cpp.cc
    LogTest.cc
    QueryTest.cc
    ReplicatorTest.cc
    ReplicatorEETest.cc
    ReplicatorPropEncTest.cc
    ${TOP}vendor/couchbase-lite-core/vendor/fleece/Fleece/Support/Backtrace.cc
    ${TOP}vendor/couchbase-lite-core/vendor/fleece/Fleece/Support/LibC++Debug.cc
    ${TOP}vendor/couchbase-lite-core/vendor/fleece/Fleece/Support/betterassert.cc
)
add_executable(CBL_C_Tests ${TEST_SRC} )

if(EMSCRIPTEN)
    target_link_libraries(CBL_C_Tests PRIVATE ${CBL_LIBRARIES_PRIVATE})

    target_link_options(
        CBL_C_Tests PRIVATE
        "-pthread"
        "-fwasm-exceptions"
        "-lembind"
        "SHELL:-s EXIT_RUNTIME=1"
        "SHELL:-s PROXY_TO_PTHREAD=1"
        "SHELL:-s ALLOW_MEMORY_GROWTH=1"
        "SHELL:-s DEMANGLE_SUPPORT=1"
        "SHELL:-s WASM_BIGINT=1"
    )

    if(${CBL_JS_ENV} STREQUAL "Web")
        target_link_options(
            CBL_C_Tests PRIVATE
            "SHELL:-s ENVIRONMENT=web,worker"
            "SHELL:-s --embed-file ${CMAKE_CURRENT_BINARY_DIR}/test@test"
            "-lwebsocket.js"
        )
    elseif(${CBL_JS_ENV} STREQUAL "NodeJS")
        target_link_options(
            CBL_C_Tests PRIVATE
            "SHELL:-s ENVIRONMENT=node"
            "-lnodefs.js"
            "-lnoderawfs.js"
        )
    else()
        message(FATAL_ERROR "Unknown JS environment ${CBL_JS_ENV}")
    endif()
else()
    target_link_libraries(CBL_C_Tests PRIVATE  cblite)
endif()

if(MSVC)
    # For internal fleece dependencies:
    target_sources(
        CBL_C_Tests PRIVATE
        ${TOP}vendor/couchbase-lite-core/vendor/fleece/MSVC/asprintf.c
        ${TOP}vendor/couchbase-lite-core/vendor/fleece/MSVC/vasprintf-msvc.c
    )
    target_include_directories(
        CBL_C_Tests PRIVATE
        ${TOP}vendor/couchbase-lite-core/MSVC
        ${TOP}vendor/couchbase-lite-core/vendor/fleece/MSVC
    )

    set(BIN_TOP "${PROJECT_BINARY_DIR}/..")
    set(FilesToCopy ${BIN_TOP}/\$\(Configuration\)/cblite)

    add_custom_command(TARGET CBL_C_Tests POST_BUILD
        COMMAND ${CMAKE_COMMAND}
        -DFilesToCopy="${FilesToCopy}"
        -DDestinationDirectory=${PROJECT_BINARY_DIR}/\$\(Configuration\)
        -P ${TOP}vendor/couchbase-lite-core/MSVC/copy_artifacts.cmake)
elseif(ANDROID)
    target_link_libraries(CBL_C_Tests PUBLIC  "log")
elseif(APPLE AND BUILD_ENTERPRISE)
    target_link_libraries(CBL_C_Tests PUBLIC  "-framework CoreFoundation"
                                          "-framework CoreML"
                                          "-framework Vision")
elseif(UNIX)
    target_link_libraries(CBL_C_Tests PUBLIC dl)
endif()

file(COPY names_100.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/test)
