include (CTest)

add_executable(
	future_tests
	main.cpp
	is_string.cpp
	future.cpp
	utils.cpp
)

add_executable(
	qc_tests
	main.cpp
	qc.cpp
)
target_link_libraries(
	qc_tests
	rapidcheck
)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC future_tests "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(future_tests "${CMAKE_THREAD_LIBS_INIT}")
endif()

add_test (future_tests future_tests -r junit -o future_tests.xml)
add_test (qc_tests qc_tests -r junit -o qc_tests.xml)

