#include "catch.hpp"

#include <string>
#include <cps/future.h>

SCENARIO("our template helper can identify strings") {
	WHEN("we have a string") {
		std::string data;
		THEN("is_string<> reports true") {
			REQUIRE(cps::is_string<decltype(data)>::value);
		}
	}
	WHEN("we have a const string") {
		const std::string data;
		THEN("is_string<> reports true") {
			REQUIRE(cps::is_string<decltype(data)>::value);
		}
	}
	WHEN("we have a ref to a string") {
		const std::string s;
		const std::string &data(s);
		THEN("is_string<> reports true") {
			REQUIRE(cps::is_string<decltype(data)>::value);
		}
	}
	WHEN("we have char *") {
		char *data;
		THEN("is_string<> reports true") {
			REQUIRE(cps::is_string<decltype(data)>::value);
		}
	}
	WHEN("we have const char *") {
		const char *data;
		THEN("is_string<> reports true") {
			REQUIRE(cps::is_string<decltype(data)>::value);
		}
	}
}

SCENARIO("our template helper reports non-string types as not being strings") {
	WHEN("we have an int") {
		int data;
		THEN("is_string<> reports false") {
			REQUIRE(cps::is_string<decltype(data)>::value == false);
		}
	}
	WHEN("we have a float") {
		float data;
		THEN("is_string<> reports false") {
			REQUIRE(cps::is_string<decltype(data)>::value == false);
		}
	}
	WHEN("we have an exception class") {
		std::exception data;
		THEN("is_string<> reports false") {
			REQUIRE(cps::is_string<decltype(data)>::value == false);
		}
	}
	WHEN("we have a ref to an exception class") {
		std::exception e;
		std::exception &data(e);
		THEN("is_string<> reports false") {
			REQUIRE(cps::is_string<decltype(data)>::value == false);
		}
	}
	WHEN("we have a const ref to an exception class") {
		std::exception e;
		const std::exception &data(e);
		THEN("is_string<> reports false") {
			REQUIRE(cps::is_string<decltype(data)>::value == false);
		}
	}
	WHEN("we have a ref to a const exception class") {
		const std::exception e;
		std::exception const &data(e);
		THEN("is_string<> reports false") {
			REQUIRE(cps::is_string<decltype(data)>::value == false);
		}
	}
}
