#include <variant>
#include <vector>
#include <optional>
#include <cmath>

#include <fmt/printf.h>

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

// Formal arguments
enum opterm {
	multiply, divide
};

enum opexpr {
	add, subtract
};

enum comparator {
	eq, neq,
	ge, geq,
	le, leq
};

struct Factor;
struct Term;
struct Expression;
struct Statement;

struct Factor {};

struct Term {
	Factor lhs;
	Factor rhs;
	opterm op;
};

struct Expression {
	Term lhs;
	Term rhs;
	opexpr op;
};

struct Statement {
	Expression lhs;
	Expression rhs;
	comparator cmp;
};

// Lexing compound
using token = std::variant <Integer, Real, std::string>;

template <typename T, typename E = int>
struct ParseResult {
	T value;
	size_t next;
	bool success;
	E extra;

	operator bool() const {
		return success;
	}

	static ParseResult ok(T v, size_t n) {
		return { v, n, true };
	}

	static ParseResult ok(T v, size_t n, E e) {
		return { v, n, true, e };
	}

	static ParseResult fail() {
		return { T(), 0, false };
	}
};

ParseResult <Integer> lex_integer(const std::string &s, size_t pos)
{
	// Must be non-negative... minus sign is
	// treated as a subtraction operation
	if (!std::isdigit(s[pos])) {
		fprintf(stderr, "expected digit when lexing an integer");
		return ParseResult <Integer> ::fail();
	}

	Integer::value_type value = 0;

	char c;
	while ((c = s[pos]) && std::isdigit(c)) {
		value = 10 * value + (c - '0');
		pos++;
	}

	return ParseResult <Integer> ::ok(value, pos);
}

ParseResult <Real, bool> lex_real(const std::string &s, size_t pos)
{
	// Must be non-negative... minus sign is
	// treated as a subtraction operation
	if (!std::isdigit(s[pos])) {
		fprintf(stderr, "expected digit when lexing a real");
		return ParseResult <Real, bool> ::fail();
	}

	Real::value_type value = 0;

	char c;
	while ((c = s[pos]) && std::isdigit(c)) {
		value = 10 * value + (c - '0');
		pos++;
	}

	// Decimal point
	bool decimal = false;

	if (s[pos] == '.') {
		decimal = true;
		pos++;

		int power = 1;
		Real::value_type sub = 0;

		while ((c = s[pos]) && std::isdigit(c)) {
			sub += Real::value_type(c - '0') * std::powl(10.0, -power);
			power++;
			pos++;
		}

		value += sub;
	}

	return ParseResult <Real, bool> ::ok(value, pos, decimal);
}

std::optional <std::vector <token>> lex(const std::string &s)
{
	std::vector <token> result;

	for (size_t i = 0; i < s.length(); i++) {
		if (std::isspace(s[i]))
			continue;
	}

	return result;
}

int main()
{
	Scalar <Integer> scalar;
	Variable x = Variable::from <Integer> (0);

	auto p = lex_integer("123", 0);
	fmt::println("p = {} ({})", p.value.value, p.success);

	auto r = lex_real("123.007773", 0);
	fmt::println("p = {} ({}, {})", r.value.value, r.success, r.extra);
}
