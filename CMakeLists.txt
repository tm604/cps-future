cmake_minimum_required(VERSION 2.8)

PROJECT(libcps-future)
option(USE_CLANG "build application with clang" OFF)
if(USE_CLANG)
	set(CMAKE_CXX_COMPILER "/usr/bin/clang++-3.6")
endif()
enable_testing()

add_subdirectory("rapidcheck")

set (VERSION_MAJOR 1)
set (VERSION_MINOR 0)
set (VERSION_PATCH 4)
set (PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set (PROJECT_VERSION_REVISION 2)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(warnings "-Wall -Wextra -Wconversion")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(warnings "/W4 /WX /EHsc")
endif()
set(CMAKE_CXX_FLAGS "-std=c++11 ${warnings}")

include_directories(include/)
include_directories(deps/)

add_executable(
	future_tests
	tests/main.cpp
	tests/helpers.cpp
	tests/future.cpp
)

#add_executable(
#	qc_tests
#	tests/main.cpp
#	tests/qc.cpp
#)
#target_link_libraries(
#	qc_tests
#	rapidcheck
#)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC future_tests "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(future_tests "${CMAKE_THREAD_LIBS_INIT}")
endif()

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

#add_executable(
#	standalone_tests
#	tests/standalone.cpp
#)

install(
	DIRECTORY include/
	DESTINATION include
)

include (CTest)

add_test (future_tests future_tests -r junit -o future_tests.xml)
add_test (qc_tests qc_tests -r junit -o qc_tests.xml)

include (InstallRequiredSystemLibraries)
set (CPACK_PACKAGE_NAME "libcps-future")
set (CPACK_PACKAGE_VENDOR "")
set (CPACK_PACKAGE_VERSION_MAJOR "${VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${VERSION_PATCH}")
set (CPACK_PACKAGE_CONTACT "Tom Molesworth <tom@perlsite.co.uk>")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "C++ futures implementation")
set (CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.debian")
set (CPACK_GENERATOR "TGZ;DEB")
set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS "OFF")
string(TOLOWER "${CPACK_PACKAGE_NAME}" CPACK_PACKAGE_NAME_LOWERCASE)
find_program(DPKG_PROGRAM dpkg)
if(DPKG_PROGRAM)
	execute_process(
		COMMAND ${DPKG_PROGRAM} --print-architecture
		OUTPUT_VARIABLE CPACK_DEBIAN_PACKAGE_ARCHITECTURE
		OUTPUT_STRIP_TRAILING_WHITESPACE
		)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${PROJECT_VERSION}-${PROJECT_VERSION_REVISION}_${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")   
else(DPKG_PROGRAM)
	set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME_LOWERCASE}_${PROJECT_VERSION}-${PROJECT_VERSION_REVISION}_${CMAKE_SYSTEM_NAME}")
endif(DPKG_PROGRAM)
# set (CPACK_DEBIAN_PACKAGE_SHLIBDEPS "ON")
include (CPack)
