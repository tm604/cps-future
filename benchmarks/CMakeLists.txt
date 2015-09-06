add_executable(
	benchmark
	benchmark.cpp
)

if(THREADS_HAVE_PTHREAD_ARG)
	target_compile_options(PUBLIC benchmark "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
	target_link_libraries(benchmark "${CMAKE_THREAD_LIBS_INIT}")
endif()


