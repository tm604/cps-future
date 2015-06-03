#pragma once

#include <cps/base_future.h>

namespace cps {

/**
 * A subclass of cps::base_future which can also store a value
 */
template<typename T>
class leaf_future : public base_future {
public:
	typedef T value_type;
	typedef T& value_ref;
	typedef std::shared_ptr<leaf_future<T>> ptr;

	virtual const base_future::future_type type() const { return base_future::leaf; }

	static
	ptr
	create(
		const std::string &l = "unknown leaf future"
	) {
		// struct accessor : public leaf_future<T> { };
		return std::make_shared<leaf_future<T>>(l);
	}

	value_ref get() {
		if(!is_ready())
			throw ready_exception();
		if(is_failed())
			throw fail_exception(failure());
		if(is_cancelled())
			throw cancel_exception();
		return value_;
	}

	ptr on_done(std::function<void()> in) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	ptr on_done(std::function<void(value_ref)> code) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
		if(!is_ready()) {
			on_done_.push_back(code);
			return self;
		} 
		if(is_done()) code(get());
		return self;
	}

	ptr done(value_type v) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	leaf_future(
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

virtual ~leaf_future() {
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
	) {
		// std::shared_ptr<leaf_future<T>> self = shared_from_this();
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	template<typename U>
	std::shared_ptr<leaf_future<U>>
	leaf_then(
		/** Chained handler for dealing with success */
		std::function<std::shared_ptr<leaf_future<U>>(value_ref)> ok,
		/** Optional chained handler for when we fail */
		std::function<std::shared_ptr<leaf_future<U>>(base_future::exception &)> err = nullptr
	) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	std::shared_ptr<leaf_future<value_type>>
	propagate(std::shared_ptr<leaf_future<value_type>> f) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
		on_done([self, f]() {
			f->done(self->get());
		});
		on_cancel([f]() {
			f->cancel();
		});
		on_fail([f](exception &e) {
			f->fail(e);
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
	value_type value_;
#if FUTURE_TRACE
	std::string item_type_;
#endif

protected:
	std::vector<std::function<void(value_ref)>> on_done_;
};

template<>
class leaf_future<int> : public base_future {
public:
	typedef int value_type;
	typedef int value_ref;
	typedef std::shared_ptr<leaf_future<int>> ptr;

	virtual const base_future::future_type type() const { return base_future::leaf; }

	static
	ptr
	create(
		const std::string &l = "unknown leaf future"
	) {
		// struct accessor : public leaf_future<T> { };
		return std::make_shared<leaf_future<value_type>>(l);
	}

	value_ref get() {
		if(!is_ready())
			throw ready_exception();
		if(is_failed())
			throw fail_exception(failure());
		if(is_cancelled())
			throw cancel_exception();
		return value_;
	}

	ptr on_done(std::function<void()> in) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	ptr on_done(std::function<void(value_ref)> code) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
		if(!is_ready()) {
			on_done_.push_back(code);
			return self;
		} 
		if(is_done()) code(get());
		return self;
	}

	ptr done(value_type v) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	leaf_future(
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

virtual ~leaf_future() {
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
	) {
		// std::shared_ptr<leaf_future<T>> self = shared_from_this();
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	template<typename U>
	std::shared_ptr<leaf_future<U>>
	leaf_then(
		/** Chained handler for dealing with success */
		std::function<std::shared_ptr<leaf_future<U>>(value_ref)> ok,
		/** Optional chained handler for when we fail */
		std::function<std::shared_ptr<leaf_future<U>>(base_future::exception &)> err = nullptr
	) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	std::shared_ptr<leaf_future<value_type>>
	propagate(std::shared_ptr<leaf_future<value_type>> f) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
		on_done([self, f]() {
			f->done(self->get());
		});
		on_cancel([f]() {
			f->cancel();
		});
		on_fail([f](exception &e) {
			f->fail(e);
		});
		return f;
	}

private:
#if FUTURE_TRACE
	std::string item_type() const { return "int"; }
#endif

private:
	value_type value_;
#if FUTURE_TRACE
	std::string item_type_;
#endif

protected:
	std::vector<std::function<void(value_ref)>> on_done_;
};

/**
 * A subclass of cps::base_future which can also store a value
 */
template<typename T>
class leaf_future<std::shared_ptr<T>> : public base_future {
public:

	typedef std::shared_ptr<T> value_type;
	typedef std::shared_ptr<T> value_ref;
	typedef std::shared_ptr<leaf_future<value_type>> ptr;

	virtual const base_future::future_type type() const { return base_future::leaf; }

	static
	ptr
	create(
		const std::string &l = "unknown leaf future"
	) {
		// struct accessor : public leaf_future<T> { };
		return std::make_shared<leaf_future<value_type>>(l);
	}

	value_ref get() {
		if(!is_ready())
			throw ready_exception();
		if(is_failed())
			throw fail_exception(failure());
		if(is_cancelled())
			throw cancel_exception();
		return value_;
	}

	ptr on_done(std::function<void()> in) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	ptr on_done(std::function<void(value_ref)> code) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
		if(!is_ready()) {
			on_done_.push_back(code);
			return self;
		} 
		if(is_done()) code(get());
		return self;
	}

	ptr done(value_type v) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	leaf_future(
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

virtual ~leaf_future() {
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
	) {
		// std::shared_ptr<leaf_future<T>> self = shared_from_this();
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	template<typename U>
	std::shared_ptr<leaf_future<U>>
	leaf_then(
		/** Chained handler for dealing with success */
		std::function<std::shared_ptr<leaf_future<U>>(value_ref)> ok,
		/** Optional chained handler for when we fail */
		std::function<std::shared_ptr<leaf_future<U>>(base_future::exception &)> err = nullptr
	) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
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

	std::shared_ptr<leaf_future<value_type>>
	propagate(std::shared_ptr<leaf_future<value_type>> f) {
		auto self = std::dynamic_pointer_cast<leaf_future<value_type>>(base_future::shared_from_this());
		on_done([self, f]() {
			f->done(self->get());
		});
		on_cancel([f]() {
			f->cancel();
		});
		on_fail([f](exception &e) {
			f->fail(e);
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
	value_type value_;
#if FUTURE_TRACE
	std::string item_type_;
#endif

protected:
	std::vector<std::function<void(value_ref)>> on_done_;
};

};

