#pragma once

#ifndef FUTURE_TIMERS
#define FUTURE_TIMERS 1
#endif // FUTURE_TIMERS

#ifndef FUTURE_TRACE
#define FUTURE_TRACE 0
#endif // FUTURE_TRACE

/**
 * \page futures
 *
 * A base_future represents the base implementation. It provides chaining and sequencing methods,
 * and is able to retain state (pending, failed, cancelled, complete) but does not store
 * values. cps::future is provided as an alias.
 *
 * A leaf_future is a template class which extends future to include the concept of a value.
 * It introduces get(), and (optionally) provides the result to ->on_done, ->then callbacks.
 *
 * A sequence_future is what you get back from ->then or ->else. It retains the inner future.
 * The actual type of the inner future isn't known until the callback runs.
 */

#include <cps/base_future.h>
#include <cps/leaf_future.h>
#include <cps/sequence_future.h>

namespace cps {

class future : public base_future {
public:
	static
	base_future::ptr
	repeat(std::function<bool(base_future::ptr)> check, std::function<base_future::ptr(base_future::ptr)> each) {
		auto f = create();
		// Keep f around until it's finished
		f->on_ready([f](ptr in) { });

		auto next = create();
		next->done();
#if FUTURE_TRACE
		TRACE << "->repeat...";
#endif
		std::shared_ptr<std::function<base_future::ptr(base_future::ptr)>> code = std::make_shared<std::function<base_future::ptr(base_future::ptr)>>([f, check, code, each] (base_future::ptr in) mutable -> base_future::ptr {
#if FUTURE_TRACE
			TRACE << "Entering code() with " << (void*)(&(*code));
#endif
			std::function<base_future::ptr(base_future::ptr)> recurse;
			recurse = [&,f](base_future::ptr in) -> base_future::ptr {
#if FUTURE_TRACE
				TRACE << "Entering recursion with " << f->describe_state() << " on " << f->label();
#endif
				if(f->is_ready()) return f;

				{
					bool status = check(in);
#if FUTURE_TRACE
					TRACE << "Check returns " << status;
#endif
					if(status) {
#if FUTURE_TRACE
						TRACE << "And we are done";
#endif
						return f->done();
					}
				}

				in = each(in);
				return in->then([f, in, &recurse]() -> base_future::ptr {
#if FUTURE_TRACE
					TRACE << "each then with f = " << f->describe_state() << " and in = " << in->describe_state() << " on " << f->label();
#endif
					auto v = recurse(in);
#if FUTURE_TRACE
					TRACE << "v was " << v << " on " << f->label();
#endif
					return v;
				})->on_fail([f](exception &ex) {
					f->fail(ex);
				})->on_cancel([f]() {
					f->fail("cancelled");
				});
			};
			return recurse(in);
		});
#if FUTURE_TRACE
		TRACE << "Calling next";
#endif
		next = (*code)(next);
		f->on_ready([code](base_future::ptr) -> base_future::ptr { return create()->done(); });
		return f;
	}

	static ptr needs_all(std::vector<ptr> pending) {
		auto f = create();
		auto count = std::make_shared<std::atomic<int>>();
		auto p = std::make_shared<std::vector<ptr>>(std::move(pending));
		*count = pending.size();
		auto h = [f, count]() {
#if FUTURE_TRACE
			TRACE << "as " << f->describe_state() << " with count = " << *count << " on " << f->label();
#endif
			if(0 == --*count && !f->is_ready()) f->done();
#if FUTURE_TRACE
			TRACE << "now " << f->describe_state() << " with count = " << *count << " on " << f->label();
#endif
		};
		auto ok = [f, h]() {
			if(f->is_ready()) return;
			h();
		};
		auto fail_handler = [h, p, f](exception &ex) {
			if(f->is_ready()) return;
			for(auto &it : *p) {
				if(!it->is_ready()) it->cancel();
			}
			f->fail(ex);
		};
		auto can = [h, p, f]() {
			if(f->is_ready()) return;

			for(auto &it : *p) {
				if(!it->is_ready()) it->cancel();
			}
			f->cancel();
		};
		for(auto &it : *p) {
			it->on_done(ok);
			it->on_fail(fail_handler);
			it->on_cancel(can);
		}
		return f;
	}
};

};

