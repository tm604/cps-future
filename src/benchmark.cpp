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
		auto f = leaf_future<std::string>::create();
		auto expected = "happy";
		auto seq = f->then([expected](const std::string &v) {
			return leaf_future<std::string>::create()->done("very happy");
		});
		f->done(expected);
		// BOOST_CHECK(seq->as<std::string>()->get() == "very happy");
	}
	auto elapsed = boost::chrono::high_resolution_clock::now() - start;
	std::cout << "Average iteration: " << boost::chrono::symbol_format << (elapsed / (float)count) << std::endl;
	return 0;
}
