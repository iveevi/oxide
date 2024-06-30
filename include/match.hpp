#pragma once

#include <unordered_map>

#include "include/types.hpp"
#include "include/formalism.hpp"

using Substitution = std::unordered_map <Symbol, Expression>;

bool operator==(const Integer &, const Integer &);
bool operator==(const Real &, const Real &);

bool equal(const Atom &, const Atom &);
bool equal(const ETN_ref &, const ETN_ref &);
bool equal(const Expression &, const Expression &);

std::optional <Substitution> add_substitution(const Substitution &, const Symbol &, const Expression &);
std::optional <Substitution> join(const Substitution &, const Substitution &);
std::optional <Substitution> match(const ETN_ref &, const ETN_ref &);
std::optional <Substitution> match(const Expression &, const Expression &);
