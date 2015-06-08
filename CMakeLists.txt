cmake_minimum_required(VERSION 2.8)

PROJECT(libcps-future)
enable_testing()

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)

add_definitions(-std=c++11)

add_definitions(-DBOOST_LOG_DYN_LINK)
find_package(Boost COMPONENTS chrono system thread log unit_test_framework REQUIRED)

include_directories(include/)

set(Future_LIBRARY cps-future)
add_library(
	${Future_LIBRARY}
	src/future.cpp
)

add_executable(
	future_tests
	tests/future.cpp
)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC future_tests "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(future_tests "${CMAKE_THREAD_LIBS_INIT}")
endif()
target_link_libraries(
	future_tests
	${Future_LIBRARY}
	${Boost_LIBRARIES}
)

add_executable(
	benchmark
	src/benchmark.cpp
)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC benchmark "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(benchmark "${CMAKE_THREAD_LIBS_INIT}")
endif()
target_link_libraries(
	benchmark
	${Future_LIBRARY}
	${Boost_LIBRARIES}
)

install(
	DIRECTORY include/
	DESTINATION include
)
install(
	TARGETS ${Future_LIBRARY}
	ARCHIVE DESTINATION lib
)

include (CTest)

add_test (future_tests future_tests --log_level=all --log_format=XML --log_sink=future_tests.xml)

