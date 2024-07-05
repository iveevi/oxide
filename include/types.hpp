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

// Basic types
using Truth = bool;
using Integer = long long int;
using Real = long double;
using Symbol = std::string;
