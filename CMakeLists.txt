cmake_minimum_required(VERSION 3.14)

project(RtspServer)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(thirdparty/libv4l2cc)
add_subdirectory(thirdparty/libjpeg-turbo)

set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/inc")
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")

set(X264_INC		"${CMAKE_SOURCE_DIR}/thirdparty/x264/include")
set(X264_LIB		"${CMAKE_SOURCE_DIR}/thirdparty/x264/lib")
set(V4L2CC_INC		"${CMAKE_SOURCE_DIR}/thirdparty/libv4l2cc/inc")
set(JPEGTURBO_INC   "${CMAKE_SOURCE_DIR}/thirdparty/libjpeg-turbo")

#message("-- find x264 lib ${X264_LIB}")
#message("-- find x264 headfile ${X264_LIB}")
#message("-- find v4l2cc lib ${V4L2CC_INC}")
#message("-- find v4l2cc headfile ${V4L2CC_INC}")

include_directories(
	${INCLUDE_DIR}
	${X264_INC}
	${V4L2CC_INC}
	${JPEGTURBO_INC}
) 

link_directories(
		${X264_LIB}
)

file(GLOB_RECURSE SRC_FILES
	"${SOURCE_DIR}/*c*"
)

file (GLOB_RECURSE INC_FILES
    "${INCLUDE_DIR}/*.h"
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
add_executable(${PROJECT_NAME} ${SRC_FILES} ${INC_FILES})

# Add the target includes for MY_PROJECT 
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${SOURCE_DIR})

#===================== LINKING LIBRARIES =======================#
target_link_libraries(${PROJECT_NAME} v4l2cc turbojpeg x264 pthread ${EXTRA_LIBS})

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/configs
		DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
