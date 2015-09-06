#define FUTURE_TRACE 0
#include <cps/future.h>

#include "catch.hpp"

using namespace cps;
using namespace std;

SCENARIO("generator handling") {
	GIVEN("an int generator from a vector") {
		std::vector<int> items { 1, 2, 3, 4, 5 };
		auto gen = cps::foreach<int>(items);
		WHEN("we call the generator") {
			THEN("we get the expected values") {
				for(int i = 0; i < 5; ++i) {
					std::error_code ec;
					auto v = gen.next(ec);
					REQUIRE(!ec);
					CHECK(v == items[i]);
				}
				AND_THEN("the next item returns false") {
					std::error_code ec;
					auto v = gen.next(ec);
					REQUIRE(ec);
				}
			}
		}
	}
	GIVEN("a coderef generator") {
		bool called = false;
		auto gen = cps::generator<int>([&called](std::error_code &) {
			REQUIRE(!called);
			called = true;
			return 42;
		});
		WHEN("we create the generator") {
			THEN("we have not yet called the code") {
				REQUIRE(!called);
			}
		}
		WHEN("we call the generator") {
			std::error_code ec;
			auto v = gen.next(ec);
			THEN("our code was called") {
				REQUIRE(called);
			}
			THEN("the value was correct") {
				REQUIRE(v == 42);
			}
		}
	}
}

SCENARIO("needs_all", "[composed][shared]") {
	GIVEN("an empty list of futures") {
		auto na = needs_all();
		WHEN("we check status") {
			THEN("it reports as complete") {
				CHECK(na->is_done());
			}
		}
	}
	GIVEN("some pending futures") {
		auto f1 = future<int>::create_shared();
		auto f2 = future<int>::create_shared();
		auto na = needs_all(f1, f2);
		CHECK(!na->is_ready());
		CHECK(!na->is_done());
		CHECK(!na->is_failed());
		CHECK(!na->is_cancelled());
		WHEN("f1 marked as done") {
			f1->done(123);
			THEN("needs_all is still pending") {
				CHECK(!na->is_ready());
			}
		}
		WHEN("f2 marked as done") {
			f2->done(123);
			THEN("needs_all is still pending") {
				CHECK(!na->is_ready());
			}
		}
		WHEN("all dependents marked as done") {
			f1->done(34);
			f2->done(123);
			THEN("needs_all is complete") {
				CHECK(na->is_done());
			}
		}
		WHEN("a dependent fails") {
			f1->fail("...");
			THEN("needs_all is now failed") {
				CHECK(na->is_failed());
			}
		}
		WHEN("a dependent is cancelled") {
			f1->cancel();
			THEN("needs_all is now failed") {
				CHECK(na->is_failed());
			}
		}
	}
}

