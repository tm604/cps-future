#pragma once

/**
 * This flag controls whether it's possible to copy-construct or assign
 * a cps::future. Normally this is undesirable: although it's possible
 * to move a future, you normally wouldn't want to have stray copies,
 * since this would break the callback guarantee ("you'll get called
 * once or discarded").
 */
#define CAN_COPY_FUTURES 0

/**
 * This flag... this flag should not exist.
 * However, sometimes we seem to be trying to throw an exception within
 * an unwinding stack, so it's here to turn on various bits of dodgy
 * debugging code to trace that.
 */
// #define UNCAUGHT_EXCEPTION_DEBUGGING

/* Bring in our cps:;future_errc definitions */
#include <cps/future/error_code.h>
#include <cps/future/implementation.h>
#include <cps/future/utils.h>

