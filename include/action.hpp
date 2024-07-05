#pragma once

#include "formalism.hpp"
#include "include/std.hpp"

// Value type
struct Value;
struct Tuple;

using _rvalue_base = auto_variant <
	Truth, Integer, Real,
	Symbol, Expression, Statement,
	Tuple
>;

struct Tuple : public std::vector <Value> {
	using std::vector <Value> ::vector;
};

struct Value : _rvalue_base {
	using _rvalue_base::_rvalue_base;
};

// Types of actions
struct DefineSymbol {
	std::string identifier;
	Value value;
};

struct DefineAxiom {
	Symbolic value;
};

struct Call {
	Symbol ftn;
	std::vector <Value> args;
};

struct PushOption {
	Symbol name;
	Value arg;
};

// Arbitrary action (e.g. define... axiom... call...)
using _action_base = auto_variant <
	DefineSymbol, DefineAxiom,
	Call, PushOption
>;

struct Action : _action_base {
	using _action_base::_action_base;
};

// Result of functions
struct Void {};
struct Error {};

using _result_base = auto_variant <
	Void, Value, Error
>;

struct Result : _result_base {
	using _result_base::_result_base;
};

