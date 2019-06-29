cmake_minimum_required(VERSION 3.0.0)
project(ipcaster VERSION 0.1.0)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(CTest)
enable_testing()

if(MSVC)
	if(NOT HOME)
    		# Try for USERPROFILE as HOME equivalent:
		string(REPLACE "\\" "/" HOME "$ENV{USERPROFILE}")
	endif()

	set (BOOST_ROOT ${HOME}/repos/boost_1_67_0)
	set (BOOST_INCLUDEDIR ${HOME}/repos/boost_1_67_0)
	set (BOOST_LIBRARYDIR ${HOME}/repos/boost_1_67_0/stage/lib)

    set(Boost_USE_STATIC_LIBS        ON) # only find static libs
    set(Boost_USE_MULTITHREADED      ON)
    set(Boost_USE_STATIC_RUNTIME    OFF)
    find_package(Boost 1.67.0 REQUIRED COMPONENTS system program_options date_time regex)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

add_subdirectory(src)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
