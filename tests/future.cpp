#include <cps/future.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <boost/mpl/list.hpp>

/* For symbol_thingey */
#define BOOST_CHRONO_VERSION 2
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/chrono_io.hpp>

#define FUTURE_TRACE 0

// #include "Log.h"

using namespace cps;
using namespace std;

#define ok CHECK

SCENARIO("future as an object", "[string]") {
	GIVEN("an empty future") {
		future<int> f { };
		ok(!f.is_ready());
		ok(!f.is_done());
		ok(!f.is_failed());
		ok(!f.is_cancelled());
	}
}

SCENARIO("future as a shared pointer", "[string][shared]") {
	GIVEN("an empty future") {
		auto f = future<int>::create_shared();
		ok(!f->is_ready());
		ok(!f->is_done());
		ok(!f->is_failed());
		ok(!f->is_cancelled());
		WHEN("marked as done") {
			f->done(123);
			THEN("state is correct") {
				ok( f->is_ready());
				ok( f->is_done());
				ok(!f->is_failed());
				ok(!f->is_cancelled());
			}
		}
		WHEN("marked as failed") {
			f->fail("...");
			THEN("state is correct") {
				ok( f->is_ready());
				ok(!f->is_done());
				ok( f->is_failed());
				ok(!f->is_cancelled());
			}
		}
		WHEN("marked as cancelled") {
			f->cancel();
			THEN("state is correct") {
				ok( f->is_ready());
				ok(!f->is_done());
				ok(!f->is_failed());
				ok( f->is_cancelled());
			}
		}
	}
}

SCENARIO("failed future handling", "[string][shared]") {
	GIVEN("a failed future") {
		auto f = future<int>::create_shared();
		f->fail("some reason");
		REQUIRE( f->is_ready());
		REQUIRE(!f->is_done());
		REQUIRE( f->is_failed());
		REQUIRE(!f->is_cancelled());
		WHEN("we call ->failure_reason") {
			auto reason = f->failure_reason();
			THEN("we get the failure reason") {
				ok(reason == "some reason");
			}
		}
		WHEN("we call ->value") {
			THEN("we get an exception") {
				REQUIRE_THROWS(f->value());
			}
		}
	}
}

SCENARIO("successful future handling", "[string][shared]") {
	GIVEN("a completed future") {
		auto f = future<string>::create_shared();
		f->done("all good");
		REQUIRE( f->is_ready());
		REQUIRE( f->is_done());
		REQUIRE(!f->is_failed());
		REQUIRE(!f->is_cancelled());
		WHEN("we call ->failure_reason") {
			THEN("we get an exception") {
				REQUIRE_THROWS(f->failure_reason());
			}
		}
		WHEN("we call ->value") {
			THEN("we get the original value") {
				ok(f->value() == "all good");
			}
		}
	}
}

SCENARIO("cancelled future handling", "[string][shared]") {
	GIVEN("a cancelled future") {
		auto f = future<string>::create_shared();
		f->cancel();
		REQUIRE( f->is_ready());
		REQUIRE(!f->is_done());
		REQUIRE(!f->is_failed());
		REQUIRE( f->is_cancelled());
		WHEN("we call ->failure_reason") {
			THEN("we get an exception") {
				REQUIRE_THROWS(f->failure_reason());
			}
		}
		WHEN("we call ->value") {
			THEN("we get an exception") {
				REQUIRE_THROWS(f->value());
			}
		}
	}
}

SCENARIO("needs_all", "[composed][string][shared]") {
	GIVEN("an empty list of futures") {
		auto na = needs_all();
		WHEN("we check status") {
			THEN("it reports as complete") {
				ok(na->is_done());
			}
		}
	}
	GIVEN("some pending futures") {
		auto f1 = future<int>::create_shared();
		auto f2 = future<int>::create_shared();
		auto na = needs_all(f1, f2);
		ok(!na->is_ready());
		ok(!na->is_done());
		ok(!na->is_failed());
		ok(!na->is_cancelled());
		WHEN("f1 marked as done") {
			f1->done(123);
			THEN("needs_all is still pending") {
				ok(!na->is_ready());
			}
		}
		WHEN("f2 marked as done") {
			f2->done(123);
			THEN("needs_all is still pending") {
				ok(!na->is_ready());
			}
		}
		WHEN("all dependents marked as done") {
			f1->done(34);
			f2->done(123);
			THEN("needs_all is complete") {
				ok(na->is_done());
			}
		}
		WHEN("a dependent fails") {
			f1->fail("...");
			THEN("needs_all is now failed") {
				ok(na->is_failed());
			}
		}
		WHEN("a dependent is cancelled") {
			f1->cancel();
			THEN("needs_all is now failed") {
				ok(na->is_failed());
			}
		}
	}
}
