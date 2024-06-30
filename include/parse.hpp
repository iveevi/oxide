#pragma once

#include <vector>

#include "include/action.hpp"
#include "include/formalism.hpp"
#include "include/lex.hpp"
#include "include/std.hpp"

struct RPE;

struct TokenStreamParser {
	using Stream = std::vector <Token>;

	Stream &stream;
	size_t pos;

	TokenStreamParser(Stream &s, size_t p)
			: stream(s), pos(p) {}

	std::optional <Token> next(bool move = true) {
		if (pos >= stream.size())
			return std::nullopt;

		size_t p = pos;
		pos += move;
		return stream[p];
	}

	std::optional <Expression> parse_symbolic_expression(const std::vector <RPE> &);
	std::optional <Statement> parse_symbolic_statement(const std::vector <RPE> &);
	std::optional <Symbolic> parse_symbolic();

	std::optional <Symbolic> parse_from_symbol_define(const std::string &);
	std::optional <DefineSymbolic> parse_from_symbol(const std::string &);

	std::optional <Action> parse_action();

	std::vector <Action> parse();
};
