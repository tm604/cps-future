/* For symbol_thingey */
#define BOOST_CHRONO_VERSION 2
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>

#define FUTURE_TRACE 0

#include <cps/future.h>
#include <iostream>

using namespace cps;

int
main(void)
{
	auto start = boost::chrono::high_resolution_clock::now();
	const int count = 100000;
	for(int i = 0; i < count; ++i) {
		auto f = future<std::string>::create_shared();
		auto expected = "happy";
		f->on_done([expected](const std::string &v) {
			// return future<std::string>::create_shared()->done("very happy");
		})->done(expected);
		// BOOST_CHECK(seq->as<std::string>()->get() == "very happy");
	}
	auto elapsed = boost::chrono::high_resolution_clock::now() - start;
	std::cout << "Average iteration: " << boost::chrono::symbol_format << (elapsed / (float)count) << std::endl;
	return 0;
}
