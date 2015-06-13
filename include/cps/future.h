#pragma once
#include <cassert>
#include <mutex>
#include <atomic>
#include <memory>
#include <iostream>
#include <string>
#include <vector>

#define CAN_COPY_FUTURES 0

namespace cps {

/**
 * Holds information about a failure.
 * This should really be called "future_failure" or similar, since although it
 * contains an exception, it's not a subclass of std::exception.
 */
class future_exception {
public:
	future_exception(
		std::shared_ptr<std::exception> e,
		const std::string &component
	):ex_(),
	  component_(component),
	  reason_(e->what())
	{
	}

	virtual ~future_exception() = default;

	virtual std::exception &ex() const { return *ex_; }
	virtual const std::string &reason() const { return reason_; }

private:
	std::shared_ptr<std::exception> ex_;
	std::string component_;
	std::string reason_;
};

template<typename T>
class future:public std::enable_shared_from_this<future<T>> {

public:
	/* Probably not very useful since the API is returning shared_ptr all over the shop */
	static std::unique_ptr<future<T>> create() { return std::unique_ptr<future<T>>(new future<T>()); }
	static std::shared_ptr<future<T>> create_shared() { return std::make_shared<future<T>>(); }

	enum class state { pending, done, failed, cancelled };

	class fail_exception : public std::runtime_error {
	public:
		fail_exception(
			const std::string &msg
		):std::runtime_error{ msg }
		{
		}
	};

#if CAN_COPY_FUTURES
	/**
	 * Copy constructor with locking semantics.
	 */
	future(
		const future<T> &src
	):future(src, std::lock_guard<std::mutex>(src.mutex_))
	{
	}
#else
	/**
	 * Disable copy constructor. You can move a future, but copying it runs the
	 * risk of existing callbacks triggering more than once.
	 */
	future(
		const future<T> &src
	) = delete;
#endif

	/**
	 * Move constructor with locking semantics.
	 * @param src source future to move from
	 */
	future(
		future<T> &&src
	):future(
		std::move(src),
		std::lock_guard<std::mutex>(src.mutex_)
	 )
	{
	}

	future() = default;
	virtual ~future() = default;

	/** Returns the shared_ptr associated with this instance */
	std::shared_ptr<future<T>>
	ptr() const
	{
		/* Member function is from a dependent base so give the name lookup a nudge
		 * in the right (this->) direction
		 */
		return this->shared_from_this();
	}

	/** Add a handler to be called when this future is marked as ready */
	std::shared_ptr<future<T>>
	on_ready(std::function<void(future<T> &)> code)
	{
		return call_when_ready(code);
	}

	/** Add a handler to be called when this future is marked as done */
	std::shared_ptr<future<T>>
	on_done(std::function<void(T)> code)
	{
		return call_when_ready([code](future<T> &f) {
			if(f.is_done())
				code(f.value());
		});
	}

	/** Add a handler to be called if this future fails */
	std::shared_ptr<future<T>>
	on_fail(std::function<void(const future_exception&)> code)
	{
		return call_when_ready([code](future<T> &f) {
			if(f.is_failed())
				code(f.failure());
		});
	}

	/** Add a handler to be called if this future fails */
	std::shared_ptr<future<T>> on_fail(std::function<void(std::string)> code)
	{
		return call_when_ready([code](future<T> &f) {
			if(f.is_failed())
				code(f.failure_reason());
		});
	}

	/** Add a handler to be called if this future is cancelled */
	std::shared_ptr<future<T>> on_cancel(std::function<void(future<T> &)> code)
	{
		return call_when_ready([code](future<T> &f) {
			if(f.is_cancelled())
				code(f);
		});
	}

	/** Add a handler to be called if this future is cancelled */
	std::shared_ptr<future<T>> on_cancel(std::function<void()> code)
	{
		return call_when_ready([code](future<T> &f) {
			if(f.is_cancelled())
				code();
		});
	}

	/** Mark this future as done */
	std::shared_ptr<future<T>> done(T v)
	{
		return apply_state([&v](future<T>&f) {
			f.value_ = v;
		}, state::done);
	}

	/** Mark this future as failed */
	std::shared_ptr<future<T>> fail(
		const std::string &ex,
		const std::string &component = u8"unknown"
	)
	{
		auto fe = std::make_shared<fail_exception>(ex);
		return apply_state([fe, &component](future<T>&f) {
			f.ex_ = std::unique_ptr<future_exception>(
				new future_exception(
					fe,
					component
				)
			);
		}, state::failed);
	}

	/** Returns the current value for this future.
	 * Will throw a std::runtime_error if we're not marked as done.
	 */
	T value() const {
		if(state_ != state::done)
			throw std::runtime_error("future is not complete");
		return value_;
	}

	template<typename U>
	std::shared_ptr<future<U>>
	then(
		std::function<std::shared_ptr<future<U>>(T)> ok  = nullptr,
		std::function<std::shared_ptr<future<U>>(const std::string &)> err = nullptr
	)
	{
		auto f = future<U>::create_shared();
		call_when_ready([f, ok, err](future<T> &me) {
			try {
				if(me.is_done()) {
					if(ok) {
						auto inner = ok(me.value())
							->on_done([f](U v) { f->done(v); })
							->on_fail([f](const std::string &msg) { f->fail(msg); })
							->on_cancel([f]() { f->fail("cancelled"); });
						f->on_cancel([inner]() { inner->cancel(); });
					} else {
						/* We can only handle this case if U and T are the same */
						f->fail("->then without ok callback");
					}
				} else if(me.is_failed()) {
					if(err) {
						auto inner = err(me.failure_reason())
							->on_done([f](U v) { f->done(v); })
							->on_fail([f](const std::string &msg) { f->fail(msg); })
							->on_cancel([f]() { f->fail("cancelled"); });
						f->on_cancel([inner]() { inner->cancel(); });
					} else {
						f->fail(
							me.failure_reason(),
							u8"chained future"
						);
					}
				} else if(me.is_cancelled()) {
					f->fail("cancelled");
				}
			} catch(const future_exception &ex) {
				f->fail(ex.reason());
			} catch(const std::runtime_error &ex) {
				f->fail(ex.what());
			}
		});
		return f;
	}

	void cancel() { }

	bool is_ready() { return state_ != state::pending; }
	bool is_done() { return state_ == state::done; }
	bool is_failed() { return state_ == state::failed; }
	bool is_cancelled() { return state_ == state::cancelled; }

	const future_exception &failure() const {
		if(state_ != state::failed)
			throw std::runtime_error("future is not failed");
		return *ex_;
	}
	const std::string &failure_reason() const {
		if(state_ != state::failed)
			throw std::runtime_error("future is not failed");
		return ex_->reason();
	}

protected:
	/**
	 * Queues the given function if we're not yet ready, otherwise
	 * calls it immediately. Will obtain a lock during the
	 * ready-or-queue check.
	 */
	std::shared_ptr<future<T>> call_when_ready(std::function<void(future<T> &)> code)
	{
		bool ready = false;
		{
			std::lock_guard<std::mutex> guard { mutex_ };
			ready = state_ != state::pending;
			if(!ready) {
				tasks_.push_back(code);
			}
		}
		if(ready) code(*this);
		return this->shared_from_this();
	}

	/**
	 * Runs the given code then updates the state.
	 */
	std::shared_ptr<future<T>> apply_state(std::function<void(future<T>&)> code, state s)
	{
		/* Cannot change state to pending, since we assume that we want
		 * to call all deferred tasks.
		 */
		assert(s != state::pending);

		std::vector<std::function<void(future<T> &)>> pending { };
		{
			std::lock_guard<std::mutex> guard { mutex_ };
			code(*this);
			pending = std::move(tasks_);
			tasks_.clear();
			/* This must happen last */
			state_ = s;
		}
		for(auto &v : pending) {
			v(*this);
		}
		return this->shared_from_this();
	}

private:
//#if CAN_COPY_FUTURES
	/**
	 * Locked constructor for internal use.
	 * Copies from the source instance, protected by the given mutex.
	 */
	future(
		const future<T> &src,
		const std::lock_guard<std::mutex> &
	):state_(src.state_.load()),
	  tasks_(src.tasks_),
	  value_(src.value_)
	{
		std::cout << "did the copy thing" << std::endl;
	}
//#endif

	/**
	 * Locked move constructor, for internal use.
	 */
	future(
		future<T> &&src,
		const std::lock_guard<std::mutex> &
	):state_(move(src.state_)),
	  ex_(move(src.ex_)),
	  tasks_(move(src.tasks_)),
	  value_(move(src.value_))
	{
		std::cout << "we moved" << std::endl;
	}

protected:
	mutable std::mutex mutex_;
	std::atomic<state> state_;
	std::unique_ptr<future_exception> ex_;
	std::vector<std::function<void(future<T> &)>> tasks_;
	T value_;
};

#if 0
class thingey {
public:
	add()
	{
		if(f->is_ready()) return;
		if(!in.is_done()) {
			f->fail("error");
			return;
		}
		f->done(0);
	}

protected:
	mutable std::mutex mutex_;
	atomic<size_t> remaining_;
};
#endif

/* Degenerate case - no futures => instant success */
static
std::shared_ptr<future<int>>
needs_all()
{
	auto f = future<int>::create_shared();
	f->done(0);
	return f;
}

/* Base case - single future */
template<typename T>
static
std::shared_ptr<future<int>>
needs_all(std::shared_ptr<future<T>> first)
{
	auto f = future<int>::create_shared();
	std::function<void(future<T> &)> code = [f, first](future<T> &in) {
		if(f->is_ready()) return;
		if(!in.is_done()) {
			f->fail("error");
			return;
		}
		f->done(0);
	};
	first->on_ready(code);
	return f;
}

/* Allow runtime-varying list too */
template<typename T>
static
std::shared_ptr<future<int>>
needs_all(std::vector<std::shared_ptr<future<T>>> first)
{
	auto f = future<int>::create_shared();
	auto pending = std::make_shared<std::atomic<int>>(first.size());
	std::function<void(future<T> &)> code = [f, first, pending](future<T> &in) {
		if(f->is_ready()) return;
		if(!in.is_done()) {
			f->fail("error");
			return;
		}
		if(!--(*pending)) f->done(0);
	};
	for(auto &it : first)
		it->on_ready(code);
	return f;
}

template<typename T, typename ... Types>
static
std::shared_ptr<future<int>>
needs_all(std::shared_ptr<future<T>> first, Types ... rest)
{
	auto remainder = needs_all(rest...);
	auto f = future<int>::create_shared();
	auto pending = std::make_shared<std::atomic<int>>(2);
	std::function<void(future<T> &)> code = [f, first, remainder, pending](future<T> &in) {
		if(f->is_ready()) return;
		if(!in.is_done()) {
			f->fail("error");
			return;
		}
		if(!--(*pending)) f->done(0);
	};
	first->on_ready(code);
	remainder->on_ready(code);
	return f;
}

/* Degenerate case - no futures => instant fail */
static
std::shared_ptr<future<int>>
needs_any()
{
	auto f = future<int>::create_shared();
	f->fail("no elements");
	return f;
}

/* Base case - single future */
template<typename T>
static
std::shared_ptr<future<int>>
needs_any(std::shared_ptr<future<T>> first)
{
	return needs_all(first);
}

/* Allow runtime-varying list too */
template<typename T>
static
std::shared_ptr<future<int>>
needs_any(std::vector<std::shared_ptr<future<T>>> first)
{
	auto f = future<int>::create_shared();
	std::function<void(future<T> &)> code = [f, first](future<T> &in) {
		if(f->is_ready()) return;
		if(!in.is_done()) {
			f->fail("error");
			return;
		}
		f->done(0);
	};
	for(auto &it : first)
		it->on_ready(code);
	return f;
}

template<typename T, typename ... Types>
static
std::shared_ptr<future<int>>
needs_any(std::shared_ptr<future<T>> first, Types ... rest)
{
	auto remainder = needs_all(rest...);
	auto f = future<int>::create_shared();
	std::function<void(future<T> &)> code = [f, first, remainder](future<T> &in) {
		if(f->is_ready()) return;
		if(!in.is_done()) {
			f->fail("error");
			return;
		}
		f->done(0);
	};
	first->on_ready(code);
	remainder->on_ready(code);
	return f;
}

};

