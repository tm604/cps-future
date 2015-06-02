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
	{
		auto f = leaf_future<std::string>::create();
		f->on_done([f](const std::string &str) {
			BOOST_CHECK(str == f->get());
			BOOST_CHECK(str == "test");
		});
		f->done("test");
	}
	{
		auto f = leaf_future<std::shared_ptr<std::string>>::create();
		auto expected = std::make_shared<std::string>("some test string");
		f->on_done([f](std::shared_ptr<std::string> str) {
			BOOST_CHECK(str == f->get());
			BOOST_CHECK(*str == "some test string");
		});
		f->done(expected);
	}
}

BOOST_AUTO_TEST_CASE(leaf_future_bool)
{
	{
		auto f = leaf_future<bool>::create();
		f->on_done([f](bool v) {
			BOOST_CHECK(v);
		});
		f->done(true);
		BOOST_CHECK(f->get());
	}
	{
		auto f = leaf_future<bool>::create();
		f->on_done([f](bool v) {
			BOOST_CHECK(!v);
		});
		f->done(false);
		BOOST_CHECK(!f->get());
	}
}

BOOST_AUTO_TEST_CASE(leaf_future_sequencing)
{
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
		BOOST_CHECK(seq->type() == base_future::sequence);

		f->done(expected);
		BOOST_CHECK(seq->is_done());
		BOOST_CHECK(seq->as_leaf<std::string>()->get() == "very happy");
	}
	{
		auto concat = leaf_future<std::string>::create("first string")->done(
			"some"
		)->leaf_then<std::string>([](const std::string &v) {
			return leaf_future<std::string>::create("first concat")->done(
				v + "times"
			);
		})->leaf_then<std::string>([](const std::string &v) {
			return leaf_future<std::string>::create("second concat")->done(
				v + " things"
			);
		})->leaf_then<std::string>([](const std::string &v) {
			return leaf_future<std::string>::create("third concat")->done(
				v + " just"
			);
		})->leaf_then<std::string>([](const std::string &v) {
			return leaf_future<std::string>::create("fourth concat")->done(
				v + " work"
			);
		})->get();
		BOOST_CHECK(concat == "sometimes things just work");
	}
	{
		DEBUG << "new test";
		auto concat = leaf_future<int>::create("number 23")->done(
			23
		)->leaf_then<int>([](const int v) {
			DEBUG << "first a number: " << v;
			return leaf_future<int>::create("double 23")->done(
				v * 2
			);
		})->leaf_then<std::string>([](const int v) {
			DEBUG << "another number: " << v;
			return leaf_future<std::string>::create("stringified 46")->done(
				"had " + std::to_string(v) + " things"
			);
		})->leaf_then<std::string>([](const std::string &v) {
			DEBUG << "and a thing: " << v;
			return leaf_future<std::string>::create("concat 46")->done(
				"we " + v
			);
		})->leaf_then<std::pair<std::string, size_t>>([](const std::string &v) {
			DEBUG << "with a whatever: " << v;
			return leaf_future<std::pair<std::string, size_t>>::create("a tuple")->done(
				std::pair<std::string, size_t>(v, v.size())
			);
		})->get();
		BOOST_CHECK(concat.first == "we had 46 things");
		DEBUG << "Actual value: " << concat.second;
		BOOST_CHECK(concat.second == 16);
	}
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

