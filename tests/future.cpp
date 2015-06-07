#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE Future2Tests

#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/test_case_template.hpp>

/* For symbol_thingey */
#define BOOST_CHRONO_VERSION 2
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>

#define FUTURE_TRACE 0

#include <future2.h>
#include "Log.h"

using namespace cps;
using namespace std;

BOOST_AUTO_TEST_CASE(leaf_future_string)
{
	future<int> f { };
	BOOST_CHECK(!f.is_ready());
	BOOST_CHECK(!f.is_done());
	BOOST_CHECK(!f.is_failed());
	BOOST_CHECK(!f.is_cancelled());
	auto f2 = move(f.done(445).on_done([](int v) {
		BOOST_CHECK_EQUAL(v, 445);
		cout << "Have value " << v << endl;
	}).then<string>([](int v) {
		BOOST_CHECK_EQUAL(v, 445);
		cout << "Value is still " << v << endl;
		return future<string> { };
	}).on_done([](string v) {
		BOOST_CHECK_EQUAL(v, "test");
		cout << "New futue is " << v << endl;
	}).done("test").on_done([](string v) {
		BOOST_CHECK_EQUAL(v, "test");
		cout << "Finally " << v << endl;
	}).on_done([](const string &v) {
		BOOST_CHECK_EQUAL(v, "test");
		cout << "Alternatively " << v << endl;
	}));
}

typedef boost::mpl::list<
	int,
	uint8_t, int8_t,
	uint16_t, int16_t,
	uint32_t, int32_t,
	uint64_t, int64_t,
	short, int, long, long long
> leaf_integral_types;
BOOST_AUTO_TEST_CASE_TEMPLATE(leaf_future_integral, T, leaf_integral_types)
{
	future<T> f { };
	BOOST_CHECK(!f.is_ready());
	BOOST_CHECK(!f.is_done());
	BOOST_CHECK(!f.is_failed());
	BOOST_CHECK(!f.is_cancelled());
	auto f2 = move(f.done(17).on_done([](T v) {
		BOOST_CHECK_EQUAL(v, 17);
		cout << "Have value " << v << endl;
	}));
}

BOOST_AUTO_TEST_CASE_TEMPLATE(leaf_future_other, T, boost::mpl::list<
	bool
>)
{
	future<T> f { };
	BOOST_CHECK(!f.is_ready());
	BOOST_CHECK(!f.is_done());
	BOOST_CHECK(!f.is_failed());
	BOOST_CHECK(!f.is_cancelled());
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

