#define BOOST_TEST_MODULE FutureTests
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>

#include "Future.h"
#include "Log.h"

BOOST_AUTO_TEST_CASE(create_future)
{
	auto f = Future::create();
    BOOST_CHECK(f);
    BOOST_CHECK(!f->is_ready());
    BOOST_CHECK(!f->is_done());
    BOOST_CHECK(!f->is_failed());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(mark_done)
{
	auto f = Future::create();
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
	auto f = Future::create();
    BOOST_CHECK(f);
    BOOST_CHECK(!f->is_done());
	bool called = false;
	f->on_done([&called](Future::ptr) { called = true; });
	f->done();
    BOOST_CHECK(called);
    BOOST_CHECK(f->is_done());
    BOOST_CHECK(f->is_ready());
    BOOST_CHECK(!f->is_failed());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(on_fail)
{
	auto f = Future::create();
    BOOST_CHECK(f && !f->is_failed());
	bool called = false;
	f->on_fail([&called](Future::ptr) { called = true; });
	f->fail(u8"something");
    BOOST_CHECK(called);
    BOOST_CHECK(f->is_failed());
    BOOST_CHECK(f->is_ready());
    BOOST_CHECK(!f->is_done());
    BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(then)
{
	auto f = Future::create();
	{
		bool called = false;
		auto seq = f->then([&called](Future::ptr p) -> Future::ptr {
			called = true;
			auto v = Future::create();
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
	int i = 1;
	std::vector<int> items { 1, 2, 3, 4, 5 };
	int count = items.size();
	{
	BOOST_CHECK(count == 5);
	auto f = Future::repeat([&i, &items](Future::ptr in) -> bool {
		DEBUG << "Check for items, have " << items.size() << " with front " << items.front();
		BOOST_CHECK(i == items.front());
		return items.empty();
	}, [&count, &done, &i, &items](Future::ptr in) -> Future::ptr {
		BOOST_CHECK(i == items.front());
		++i;
		items.erase(items.begin());
		BOOST_CHECK(i == items.front());
		BOOST_CHECK(count > 0);
		if(--count < 0) return Future::create()->fail("too many iterations");
		done = !count;
		DEBUG << " done = " << done;
		return in;
	});
	BOOST_CHECK(done);
	BOOST_CHECK(f->is_ready());
	BOOST_CHECK(f->is_done());
	}
}

