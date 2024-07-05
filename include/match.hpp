#pragma once

#include <unordered_map>

#include "include/memory.hpp"
#include "include/types.hpp"
#include "include/formalism.hpp"

using _substitution_base = std::unordered_map <Symbol, Expression>;

struct Substitution : _substitution_base {
	using _substitution_base::_substitution_base;

	ETN_ref apply(const ETN_ref &);
	Expression apply(const Expression &);

	Substitution &drop(scoped_memory_manager &);
};

bool equal(const Atom &, const Atom &);
bool equal(const ETN_ref &, const ETN_ref &);
bool equal(const Expression &, const Expression &);

std::optional <Substitution> add_substitution(const Substitution &, const Symbol &, const Expression &);
std::optional <Substitution> join(const Substitution &, const Substitution &);
std::optional <Substitution> match(const ETN_ref &, const ETN_ref &);
std::optional <Substitution> match(const Expression &, const Expression &);
