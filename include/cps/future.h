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
 * A future represents the base implementation. It provides chaining and sequencing methods,
 * and is able to retain state (pending, failed, cancelled, complete) but does not store
 * values.
 *
 * A leaf_future is a template class which extends future to include the concept of a value.
 *
 * A sequence_future extends future to provide a container for zero or more futures.
 */

#include <cps/base_future.h>
#include <cps/leaf_future.h>

namespace cps {
	typedef base_future future;
};

