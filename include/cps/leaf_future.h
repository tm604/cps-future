#pragma once

#include <cps/base_future.h>

namespace cps {

/**
 * A subclass of cps::base_future which can also store a value
 */
template<typename T>
class leaf_future : public base_future {
public:
	typedef std::shared_ptr<leaf_future<T>> ptr;

	virtual const base_future::future_type type() const { return base_future::leaf; }

	static
	ptr
	create() {
		struct accessor : public leaf_future<T> { };
		return std::make_shared<accessor>();
	}

	const T &get() const {
		if(!is_ready())
			throw ready_exception();
		if(is_failed())
			throw fail_exception(failure());
		if(is_cancelled())
			throw cancel_exception();
		return value_;
	}

	ptr done(const T &v) {
		auto self = std::dynamic_pointer_cast<leaf_future<T>>(base_future::shared_from_this());
#if FUTURE_TRACE
		TRACE << " ->done() was " << describe_state() << " on " << label_;
#endif
		value_ = v;
		state_ = complete;
		on_fail_.clear();
		on_cancel_.clear();
		while(!on_done_.empty()) {
			auto copy = on_done_;
			on_done_.clear();
			for(auto &it : copy) {
#if FUTURE_TRACE
				TRACE << "trying handler " << (void*)(&it) << " on " << label_;
#endif
				try {
					(it)();
				} catch(std::string ex) {
					std::cerr << "Exception in callback - " << ex;
				} catch(...) {
					std::cerr << "Unknown exception in callback";
					throw;
				}
			}
#if FUTURE_TRACE
			TRACE << "finished handler as " << describe_state() << " with " << on_done_.size() << " remaining" << " on " << label_;
#endif
		}
		return self;
	}

private:
	leaf_future(
	)
#if FUTURE_TRACE
	 :item_type_{ item_type() }
#endif
	{
#if FUTURE_TRACE
		TRACE << " leaf_future<" << item_type_ << ">()" << " on " << label_;
#endif
	}

virtual ~leaf_future() {
#if FUTURE_TRACE
		TRACE << "~leaf_future<" << item_type_ << ">() " << describe_state() << " on " << label_;
#endif
	}

public:
	std::shared_ptr<sequence_future>
	then(
		/** Chained handler for dealing with success */
		std::function<base_future::ptr(const T &)> ok,
		/** Optional chained handler for when we fail */
		std::function<base_future::ptr(base_future::exception &)> err = nullptr
	) {
		// std::shared_ptr<leaf_future<T>> self = shared_from_this();
		auto self = std::dynamic_pointer_cast<leaf_future<T>>(base_future::shared_from_this());
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

template<typename T>
class leaf_future<std::shared_ptr<T>> : public base_future {
public:
	static
	std::shared_ptr<leaf_future<std::shared_ptr<T>>>
	create() {
		struct accessor : public leaf_future<std::shared_ptr<T>> { };
		return std::make_shared<accessor>();
	}

	const T get() const {
		if(!is_ready())
			throw ready_exception();
		if(is_failed())
			throw fail_exception(failure());
		if(is_cancelled())
			throw cancel_exception();
		return value_;
	}

	ptr done(const T v) {
		auto self = shared_from_this();
#if FUTURE_TRACE
		TRACE << " ->done() was " << describe_state() << " on " << label_;
#endif
		value_ = v;
		state_ = complete;
		on_fail_.clear();
		on_cancel_.clear();
		while(!on_done_.empty()) {
			auto copy = on_done_;
			on_done_.clear();
			for(auto &it : copy) {
#if FUTURE_TRACE
				TRACE << "trying handler " << (void*)(&it) << " on " << label_;
#endif
				try {
					(it)(self);
				} catch(std::string ex) {
					std::cerr << "Exception in callback - " << ex;
				} catch(...) {
					std::cerr << "Unknown exception in callback";
					throw;
				}
			}
#if FUTURE_TRACE
			TRACE << "finished handler as " << describe_state() << " with " << on_done_.size() << " remaining" << " on " << label_;
#endif
		}
		return self;
	}

private:
	leaf_future(
	)
#if FUTURE_TRACE
	 :item_type_{ item_type() }
#endif
	{
#if FUTURE_TRACE
		TRACE << " leaf_future<" << item_type_ << ">(" << label_ << ")";
#endif
	}

virtual ~leaf_future() {
#if FUTURE_TRACE
		TRACE << "~leaf_future<" << item_type_ << ">(" << label_ << ") " << describe_state();
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
	mutable std::shared_ptr<T> value_;
#if FUTURE_TRACE
	std::string item_type_;
#endif
};

};

