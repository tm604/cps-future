#define BOOST_TEST_MAIN
#if !defined( WIN32 )
    #define BOOST_TEST_DYN_LINK
#endif
#define BOOST_TEST_MODULE NeoTests

#include <boost/test/unit_test.hpp>

#include <vector>
#include "Log.h"

class base {
public:
	/*
	class sequenced {
	public:
		sequenced():pending_(2) { }

	private:
		int pending_;
	};

	template<typename ... Types>
	static
	base needs_all(base &first, Types ... rest)
	{
		auto remainder = needs_all(rest...);
		std::unique_ptr<sequenced>();
		remainder.on_done(
		first
			.on_done([]() { })
			.on_fail()
			.on_cancel();
	}

	static
	base needs_all()
	{
		return base();
	}
	*/
};

template <typename T>
class with_value:public base {
public:
	T &get() const { return value_; }

	void done(T &v) { value_ = v; }

private:
	T value_;
};

template<>
class with_value<int>:public base {
public:
	int get() const { return value_; }

	void done(int v) { value_ = v; }

private:
	int value_;
};

BOOST_AUTO_TEST_CASE(thing)
{
	auto wv = with_value<int>();
	wv.done(432);

	// base::needs_all(wv);
    BOOST_CHECK(true);
}
