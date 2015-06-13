
# Other implementations

Boost.Future generally follows the same pattern used in Java/C++11 futures, where it's expect that a future is a task being processed on another thread. We're more interested in providing a thread-safe, but implementation-agnostic set of handlers: distributing work across threads is a user decision.

Current wording from [Boost.Thread](http://www.boost.org/doc/libs/1_58_0/doc/html/thread/synchronization.html#thread.synchronization.futures) mentions this in the documentation for .then:

    One option which can be considered is to take two functions, one for success and one for error handling. However this option has not been retained for the moment.

i.e. there's no error callback for .then, so we'd either need to patch that, or add a separate .else (not quite the same semantics, although it can be made to work).

