#pragma once

#include <variant>
#include <vector>

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

struct Expression;
struct Statement;

using Atom = std::variant <Integer, Real, Symbol>;

// Element in a reverse polish stack
struct RPE : std::variant <Atom, Operation> {
	using std::variant <Atom, Operation> ::variant;

	Atom atom() {
		return std::get <0> (*this);
	}

	Operation op() {
		return std::get <1> (*this);
	}

	bool has_atom() const {
		return std::holds_alternative <Atom> (*this);
	}

	bool has_op() const {
		return std::holds_alternative <Operation> (*this);
	}
};

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
	ETN_ref down;
	ETN_ref next; // For next operand
};

struct ETN : std::variant <expr_tree_atom, expr_tree_op> {
	using std::variant <expr_tree_atom, expr_tree_op> ::variant;

	ETN_ref &next() {
		if (std::holds_alternative <expr_tree_atom> (*this))
			return std::get <0> (*this).next;

		return std::get <1> (*this).next;
	}
};

struct Expression {
	// Reverse Polish representation
	std::vector <RPE> rpe;

	// Expression tree, linearized (root @0)
	ETN *etn;
};

struct Statement {
	Expression lhs;
	Expression rhs;
	Comparator cmp;
};
