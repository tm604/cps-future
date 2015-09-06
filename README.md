# Overview

This is yet another implementation for futures in C++11. The API is based mostly on [Future.pm][] where possible.

# Design quirks

The intention is to provide an API that mostly tries to guarantee nonblocking execution, to support asynchronous programming for tasks such as network I/O.

* Everything is a [shared_ptr][]
* We return shared_from_this() from most member functions for chaining. This may change in future.
* Error handling uses either exceptions or error codes. Error code support is currently very limited.
* We ignore threads where possible. There's some half-hearted attempts at mutex protection and atomic guards for state updates.

There's (currently) no "wait until this future is ready" or "run this code on another thread pool" support. 

# Error handling

Exception-based error handling relies on std::current_exception and std::rethrow_exception. These are likely to be quite
slow if you expect to encounter many error cases.

# Other implementations

## std::future

Ideally we'd be able to use the core library feature for futures... unfortunately, the design is not very well suited to async programming - it's more of a basic utility for passing values between threads.

Some issues include:

* No support for composition
* No building blocks for "run this task when the future completes" - you get to poll, or block a thread until the future resolves

## Boost.Future

Boost.Future generally follows the same pattern used in Java/C++11 futures, where it's expected that
a future is a task being processed on another thread. We're more interested in providing a thread-safe,
but implementation-agnostic set of handlers: distributing work across threads is a user decision.

Current wording from [Boost.Thread](http://www.boost.org/doc/libs/1_58_0/doc/html/thread/synchronization.html#thread.synchronization.futures) mentions this in the documentation for .then:

    One option which can be considered is to take two functions,
	one for success and one for error handling. However this
	option has not been retained for the moment.

i.e. there's currently no error callback for then(). This means we'd either need to patch that, or add
a separate .else()

## Folly Futures

Facebook have also released [Folly Futures][].

This provides proper composition via then(), executor support, and exception/value wrapping. There are also various utility functions analogous to fmap/needs_all/etc.

[Future.pm]: http://search.cpan.org/perldoc?Future "Perl module Future.pm"
[shared_ptr]: http://en.cppreference.com/w/cpp/memory/shared_ptr "std::shared_ptr"
[Folly Futures]: https://github.com/facebook/folly/tree/master/folly/futures "Facebook's Folly Futures library"

