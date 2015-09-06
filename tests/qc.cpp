#define FUTURE_TRACE 0
#include <cps/future.h>

#include "catch.hpp"
#include "rapidcheck.h"

using namespace cps;
using namespace std;

#define ok CHECK

SCENARIO("future as a shared pointer", "[shared]") {
	GIVEN("an empty future") {
		auto f = make_future<int>("some future");
		ok(!f->is_ready());
		ok(!f->is_done());
		ok(!f->is_failed());
		ok(!f->is_cancelled());
		ok(f->current_state() == "pending");
		ok(f->label() == "some future");
		WHEN("marked as done") {
			f->done(123);
			THEN("state is correct") {
				ok( f->is_ready());
				ok( f->is_done());
				ok(!f->is_failed());
				ok(!f->is_cancelled());
				ok(f->current_state() == "done");
			}
			AND_THEN("elapsed is nonzero") {
				ok(f->elapsed().count() > 0);
			}
			AND_THEN("description looks about right") {
				ok(string::npos != f->describe().find("some future (done), "));
			}
			auto weak = std::weak_ptr<cps::future<int>>(f);
			f.reset();
			AND_THEN("shared_ptr goes away correctly") {
				ok(weak.expired());
			}
		}
		WHEN("marked as failed") {
			f->fail("...");
			THEN("state is correct") {
				ok( f->is_ready());
				ok(!f->is_done());
				ok( f->is_failed());
				ok(!f->is_cancelled());
				ok(f->current_state() == "failed");
			}
			AND_THEN("elapsed is nonzero") {
				ok(f->elapsed().count() > 0);
			}
			AND_THEN("description looks about right") {
				ok(string::npos != f->describe().find("some future (failed), "));
			}
			auto weak = std::weak_ptr<cps::future<int>>(f);
			f.reset();
			AND_THEN("shared_ptr goes away correctly") {
				ok(weak.expired());
			}
		}
		WHEN("marked as cancelled") {
			f->cancel();
			THEN("state is correct") {
				ok( f->is_ready());
				ok(!f->is_done());
				ok(!f->is_failed());
				ok( f->is_cancelled());
				ok(f->current_state() == "cancelled");
			}
			AND_THEN("elapsed is nonzero") {
				ok(f->elapsed().count() > 0);
			}
			AND_THEN("description looks about right") {
				ok(string::npos != f->describe().find("some future (cancelled), "));
			}
			auto weak = std::weak_ptr<cps::future<int>>(f);
			f.reset();
			AND_THEN("shared_ptr goes away correctly") {
				ok(weak.expired());
			}
		}
	}
}
