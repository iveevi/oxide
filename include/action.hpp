#pragma once

#include "formalism.hpp"
#include "include/std.hpp"

// Types of actions
struct DefineSymbolic {
	std::string identifier;
	Symbolic value;
};

struct DefineAxiom {
	Symbolic value;
};

// Arbitrary action (e.g. define... axiom... call...)
using _action_base = auto_variant <DefineSymbolic, DefineAxiom>;

struct Action : _action_base {
	using _action_base::_action_base;
};
