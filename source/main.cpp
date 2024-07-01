#include <functional>

#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/action.hpp"
#include "include/formalism.hpp"
#include "include/format.hpp"
#include "include/lex.hpp"
#include "include/parse.hpp"
#include "include/match.hpp"
#include "include/memory.hpp"
#include "include/std.hpp"
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
commutativity := $(a + b = b + a);
E := $(x + y + z);
transform(E, commutativity);
)";

// Explicit void type for distinction
struct Void {};

// TODO: tuple type
struct Error {};

using _result_base = auto_variant <
	Void, RValue, Error
>;

struct Result : _result_base {
	using _result_base::_result_base;
};

Result _transform(const Expression &expr, const Statement &stmt)
{
	scoped_memory_manager smm;

	fmt::println("transform: on {}", expr);

	auto opt_sub_lhs = match(stmt.lhs, expr);
	if (opt_sub_lhs) {
		auto sub_lhs = opt_sub_lhs.value().drop(smm);
		fmt::println("lhs sub:");
		for (const auto &[s, e] : sub_lhs)
			fmt::println("  {} -> {}", s, e);

		auto subbed = sub_lhs.apply(stmt.rhs).drop(smm);
		fmt::println("result: {}", subbed);
	}

	auto opt_sub_rhs = match(stmt.rhs, expr);
	if (opt_sub_rhs) {
		auto sub_rhs = opt_sub_rhs.value().drop(smm);
		fmt::println("rhs sub:");
		for (const auto &[s, e] : sub_rhs)
			fmt::println("  {} -> {}", s, e);

		auto subbed = sub_rhs.apply(stmt.lhs).drop(smm);
		fmt::println("result: {}", subbed);
	}

	return Void();
}

using Function = std::function <Result (const std::vector <Symbolic> &)>;

// TODO: automatic overload && argc resolver
Result transform(const std::vector <Symbolic> &args)
{
	if (args.size() != 2) {
		fmt::println("transform expected two arguments");
		return Error();
	}

	auto rv1 = args[0];
	auto rv2 = args[1];

	if (!rv1.is <Expression> () || !rv2.is <Statement> ()) {
		fmt::println("transform expected (expr, stmt)");
		return Error();
	}

	auto expr = rv1.as <Expression> ();
	auto stmt = rv2.as <Statement> ();

	return _transform(expr, stmt);
}

// Assigning general values to symbols
// (1) UnresolvedValue -> includes Symbols
// (2) ResolvedValue -> no Symbols
using SymbolTable = std::unordered_map <Symbol, Symbolic>;

struct _assignment_dispatcher {
	SymbolTable &table;
	const Symbol &symbol;

	Result operator()(const Statement &stmt) {
		// TODO: error on existing (immutable)
		table[symbol] = stmt;
		return Void();
	}

	Result operator()(const Expression &expr) {
		table[symbol] = expr;
		return Void();
	}

	template <typename T>
	Result operator()(const T &) {
		// TODO: way to print r-value types...
		fmt::println("cannot assign type ??");
		return Error();
	}
};

struct _drop_dispatcher {
	scoped_memory_manager &smm;

	void operator()(DefineSymbol &ds) {
		drop_rvalue(ds.value);
	}

	void operator()(Call &call) {
		for (auto &arg : call.args)
			drop_rvalue(arg);
	}

	template <typename T>
	void operator()(const T &) {
		fmt::println("drop not implemented for this type...");
	}

	void drop_rvalue(RValue &rv) {
		if (rv.is <Statement> ())
			rv.as <Statement> ().drop(smm);

		if (rv.is <Expression> ())
			rv.as <Expression> ().drop(smm);
	}
};

// Set of functions
static std::unordered_map <Symbol, Function> functions {
	{ "transform", transform }
};

// Context for any session
struct Oxidius {
	scoped_memory_manager smm;

	SymbolTable table;

	auto_optional <Symbolic> resolve_rvalue(const RValue &rv) {
		if (rv.is <Symbol> ()) {
			Symbol sym = rv.as <Symbol> ();
			if (!table.contains(sym)) {
				fmt::println("symbol {} not defined", sym);
				return std::nullopt;
			}

			return table[sym];
		} else if (rv.is <Expression> ()) {
			return rv.as <Expression> ();
		} else if (rv.is <Statement> ()) {
			return rv.as <Statement> ();
		}

		return std::nullopt;

		// TODO: translate with enable ifs...
		// return rv.translate(Symbolic());
	}

	void run(const std::string &program) {
		auto tokens = lex(program);
		auto actions = TokenStreamParser(tokens, 0).parse();

		// TODO: first drop all actions
		for (auto &action : actions)
			std::visit(_drop_dispatcher(smm), action);

		for (auto action : actions) {
			// TODO: use std visit with Oxidius itself
			if (action.is <DefineSymbol> ()) {
				auto ds = action.as <DefineSymbol> ();
				fmt::println("assigned {} -> {}", ds.identifier, ds.value);
				_assignment_dispatcher ad(table, ds.identifier);
				auto value = resolve_rvalue(ds.value);
				if (!value)
					break;
				auto result = std::visit(ad, value.value());
				if (result.is <Error> ())
					break;
			} else if (action.is <Call> ()) {
				auto call = action.as <Call> ();

				if (!functions.contains(call.ftn)) {
					fmt::println("no function {} defined", call.ftn);
					break;
				}

				Result result = Void();
				std::vector <Symbolic> resolved;

				fmt::println("call with {}", call.ftn);
				for (auto &rv : call.args) {
					fmt::println("  arg: {}", rv);
					auto rrv = resolve_rvalue(rv);
					if (!rrv) {
						result = Error();
						break;
					}

					resolved.push_back(rrv.value());
				}

				if (result.is <Error> ())
					break;

				result = functions[call.ftn](resolved);
				if (result.is <Error> ())
					break;

				// TODO: otherwise...
			} else {
				fmt::println("unsupported action...");
			}
		}
	}
};

int main()
{
	Oxidius context;
	context.run(program);

	fmt::println("sizes:");
	fmt::println("  Atom: {} bytes", sizeof(Atom));
	fmt::println("  ETN: {} bytes", sizeof(ETN));
	fmt::println("  Signature: {} bytes", sizeof(Signature));
	fmt::println("  Expression: {} bytes", sizeof(Expression));
	fmt::println("  Statement: {} bytes", sizeof(Statement));
}
