#pragma once

#include <variant>
#include <optional>
#include <vector>

#include "include/types.hpp"
#include "include/formalism.hpp"

// Special tokens
struct Equals {};
struct Comma {};

using Token = std::variant <
	Integer, Real, Symbol,
	Operation,
	Equals, Comma
>;

std::vector <Token> lex(const std::string &);
