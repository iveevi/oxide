#include <iostream>
#include <type_traits>
#include <functional>

#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/action.hpp"
#include "include/formalism.hpp"
#include "include/format.hpp"
#include "include/function.hpp"
#include "include/hash.hpp"
#include "include/lex.hpp"
#include "include/match.hpp"
#include "include/memory.hpp"
#include "include/parse.hpp"
#include "include/std.hpp"
#include "include/types.hpp"

// TODO: scoring system to incentivize
// generating f(x)=0 -> x=g(...) solutions
// and probably other forms

// Expample derivation: a * x + b = 0 => x = (0 - b)/a
// a * x + b = 0
// a * x = 0 - b <- (a + b = 0 => a = 0 - b)
// x = (0 - b)/a <- (a * b = c => b = c/a)

// Example oxidius program
// axiom: anonymous within the scope

// axiom := $(a = b) => $(b = a)
// axiom := $(a + b = c) => $(a = c - b)
// TODO: # for comments

void _transform(ExprTable_L1 &table, const Expression &expr, const Statement &stmt, push_marker &pm, bool exhaustive, int depth)
{
	if (depth == 0)
		return;

	push_marker novel;

	// The original expression itself goes in here
	table.push(expr, pm);

	// For now there is nothing to do for atoms
	if (expr.etn->is <_expr_tree_atom> ())
		return;

	scoped_memory_manager smm;
	auto opt_sub_lhs = match(stmt.lhs, expr);
	if (opt_sub_lhs) {
		auto sub_lhs = opt_sub_lhs.value().drop(smm);
		auto subbed = sub_lhs.apply(stmt.rhs).drop(table.smm);
		table.push(subbed, novel);
	}

	auto opt_sub_rhs = match(stmt.rhs, expr);
	if (opt_sub_rhs) {
		auto sub_rhs = opt_sub_rhs.value().drop(smm);
		auto subbed = sub_rhs.apply(stmt.lhs).drop(table.smm);
		table.push(subbed, novel);
	}

	// Trying all children as well
	// TODO: use the same table, create a mark stack, then clear it all
	std::vector <push_marker> markers;

	expr.etn->forall_operands(
		[&](const ETN_ref &child) {
			// TODO: subsignature if small enough?
			push_marker pm;
			Expression cexpr { child, expr.signature };
			_transform(table, cexpr, stmt, pm, exhaustive, std::max(depth - 1, -1));
			markers.push_back(pm);
		}
	);

	// Run through all permutations
	// TODO: specialization for low operand counts
	if (markers.size() != 2) {
		fmt::println("subexpression transforms are only supported for binary ops");
	} else {
		// TODO: product function
		for (size_t i : markers[0]) {
			for (size_t j : markers[1]) {
				const auto &exlhs = table.flat_at(i);
				const auto &exrhs = table.flat_at(j);

				auto lhs = clone(exlhs.etn);
				auto rhs = clone(exrhs.etn);
				auto top = clone_soft(expr.etn);

				top->as <_expr_tree_op> ().down = lhs;
				lhs->next() = rhs;

				Expression combined { top, expr.signature };
				table.push(combined, novel);
				table.smm.drop(top);
			}
		}
	}

	// If exhaustive, repeat the seach over novel expressions
	if (exhaustive) {
		for (size_t i : novel) {
			Expression expr = table.flat_at(i);
			_transform(table, expr, stmt, pm, exhaustive, depth);
		}
	}

	// Record the novel expressions as well
	pm.insert(pm.end(), novel.begin(), novel.end());

	// Erase generated expressions
	for (const auto &pm : markers)
		table.clear(pm);
}

Result transform(const std::vector <RValue> &args, const Options &options)
{
	if (auto expr_stmt = overload <Expression, Statement> (args)) {
		auto [expr, stmt] = expr_stmt.value();

		Integer depth = check_option(options, "depth", (Integer) -1);
		bool exhaustive = check_option(options, "exhaustive", true);

		ExprTable_L1 table;
		push_marker pm;
		_transform(table, expr, stmt, pm, exhaustive, depth);
		fmt::println("# of expressions generated: {}", table.unique);
		list_table(table);
		return Void();
	}

	// TODO: pass error message to string
	fmt::println("transform expected (expr, stmt)");
	return Error();
}

// Assigning general values to symbols
using SymbolTable = std::unordered_map <Symbol, RValue>;

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
		smm.drop(ds.value);
	}

	void operator()(Call &call) {
		for (auto &arg : call.args)
			smm.drop(arg);
	}

	template <typename T>
	void operator()(const T &) {
		fmt::println("drop not implemented for this type...");
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

	Options options;

	auto_optional <RValue> resolve_rvalue(const RValue &rv) {
		if (rv.is <Symbol> ()) {
			Symbol sym = rv.as <Symbol> ();
			if (!table.contains(sym)) {
				fmt::println("symbol {} not defined", sym);
				return std::nullopt;
			}

			return table[sym];
		}

		return rv;
	}

	Result operator()(const DefineSymbol &ds) {
		fmt::println("assigned {} -> {}", ds.identifier, ds.value);
		_assignment_dispatcher ad(table, ds.identifier);
		auto value = resolve_rvalue(ds.value);
		return value ? std::visit(ad, value.value()) : Error();
	}

	Result operator()(const Call &call) {
		if (!functions.contains(call.ftn)) {
			fmt::println("no function {} defined", call.ftn);
			return Error();
		}

		std::vector <RValue> resolved;
		for (auto &rv : call.args) {
			auto rrv = resolve_rvalue(rv);
			if (!rrv)
				return Error();

			resolved.push_back(rrv.value());
		}

		return functions[call.ftn](resolved, options)
			.passthrough([&]() {
				options.clear();
			});
	}

	Result operator()(const PushOption &option) {
		fmt::println("option: {} -- {}", option.name, option.arg);
		options[option.name] = option.arg;
		return Void();
	}

	template <typename T>
	requires std::is_constructible_v <Action, T>
	Result operator()(const T &) {
		fmt::println("action is not yet implemented!");
		return Error();
	}

	void run(const std::string &program) {
		auto tokens = lex(program);
		auto actions = TokenStreamParser(tokens, 0).parse();

		// Memory management
		for (auto &action : actions)
			std::visit(_drop_dispatcher(smm), action);

		for (auto action : actions) {
			if (std::visit(*this, action).is <Error> ())
				break;
		}
	}
};

inline const std::string readfile(const std::filesystem::path &path)
{
	std::ifstream f(path);
	if (!f.good()) {
		fmt::println("failed to find file: {}", path);
		return "";
	}

	std::stringstream s;
	s << f.rdbuf();
	return s.str();
}

int main()
{
	static const std::filesystem::path program = "programs/experimental.oxide";

	Oxidius context;
	context.run(readfile(program));
}
