#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE TypedFutureTests

#include <boost/test/unit_test.hpp>

#define FUTURE_TRACE 1

#include "cps/future.h"
#include "Log.h"

BOOST_AUTO_TEST_CASE(typed_future_int)
{
	auto f = cps::typed_future<int>::create();
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

BOOST_AUTO_TEST_CASE(typed_future_string)
{
	auto f = cps::typed_future<std::string>::create();
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

