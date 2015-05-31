#pragma once

#include <cps/base_future.h>

namespace cps {

/**
 * A subclass of cps::base_future which can also store a value
 */
template<typename T>
class leaf_future : public base_future {
public:
	static
	std::shared_ptr<leaf_future<T>>
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
		TRACE << " leaf_future<" << item_type_ << ">()" << " on " << label_;
#endif
	}

virtual ~leaf_future() {
#if FUTURE_TRACE
		TRACE << "~leaf_future<" << item_type_ << ">() " << describe_state() << " on " << label_;
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

