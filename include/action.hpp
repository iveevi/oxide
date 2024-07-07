#pragma once

#include "formalism.hpp"
#include "include/std.hpp"
#include "include/lex.hpp"

// Value type; unresolved means that the value relies
// on a symbol that has not yet been resolved

// Unresolved types
struct UnresolvedValue;
struct UnresolvedTuple;
struct UnresolvedArgument;

using UnresolvedConclusion = auto_variant <Symbol, Statement>;

using _unresolved_value_base = auto_variant <
	// TODO: custom types?
	Truth,
	Integer,
	Real,
	Symbol,
	Statement,
	UnresolvedTuple,
	UnresolvedArgument,
	LiteralString
>;

struct UnresolvedTuple : public std::vector <UnresolvedValue> {
	using std::vector <UnresolvedValue> ::vector;
};

struct UnresolvedArgument {
	UnresolvedTuple predicates;
	UnresolvedConclusion result;
};

struct UnresolvedValue : _unresolved_value_base {
	using _unresolved_value_base::_unresolved_value_base;
};

// Fully resolved values
struct Value;
struct Tuple;
struct Argument;

using _value_base = auto_variant <
	Truth,
	Integer,
	Real,
	Statement,
	Tuple,
	Argument,
	LiteralString
>;

struct Tuple : public std::vector <Value> {
	using std::vector <Value> ::vector;
};

struct Argument {
	std::vector <Statement> predicates;
	Statement result;
};

struct Value : _value_base {
	using _value_base::_value_base;
};

// Types of actions
struct DefineSymbol {
	std::string identifier;
	UnresolvedValue value;
};

struct DefineAxiom {
	Symbolic value;
};

struct Call {
	Symbol ftn;
	std::vector <UnresolvedValue> args;
};

struct PushOption {
	Symbol name;
	UnresolvedValue arg;
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
	Void, UnresolvedValue, Error
>;

struct Result : _result_base {
	using _result_base::_result_base;
};

