cmake_minimum_required(VERSION 3.10)
project(RkYoloRtspServer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

option (WITH_SSL "Enable SSL support" ON)

include (cmake/live555.cmake)
add_subdirectory(thirdparty/libv4l2cc)

set (SOURCE_DIR 		"${CMAKE_SOURCE_DIR}/src")
set (INCLUDE_DIR 		"${CMAKE_SOURCE_DIR}/inc")
set (V4L2CC_INC         "${CMAKE_SOURCE_DIR}/thirdparty/libv4l2cc/inc")
set (RKNN_INC           "${CMAKE_CURRENT_LIST_DIR}/thirdparty/librknn_api/include")
set (RKNN_IIB           "${CMAKE_CURRENT_LIST_DIR}/thirdparty/librknn_api/aarch64")
set (RGA_INC            "${CMAKE_CURRENT_LIST_DIR}/thirdparty/rga/include")
set (RGA_LIB            "${CMAKE_CURRENT_LIST_DIR}/thirdparty/rga/aarch64")
set (MPP_INC            "${CMAKE_SOURCE_DIR}/thirdparty/mpp/include")
set (MPP_LIB            "${CMAKE_SOURCE_DIR}/thirdparty/mpp/lib")

find_package(OpenCV REQUIRED)
message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")

link_directories(
    ${RKNN_IIB}
    ${RGA_LIB}
    ${MPP_LIB}
    ${OpenCV_LIBRARIES}
)

include_directories(
    ${INCLUDE_DIR}
    ${V4L2CC_INC}
    ${RKNN_INC}
    ${RGA_INC}
    ${LIVE555_INC}
    ${MPP_INC}
    ${OpenCV_INCLUDE_DIRS} 
)

file(GLOB_RECURSE SRC_FILES
    "${SOURCE_DIR}/*.c*"
)

add_executable(${PROJECT_NAME} ${SRC_FILES} ${LIVE555_FILES} ${LIVE555_INC_FILES})

# Add the target includes for MY_PROJECT 
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${LIVE555_INC})
target_compile_definitions(${PROJECT_NAME} PUBLIC  ${LIVE555_CFLAGS})


list(APPEND EXTRA_LIBS
    ${OpenCV_LIBS}
    rknnrt
    rga
    pthread
    v4l2cc 
    rockchip_vpu 
    rockchip_mpp 
)

target_link_libraries ( ${PROJECT_NAME}
	${EXTRA_LIBS}
)

if (OpenSSL_FOUND)
	target_link_libraries(${PROJECT_NAME} OpenSSL::SSL)
endif ()

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/configs DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/model DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
