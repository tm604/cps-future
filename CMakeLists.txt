cmake_minimum_required(VERSION 2.8)

PROJECT(libcps-future)
enable_testing()

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)

add_definitions(-std=c++11)

add_definitions(-DBOOST_LOG_DYN_LINK)
find_package(Boost COMPONENTS chrono system thread log unit_test_framework REQUIRED)

include_directories(include/)

add_executable(
	future_tests
	src/future_tests.cpp
	include/cps/future.h
)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC future_tests "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(future_tests "${CMAKE_THREAD_LIBS_INIT}")
endif()
target_link_libraries(future_tests ${Boost_LIBRARIES})

add_executable(
	leaf_future_tests
	src/leaf_future_tests.cpp
	include/cps/future.h
)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC leaf_future_tests "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(leaf_future_tests "${CMAKE_THREAD_LIBS_INIT}")
endif()
target_link_libraries(leaf_future_tests ${Boost_LIBRARIES})

install(
	FILES include/cps/future.h
	DESTINATION "include/cps"
)

include (CTest)

add_test (future_tests future_tests --log_level=all --log_format=XML --log_sink=future_tests.xml)
add_test (leaf_future_tests leaf_future_tests --log_level=all --log_format=XML --log_sink=leaf_future_tests.xml)

