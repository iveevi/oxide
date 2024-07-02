#pragma once

#include "formalism.hpp"
#include "include/std.hpp"

// Types of actions
using RValue = auto_variant <Symbol, Expression, Statement>;

struct DefineSymbol {
	std::string identifier;
	RValue value;
};

struct DefineAxiom {
	Symbolic value;
};

struct Call {
	Symbol ftn;
	std::vector <RValue> args;
};

// Arbitrary action (e.g. define... axiom... call...)
using _action_base = auto_variant <DefineSymbol, DefineAxiom, Call>;

struct Action : _action_base {
	using _action_base::_action_base;
};

// Result of functions
struct Void {};

// TODO: tuple type
struct Error {};

using _result_base = auto_variant <
	Void, RValue, Error
>;

struct Result : _result_base {
	using _result_base::_result_base;
};

