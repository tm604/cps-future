#pragma once
#include <cps/future.h>

namespace cps {

template<typename T>
class generator {
public:
	using gen = std::function<T(std::error_code &)>;

	generator(
		gen code
	):code_(code)
	{
	}

	T next(std::error_code &ec) {
		return code_(ec);
	}

private:
	bool finished_;
	gen code_;
};

template<typename T>
generator<T>
foreach(std::vector<T> items)
{
	size_t idx = 0;
	return generator<T> {
		[idx = idx, items = std::move(items)](std::error_code &ec) mutable -> T {
			if(idx >= items.size()) {
				ec = make_error_code(future_errc::no_more_items);
				return T();
			}
			return items[idx++];
		}
	};
}

/* Degenerate case - no futures => instant success */
static inline
std::shared_ptr<future<int>>
needs_all()
{
	auto f = future<int>::create_shared();
	f->done(0);
	return f;
}

/* Base case - single future */
template<typename T>
static inline
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
static inline
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
static inline
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
static inline
std::shared_ptr<future<int>>
needs_any()
{
	auto f = future<int>::create_shared();
	f->fail("no elements");
	return f;
}

/* Base case - single future */
template<typename T>
static inline
std::shared_ptr<future<int>>
needs_any(std::shared_ptr<future<T>> first)
{
	return needs_all(first);
}

/* Allow runtime-varying list too */
template<typename T>
static inline
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
static inline
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

/**
 * cps::fmap0<T> some_job {
 *  []() -> std::shared_ptr<cps::future<T>> {
 *  },
 *  cps::foreach(
 *   std::vector<U> &&items
 *  ),
 *  cps::generate(
 *   [] -> std::pair<bool, U> {
 *   }
 *  )
 *  [](const cps::future<T> &prev) -> bool {
 *  },
 *  16
 * };
 */
template<typename T>
class fmap0 {
public:
	using Task = std::function<std::shared_ptr<cps::future<T>>()>;

	bool more_tasks() const { return false; }

	std::shared_ptr<cps::future<T>>
	next_task()
	{
	}

	void
	check_next_task()
	{
		auto self = this;
		while(more_tasks() && tasks_.size() < task_count_) {
			tasks_.push_back(
				next_task()->on_ready([self](const T &) {
					++self->finished_;
					self->check_next_task();
				})
			);
		}
	}

private:
	size_t task_count_;
	size_t finished_;
	std::vector<Task> tasks_;
};

};

