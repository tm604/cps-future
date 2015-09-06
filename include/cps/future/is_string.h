#pragma once
#include <string>
#include <type_traits>

namespace cps {

namespace {

template<typename C> class function_traits;

template<typename R, typename C, typename Parameter>
class function_traits<R (C::*)(const Parameter &) const> {
public:
	typedef Parameter arg_type;
};

template<typename U> typename function_traits<U>::arg_type* arg_type_for(U);

};

/**
 * A template that indicates true for a string, false for any other type.
 *
 * The following types are considered to be strings:
 *
 * * std::string
 * * std::string &
 * * const std::string
 * * const std::string &
 * * const std::string const &
 * * char *
 * * const char *
 * * char
 *
 * That last one might not be entirely obvious or ideal, but for our purposes
 * (string / exception differentiation) it's hopefully more acceptable than leaving
 * it out.
 */
template<typename U>
class is_string : public std::integral_constant<
	bool,
	std::is_same<
		typename std::remove_cv<
			typename std::remove_reference<
				typename std::remove_cv<U>::type
			>::type
		>::type,
		std::string
	>::value
	||
	std::is_same<
		typename std::remove_cv<
			typename std::remove_pointer<U>::type
		>::type,
		char
	>::value
> { };

};

