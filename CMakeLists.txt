cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

project("dwThreadPool")

set(DWTP_INCLUDE_DIRS "include")
set(REMOTERY_INCLUDE_DIRS "external/Remotery/lib")

message("${DWTP_INCLUDE_DIRS}")
message("${REMOTERY_INCLUDE_DIRS}")

IF(APPLE)
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LANGUAGE_STANDARD "c++11")
	set(CMAKE_XCODE_ATTRIBUTE_CLANG_CXX_LIBRARY "libc++")
ENDIF()

include_directories("${DWTP_INCLUDE_DIRS}" 
					"${REMOTERY_INCLUDE_DIRS}")

set(DWTP_SOURCE include/thread_pool.hpp
				include/semaphore.hpp
				example/unit_test.cpp
				external/Remotery/Remotery.c
				external/Remotery/Remotery.h)

add_executable(example ${DWTP_SOURCE})