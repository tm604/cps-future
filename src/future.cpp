#include <cps/future.h>

namespace cps {

std::shared_ptr<sequence_future>
base_future::then(seq ok, std::function<ptr(exception&)> err) {
	auto self = shared_from_this();
	auto f = sequence_future::create();
	on_done([self, ok, f]() {
#if FUTURE_TRACE
		TRACE << "Marking me as done" << " on " << self->label_;
#endif
		if(f->is_ready()) return;
		auto s = ok();
		s->propagate(f);
	});
	on_fail([self, f](exception &ex) {
#if FUTURE_TRACE
		TRACE << "Marking me as failed" << " on " << self->label_;
#endif
		if(f->is_ready()) return;
		f->fail(ex);
	});
	on_cancel([self, f]() {
#if FUTURE_TRACE
		TRACE << "Marking me as cancelled" << " on " << self->label_;
#endif
		if(f->is_ready()) return;
		f->cancel();
	});
	return f;
}

};

