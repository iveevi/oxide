#include <cassert>
#include <cmath>

#include "include/lex.hpp"
#include "include/action.hpp"
#include "include/formalism.hpp"
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

	Integer value = 0;

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

	Real value = 0;

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
		Real sub = 0;

		while ((c = s[pos]) && std::isdigit(c)) {
			sub += Real(c - '0') * std::pow(10.0, -power);
			power++;
			pos++;
		}

		value += sub;
	}

	return ParseResult <Real, bool> ::ok(value, pos, decimal);
}

ParseResult <Token> lex_operation(const std::string &s, size_t pos)
{
	char c = s[pos++];

	// Single character operations
	switch (c) {
	case '+':
		return ParseResult <Token> ::ok(add, pos);
	case '-':
		// Handle unary minus here, then we can forget about it elsewhere
		if (auto real_result = lex_real(s, pos)) {
			// Only return a real if there was a '.'
			if (real_result.extra) {
				return ParseResult <Token>
					::ok(-1.0f * real_result.value,
					real_result.next);
			}
		}

		if (auto integer_result = lex_integer(s, pos)) {
			return ParseResult <Token>
				::ok(-1 * integer_result.value,
				integer_result.next);
		}

		return ParseResult <Token> ::ok(subtract, pos);
	case '*':
		return ParseResult <Token> ::ok(multiply, pos);
	case '/':
		return ParseResult <Token> ::ok(divide, pos);
	default:
		// Failed, go back
		pos--;
		break;
	}

	return ParseResult <Token> ::fail();
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

template <typename T, T value>
ParseResult <Token> lex_keyword(const std::string &s, size_t pos, const std::string &kw)
{
	if (lex_keyword <T> (s, pos, kw))
		return ParseResult <Token> ::ok(T(value), pos);

	return ParseResult <Token> ::fail();
}

ParseResult <Token> lex_keyword(const std::string &s, size_t pos)
{
	using kw_lexer = decltype(&lex_keyword <void>);

	static const std::unordered_map <std::string, kw_lexer> kw_lexers {
		{ "axiom", &lex_keyword <Axiom> },
		{ "true", &lex_keyword <Truth, true> },
		{ "false", &lex_keyword <Truth, false> },
	};

	for (const auto &[kw, lexer] : kw_lexers) {
		auto result = lexer(s, pos, kw);
		if (result)
			return result;
	}

	return ParseResult <Token> ::fail();
}

ParseResult <Comparator> lex_comparator(const std::string &s, size_t pos)
{
	// TODO: sort by length
	// TODO: only activate inside math blocks..

	auto list = Comparator::list;
	auto cmp = [&](const Comparator &A, const Comparator &B) {
		return A.s.size() > B.s.size();
	};

	// fmt::println("CMP SIZE: {}", list.size());

	std::sort(list.begin(), list.end(), cmp);

	for (auto cmp : list) {
		// fmt::println("CMP: {}", cmp);
		std::string c = cmp.s;

		bool fail = false;
		size_t p = pos;
		for (size_t i = 0; i < c.size() && p < s.size(); i++, p++) {
			if (c[i] != s[p]) {
				fail = true;
				break;
			}
		}

		if (!fail)
			return ParseResult <Comparator> ::ok(cmp, p);
	}

	return ParseResult <Comparator> ::fail();
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

		// TODO: check for braces, e.g. f_{new}
	}

	return ParseResult <Symbol> ::ok(result, pos);
}

ParseResult <Token> lex_special(const std::string &s, size_t pos)
{
	// Special single characters
	char c = s[pos++];

	LiteralString literal;
	switch (c) {
	case '=':
		if (pos < s.size() && s[pos] == '>')
			return ParseResult <Token> ::ok(Implies(), ++pos);

		return ParseResult <Token> ::fail();
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
	case '"':
		while (pos < s.size() && s[pos] != '"')
			literal += s[pos++];

		if (s[pos] != '"') {
			fmt::println("unclosed string literal");
			return ParseResult <Token> ::fail();
		}

		return ParseResult <Token> ::ok(literal, ++pos);
	default:
		break;
	}

	return ParseResult <Token> ::fail();
}

// Refining tokens; combining symbols outside symbolic blocks and removing spaces
std::vector <Token> refine(const std::vector <Token> &tokens)
{
	std::vector <Token> result;

	size_t i = 0;
	while (i < tokens.size()) {
		if (tokens[i].is <Space> ()) {
			i++;
		} else if (tokens[i].is <Symbol> ()) {
			Symbol s;
			while (tokens[i].is <Symbol> ())
				s += tokens[i++].as <Symbol> ();

			result.push_back(s);
		} else {
			result.push_back(tokens[i]);
			i++;
		}
	}

	return result;
}

// TODO: infer multiplication from consecutive symbols in shunting yards
// TODO: bool math block to check appropriately
std::vector <Token> lex(const std::string &s)
{
	std::vector <Token> result;

	size_t pos = 0;
	while (pos < s.length()) {
		char c = s[pos];
		if (std::isspace(c)) {
			result.push_back(Space());
			while (std::isspace(s[pos])) {
				pos++;
			}
		} else if (c == '#') {
			while (s[pos] != '\n')
				pos++;
		} else if (auto op_result = lex_operation(s, pos)) {
			result.push_back(op_result.value);
			pos = op_result.next;
		} else if (auto special_result = lex_special(s, pos)) {
			result.push_back(special_result.value);
			pos = special_result.next;
		} else if (auto cmp_result = lex_comparator(s, pos)) {
			result.push_back(cmp_result.value);
			pos = cmp_result.next;
		} else if (auto keyword_result = lex_keyword(s, pos)) {
			result.push_back(keyword_result.value);
			pos = keyword_result.next;
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
		} else {
			fprintf(stderr, "encountered unexpected character '%c'\n", c);
			break;
		}
	}

	// fmt::println("tokens:");
	// for (auto t : result)
	// 	fmt::print("{} ", t);
	// fmt::println("");

	result = refine(result);

	fmt::println("refined:");
	for (auto t : result)
		fmt::print("{} ", t);
	fmt::println("");

	return result;
}
