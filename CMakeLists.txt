cmake_minimum_required(VERSION 2.8)

project(libcps-future)

set (VERSION_MAJOR 1)
set (VERSION_MINOR 1)
set (VERSION_PATCH 0)
set (PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
set (PROJECT_VERSION_REVISION 1)

enable_testing()

set(RC_ENABLE_CATCH ON)

option(USE_CLANG "build application with clang" OFF)
if(USE_CLANG)
	set(CMAKE_CXX_COMPILER "/usr/bin/clang++-3.7")
endif()

option(USE_CSDBG "build application with csdbg" OFF)

if(USE_CSDBG)
	add_definitions(
		-fPIC -g
		-finstrument-functions
		-finstrument-functions-exclude-file-list=/usr/include
		-finstrument-functions-exclude-file-list=iostream
		-finstrument-functions-exclude-file-list=ios
		-finstrument-functions-exclude-file-list=istream
		-finstrument-functions-exclude-file-list=ostream
		-finstrument-functions-exclude-file-list=csdbg
	)
endif()

option(USE_TSAN "thread sanitizer" OFF)
option(USE_ASAN "address sanitizer" OFF)
option(USE_UBSAN "undefined behaviour sanitizer" OFF)
if(USE_TSAN)
	add_definitions(-fsanitize=thread)
elseif(USE_ASAN)
	add_definitions(-fsanitize=address -fno-omit-frame-pointer)
elseif(USE_UBSAN)
	add_definitions(-fsanitize=undefined)
endif()

add_subdirectory("rapidcheck")

include(set_cxx_norm.cmake)
set_cxx_norm(${CXX_NORM_CXX14})

include_directories(include)
include_directories(deps)

add_subdirectory(tests)
add_subdirectory(benchmarks)

find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(
		${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
		${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		@ONLY
	)
	add_custom_target(
		doc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif(DOXYGEN_FOUND)

#add_executable(
#	standalone_tests
#	tests/standalone.cpp
#)

install(
	DIRECTORY include/
	DESTINATION include
)

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
