#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE LeafFutureTests

#include <boost/test/unit_test.hpp>

/* For symbol_thingey */
#define BOOST_CHRONO_VERSION 2
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>

#define FUTURE_TRACE 0

#include <cps/future.h>
#include "Log.h"

using namespace cps;

BOOST_AUTO_TEST_CASE(leaf_future_string)
{
	auto f = leaf_future<std::string>::create();
    BOOST_CHECK(f);
    BOOST_CHECK(!f->is_ready());
    BOOST_CHECK(!f->is_done());
    BOOST_CHECK(!f->is_failed());
    BOOST_CHECK(!f->is_cancelled());
	f->done("test");
    BOOST_CHECK( f->is_ready());
    BOOST_CHECK( f->is_done());
    BOOST_CHECK(!f->is_failed());
    BOOST_CHECK(!f->is_cancelled());
    BOOST_CHECK(f->get() == "test");
}

BOOST_AUTO_TEST_CASE(leaf_future_sequencing)
{
	/* Don't really care about the type, as long as it hits the default template (e.g. not
	 * a specialised one like int)
	 */
	auto f = leaf_future<std::string>::create();
	auto expected = "happy";
	auto seq = f->then([expected](const std::string &v) {
		BOOST_CHECK(v == expected);
		return leaf_future<std::string>::create()->done("very happy");
	});
	f->done(expected);
	BOOST_CHECK(seq->is_done());
	// BOOST_CHECK(seq->as<std::string>()->get() == "very happy");
}

/* int is special-cased to avoid references */
BOOST_AUTO_TEST_CASE(leaf_future_int)
{
	{
		auto f = leaf_future<int>::create();
		BOOST_CHECK(f);
		BOOST_CHECK(!f->is_ready());
		BOOST_CHECK(!f->is_done());
		BOOST_CHECK(!f->is_failed());
		BOOST_CHECK(!f->is_cancelled());
		f->done(123);
		BOOST_CHECK( f->is_ready());
		BOOST_CHECK( f->is_done());
		BOOST_CHECK(!f->is_failed());
		BOOST_CHECK(!f->is_cancelled());
		BOOST_CHECK(f->get() == 123);
	}
	{
		auto f = leaf_future<int>::create();
		f->cancel();
		BOOST_CHECK( f->is_ready());
		BOOST_CHECK(!f->is_done());
		BOOST_CHECK(!f->is_failed());
		BOOST_CHECK( f->is_cancelled());
	}
	{
		auto f = leaf_future<int>::create();
		f->fail("something");
		DEBUG << "Elapsed: " << boost::chrono::symbol_format << f->elapsed();
		BOOST_CHECK( f->is_ready());
		BOOST_CHECK(!f->is_done());
		BOOST_CHECK( f->is_failed());
		BOOST_CHECK(!f->is_cancelled());
	}
}

