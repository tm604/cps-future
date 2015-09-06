#define FUTURE_TRACE 0
#include <cps/future.h>

#include "catch.hpp"

using namespace cps;
using namespace std;

SCENARIO("we can chain futures via ->then", "[composed][shared]") {
	GIVEN("a two-item ->then chain") {
		auto f1 = cps::make_future<string>();
		auto f2 = cps::make_future<string>();
		bool called = false;
		auto seq = f1->then([f2, &called](string v) -> shared_ptr<future<string>> {
			CHECK(v == "input");
			called = true;
			return f2;
		});
		WHEN("dependent completes") {
			f1->done("input");
			THEN("our callback was called") {
				CHECK(called);
			}
			AND_THEN("->then result is unchanged") {
				CHECK(!seq->is_ready());
			}
			f2->done("inner");
			AND_THEN("->then is now complete") {
				CHECK(seq->is_done());
			}
			AND_THEN("and propagated the value") {
				CHECK(seq->value() == "inner");
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
			CHECK(v == "input");
			called = true;
			return f2;
		});
		WHEN("dependent fails") {
			f1->fail("breakage");
			THEN("our callback was not called") {
				CHECK(!called);
				AND_THEN("->then result is also failed") {
					CHECK(seq->is_failed());
					CHECK(seq->failure_reason() == f1->failure_reason());
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
			CHECK(v == "input");
			called = true;
			return f2;
		});
		WHEN("dependent is cancelled") {
			f1->cancel();
			THEN("our callback was not called") {
				CHECK(!called);
			}
			AND_THEN("->then result is also cancelled") {
				CHECK(seq->is_cancelled());
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
			CHECK(v == "input");
			called = true;
			return f2;
		});
		WHEN("sequence future is cancelled") {
			seq->cancel();
			THEN("our callback was not called") {
				CHECK(!called);
			}
			AND_THEN("->then result is cancelled") {
				CHECK(seq->is_cancelled());
			}
			AND_THEN("leaf future was not touched") {
				CHECK(!f1->is_ready());
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
			CHECK(v == "input");
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
				CHECK(called);
				CHECK(!errored);
			}
			AND_THEN("->then result is unchanged") {
				CHECK(!seq->is_ready());
			}
			failure.reset();
			AND_THEN("other future pointer expired") {
				CHECK(weak.expired());
			}
		}
		WHEN("dependent fails") {
			auto weak = std::weak_ptr<cps::future<string>>(success);
			initial->fail("error");
			THEN("our error handler was called") {
				CHECK(!called);
				CHECK(errored);
			}
			AND_THEN("->then result is unchanged") {
				CHECK(!seq->is_ready());
			}
			success.reset();
			AND_THEN("other future pointer expired") {
				CHECK(weak.expired());
			}
		}
		WHEN("->then cancelled") {
			auto weak1 = std::weak_ptr<cps::future<string>>(success);
			auto weak2 = std::weak_ptr<cps::future<string>>(failure);
			seq->cancel();
			THEN("neither handler was called") {
				CHECK(!called);
				CHECK(!errored);
			}
			AND_THEN("->then is marked as cancelled") {
				CHECK(seq->is_cancelled());
			}
			success.reset();
			failure.reset();
			// FIXME These should expire immediately,
			// we should not have to mark the initial
			// future as ready first
			initial->cancel();
			AND_THEN("both future pointers expired") {
				CHECK(weak1.expired());
				CHECK(weak2.expired());
			}
		}
	}
}

struct CustomException : public runtime_error { using runtime_error::runtime_error; };

SCENARIO("->then with multiple branches") {
	GIVEN("a simple chained future") {
		auto initial = cps::make_future<string>();
		auto seq = initial->then([](string v) -> shared_ptr<future<string>> {
			return cps::resolved_future<string>("original: " + v);
		}, [](const CustomException &) {
			return cps::resolved_future<string>("from a custom exception");
		}, [](const std::runtime_error &) {
			return cps::resolved_future<string>("from std::runtime_error");
		}, [](const std::exception &) {
			return cps::resolved_future<string>("from std::exception");
		});
		WHEN("we succeed") {
			initial->done("valid");
			THEN("things look good") {
				CHECK(seq->value() == "original: valid");
			}
		}
		WHEN("we fail with a generic exception") {
			initial->fail(std::exception { });
			THEN("we see that exception") {
				CHECK(seq->value() == "from std::exception");
			}
		}
		WHEN("we fail with a runtime error") {
			initial->fail(std::runtime_error { "hello" });
			THEN("we see that exception") {
				CHECK(seq->value() == "from std::runtime_error");
			}
		}
		WHEN("we fail with a custom exception") {
			initial->fail(CustomException { "me too" });
			THEN("we see that exception") {
				CHECK(seq->value() == "from a custom exception");
			}
		}
	}
}

SCENARIO("exception within ->then branches") {
	GIVEN("a simple chained future") {
		auto initial = cps::make_future<string>();
		auto seq = initial->then([](string v) -> shared_ptr<future<string>> {
			throw std::runtime_error("ok branch");
		}, [](const CustomException &) -> shared_ptr<future<string>> {
			throw std::runtime_error("custom exception branch");
		}, [](const std::runtime_error &) -> shared_ptr<future<string>> {
			throw std::runtime_error("runtime_error branch");
		}, [](const std::exception &) -> shared_ptr<future<string>> {
			throw std::runtime_error("exception branch");
		});
		WHEN("we succeed") {
			initial->done("valid");
			THEN("things look good") {
				CHECK(seq->failure_reason() == "ok branch");
			}
		}
		WHEN("we fail with a generic exception") {
			initial->fail(std::exception { });
			THEN("we see that exception") {
				CHECK(seq->failure_reason() == "exception branch");
			}
		}
		WHEN("we fail with a runtime error") {
			initial->fail(std::runtime_error { "hello" });
			THEN("we see that exception") {
				CHECK(seq->failure_reason() == "runtime_error branch");
			}
		}
		WHEN("we fail with a custom exception") {
			initial->fail(CustomException { "me too" });
			THEN("we see that exception") {
				CHECK(seq->failure_reason() == "custom exception branch");
			}
		}
	}
}

