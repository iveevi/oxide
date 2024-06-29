#pragma once

#include <optional>
#include <variant>
#include <vector>
#include <unordered_map>

#include "include/types.hpp"

// Formal arguments
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
Signature default_signature(E e)
{
	Signature result;

	auto symbols = e.symbols();
	for (const auto &s : symbols)
		result[s] = real;

	return result;
}

template <typename E>
Signature default_signature(const Signature &S, E e)
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
using Atom = std::variant <Integer, Real, Symbol>;

// Node in an expression tree
struct ETN;

using ETN_ref = ETN *;

struct expr_tree_atom {
	Atom atom;
	ETN_ref next;

	expr_tree_atom(const Atom &a) : atom(a), next(nullptr) {}
};

struct expr_tree_op {
	Operation op;
	Domain dom;
	ETN_ref down;
	ETN_ref next; // For next operand
};

struct ETN : std::variant <expr_tree_atom, expr_tree_op> {
	using std::variant <expr_tree_atom, expr_tree_op> ::variant;

	bool has_atom() const;
	bool has_op() const;

	std::vector <Symbol> symbols() const;

	ETN_ref &next();
};

struct Expression {
	// Expression tree, linearized (root @0)
	ETN *etn;
	Signature signature;

	std::vector <Symbol> symbols() const;

	static std::optional <Expression> from(const std::string &);
};

struct Statement {
	Expression lhs;
	Expression rhs;
	Comparator cmp;
	Signature signature;

	std::vector <Symbol> symbols() const;

	static std::optional <Statement> from(const std::string &);
};
