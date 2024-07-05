#pragma once

#include <variant>
#include <optional>
#include <vector>

#include "include/types.hpp"
#include "include/formalism.hpp"
#include "include/std.hpp"

// Special tokens
struct Space {};
struct Equals {};
struct Comma {};
struct In {};
struct Define {};
struct Implies {};
struct At {};
struct Semicolon {};

// Grouping
struct SymbolicBegin {};
struct ParenthesisBegin {};
struct GroupEnd {};

struct SignatureBegin {};
struct SignatureEnd {};

// Keywords
struct Axiom {};

using _token_base = auto_variant <
	Truth, Integer, Real, Symbol,
	Operation, Equals, Comma, In,
	Define, Implies, At, Semicolon,
	SymbolicBegin, ParenthesisBegin, GroupEnd,
	SignatureBegin, SignatureEnd,
	Axiom, Space
>;

// TODO: line information... (l, c)
struct Token : _token_base {
	using _token_base::_token_base;
};

std::vector <Token> lex(const std::string &);
