set(LIVE555_GIT https://github.com/rgaufman/live555.git CACHE STRING "live555 git repo")

set(LIVE555_CFLAGS
    -DBSD=1
    -DSOCKLEN_T=socklen_t
    -D_FILE_OFFSET_BITS=64
    -D_LARGEFILE_SOURCE=1
    -DALLOW_RTSP_SERVER_PORT_REUSE=1
    -DNO_STD_LIB=1
    CACHE STRING "live555 CFLAGS"
)

if (WITH_SSL)
  find_package(OpenSSL QUIET)
endif()

set(LIVE ${CMAKE_BINARY_DIR}/live)

set(LIVE555_INC
    ${LIVE}/groupsock/include
    ${LIVE}/liveMedia/include
    ${LIVE}/UsageEnvironment/include
    ${LIVE}/BasicUsageEnvironment/include
)

if (NOT EXISTS ${LIVE})
    message(STATUS "Cloning live555...")

    execute_process(
        COMMAND git clone --depth 1 ${LIVE555_GIT} ${LIVE}
        RESULT_VARIABLE clone_result
    )

    if(NOT clone_result EQUAL 0)
        message(FATAL_ERROR "Fetching live555 failed!")
    endif()
endif()

file(GLOB LIVE555_FILES
    ${LIVE}/groupsock/*.c*
    ${LIVE}/liveMedia/*.c*
    ${LIVE}/UsageEnvironment/*.c*
    ${LIVE}/BasicUsageEnvironment/*.c*
)

file(GLOB LIVE555_INC_FILES
    ${LIVE}/groupsock/*.h*
    ${LIVE}/liveMedia/*.h*
    ${LIVE}/UsageEnvironment/*.h*
    ${LIVE}/BasicUsageEnvironment/*.h*
)

if (NOT OpenSSL_FOUND)
    set(LIVE555_CFLAGS ${LIVE555_CFLAGS} -DNO_OPENSSL=1)
endif()
