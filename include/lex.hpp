#pragma once

#include <variant>
#include <optional>
#include <vector>

#include "include/types.hpp"
#include "include/formalism.hpp"
#include "include/std.hpp"

// Special tokens
struct Space {};
struct Comma {};
struct In {};
struct Define {};
struct Implies {};
struct At {};
struct Semicolon {};

struct LiteralString : Symbol {
	using Symbol::Symbol;
};

// Grouping
struct SymbolicBegin {};
struct ParenthesisBegin {};
struct GroupEnd {};

struct SignatureBegin {};
struct SignatureEnd {};

// Keywords
struct Axiom {};

using _token_base = auto_variant <
	Axiom, Space, LiteralString,
	Comparator, Operation, Comma, In,
	Define, Implies, At, Semicolon,
	SignatureBegin, SignatureEnd,
	SymbolicBegin, ParenthesisBegin, GroupEnd,
	Truth, Integer, Real, Symbol
>;

// TODO: line information... (l, c)
struct Token : _token_base {
	using _token_base::_token_base;
};

std::vector <Token> lex(const std::string &);
