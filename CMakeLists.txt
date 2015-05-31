cmake_minimum_required(VERSION 2.8)

PROJECT(libcps-future)
enable_testing()

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)

add_definitions(-std=c++11)

add_definitions(-DBOOST_LOG_DYN_LINK)
find_package(Boost COMPONENTS system thread log unit_test_framework REQUIRED)

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
	typed_future_tests
	src/typed_future_tests.cpp
	include/cps/future.h
)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC typed_future_tests "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(typed_future_tests "${CMAKE_THREAD_LIBS_INIT}")
endif()
target_link_libraries(typed_future_tests ${Boost_LIBRARIES})

add_executable(
	neo
	src/neo.cpp
	include/cps/future.h
)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC neo "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(neo "${CMAKE_THREAD_LIBS_INIT}")
endif()
target_link_libraries(neo ${Boost_LIBRARIES})

install(
	FILES include/cps/future.h
	DESTINATION "include/cps"
)

include (CTest)

add_test (boost future_tests --log_level=all --log_format=XML --log_sink=future_tests.xml)
add_test (boost typed_future_tests --log_level=all --log_format=XML --log_sink=typed_future_tests.xml)
add_test (boost neo --log_level=all --log_format=XML --log_sink=neo.xml)

