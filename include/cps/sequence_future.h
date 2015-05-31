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

private:
	/** This represents the future created by the callback, when it eventually runs */
	std::shared_ptr<base_future> inner_;
};

};

