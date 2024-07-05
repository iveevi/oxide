#pragma once

#include <vector>

#include "include/action.hpp"
#include "include/formalism.hpp"
#include "include/lex.hpp"
#include "include/std.hpp"
#include "include/types.hpp"

struct RPE;

struct TokenStreamParser {
	using Stream = std::vector <Token>;
	using Value_vec = std::vector <Value>;

	Stream &stream;
	size_t pos;

	TokenStreamParser(Stream &s, size_t p)
			: stream(s), pos(p) {}

	auto_optional <Token> next(bool move = true) {
		if (pos >= stream.size())
			return std::nullopt;

		size_t p = pos;
		pos += move;
		return stream[p];
	}

	auto backup() {
		// Do not go past the beginning,
		// and preserve EOF state
		if (pos > 0 && pos < stream.size())
			pos--;

		return *this;
	}

	auto_optional <Token> end_statement();

	auto_optional <Expression> parse_symbolic_expression(const std::vector <RPE> &);
	auto_optional <Statement> parse_symbolic_statement(const std::vector <RPE> &);
	auto_optional <Symbolic> parse_symbolic_scope();

	auto_optional <Symbolic> parse_symbolic();
	auto_optional <Symbol> parse_symbol();
	auto_optional <Real> parse_real();
	auto_optional <Integer> parse_int();
	auto_optional <Truth> parse_truth();

	auto_optional <Value> parse_rvalue();

	auto_optional <Tuple> parse_args();

	auto_optional <Action> parse_statement_from_symbol(const Symbol &);
	auto_optional <Action> parse_statement_from_at();
	auto_optional <Action> parse_statement();

	auto_optional <Action> parse_action();

	std::vector <Action> parse();
};
