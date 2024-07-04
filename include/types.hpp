#pragma once

#include <string>
#include <concepts>

// Classification of domain sets
// TODO: use more complex types
enum Domain {
	integer,
	rational,
	real,
	complex
};

// Wrapped types
using Int = long long int;

struct Integer {
	static constexpr Domain signature = integer;

	// TODO: fill in macro
	Int value;
	Integer(Int v = Int()) : value(v) {}
	operator Int() { return value; }
};

struct Real {
	using value_type = long double;
	static constexpr Domain signature = real;

	value_type value;
	Real(value_type v = value_type()) : value(v) {}
	operator value_type() { return value; }
};

using Symbol = std::string;

// Collecting all types under a concept
template <typename T>
concept domain_type = requires {
	typename T::value_type;
} && std::same_as <std::decay_t <decltype(T::signature)>, Domain>;
