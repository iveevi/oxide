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
	Atom atom() {
		return std::get <0> (*this);
	}

	Operation op() {
		return std::get <1> (*this);
	}
};

// Node in an expression tree
struct expr_tree_ref {
	Operation op;
	int down;
	int next; // For next operand
};

struct ETN : std::variant <Atom, expr_tree_ref> {};

struct Expression {
	// Reverse Polish representation
	std::vector <RPE> rpe;

	// Expression tree, linearized (root @0)
	std::vector <ETN> etn;
};

struct Statement {
	Expression lhs;
	Expression rhs;
	Comparator cmp;
};
