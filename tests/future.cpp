#define FUTURE_TRACE 0
#include <cps/future.h>

#include "catch.hpp"

using namespace cps;
using namespace std;

#define ok CHECK

SCENARIO("future as a shared pointer", "[shared]") {
	GIVEN("an empty future") {
		auto f = future<int>::create_shared("some future");
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
				ok(reason == "some reason");
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
				ok(f->value() == "all good");
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

SCENARIO("needs_all", "[composed][shared]") {
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

SCENARIO("we can chain futures via ->then", "[composed][shared]") {
	GIVEN("a two-item ->then chain") {
		auto f1 = cps::make_future<string>();
		auto f2 = cps::make_future<string>();
		bool called = false;
		auto seq = f1->then([f2, &called](string v) -> shared_ptr<future<string>> {
			ok(v == "input");
			called = true;
			return f2;
		});
		WHEN("dependent completes") {
			f1->done("input");
			THEN("our callback was called") {
				ok(called);
			}
			AND_THEN("->then result is unchanged") {
				ok(!seq->is_ready());
			}
			f2->done("inner");
			AND_THEN("->then is now complete") {
				ok(seq->is_done());
			}
			AND_THEN("and propagated the value") {
				ok(seq->value() == "inner");
			}
		}
	}
}

SCENARIO("we can handle dependent failure in ->then", "[composed][shared]") {
	GIVEN("a two-item ->then chain") {
		auto f1 = cps::make_future<string>();
		auto f2 = cps::make_future<string>();
		bool called = false;
		auto seq = f1->then([f2, &called](string v) -> shared_ptr<future<string>> {
			ok(v == "input");
			called = true;
			return f2;
		});
		WHEN("dependent fails") {
			f1->fail("breakage");
			THEN("our callback was not called") {
				ok(!called);
				AND_THEN("->then result is also failed") {
					ok(seq->is_failed());
					ok(seq->failure_reason() == f1->failure_reason());
				}
			}
		}
	}
}

SCENARIO("we can handle cancellation in ->then", "[composed][shared]") {
	GIVEN("a two-item ->then chain") {
		auto f1 = cps::make_future<string>();
		auto f2 = cps::make_future<string>();
		bool called = false;
		auto seq = f1->then([f2, &called](string v) -> shared_ptr<future<string>> {
			ok(v == "input");
			called = true;
			return f2;
		});
		WHEN("dependent is cancelled") {
			f1->cancel();
			THEN("our callback was not called") {
				ok(!called);
			}
			AND_THEN("->then result is also cancelled") {
				ok(seq->is_cancelled());
			}
		}
	}
}

SCENARIO("we can cancel the future returned by ->then", "[composed][shared]") {
	GIVEN("a two-item ->then chain") {
		auto f1 = cps::make_future<string>();
		auto f2 = cps::make_future<string>();
		bool called = false;
		auto seq = f1->then([f2, &called](string v) -> shared_ptr<future<string>> {
			ok(v == "input");
			called = true;
			return f2;
		});
		WHEN("sequence future is cancelled") {
			seq->cancel();
			THEN("our callback was not called") {
				ok(!called);
			}
			AND_THEN("->then result is cancelled") {
				ok(seq->is_cancelled());
			}
			AND_THEN("leaf future was not touched") {
				ok(!f1->is_ready());
			}
		}
	}
}

SCENARIO("we can remap errors via ->then", "[composed][shared]") {
	GIVEN("a simple ->then chain with std::exception handler") {
		auto initial = cps::future<string>::create_shared();
		auto success = cps::future<string>::create_shared();
		auto failure = cps::future<string>::create_shared();
		bool called = false;
		bool errored = false;
		auto seq = initial->then([success, &called](string v) -> shared_ptr<future<string>> {
			ok(v == "input");
			called = true;
			return success;
		}, [failure, &errored](const std::exception &) {
			errored = true;
			return failure;
		});
		WHEN("dependent completes") {
			auto weak = std::weak_ptr<cps::future<string>>(failure);
			initial->done("input");
			THEN("our callback was called") {
				ok(called);
				ok(!errored);
			}
			AND_THEN("->then result is unchanged") {
				ok(!seq->is_ready());
			}
			failure.reset();
			AND_THEN("other future pointer expired") {
				ok(weak.expired());
			}
		}
		WHEN("dependent fails") {
			auto weak = std::weak_ptr<cps::future<string>>(success);
			initial->fail("error");
			THEN("our error handler was called") {
				ok(!called);
				ok(errored);
			}
			AND_THEN("->then result is unchanged") {
				ok(!seq->is_ready());
			}
			success.reset();
			AND_THEN("other future pointer expired") {
				ok(weak.expired());
			}
		}
		WHEN("->then cancelled") {
			auto weak1 = std::weak_ptr<cps::future<string>>(success);
			auto weak2 = std::weak_ptr<cps::future<string>>(failure);
			seq->cancel();
			THEN("neither handler was called") {
				ok(!called);
				ok(!errored);
			}
			AND_THEN("->then is marked as cancelled") {
				ok(seq->is_cancelled());
			}
			success.reset();
			failure.reset();
			// FIXME These should expire immediately,
			// we should not have to mark the initial
			// future as ready first
			initial->cancel();
			AND_THEN("both future pointers expired") {
				ok(weak1.expired());
				ok(weak2.expired());
			}
		}
	}
}

