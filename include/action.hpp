#pragma once

#include "formalism.hpp"
#include "include/std.hpp"

// Basic values
// TODO: tuple type
using Truth = bool;

using _rvalue_base = auto_variant <
	Truth, Int,
	Symbol, Expression, Statement
>;

struct RValue : _rvalue_base {
	using _rvalue_base::_rvalue_base;
};

// Types of actions
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

struct PushOption {
	Symbol name;
	RValue arg;
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

// TODO: tuple type
struct Error {};

using _result_base = auto_variant <
	Void, RValue, Error
>;

struct Result : _result_base {
	using _result_base::_result_base;
};

