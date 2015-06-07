#pragma once

#include <cps/base_future.h>

namespace cps {

/**
 * Most of the value-storage handling is common between value, ref and pointer cases.
 */

template<
	typename T,
	bool = ( std::is_fundamental<T>::value
		||   std::is_enum<T>::value)
		&& !(std::is_void<T>::value)
> class leaf_future;

/**
 * Building block for leaf_future.
 * This provides the abstraction used for storing a value of a given
 * type, ad passing it to callbacks by ref or value.
 */
template<typename Stored, typename Passed>
class typed_future : public base_future {
public:
	using value_type = Stored;
	using value_ref = Passed;
	using ptr = std::shared_ptr<typed_future<Stored, Passed>>;
	using derived = std::shared_ptr<leaf_future<Stored>>;

	virtual const base_future::future_type type() const { return base_future::leaf; }

	virtual derived shared_from_this() {
		return std::dynamic_pointer_cast<leaf_future<Stored>>(base_future::shared_from_this());
	}

	template<typename U>
	std::shared_ptr<leaf_future<U>>
	leaf_then(
		/** Chained handler for dealing with success */
		std::function<std::shared_ptr<leaf_future<U>>(Passed)> ok,
		/** Optional chained handler for when we fail */
		std::function<std::shared_ptr<leaf_future<U>>(base_future::exception &)> err = nullptr
	) {
		auto self = shared_from_this();
		auto f = leaf_future<U>::create();
		on_done([self, ok, f]() {
#if FUTURE_TRACE
			TRACE << "Marking me as done" << " on " << self->label_;
#endif
			if(f->is_ready()) return;
			if(ok) {
				auto inner = ok(self->get());
				inner->propagate(f);
			} else {
				// FIXME no
				// f->done();
			}
		});
		on_fail([self, err, f](base_future::exception &ex) {
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

	value_ref get() {
		if(!is_ready())
			throw ready_exception();
		if(is_failed())
			throw fail_exception(failure());
		if(is_cancelled())
			throw cancel_exception();
		return this->value_;
	}

	/**
	 * Act like a base_future for ->on_done(void)
	 */
	base_future::ptr
	on_done(std::function<void()> in) override
	{
		auto self = shared_from_this();
		auto code = [in](value_ref) {
			in();
		};
		if(!is_ready()) {
			on_done_.push_back(code);
			return self;
		} 
		if(is_done()) code(get());
		return self;
	}

	derived
	on_done(std::function<void(value_ref)> code) {
		auto self = shared_from_this();
		if(!is_ready()) {
			on_done_.push_back(code);
			return self;
		} 
		if(is_done()) code(get());
		return self;
	}

	derived
	done(value_type v) {
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
					(it)(v);
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

	typed_future(
		const std::string &l = "unknown leaf future"
	):base_future(l)
#if FUTURE_TRACE
	 ,item_type_{ item_type() }
#endif
	{
#if FUTURE_TRACE
		TRACE << " leaf_future<" << item_type_ << ">()" << " on " << label_;
#endif
	}

	typed_future(typed_future<Stored, Passed> &src) = default;
	typed_future(typed_future<Stored, Passed> &&src) = default;

virtual ~typed_future() {
#if FUTURE_TRACE
		TRACE << "~leaf_future<" << item_type_ << ">() " << describe_state() << " on " << label_;
#endif
	}

	std::shared_ptr<sequence_future>
	then(
		/** Chained handler for dealing with success */
		std::function<base_future::ptr(value_ref)> ok,
		/** Optional chained handler for when we fail */
		std::function<base_future::ptr(base_future::exception &)> err = nullptr
	);

	derived
	propagate(ptr f) {
		auto self = shared_from_this();
		on_done([self, f]() {
			f->done(self->get());
		});
		on_cancel([f]() {
			f->cancel();
		});
		on_fail([f](base_future::exception &e) {
			f->fail(e);
		});
		return self;
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

protected:
	value_type value_;
	std::vector<std::function<void(value_ref)>> on_done_;

#if FUTURE_TRACE
private:
	std::string item_type_;
#endif

};

/**
 * A subclass of cps::base_future which can also store a value.
 * Fundamental types such as int, float, bool pass by value,
 * as are smart pointers. Other types are passed by reference.
 *
 * This one's by-value:
 */
template<
	typename T,
	bool
>
class leaf_future : public typed_future<T, T> {
public:
	using ptr = std::shared_ptr<leaf_future<T>>;

	static
	ptr
	create(
		const std::string &l = "unknown leaf future"
	) {
		return std::make_shared<leaf_future<T>>(l);
	}
};

// then we have pointers:
template<typename T>
class leaf_future<std::shared_ptr<T>, false> : public typed_future<std::shared_ptr<T>, std::shared_ptr<T>>  {
public:
	using ptr = std::shared_ptr<leaf_future<std::shared_ptr<T>>>;

	static
	ptr
	create(
		const std::string &l = "unknown leaf future"
	) {
		return std::make_shared<leaf_future<std::shared_ptr<T>>>(l);
	}
};

// finally other types default to pass-by-ref. These are a little bit more involved
// since we might want the ability to pass a temporary.
template<typename T>
class leaf_future<T, false> : public typed_future<T, T&>  {
public:
	using ptr = std::shared_ptr<leaf_future<T>>;

	static
	ptr
	create(
		const std::string &l = "unknown leaf future"
	) {
		return std::make_shared<leaf_future<T>>(l);
	}

#if 0
	template<typename U>
	std::shared_ptr<leaf_future<U>>
	leaf_then(
		/** Chained handler for dealing with success */
		std::function<std::shared_ptr<leaf_future<U>>(T &)> ok,
		/** Optional chained handler for when we fail */
		std::function<std::shared_ptr<leaf_future<U>>(base_future::exception &)> err = nullptr
	) {
		auto self = std::dynamic_pointer_cast<leaf_future<T>>(base_future::shared_from_this());

		auto f = leaf_future<U>::create();
		on_done([self, ok, f]() {
#if FUTURE_TRACE
			TRACE << "Marking me as done" << " on " << self->label_;
#endif
			if(f->is_ready()) return;
			if(ok) {
				auto inner = ok(self->get());
				inner->propagate(f);
			} else {
				// FIXME no
				// f->done();
			}
		});
		on_fail([self, err, f](base_future::exception &ex) {
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

};

