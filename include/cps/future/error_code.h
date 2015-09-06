#pragma once

namespace {

// Helper for extracting some sort of integer from an enum class
template<typename Enum>
constexpr inline typename std::underlying_type<Enum>::type from_enum(Enum e) {
	return static_cast<typename std::underlying_type<Enum>::type>(e);
}

// Does the reverse of the above
template<typename Enum, typename Type>
constexpr inline typename std::enable_if<
	std::is_enum<Enum>::value && std::is_integral<Type>::value,
	Enum
>::type to_enum(Type v) noexcept 
{
	return static_cast<Enum>(v);
}

};

namespace cps {

/**
 * cps::future error codes.
 */
enum class future_errc {
	is_pending = 1,
	is_failed,
	is_cancelled,
	no_more_items
};

namespace detail {

/**
 * Categories for cps::future errors.
 *
 * Accessible from cps::future_category().
 */
class future_category : public std::error_category {
public:
	/**
	 * Returns the name for this category - "cps::future".
	 */
	virtual const char* name() const noexcept override { return "cps::future"; }
	/**
	 * Returns the message corresponding to the given error code.
	 */
	virtual std::string message(int ev) const override {
		switch(to_enum<cps::future_errc>(ev)) {
		case cps::future_errc::is_pending:
			return "future is still pending";
		case cps::future_errc::is_failed:
			return "future is failed";
		case cps::future_errc::is_cancelled:
			return "future is cancelled";
		case cps::future_errc::no_more_items:
			return "no more items";
		default:
			return "unknown cps::future error";
		}
	}

	virtual bool equivalent(
		const std::error_code &code,
		int condition
	) const noexcept override;
};

}

/**
 * Returns the future category instance.
 */
inline const std::error_category &get_future_category() {
	static detail::future_category instance;
	return instance;
}

/**
 * Generates a new std::error_code for the given cps::future_errc code.
 */
inline std::error_code
make_error_code(future_errc e)
{
	return std::error_code(
		static_cast<int>(e),
		get_future_category()
	);
}

/**
 * Generates a new std::error_condition for the given cps::future_errc code.
 */
inline std::error_condition
make_error_condition(future_errc e)
{
	return std::error_condition(
		static_cast<int>(e),
		get_future_category()
	);
}


namespace detail {

inline bool
future_category::equivalent(
	const std::error_code &code,
	int condition
) const noexcept {
	switch(to_enum<future_errc>(condition)) {
	case future_errc::is_pending:
		return code == make_error_code(future_errc::is_pending);
	case future_errc::is_failed:
		return code == make_error_code(future_errc::is_failed);
	case future_errc::is_cancelled:
		return code == make_error_code(future_errc::is_cancelled);
	case future_errc::no_more_items:
		return code == make_error_code(future_errc::no_more_items);
	default:
		return false;
	}
}

}

static const std::error_category &future_category = get_future_category();

};

namespace std {

template <>
struct is_error_code_enum<cps::future_errc> : public true_type { };

};

