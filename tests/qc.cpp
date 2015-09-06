#define FUTURE_TRACE 0
#include <cps/future.h>

#include "rapidcheck-catch.h"

using namespace cps;
using namespace std;

using namespace rc;
using namespace rc::detail;

namespace rc {

template<typename T>
struct Arbitrary<std::shared_ptr<cps::future<T>>> {
	static Gen<std::shared_ptr<cps::future<T>>> arbitrary() {
		return gen::exec([] {
			auto f = cps::make_future<T>(*gen::arbitrary<std::string>());
			switch(*rc::gen::inRange(0, 3)) {
			case 0:
				return f;
			case 1:
				return f->done(*gen::arbitrary<T>());
			case 2:
				return f->fail(*gen::arbitrary<std::string>());
			case 3:
				return f->cancel();
			default:
				throw std::logic_error("unexpected value from inRange");
			}
		});
	}
};

}

SCENARIO("validate state") {
	prop("check something", [](std::shared_ptr<cps::future<std::string>> f) {
		// std::cerr << f->describe() << "\n";
		if(!f)
			return false;
		if(f->is_ready() && f->is_pending())
			return false;
		if(f->is_pending()) {
			try {
				f->value();
				return false;
			} catch(...) {
			}
			try {
				f->failure_reason();
				return false;
			} catch(...) {
			}
			return true;
		}
		if(f->is_done()) {
			try {
				f->value();
			} catch(...) {
				return false;
			}
			try {
				f->failure_reason();
				return false;
			} catch(const std::runtime_error &e) {
			} catch(...) {
				return false;
			}
			return true;
		}
		if(f->is_cancelled()) {
			try {
				f->value();
				return false;
			} catch(...) {
			}
			try {
				f->failure_reason();
				return false;
			} catch(...) {
			}
			return true;
		}
		if(f->is_failed()) {
			try {
				f->value();
				return false;
			} catch(...) {
			}
			try {
				f->failure_reason();
			} catch(...) {
				return false;
			}
			return true;
		}
		return false;
	});
}

#if 0
struct FutureChain { };

template<typename T>
Gen<FutureChain>
then_chain() {
	return gen::oneOf(
		[] {
			return cps::make_future<T>();
		},
		gen::lazy([] {
			return cps::make_future<T>()->then(
				&then_chain)
			);
		}
	);
}

SCENARIO("validate state") {
	auto v = then_chain();
	prop("check something", [](FutureChain v) {
		return true;
	});
}
#endif

