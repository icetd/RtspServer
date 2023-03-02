cmake_minimum_required(VERSION 3.14)

project(RtspServer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(thirdparty/libv4l2cc)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/inc")
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")

set(X264_INC	"${CMAKE_SOURCE_DIR}/thirdparty/x264/include")
set(X264_LIB	"${CMAKE_SOURCE_DIR}/thirdparty/x264/lib")
set(V4L2CC_INC	"${CMAKE_SOURCE_DIR}/thirdparty/libv4l2cc/inc")
#set(V4L2CC_LIB	"${CMAKE_SOURCE_DIR}/thirdparty/libv4l2cc/lib")

#message("-- find x264 lib ${X264_LIB}")
#message("-- find x264 headfile ${X264_LIB}")
#message("-- find v4l2cc lib ${V4L2CC_INC}")
#message("-- find v4l2cc headfile ${V4L2CC_INC}")

include_directories(
	${INCLUDE_DIR}
	${INCLUDE_DIR}/x264encoder
	${INCLUDE_DIR}/rtppackage
	${INCLUDE_DIR}/rtspserver
	${X264_INC}
	${V4L2CC_INC}				
) 

link_directories(
		${X264_LIB}
	#   ${V4L2CC_LIB}
)

file(GLOB_RECURSE SRC_FILE
    "${SOURCE_DIR}/x264encoder/*.cc"
    "${SOURCE_DIR}/rtppackage/*.cc"
    "${SOURCE_DIR}/rtspserver/*.cc"
	"${SOURCE_DIR}/*cc"
)

file (GLOB_RECURSE INC_FILE
    "${INCLUDE_DIR}/x264encoder/*.h"
    "${INCLUDE_DIR}/rtppackage/*.h"
    "${INCLUDE_DIR}/rtspserver/*.h"
    "${X264_INC}/*.h"
)


if(UNIX)
    list(APPEND EXTRA_LIBS
    )
else(WIN32)
    list(APPEND EXTRA_LIBS
    )
endif()

# Add the executable
if (WIN32) 
    add_executable(${PROJECT_NAME} ${SRC_FILE})
else(UNIX)
    add_executable(${PROJECT_NAME} ${SRC_FILE})
endif()

# Add the target includes for MY_PROJECT 
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIR})

#===================== LINKING LIBRARIES =======================#
target_link_libraries(${PROJECT_NAME} v4l2cc x264 pthread ${EXTRA_LIBS})
