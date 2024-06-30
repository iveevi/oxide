#pragma once

#include <optional>
#include <variant>
#include <vector>
#include <unordered_map>

#include "include/std.hpp"
#include "include/types.hpp"
#include "include/memory.hpp"

// Formal arguments
// TODO:: Operation to be a variant of function and binary op
enum Operation {
	none,
	add, subtract,
	multiply, divide
};

enum Comparator {
	eq, neq,
	ge, geq,
	le, leq
};

// A domain signature indicates the types of each *symbol*
using Signature = std::unordered_map <Symbol, Domain>;

bool add_signature(Signature &, const std::string &, Domain);
std::optional <Signature> join(const Signature &, const Signature &);

template <typename E>
Signature default_signature(const E &e)
{
	Signature result;

	auto symbols = e.symbols();
	for (const auto &s : symbols)
		result[s] = real;

	return result;
}

template <typename E>
Signature default_signature(const Signature &S, const E &e)
{
	Signature result = S;

	auto symbols = e.symbols();
	for (const auto &s : symbols) {
		if (!S.count(s))
			result[s] = real;
	}

	return result;
}

// Leaf element in an expression tree
using Atom = auto_variant <Integer, Real, Symbol>;

// Node in an expression tree
struct ETN;

using ETN_ref = ETN *;

struct _expr_tree_atom {
	Atom atom;
	ETN_ref next;

	_expr_tree_atom(const Atom &a) : atom(a), next(nullptr) {}
};

struct _expr_tree_op {
	Operation op;
	Domain dom;
	ETN_ref down;
	ETN_ref next; // For next operand
};

struct ETN : auto_variant <_expr_tree_atom, _expr_tree_op> {
	using auto_variant <_expr_tree_atom, _expr_tree_op> ::auto_variant;

	ETN(const ETN &) = delete;
	ETN &operator=(const ETN &) = delete;

	std::vector <Symbol> symbols() const;

	ETN_ref &next();
};

struct Expression {
	// Expression tree, linearized (root @0)
	ETN *etn;
	Signature signature;

	std::vector <Symbol> symbols() const;

	static std::optional <Expression> from(const std::string &);

	Expression &drop(scoped_memory_manager &);
};

struct Statement {
	Expression lhs;
	Expression rhs;
	Comparator cmp;
	Signature signature;

	std::vector <Symbol> symbols() const;

	static std::optional <Statement> from(const std::string &);

	Statement &drop(scoped_memory_manager &);
};

using Symbolic = auto_variant <Expression, Statement>;
