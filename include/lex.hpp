#pragma once

#include <variant>
#include <optional>
#include <vector>

#include "include/types.hpp"
#include "include/formalism.hpp"

// Special tokens
struct Equals {};
struct Comma {};
struct In {};

struct SignatureBegin {};
struct SignatureEnd {};

using _token_base = std::variant <
	Integer, Real, Symbol,
	Operation,
	Equals, Comma, In,
	SignatureBegin, SignatureEnd
>;

struct Token : _token_base {
	using _token_base::_token_base;

	template <typename T>
	bool is() const {
		return std::holds_alternative <T> (*this);
	}

	template <typename T>
	T as() const {
		return std::get <T> (*this);
	}
};

std::vector <Token> lex(const std::string &);
