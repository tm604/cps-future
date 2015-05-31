#pragma once

#include <cps/base_future.h>

namespace cps {

/**
 * A subclass of cps::base_future which represents a chained future.
 */
class sequence_future : public base_future {
public:
	sequence_future(
		const std::string &label = u8"unlabelled future"
	):base_future(label)
	{
	}

	virtual ~sequence_future() { }

	virtual const base_future::future_type type() const { return base_future::sequence; }

	static
	std::shared_ptr<sequence_future>
	create() {
		// Defeat private constructor if necessary
		struct accessor : public sequence_future { };
		return std::make_shared<accessor>();
	}

	template<typename U>
	std::shared_ptr<leaf_future<U>>
	as()
	{
		if(!is_done())
			throw base_future::ready_exception();

		if(inner_->type() != base_future::leaf)
			throw type_exception();

		return std::dynamic_pointer_cast<leaf_future<U>>(inner_);
	}

	std::shared_ptr<sequence_future>
	inner(std::shared_ptr<base_future> f)
	{
		auto self = std::dynamic_pointer_cast<sequence_future>(shared_from_this());
		inner_ = f;
		f->propagate(self);
		return self;
	}

private:
	/** This represents the future created by the callback, when it eventually runs */
	std::shared_ptr<base_future> inner_;
};

};

