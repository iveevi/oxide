#pragma once

#include <variant>
#include <optional>

#include "include/types.hpp"
#include "include/formalism.hpp"

using token = std::variant <Integer, Real, Symbol, Operation>;

std::optional <std::vector <token>> lex(const std::string &);
