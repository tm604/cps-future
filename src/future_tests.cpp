#define BOOST_TEST_MODULE FutureTests
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#define FUTURE_TRACE 1

#include "cps/future.h"
#include "Log.h"

BOOST_AUTO_TEST_CASE(create_future)
{
	auto f = cps::future::create();
    BOOST_CHECK(f);
    BOOST_CHECK(!f->is_ready());
    BOOST_CHECK(!f->is_done());
    BOOST_CHECK(!f->is_failed());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(mark_done)
{
	auto f = cps::future::create();
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
	auto f = cps::future::create();
    BOOST_CHECK(f);
    BOOST_CHECK(!f->is_done());
	bool called = false;
	f->on_done([&called](cps::future::ptr) { called = true; });
	f->done();
    BOOST_CHECK(called);
    BOOST_CHECK(f->is_done());
    BOOST_CHECK(f->is_ready());
    BOOST_CHECK(!f->is_failed());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(on_fail)
{
	auto f = cps::future::create();
    BOOST_CHECK(f && !f->is_failed());
	bool called = false;
	f->on_fail([&called](cps::future::ptr) { called = true; });
	f->fail(u8"something");
    BOOST_CHECK(called);
    BOOST_CHECK(f->is_failed());
    BOOST_CHECK(f->is_ready());
    BOOST_CHECK(!f->is_done());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(then)
{
	auto f = cps::future::create();
	{
		bool called = false;
		auto seq = f->then([&called](cps::future::ptr p) -> cps::future::ptr {
			called = true;
			auto v = cps::future::create();
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
		auto f = cps::future::repeat([&i, &items, &count](cps::future::ptr in) -> bool {
			DEBUG << "Check for items, have " << items.size() << " with front " << items.front();
			BOOST_CHECK(count == items.size());
			return items.empty();
		}, [&count, &done, &i, &items](cps::future::ptr in) -> cps::future::ptr {
			DEBUG << "Iterate, have " << items.size() << " with front " << items.front() << " and i = " << i;
			BOOST_CHECK(++i == items.front());
			items.erase(items.begin());
			if(--count < 0) return cps::future::create()->fail("too many iterations");
			done = !count;
			DEBUG << " done = " << done;
			return in;
		});
		BOOST_CHECK(done);
		BOOST_CHECK(f->is_ready());
		BOOST_CHECK(f->is_done());
	}
}

