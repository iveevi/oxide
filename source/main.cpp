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

// Result transform(const std::vector <Value> &args, const Options &options)
// {
// 	if (auto expr_stmt = overload <Expression, Statement> (args)) {
// 		auto [expr, stmt] = expr_stmt.value();
//
// 		Integer depth = check_option(options, "depth", (Integer) -1);
// 		bool exhaustive = check_option(options, "exhaustive", true);
//
// 		ExprTable_L1 table;
// 		push_marker pm;
// 		_transform(table, expr, stmt, pm, exhaustive, depth);
// 		fmt::println("# of expressions generated: {}", table.unique);
// 		list_table(table);
// 		return Void();
// 	}
//
// 	// TODO: pass error message to string
// 	fmt::println("transform expected (expr, stmt)");
// 	return Error();
// }

Result relation(const std::vector <Value> &args, const Options &)
{
	if (auto lit = overload <LiteralString> (args)) {
		auto [lits] = lit.value();
		Comparator::list.push_back(Comparator(lits));
		return Void();
	}

	fmt::println("relation expected (lit)");
	return Error();
}

// Assigning general values to symbols
using _symtable_base = std::unordered_map <Symbol, Value>;

struct SymbolTable : _symtable_base {
	using _symtable_base::_symtable_base;

	// TODO: std::visit version
	auto_optional <Value> resolve(const UnresolvedValue &v) {
		if (v.is <Symbol> ()) {
			Symbol sym = v.as <Symbol> ();
			if (!contains(sym)) {
				fmt::println("symbol {} not defined", sym);
				return std::nullopt;
			}

			return this->operator[](sym);
		}

		if (v.is <UnresolvedTuple> ()) {
			UnresolvedTuple tuple = v.as <UnresolvedTuple> ();

			Tuple result;
			for (size_t i = 0; i < tuple.size(); i++) {
				auto r = resolve(tuple[i]);
				if (!r)
					return std::nullopt;

				result.push_back(r.value());
			}

			return result;
		}

		if (v.is <UnresolvedArgument> ()) {
			UnresolvedArgument argument = v.as <UnresolvedArgument> ();
			auto predicates = resolve(argument.predicates);
			if (!predicates)
				return std::nullopt;

			if (!predicates->is <Tuple> ()) {
				// TODO: error reporting unit
				// TODO: line, column(s) information for unresolved values;
				// once resolved, tracking is no longer needed
				fmt::println("predicates of an argument must be in a tuple");
				return std::nullopt;
			}

			std::vector <Statement> pstmts;
			for (auto v : predicates.value().as <Tuple> ()) {
				if (!v.is <Statement> ()) {
					fmt::println("arguments can only be made with statements");
					return std::nullopt;
				}

				pstmts.push_back(v.as <Statement> ());
			}

			auto result = resolve(argument.result.translate(UnresolvedValue()));
			if (!result)
				return std::nullopt;

			if (!result->is <Statement> ()) {
				fmt::println("arguments can only be made with statements");
				return std::nullopt;
			}

			return Argument {
				pstmts,
				result.value().as <Statement> ()
			};
		}

		return v.translate(Value());
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
	// { "transform", transform },
	{ "relation", relation },
};

// Context for any session
struct Oxidius {
	scoped_memory_manager smm;
	SymbolTable table;
	Options options;

	Result operator()(const DefineSymbol &ds) {
		auto value = table.resolve(ds.value);
		if (!value)
			return Error();
		fmt::println("value: {}", value.value());
		table[ds.identifier] = value.value();
		return Void();
	}

	Result operator()(const Call &call) {
		if (!functions.contains(call.ftn)) {
			fmt::println("no function {} defined", call.ftn);
			return Error();
		}

		std::vector <Value> resolved;
		for (auto &rv : call.args) {
			auto rrv = table.resolve(rv);
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
		auto rrv = table.resolve(option.arg);
		if (!rrv)
			return Error();

		options[option.name] = rrv.value();
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

		TokenStreamParser parser(tokens, 0);
		while (auto opt_action = parser.parse_statement()) {
			auto action = opt_action.value();
			std::visit(_drop_dispatcher(smm), action);
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
	static const std::filesystem::path program = "programs/experimental.ox";

	Oxidius context;
	context.run(readfile(program));
}
