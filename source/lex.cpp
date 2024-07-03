#include <cassert>
#include <cmath>

#include "include/lex.hpp"
#include "include/format.hpp"

// Lexing compound
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
			sub += Real::value_type(c - '0') * std::pow(10.0, -power);
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

template <typename T>
ParseResult <Token> lex_keyword(const std::string &s, size_t pos, const std::string &kw)
{
	for (size_t i = 0; i < kw.size(); i++, pos++) {
		if (pos >= s.size())
			return ParseResult <Token> ::fail();

		if (s[pos] != kw[i])
			return ParseResult <Token> ::fail();
	}

	return ParseResult <Token> ::ok(T(), pos);
}

ParseResult <Token> lex_keyword(const std::string &s, size_t pos)
{
	using kw_lexer = decltype(&lex_keyword <Cache>);

	static const std::unordered_map <std::string, kw_lexer> kw_lexers {
		{ "axiom", &lex_keyword <Axiom> }
	};

	for (const auto &[kw, lexer] : kw_lexers) {
		auto result = lexer(s, pos, kw);
		if (result)
			return result;
	}

	return ParseResult <Token> ::fail();
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

ParseResult <Token> lex_special(const std::string &s, size_t pos)
{
	// Special single characters
	char c = s[pos++];

	switch (c) {
	case '=':
		if (pos < s.size() && s[pos] == '>')
			return ParseResult <Token> ::ok(Implies(), ++pos);

		return ParseResult <Token> ::ok(Equals(), pos);
	case ',':
		return ParseResult <Token> ::ok(Comma(), pos);
	case ':':
		if (pos < s.size() && s[pos] == '=')
			return ParseResult <Token> ::ok(Define(), ++pos);

		return ParseResult <Token> ::ok(In(), pos);
	case '$':
		if (pos < s.size() && s[pos] == '(')
			return ParseResult <Token> ::ok(SymbolicBegin(), ++pos);
		break;
	case '@':
		return ParseResult <Token> ::ok(At(), pos);
	case ';':
		return ParseResult <Token> ::ok(Semicolon(), pos);
	case '(':
		return ParseResult <Token> ::ok(ParenthesisBegin(), pos);
	case ')':
		return ParseResult <Token> ::ok(GroupEnd(), pos);
	case '[':
		return ParseResult <Token> ::ok(SignatureBegin(), pos);
	case ']':
		return ParseResult <Token> ::ok(SignatureEnd(), pos);
	default:
		break;
	}

	return ParseResult <Token> ::fail();
}

// TODO: plus special tokens, e.g. parenthesis, equals, etc
// TODO: infer multiplication from consecutive symbols in shunting yards
std::vector <Token> lex(const std::string &s)
{
	std::vector <Token> result;

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
		} else if (auto keyword_result = lex_keyword(s, pos)) {
			result.push_back(keyword_result.value);
			pos = keyword_result.next;
		} else if (std::isalpha(c)) {
			auto symbol_result = lex_symbol(s, pos);
			assert(symbol_result);

			result.push_back(symbol_result.value);
			pos = symbol_result.next;
		} else if (auto op_result = lex_operation(s, pos)) {
			result.push_back(op_result.value);
			pos = op_result.next;
		} else if (auto special_result = lex_special(s, pos)) {
			result.push_back(special_result.value);
			pos = special_result.next;
		} else {
			fprintf(stderr, "encountered unexpected character '%c'\n", c);
			break;
		}
	}

	return result;
}
