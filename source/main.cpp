#include <variant>
#include <vector>
#include <optional>
#include <cmath>
#include <cassert>

#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/types.hpp"

// Formal arguments
enum Operation {
	add, subtract,
	multiply, divide
};

enum Comparator {
	eq, neq,
	ge, geq,
	le, leq
};

struct Expression;
struct Statement;

struct Expression {
	std::vector <Expression> exprs;
	Operation op;
};

struct Statement {
	Expression lhs;
	Expression rhs;
	Comparator cmp;
};

// Lexing compound
using Symbol = std::string;

using token = std::variant <Integer, Real, Symbol, Operation>;

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

ParseResult <Operation> lex_operation(const std::string &s, size_t pos)
{
	char c = s[pos++];

	// Single character operations
	switch (c) {
	case '+':
		return ParseResult <Operation> ::ok(add, pos);
	case '-':
		return ParseResult <Operation> ::ok(subtract, pos);
	case '*':
		return ParseResult <Operation> ::ok(multiply, pos);
	case '/':
		return ParseResult <Operation> ::ok(divide, pos);
	default:
		// Failed, go back
		pos--;
		break;
	}

	return ParseResult <Operation> ::fail();
}


ParseResult <Symbol> lex_symbol(const std::string &s, size_t pos)
{
	if (!std::isalpha(s[pos])) {
		return ParseResult <Symbol> ::fail();
	}

	Symbol result;
	result += s[pos++];

	if (s[pos] == '_') {
		// Get next two characters
		result += s[pos++];
		result += s[pos++];

		// TODO: check for braces
	}

	return ParseResult <Symbol> ::ok(result, pos);
}

// TODO: plus special tokens, e.g. parenthesis, equals, etc
// TODO: infer multiplication from consecutive symbols in shunting yards
std::optional <std::vector <token>> lex(const std::string &s)
{
	std::vector <token> result;

	size_t pos = 0;
	while (pos < s.length()) {
		char c = s[pos];
		if (std::isspace(c)) {
			pos++;
		} else if (std::isdigit(c)) {
			auto real_result = lex_real(s, pos);
			assert(real_result);

			if (!real_result.extra) {
				// Decimal was not encountered
				auto integer_result = lex_integer(s, pos);
				result.push_back(integer_result.value);
				pos = integer_result.next;
			} else {
				result.push_back(real_result.value);
				pos = real_result.next;
			}
		} else if (std::isalpha(c)) {
			auto symbol_result = lex_symbol(s, pos);
			assert(symbol_result);

			result.push_back(symbol_result.value);
			pos = symbol_result.next;
		} else if (auto op_result = lex_operation(s, pos)) {
			result.push_back(op_result.value);
			pos = op_result.next;
		} else {
			fprintf(stderr, "encountered unexpected character '%c'\n", c);
			return std::nullopt;
		}
	}

	return result;
}

struct _fmt_token_dispatcher {
	std::string &ref;

	void operator()(Integer i) {
		ref += fmt::format("{}", i.value);
	}

	void operator()(Real r) {
		ref += fmt::format("{}", r.value);
	}

	void operator()(Symbol symbol) {
		ref += symbol;
	}

	void operator()(Operation op) {
		ref += op_strs[op];
	}

	template <typename T>
	void operator()(T x) {
		ref += "?";
	}

	static constexpr const char *op_strs[] = {
		"+", "-", "*", "/"
	};
};

auto format_as(const std::vector <token> &tokens)
{
	std::string result;
	for (size_t i = 0; i < tokens.size(); i++) {
		std::visit(_fmt_token_dispatcher(result), tokens[i]);
		if (i + 1 < tokens.size())
			result += ", ";
	}

	return "[" + result + "]";
}

int main()
{
	Scalar <Integer> scalar;
	Variable x = Variable::from <Integer> (0);

	auto p = lex_integer("123", 0);
	fmt::println("p = {} ({})", p.value.value, p.success);

	auto r = lex_real("123.007773", 0);
	fmt::println("p = {} ({}, {})", r.value.value, r.success, r.extra);

	auto tokens = lex("123 + a / x_b * 5.3").value_or(std::vector <token> {});

	fmt::println("tokens: {}", tokens);
}
