#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE FutureTests

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

/* For symbol_thingey */
#define BOOST_CHRONO_VERSION 2
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>

#define FUTURE_TRACE 0

#include <cps/future.h>
#include "Log.h"

using namespace cps;
using namespace std;

BOOST_AUTO_TEST_CASE(future_string)
{
	{ /* shared_ptr interface means this is about as far as we can get with these: */
		future<int> f { };
		BOOST_CHECK(!f.is_ready());
		BOOST_CHECK(!f.is_done());
		BOOST_CHECK(!f.is_failed());
		BOOST_CHECK(!f.is_cancelled());
	}
	auto f = future<int>::create_shared();
	auto f2 = f->done(445)->on_done([](int v) {
		BOOST_CHECK_EQUAL(v, 445);
		cout << "Have value " << v << endl;
	})->then<string>([](int v) {
		BOOST_CHECK_EQUAL(v, 445);
		cout << "Value is still " << v << endl;
		return future<string>::create_shared();
	})->on_done([](string v) {
		BOOST_CHECK_EQUAL(v, "test");
		cout << "New futue is " << v << endl;
	})->done("test")->on_done([](string v) {
		BOOST_CHECK_EQUAL(v, "test");
		cout << "Finally " << v << endl;
	})->on_done([](const string &v) {
		BOOST_CHECK_EQUAL(v, "test");
		cout << "Alternatively " << v << endl;
	});
}

typedef boost::mpl::list<
	int,
	uint8_t, int8_t,
	uint16_t, int16_t,
	uint32_t, int32_t,
	uint64_t, int64_t,
	short, int, long, long long,
	float, double
> integral_types;
BOOST_AUTO_TEST_CASE_TEMPLATE(future_integral, T, integral_types)
{
	{
		auto f = future<T>::create_shared();
		BOOST_CHECK(!f->is_ready());
		BOOST_CHECK(!f->is_done());
		BOOST_CHECK(!f->is_failed());
		BOOST_CHECK(!f->is_cancelled());
		// Pick a smallish number that fits all the numeric types
		f->done(17)->on_done([](T v) {
			BOOST_CHECK_EQUAL(v, 17);
			cout << "Have value " << v << endl;
		});
		BOOST_CHECK( f->is_ready());
		BOOST_CHECK( f->is_done());
		BOOST_CHECK(!f->is_failed());
		BOOST_CHECK(!f->is_cancelled());
	}
	{
		auto f = future<T>::create_shared();
		BOOST_CHECK(!f->is_ready());
		BOOST_CHECK(!f->is_done());
		BOOST_CHECK(!f->is_failed());
		BOOST_CHECK(!f->is_cancelled());
		// Pick a smallish number that fits all the numeric types
		f->fail("some problem")->on_fail([](const std::string &err) {
			BOOST_CHECK_EQUAL(err, "some problem");
		});
		BOOST_CHECK( f->is_ready());
		BOOST_CHECK(!f->is_done());
		BOOST_CHECK( f->is_failed());
		BOOST_CHECK(!f->is_cancelled());
	}
}

BOOST_AUTO_TEST_CASE_TEMPLATE(future_other, T, boost::mpl::list<
	bool
>)
{
	auto f = future<T>::create_shared();
	BOOST_CHECK(!f->is_ready());
	BOOST_CHECK(!f->is_done());
	BOOST_CHECK(!f->is_failed());
	BOOST_CHECK(!f->is_cancelled());
	f->done(true);
	BOOST_CHECK( f->is_ready());
	BOOST_CHECK( f->is_done());
	BOOST_CHECK(!f->is_failed());
	BOOST_CHECK(!f->is_cancelled());
}

BOOST_AUTO_TEST_CASE(future_then)
{
	{ // Simple chaining
		auto f = future<int>::create_shared();
		shared_ptr<future<string>> str1;
		shared_ptr<future<bool>> bool1;
		auto f2 = f->then<string>([&str1](int v) {
			BOOST_CHECK_EQUAL(v, 23);
			str1 = future<string>::create_shared();
			return str1;
		})->then<bool>([&bool1](string v) {
			BOOST_CHECK_EQUAL(v, "testing");
			bool1 = future<bool>::create_shared();
			return bool1;
		})->on_ready([&str1](future<bool> &in) {
			BOOST_CHECK_EQUAL(str1->value(), "testing");
			BOOST_CHECK(in.value());
		});
		BOOST_CHECK(!str1);
		BOOST_CHECK(!bool1);
		f->done(23);
		BOOST_CHECK( str1);
		BOOST_CHECK(!bool1);
		str1->done("testing");
		BOOST_CHECK_EQUAL( str1->value(), "testing");
		BOOST_CHECK( bool1);
		bool1->done(true);
		BOOST_CHECK(bool1->value());
	}
	{ // Error propagation
		auto f = future<int>::create_shared();
		auto f2 = f->then<int>([](int v) {
			auto f = future<int>::create_shared();
			f->fail("error in first ->then call");
			return f;
		})->then<int>([](int v) {
			BOOST_CHECK(false);
			return future<int>::create_shared();
		});
		BOOST_CHECK(!f->is_ready());
		BOOST_CHECK(!f2->is_ready());
		f->done(32);
		BOOST_CHECK( f->is_done());
		BOOST_CHECK( f2->is_failed());
		BOOST_CHECK_EQUAL( f2->failure_reason(), "error in first ->then call");
	}
	{ // Exception propagation
		auto f = future<int>::create_shared();
		auto f2 = f->then<int>([](int v) {
			auto f = future<int>::create_shared();
			throw std::runtime_error("bail out");
			return f;
		})->then<int>([](int v) {
			BOOST_CHECK(false);
			return future<int>::create_shared();
		});
		BOOST_CHECK(!f->is_ready());
		BOOST_CHECK(!f2->is_ready());
		f->done(32);
		BOOST_CHECK( f->is_done());
		BOOST_CHECK( f2->is_failed());
		BOOST_CHECK_EQUAL( f2->failure_reason(), "bail out");
	}
	{ // two-arg ->then
		auto f = future<int>::create_shared();
		auto f2 = f->then<int>([](int v) {
			auto f = future<int>::create_shared();
			throw std::runtime_error("first stage fails");
			return f;
		})->then<int>([](int v) {
			BOOST_CHECK(false);
			return future<int>::create_shared();
		}, [](const std::string &err) {
			BOOST_CHECK_EQUAL(err, "first stage fails");
			return future<int>::create_shared()->done(99);
		});
		BOOST_CHECK(!f->is_ready());
		BOOST_CHECK(!f2->is_ready());
		f->done(32);
		BOOST_CHECK( f->is_done());
		BOOST_CHECK( f2->is_done());
		BOOST_CHECK_EQUAL( f2->value(), 99);
	}
}

BOOST_AUTO_TEST_CASE(composition_needs_all)
{
	{
		auto f1 = future<int>::create_shared();
		auto f2 = future<string>::create_shared();

		auto composed = needs_all(
			f1,
			f2
		);
		composed->on_ready([](future<int> &f) {
			cout << "ready" << endl;
		});
		BOOST_CHECK(!composed->is_ready());
		f1->done(432);
		BOOST_CHECK(!composed->is_ready());
		f2->done("test");
		BOOST_CHECK( composed->is_ready());
		BOOST_CHECK( composed->is_done());
	}
	{
		auto f1 = future<int>::create_shared();
		auto f2 = future<string>::create_shared();

		auto composed = needs_all(
			f1,
			f2
		);
		composed->on_ready([](future<int> &f) {
			BOOST_CHECK(f.is_ready());
		});
		BOOST_CHECK(!composed->is_ready());
		f1->fail("aiee");
		BOOST_CHECK( composed->is_ready());
		BOOST_CHECK( composed->is_failed());
	}
	{
		std::vector<std::shared_ptr<future<int>>> pending {
			future<int>::create_shared(),
			future<int>::create_shared(),
			future<int>::create_shared(),
			future<int>::create_shared(),
			future<int>::create_shared(),
			future<int>::create_shared()
		};

		auto composed = needs_all(
			pending
		);
		composed->on_ready([](future<int> &f) {
			BOOST_CHECK(f.is_ready());
		});
		BOOST_CHECK(!composed->is_ready());
		int i = 0;
		for(auto &it : pending) {
			it->done(++i);
			if(i == pending.size()) {
				BOOST_CHECK( composed->is_done());
			} else {
				BOOST_CHECK(!composed->is_ready());
			}
		}
		BOOST_CHECK( composed->is_ready());
	}
}

BOOST_AUTO_TEST_CASE(composition_needs_any)
{
	{
		auto f1 = future<int>::create_shared();
		auto f2 = future<string>::create_shared();

		auto composed = needs_any(
			f1,
			f2
		);
		composed->on_ready([](future<int> &f) {
			cout << "ready" << endl;
		});
		BOOST_CHECK(!composed->is_ready());
		f1->done(432);
		BOOST_CHECK( composed->is_ready());
		BOOST_CHECK( composed->is_done());
		f2->done("test");
		BOOST_CHECK( composed->is_ready());
		BOOST_CHECK( composed->is_done());
	}
	{
		auto f1 = future<int>::create_shared();
		auto f2 = future<string>::create_shared();

		auto composed = needs_all(
			f1,
			f2
		);
		composed->on_ready([](future<int> &f) {
			BOOST_CHECK(f.is_ready());
		});
		BOOST_CHECK(!composed->is_ready());
		f1->fail("aiee");
		BOOST_CHECK( composed->is_ready());
		BOOST_CHECK( composed->is_failed());
	}
	{
		std::vector<std::shared_ptr<future<int>>> pending {
			future<int>::create_shared(),
			future<int>::create_shared(),
			future<int>::create_shared(),
			future<int>::create_shared(),
			future<int>::create_shared(),
			future<int>::create_shared()
		};

		auto composed = needs_all(
			pending
		);
		composed->on_ready([](future<int> &f) {
			BOOST_CHECK(f.is_ready());
		});
		BOOST_CHECK(!composed->is_ready());
		int i = 0;
		for(auto &it : pending) {
			it->done(++i);
			if(i == pending.size()) {
				BOOST_CHECK( composed->is_done());
			} else {
				BOOST_CHECK(!composed->is_ready());
			}
		}
		BOOST_CHECK( composed->is_ready());
	}
}

