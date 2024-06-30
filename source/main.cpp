#include <functional>

#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>
#include <type_traits>

#include "include/formalism.hpp"
#include "include/format.hpp"

using Substitution = std::unordered_map <Symbol, Expression>;

bool operator==(const Integer &A, const Integer &B)
{
	return A.value == B.value;
}

bool operator==(const Real &A, const Real &B)
{
	return A.value == B.value;
}

bool equal(const Atom &A, const Atom &B)
{
	if (A.index() != B.index())
		return false;

	struct _equality_dispatcher {
		const Atom &other;

		bool operator()(const Integer &i) {
			return other.is <Integer> ()
				&& (other.as <Integer> () == i);
		}

		bool operator()(const Real &r) {
			return other.is <Symbol> ()
				&& (other.as <Real> () == r);
		}

		bool operator()(const Symbol &sym) {
			return other.is <Symbol> ()
				&& (other.as <Symbol> () == sym);
		}
	};

	_equality_dispatcher ed(A);
	return std::visit(ed, B);
}

bool equal(const ETN_ref &A, const ETN_ref &B)
{
	if (A->index() != B->index())
		return false;

	if (A->is <_expr_tree_atom> ()) {
		auto atom_A = A->as <_expr_tree_atom> ().atom;
		auto atom_B = B->as <_expr_tree_atom> ().atom;

		return equal(atom_A, atom_B);
	} else {
		auto tree_A = A->as <_expr_tree_op> ();
		auto tree_B = B->as <_expr_tree_op> ();

		Operation op_A = tree_A.op;
		Operation op_B = tree_B.op;

		if (op_A != op_B)
			return false;

		ETN_ref head_A = tree_A.down;
		ETN_ref head_B = tree_B.down;

		while (head_A && head_B) {
			if (!equal(head_A, head_B))
				return false;

			head_A = head_A->next();
			head_B = head_B->next();
		}

		return true;
	}
}

bool equal(const Expression &A, const Expression &B)
{
	return equal(A.etn, B.etn);
}

std::optional <Substitution> add_substitution(const Substitution &S, const Symbol sym, const Expression &expr)
{
	Substitution result = S;

	if (result.count(sym) && !equal(result[sym], expr)) {
		fmt::println("incompatible matching:\n{}={}vs.\n{}", sym, *result[sym].etn, *expr.etn);
		return std::nullopt;
	}

	// TODO: clone...
	result[sym] = expr;

	return result;
}

std::optional <Substitution> join(const Substitution &A, const Substitution &B)
{
	Substitution result = A;
	for (const auto &[s, expr] : B) {
		auto r = add_substitution(result, s, expr);
		if (!r)
			return std::nullopt;

		result = r.value();
	}

	return result;
}

std::optional <Substitution> match(const ETN_ref &source, const ETN_ref &victim)
{
	fmt::println("checking between A:\n{}", *source);
	fmt::println("and B:\n{}", *victim);

	if (source->has_op()) {
		if (!victim->has_op())
			return std::nullopt;

		auto tree_source = std::get <_expr_tree_op> (*source);
		auto tree_victim = std::get <_expr_tree_op> (*victim);

		Operation op_source = tree_source.op;
		Operation op_victim = tree_victim.op;

		if (op_source != op_victim) {
			return std::nullopt;
		} else {
			fmt::println("matching operations, proceeding onwards...");

			Substitution sub;

			ETN_ref source_head = tree_source.down;
			ETN_ref victim_head = tree_victim.down;

			while (source_head && victim_head) {
				auto opt_sub_child = match(source_head, victim_head);
				if (!opt_sub_child)
					return std::nullopt;

				auto sub_child = opt_sub_child.value();

				auto joined = join(sub, sub_child);
				if (!joined)
					return std::nullopt;

				sub = joined.value();

				source_head = source_head->next();
				victim_head = victim_head->next();
			}

			// TODO: check uneven sizes...

			return sub;
		}
	} else {
		auto atom_source = source->as <_expr_tree_atom> ().atom;
		// TODO: custom variant with is() and as()
		if (atom_source.is <Symbol> ()) {
			fmt::println("FREE SYMBOL");
			Symbol s = atom_source.as <Symbol> ();
			fmt::println("assigning\n{}\nto free symbol {}", *victim, s);

			// TODO: clone
			Expression matched {
				victim,
				default_signature(*victim)
			};

			return Substitution { {s, matched} };
		}
	}

	return std::nullopt;
}

std::optional <Substitution> match(const Expression &source, const Expression &victim)
{
	return match(source.etn, victim.etn);
}

int main()
{
	if (false) {
		auto tokens = lex("a * x + b [a: Z, b: Z]");
		fmt::println("tokens:");
		for (const auto &t : tokens)
			fmt::print("{} ", t);
		fmt::println("");

		auto expr = Expression::from("a * x + b [a: Z, b: Z]").value();
		fmt::println("expr: {}", expr);

		auto statement = Statement::from("a * x + b = c * x + d").value();
		fmt::println("stmt: {}", statement);
	}

	if (false) {
		auto statement = Statement::from("a * x + b = c * x + d [a: R, x: Z, c: R]").value();
		fmt::println("stmt: {}", statement);
	}

	{
		auto A = Expression::from("a + b").value();
		auto B = Expression::from("k + 2 * x").value();
		auto sub = match(A, B).value();

		fmt::println("substitution:");
		for (const auto &[sym, expr] : sub)
			fmt::println("  sym: {} -> expr: {}", sym, expr);
	}

	// Types of matches between statements
	// 1. Full
	// 2. Full reverse
	// 3. Half (LL)
	// 4. Half (LR)
	// 5. Half (RL)
	// 6. Half (RR)
}
