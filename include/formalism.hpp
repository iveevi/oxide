#pragma once

#include <vector>

// Formal arguments
enum Operation {
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

struct Expression {
	std::vector <Expression> exprs;
	Operation op;
};

struct Statement {
	Expression lhs;
	Expression rhs;
	Comparator cmp;
};
