#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/formalism.hpp"
#include "include/format.hpp"
#include "include/lex.hpp"
#include "include/parse.hpp"
#include "include/match.hpp"
#include "include/memory.hpp"
#include "include/types.hpp"

// Expample derivation: a * x + b = 0 => x = (0 - b)/a
// a * x + b = 0
// a * x = 0 - b <- (a + b = 0 => a = 0 - b)
// x = (0 - b)/a <- (a * b = c => b = c/a)

// Example oxidius program
// axiom: anonymous within the scope

// axiom := $(a = b) => $(b = a)
// axiom := $(a + b = c) => $(a = c - b)
auto program = R"(
X := Y;
commutativity := $(a + b = b + a);
E := $(x + y + z);
transform(E, commutativity, $(a * b));
)";

// Context for any session
struct Oxidius {
	scoped_memory_manager smm;

	void drop_rvalue(RValue &rv) {
		if (rv.is <Statement> ())
			rv.as <Statement> ().drop(smm);

		if (rv.is <Expression> ())
			rv.as <Expression> ().drop(smm);
	}

	void run(const std::string &program) {
		auto tokens = lex(program);
		auto actions = TokenStreamParser(tokens, 0).parse();

		for (auto action : actions) {
			// TODO: use std visit with Oxidius itself
			if (action.is <DefineSymbol> ()) {
				auto ds = action.as <DefineSymbol> ();
				drop_rvalue(ds.value);
				fmt::print("assigned to {} ", ds.identifier);
				if (ds.value.is <Statement> ()) {
					fmt::println("-> {}", ds.value.as <Statement> ());
					// ds.value.as <Statement> ().drop(smm);
				} else if (ds.value.is <Expression> ()){
					fmt::println("-> {}", ds.value.as <Expression> ());
					// ds.value.as <Expression> ().drop(smm);
				} else {
					fmt::println("-> {}", ds.value.as <Symbol> ());
				}
			} else if (action.is <Call> ()) {
				auto call = action.as <Call> ();

				fmt::println("call with {}", call.ftn);

				// TODO: format...
				for (auto &rv : call.args) {
					fmt::println("  arg: {}", rv);
					drop_rvalue(rv);
				}
			} else {
				fmt::println("unsupported action...");
			}
		}
	}
};

int main()
{
	auto tokens = lex(program);
	fmt::println("tokens:");
	for (const auto &t : tokens)
		fmt::print("{} ", t);
	fmt::println("");

	Oxidius context;
	context.run(program);

	fmt::println("sizes:");
	fmt::println("  Atom: {} bytes", sizeof(Atom));
	fmt::println("  ETN: {} bytes", sizeof(ETN));
	fmt::println("  Signature: {} bytes", sizeof(Signature));
	fmt::println("  Expression: {} bytes", sizeof(Expression));
	fmt::println("  Statement: {} bytes", sizeof(Statement));

	// scoped_memory_manager gsmm;
	//
	// auto axiom = Statement::from("a + b = b + a")
	// 	.value()
	// 	.drop(gsmm);
	//
	// auto expr = Expression::from("a * x + b")
	// 	.value()
	// 	.drop(gsmm);
	//
	// auto sub = match(axiom.lhs, expr)
	// 	.value()
	// 	.drop(gsmm);
	//
	// auto subbed = sub.apply(axiom.rhs).drop(gsmm);
	//
	// fmt::println("original: {}", expr);
	// fmt::println("subbed: {}", subbed);
}
