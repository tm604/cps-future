#pragma once

#ifndef FUTURE_TRACE
#define FUTURE_TRACE 0
#endif // FUTURE_TRACE

#include <memory>
#include <atomic>

#include <string>

#if FUTURE_TRACE
#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <cxxabi.h>
#endif

#include <vector>

namespace cps {

class future : public std::enable_shared_from_this<future> {
public:
	/** A std::shared_ptr to a cps::future */
	typedef std::shared_ptr<future>  ptr;
	/** A sequence callback (then, else, etc.) */
	typedef std::function<ptr(ptr)>  seq;
	/** A one-shot callback (on_done, on_fail etc.) */
	typedef std::function<void(ptr)> evt;

	enum state {
		pending,
		cancelled,
		failed,
		complete
	};

	class ready_exception : public std::runtime_error {
	public:
		ready_exception(
		):std::runtime_error{ "cps::future is not ready" }
		{
		}
	};
	class cancel_exception : public std::runtime_error {
	public:
		cancel_exception(
		):std::runtime_error{ "cps::future is cancelled" }
		{
		}
	};
	class fail_exception : public std::runtime_error {
	public:
		fail_exception(
			cps::future &f
		):std::runtime_error{ f.failure() }
		{
		}
	};

	future(
	):state_{pending}
	{
#if FUTURE_TRACE
		std::cout << " future()" << std::endl;
#endif
	}

virtual ~future() {
#if FUTURE_TRACE
		std::cout << "~future() " << describe_state() << std::endl;
#endif
	}

	std::string
	describe_state() const
	{
		switch(state_) {
		case pending: return "pending";
		case cancelled: return "cancelled";
		case failed: return "failed";
		case complete: return "complete";
		default: return "unknown (" + std::to_string((int)state_) + ")";
		}
	}

	static
	ptr
	create() {
		struct accessor : public future { };
		return std::make_shared<accessor>();
	}

	ptr done(ptr f) {
		auto self = shared_from_this();
		return done();
	}

	ptr done() {
		auto self = shared_from_this();
#if FUTURE_TRACE
		std::cout << " ->done() was " << describe_state() << std::endl;
#endif
		state_ = complete;
		on_fail_.clear();
		on_cancel_.clear();
		while(!on_done_.empty()) {
			auto copy = on_done_;
			on_done_.clear();
			for(auto &it : copy) {
#if FUTURE_TRACE
				std::cout << "trying handler " << (void*)(&it) << std::endl;
#endif
				try {
					(it)(self);
				} catch(std::string ex) {
					std::cerr << "Exception in callback - " << ex << std::endl;
				} catch(...) {
					std::cerr << "Unknown exception in callback" << std::endl;
					throw;
				}
			}
#if FUTURE_TRACE
			std::cout << "finished handler as " << describe_state() << " with " << on_done_.size() << " remaining" << std::endl;
#endif
		}
		return self;
	}

	ptr fail(std::string ex) {
		reason_ = ex;
		return fail();
	}

	ptr fail(ptr f) {
		reason_ = "sequence";
		return fail();
	}

	ptr on_ready(evt code) {
		auto self = shared_from_this();
		on_done(code);
		on_cancel(code);
		on_fail(code);
		return self;
	}

	ptr propagate(ptr f) {
#if FUTURE_TRACE
		std::cout << "propagating " << f->describe_state() << " from " << describe_state() << std::endl;
#endif
		on_done([f](ptr) {
			f->done();
		});
		on_cancel([f](ptr) {
			f->cancel();
		});
		on_fail([f](ptr src) {
			f->fail(src->failure());
		});
		return f;
	}

	class iterator {
	public:
	};

	static
	future::ptr
	repeat(std::function<bool(future::ptr)> check, std::function<future::ptr(future::ptr)> each) {
		auto f = future::create();
		// Keep f around until it's finished
		f->on_ready([f](ptr in) { });

		auto next = future::create();
		next->done();
		std::shared_ptr<std::function<future::ptr(future::ptr)>> code = std::make_shared<std::function<future::ptr(future::ptr)>>([f, check, code, each] (future::ptr in) mutable -> future::ptr {
#if FUTURE_TRACE
			std::cout << "Entering code() with " << (void*)(&(*code)) << std::endl;
#endif
			std::function<future::ptr(future::ptr)> recurse;
			recurse = [&,f](future::ptr in) -> future::ptr {
#if FUTURE_TRACE
				std::cout << "Entering recursion with " << f->describe_state() << std::endl;
#endif
				if(f->is_ready()) return f;

				{
					bool status = check(in);
#if FUTURE_TRACE
					std::cout << "Check returns " << status << std::endl;
#endif
					if(status) {
#if FUTURE_TRACE
						std::cout << "And we are done" << std::endl;
#endif
						return f->done();
					}
				}

				auto e = each(in);
				return e->then([f, &recurse](future::ptr in) -> future::ptr {
#if FUTURE_TRACE
					std::cout << "each then with f = " << f->describe_state() << " and in = " << in->describe_state() << std::endl;
#endif
					auto v = recurse(in);
#if FUTURE_TRACE
					std::cout << "v was " << v << std::endl;
#endif
					return v;
				})->on_fail([f](future::ptr in) {
					in->propagate(f);
				})->on_cancel([f](future::ptr) {
					f->fail("cancelled");
				});
			};
			return recurse(in);
		});
#if FUTURE_TRACE
		std::cout << "Calling next" << std::endl;
#endif
		next = (*code)(next);
		f->on_ready([code](future::ptr) -> future::ptr { return future::create()->done(); });
		return f;
	}

	static ptr needs_all(std::vector<ptr> pending) {
		auto f = future::create();
		auto count = std::make_shared<std::atomic<int>>();
		auto p = std::make_shared<std::vector<ptr>>(std::move(pending));
		*count = pending.size();
		auto h = [f, count]() {
#if FUTURE_TRACE
			std::cout << "as " << f->describe_state() << " with count = " << *count << std::endl;
#endif
			if(0 == --*count && !f->is_ready()) f->done();
#if FUTURE_TRACE
			std::cout << "now " << f->describe_state() << " with count = " << *count << std::endl;
#endif
		};
		auto ok = [f, h]() {
			if(f->is_ready()) return;
			h();
		};
		auto fail = [h, p, f](future::ptr in) {
			if(f->is_ready()) return;
			for(auto &it : *p) {
				if(!it->is_ready()) it->cancel();
			}
			in->propagate(f);
		};
		auto can = [h, p, f](future::ptr in) {
			if(f->is_ready()) return;

			for(auto &it : *p) {
				if(!it->is_ready()) it->cancel();
			}
			in->propagate(f);
		};
		for(auto &it : *p) {
			it->on_done(ok);
			it->on_fail(fail);
			it->on_cancel(can);
		}
		return f;
	}

private:

	ptr fail() {
		state_ = failed;
		auto self = shared_from_this();
		on_done_.clear();
		on_cancel_.clear();
		while(!on_fail_.empty()) {
			auto copy = std::move(on_fail_);
			on_fail_.clear();
			for(auto &it : copy) {
				try {
					(it)(self);
				} catch(std::string ex) {
					std::cerr << "Exception in callback - " << ex << std::endl;
				} catch(...) {
					std::cerr << "Unknown exception in callback" << std::endl;
					throw;
				}
			}
		}
		return self;
	}

public:
	ptr cancel() {
		state_ = cancelled;
		auto self = shared_from_this();
		on_done_.clear();
		on_fail_.clear();
		while(!on_cancel_.empty()) {
			auto copy = std::move(on_cancel_);
			on_cancel_.clear();
			for(auto &it : copy) {
				try {
					(it)(self);
				} catch(std::string ex) {
					std::cerr << "Exception in callback - " << ex << std::endl;
				} catch(...) {
					std::cerr << "Unknown exception in callback" << std::endl;
					throw;
				}
			}
		}
	}

	static ptr complete_future() { auto f = future::create(); f->done(); return f; }

	ptr then(seq ok) {
		auto self = shared_from_this();
		auto f = create();
		on_done([self, ok, f](ptr in) -> void {
#if FUTURE_TRACE
			std::cout << "Marking me as done" << std::endl;
#endif
			if(f->is_ready()) return;
			auto s = ok(self);
			s->propagate(f);
		});
		on_fail([self, f](ptr in) -> void {
#if FUTURE_TRACE
			std::cout << "Marking me as failed" << std::endl;
#endif
			if(f->is_ready()) return;
			f->fail(self);
		});
		on_cancel([f](ptr in) -> void {
#if FUTURE_TRACE
			std::cout << "Marking me as cancelled" << std::endl;
#endif
			if(f->is_ready()) return;
			f->cancel();
		});
		return f;
	}

	ptr on_done(evt code) {
		auto self = shared_from_this();
		if(!is_ready()) {
			on_done_.push_back(code);
			return self;
		} 
		if(is_done()) code(self);
		return self;
	}
	ptr on_done(std::function<void(void)> code) {
		return on_done([code](ptr f) -> void { code(); });
	}

	ptr on_cancel(evt code) {
		auto self = shared_from_this();
		if(!is_ready()) {
			on_cancel_.push_back(code);
			return self;
		} 
		if(is_cancelled()) code(self);
		return self;
	}

	ptr on_fail(evt code) {
		auto self = shared_from_this();
		if(!is_ready()) {
			on_fail_.push_back(code);
			return self;
		} 
		if(is_failed()) code(self);
		return self;
	}

	ptr then(seq ok, seq fail) {
		auto f = future::create();
		if(ok != nullptr) {
			then_.push_back(ok);
		}
		else_.push_back(fail);
		return f;
	}

	bool is_pending() const { return state_ == state::pending; }
	bool is_ready() const { return state_ != state::pending; }
	bool is_failed() const { return state_ == state::failed; }
	bool is_done() const { return state_ == state::complete; }
	bool is_cancelled() const { return state_ == state::cancelled; }
	std::string failure() const { return reason_; }

protected:
	state state_;

	std::vector<seq> then_;
	std::vector<seq> else_;
	std::vector<evt> on_done_;
	std::vector<evt> on_fail_;
	std::vector<evt> on_cancel_;

	std::string reason_;
};

/** A subclass of cps::future which can also store a value */
template<typename T>
class typed_future : public future {
public:
	static
	std::shared_ptr<future>
	create() {
		struct accessor : public typed_future<T> { };
		return std::make_shared<accessor>();
	}

	T &get() const {
		if(!is_ready())
			throw ready_exception();
		if(is_failed())
			throw fail_exception(*this);
		if(is_cancelled())
			throw cancel_exception();
		return value_;
	}

private:
	typed_future(
	)
#if FUTURE_TRACE
	 :item_type_{ item_type() }
#endif
	{
#if FUTURE_TRACE
		std::cout << " typed_future<" << item_type_ << ">()" << std::endl;
#endif
	}

virtual ~typed_future() {
#if FUTURE_TRACE
		std::cout << "~typed_future<" << item_type_ << ">() " << describe_state() << std::endl;
#endif
	}

private:
#if FUTURE_TRACE
	std::string item_type() const {
		char * name = 0;
		int status;
		std::string str { "" };
		name = abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
		if (name != 0) { str += name; }
		else { str += typeid(T).name(); }
		free(name);
		return str;
	}
#endif

private:
	T value_;
#if FUTURE_TRACE
	std::string item_type_;
#endif
};

};

