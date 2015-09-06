#define FUTURE_TRACE 0
#include <cps/future.h>

#include "catch.hpp"

using namespace cps;
using namespace std;

SCENARIO("future as a shared pointer", "[shared]") {
	GIVEN("an empty future") {
		auto f = future<int>::create_shared("some future");
		CHECK(!f->is_ready());
		CHECK(!f->is_done());
		CHECK(!f->is_failed());
		CHECK(!f->is_cancelled());
		CHECK(f->current_state() == "pending");
		CHECK(f->label() == "some future");
		WHEN("marked as done") {
			f->done(123);
			THEN("state is correct") {
				CHECK( f->is_ready());
				CHECK( f->is_done());
				CHECK(!f->is_failed());
				CHECK(!f->is_cancelled());
				CHECK(f->current_state() == "done");
			}
			AND_THEN("elapsed is nonzero") {
				CHECK(f->elapsed().count() > 0);
			}
			AND_THEN("description looks about right") {
				CHECK(string::npos != f->describe().find("some future (done), "));
			}
			auto weak = std::weak_ptr<cps::future<int>>(f);
			f.reset();
			AND_THEN("shared_ptr goes away correctly") {
				CHECK(weak.expired());
			}
		}
		WHEN("marked as failed") {
			f->fail("...");
			THEN("state is correct") {
				CHECK( f->is_ready());
				CHECK(!f->is_done());
				CHECK( f->is_failed());
				CHECK(!f->is_cancelled());
				CHECK(f->current_state() == "failed");
			}
			AND_THEN("elapsed is nonzero") {
				CHECK(f->elapsed().count() > 0);
			}
			AND_THEN("description looks about right") {
				CHECK(string::npos != f->describe().find("some future (failed), "));
			}
			auto weak = std::weak_ptr<cps::future<int>>(f);
			f.reset();
			AND_THEN("shared_ptr goes away correctly") {
				CHECK(weak.expired());
			}
		}
		WHEN("marked as cancelled") {
			f->cancel();
			THEN("state is correct") {
				CHECK( f->is_ready());
				CHECK(!f->is_done());
				CHECK(!f->is_failed());
				CHECK( f->is_cancelled());
				CHECK(f->current_state() == "cancelled");
			}
			AND_THEN("elapsed is nonzero") {
				CHECK(f->elapsed().count() > 0);
			}
			AND_THEN("description looks about right") {
				CHECK(string::npos != f->describe().find("some future (cancelled), "));
			}
			auto weak = std::weak_ptr<cps::future<int>>(f);
			f.reset();
			AND_THEN("shared_ptr goes away correctly") {
				CHECK(weak.expired());
			}
		}
	}
}

SCENARIO("error category is valid") {
	REQUIRE(&cps::future_category != nullptr);
	REQUIRE(cps::future_category.name() == std::string { "cps::future" });
}

SCENARIO("failed future handling", "[shared]") {
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
				CHECK(reason == "some reason");
			}
		}
		WHEN("we call ->value") {
			THEN("we get an exception") {
				REQUIRE_THROWS(f->value());
			}
		}
		WHEN("we call ->value with an error_code") {
			std::error_code ec;
			f->value(ec);
			THEN("error code is true") {
				REQUIRE(ec);
			}
			AND_THEN("it's failed") {
				CHECK(ec == cps::future_errc::is_failed);
			}
			AND_THEN("it's in the right category") {
				CHECK(ec == cps::future_errc::is_failed);
				CHECK(ec.category() == cps::future_category);
				CHECK(std::string { ec.category().name() } == std::string { "cps::future" });
			}
		}
	}
}

SCENARIO("successful future handling", "[shared]") {
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
				CHECK(f->value() == "all good");
			}
		}
	}
}

SCENARIO("cancelled future handling", "[shared]") {
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
		WHEN("we call ->value with an error_code") {
			std::error_code ec;
			f->value(ec);
			THEN("error code is true") {
				REQUIRE(ec);
			}
			AND_THEN("it's cancelled") {
				CHECK(ec == cps::future_errc::is_cancelled);
			}
			AND_THEN("it's in the right category") {
				CHECK(ec == cps::future_errc::is_cancelled);
				CHECK(ec.category() == cps::future_category);
				CHECK(ec.category().name() == std::string { "cps::future" });
			}
		}
	}
}

