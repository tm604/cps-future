#include <cps/future.h>

template<typename T> class TD;

void
test_case()
{	using namespace cps;

	{
		auto f = resolved_future(123);
		assert(f->value() == 123);
	}

	auto f = resolved_future(std::string("test"));
	assert(f->value() == "test");

	auto f2 = f->then([](const std::string &) {
		return resolved_future(75);
	})->then([](const int &) {
		return future<size_t>::create_shared()->done(18141);
	})->then([](const size_t &) {
		return resolved_future(26814);
	})->then([](const int &) {
		return resolved_future(std::string("some test"));
	});
	assert(f2->is_ready());
	assert(f2->value() == "some test");
}

int
main()
{
	for(int i = 0; i < 100; ++i)
		test_case();
}

