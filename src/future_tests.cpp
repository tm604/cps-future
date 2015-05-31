#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE TypedFutureTests

#include <boost/test/unit_test.hpp>

#define FUTURE_TRACE 1

#include "cps/future.h"
#include "Log.h"

using namespace cps;

BOOST_AUTO_TEST_CASE(create_future)
{
	auto f = future::create();
    BOOST_CHECK(f);
    BOOST_CHECK(!f->is_ready());
    BOOST_CHECK(!f->is_done());
    BOOST_CHECK(!f->is_failed());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(mark_done)
{
	auto f = future::create();
    BOOST_CHECK(f);
    BOOST_CHECK(!f->is_done());
	f->done();
    BOOST_CHECK(f->is_done());
    BOOST_CHECK(f->is_ready());
    BOOST_CHECK(!f->is_failed());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(on_done)
{
	auto f = future::create();
    BOOST_CHECK(f);
    BOOST_CHECK(!f->is_done());
	bool called = false;
	f->on_done([&called]() { called = true; });
	f->done();
    BOOST_CHECK(called);
    BOOST_CHECK(f->is_done());
    BOOST_CHECK(f->is_ready());
    BOOST_CHECK(!f->is_failed());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(on_fail)
{
	auto f = future::create();
    BOOST_CHECK(f && !f->is_failed());
	bool called = false;
	f->on_fail([&called](future::exception &) { called = true; });
	f->fail(u8"something");
    BOOST_CHECK(called);
    BOOST_CHECK(f->is_failed());
    BOOST_CHECK(f->is_ready());
    BOOST_CHECK(!f->is_done());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(then)
{
	auto f = future::create();
	{
		bool called = false;
		auto seq = f->then([&called]() -> future::ptr {
			called = true;
			auto v = future::create();
			v->on_done([]() { std::cout << "v done\n"; });
			return v;
		});
		BOOST_CHECK(!called);
		f->done();
		BOOST_CHECK(called);
		BOOST_CHECK(!seq->is_ready());
		seq->done();
		BOOST_CHECK(seq->is_ready());
	}
}

BOOST_AUTO_TEST_CASE(repeat)
{
	bool done = false;
	int i = 0;
	std::vector<int> items { 1, 2, 3, 4, 5 };
	int count = items.size();
	{
		BOOST_CHECK(count == 5);
		auto f = future::repeat([&i, &items, &count](future::ptr in) -> bool {
			DEBUG << "Check for items, have " << items.size() << " with front " << items.front();
			BOOST_CHECK(count == items.size());
			return items.empty();
		}, [&count, &done, &i, &items](future::ptr in) -> future::ptr {
			DEBUG << "Iterate, have " << items.size() << " with front " << items.front() << " and i = " << i;
			BOOST_CHECK(++i == items.front());
			items.erase(items.begin());
			if(--count < 0) return future::create()->fail("too many iterations");
			done = !count;
			DEBUG << " done = " << done;
			return in;
		});
		BOOST_CHECK(done);
		BOOST_CHECK(f->is_ready());
		BOOST_CHECK(f->is_done());
	}
}

