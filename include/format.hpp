#pragma once

#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>

#include "include/lex.hpp"
#include "include/formalism.hpp"
#include "include/action.hpp"

// Printing
std::string format_as(const Domain &);
std::string format_as(const Atom &);
std::string format_as(const Token &);
std::string format_as(const std::vector <Token> &);
std::string format_as(const ETN &, int = 0);
std::string format_as(const Signature &);
std::string format_as(const Expression &);
std::string format_as(const Statement &);
std::string format_as(const Value &);

// Type string generation
const Symbol type_string(const Value &);
