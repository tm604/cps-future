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

#if 0
template<Stored, Passed>
std::shared_ptr<sequence_future>
typed_future<Stored, Passed>::then(
	/** Chained handler for dealing with success */
	std::function<base_future::ptr(value_ref)> ok,
	/** Optional chained handler for when we fail */
	std::function<base_future::ptr(base_future::exception &)> err = nullptr
) {
	// std::shared_ptr<leaf_future<T>> self = shared_from_this();
	auto self = std::dynamic_pointer_cast<typed_future<Stored, Passed>>(base_future::shared_from_this());
	auto f = sequence_future::create();
	on_done([self, ok, f]() {
#if FUTURE_TRACE
		TRACE << "Marking me as done" << " on " << self->label_;
#endif
		if(f->is_ready()) return;
		if(ok) {
			auto inner = ok(self->get());
			f->inner(inner);
		} else {
			// FIXME no
			f->done();
		}
	});
	on_fail([self, err, f](exception &ex) {
#if FUTURE_TRACE
		TRACE << "Marking me as failed" << " on " << self->label_;
#endif
		if(f->is_ready()) return;
		if(err)
			err(ex)->propagate(f);
		else
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
#endif

};

