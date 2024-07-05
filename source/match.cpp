#include "include/match.hpp"
#include "include/formalism.hpp"
#include "include/format.hpp"
#include "include/memory.hpp"
#include "include/types.hpp"

// Comparators
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

// Finding matches
std::optional <Substitution> add_substitution(const Substitution &S, const Symbol &sym, const Expression &expr)
{
	Substitution result = S;

	if (result.count(sym) && !equal(result[sym], expr)) {
		fmt::println("incompatible matching:\n{}={}vs.\n{}", sym, *result[sym].etn, *expr.etn);
		return std::nullopt;
	}

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
	scoped_memory_manager smm;
	if (source->is <_expr_tree_op> ()) {
		if (!victim->is <_expr_tree_op> ())
			return std::nullopt;

		auto tree_source = std::get <_expr_tree_op> (*source);
		auto tree_victim = std::get <_expr_tree_op> (*victim);

		Operation op_source = tree_source.op;
		Operation op_victim = tree_victim.op;

		if (op_source != op_victim) {
			return std::nullopt;
		} else {
			Substitution sub;

			ETN_ref source_head = tree_source.down;
			ETN_ref victim_head = tree_victim.down;

			std::vector <ETN_ref> freeze;
			while (source_head && victim_head) {
				auto opt_sub_child = match(source_head, victim_head);
				if (!opt_sub_child) {
					smm.drop(sub);
					return std::nullopt;
				}

				auto sub_child = opt_sub_child.value();

				auto joined = join(sub, sub_child);
				if (!joined) {
					smm.drop(sub);
					return std::nullopt;
				}

				sub = joined.value();

				source_head = source_head->next();
				victim_head = victim_head->next();
			}

			// TODO: check uneven sizes...

			return sub;
		}
	} else {
		auto atom_source = source->as <_expr_tree_atom> ().atom;

		if (atom_source.is <Symbol> ()) {
			Symbol s = atom_source.as <Symbol> ();

			Expression matched {
				clone(victim),
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

// Substitution methods
ETN_ref Substitution::apply(const ETN_ref &etn)
{
	if (etn->is <_expr_tree_op> ()) {
		auto tree = etn->as <_expr_tree_op> ();

		ETN_ref netn = new ETN(tree);

		ETN_ref head = tree.down;

		std::queue <ETN_ref> list;
		while (head) {
			list.push(apply(head));
			head = head->next();
		}

		ETN_ref nhead = list.front();
		list.pop();

		ETN_ref p = nhead;
		while (list.size()) {
			ETN_ref q = list.front();
			list.pop();

			p->next() = q;
			p = q;
		}

		netn->as <_expr_tree_op> ().down = nhead;

		return netn;
	} else {
		// NOTE: When cloning, we need to null the .next field
		// since nodes are not necessarily in the same order
		auto atom = etn->as <_expr_tree_atom> ().atom;
		if (atom.is <Symbol> ()) {
			Symbol sym = atom.as <Symbol> ();
			if (contains(sym)) {
				ETN_ref setn = clone(this->operator[](sym).etn);
				setn->next() = nullptr;
				return setn;
			}
		}

		ETN_ref setn = clone(etn);
		setn->next() = nullptr;
		return setn;
	}
}

Expression Substitution::apply(const Expression &expr)
{
	ETN_ref setn = apply(expr.etn);
	return Expression {
		.etn = setn,
		.signature = default_signature(*setn)
	};
}
