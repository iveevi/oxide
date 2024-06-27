#include <concepts>

// Classification of domain sets
enum domain {
	integer,
	rational,
	real,
	complex
};

// Wrapped types
struct Integer {
	using value_type = long long int;
	static constexpr domain signature = integer;

	// TODO: fill in macro
	value_type value;
	Integer(value_type v = value_type()) : value(v) {}
	operator value_type() { return value; }
};

struct Real {
	using value_type = long double;
	static constexpr domain signature = real;

	value_type value;
	Real(value_type v = value_type()) : value(v) {}
	operator value_type() { return value; }
};

// Collecting all types under a concept
template <typename T>
concept domain_type = requires {
	typename T::value_type;
} && std::same_as <std::decay_t <decltype(T::signature)>, domain>;

// Basic argument atoms
template <domain_type T>
struct Scalar {};

struct Variable {
	int id;
	domain dom;

	template <domain_type T>
	static Variable from(int id) {
		return { id, T::signature };
	}
};
