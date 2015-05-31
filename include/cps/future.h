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
 * A base_future represents the base implementation. It provides chaining and sequencing methods,
 * and is able to retain state (pending, failed, cancelled, complete) but does not store
 * values. cps::future is provided as an alias.
 *
 * A leaf_future is a template class which extends future to include the concept of a value.
 * It introduces get(), and (optionally) provides the result to ->on_done, ->then callbacks.
 *
 * A sequence_future is what you get back from ->then or ->else. It retains the inner future.
 * The actual type of the inner future isn't known until the callback runs.
 */

#include <cps/base_future.h>
#include <cps/leaf_future.h>
#include <cps/sequence_future.h>

namespace cps {
	typedef base_future future;
};

