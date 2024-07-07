#include <queue>

#include "include/format.hpp"
#include "include/formalism.hpp"

// Standard comparators
std::vector <Comparator> Comparator::list { { "=" } };

// Signature
bool add_signature(Signature &S, const std::string &sym, Domain dom)
{
	if (S.count(sym) && S[sym] != dom) {
		fmt::println("conflicting domain signature for symbol '{}' between {} and {}", sym, S[sym], dom);
		return false;
	}

	S[sym] = dom;
	return true;
}

std::optional <Signature> join(const Signature &A, const Signature &B)
{
	Signature result = A;
	for (const auto &[s, dom] : B) {
		if (!add_signature(result, s, dom))
			return std::nullopt;
	}

	return result;
}

// ETN
std::vector <Symbol> ETN::symbols() const
{
	std::vector <Symbol> symbols;
	std::queue <const ETN *> refs;

	refs.push(this);
	while (refs.size()) {
		auto e = refs.front();
		refs.pop();

		if (e->is <_expr_tree_op> ()) {
			auto etop = e->as <_expr_tree_op> ();

			ETN_ref head = etop.down;
			while (head) {
				refs.push(head);
				head = head->next();
			}
		} else {
			auto eta = e->as <_expr_tree_atom> ().atom;
			if (eta.is <Symbol> ())
				symbols.push_back(eta.as <Symbol> ());
		}
	}

	return symbols;
}

ETN_ref &ETN::next()
{
	if (std::holds_alternative <_expr_tree_atom> (*this))
		return std::get <0> (*this).next;

	return std::get <1> (*this).next;
}

// Expression
std::vector <Symbol> Expression::symbols() const
{
	return etn->symbols();
}

// Statement
std::vector <Symbol> Statement::symbols() const
{
	auto sl = lhs.symbols();
	auto sr = rhs.symbols();
	sl.insert(sl.end(), sr.begin(), sr.end());
	return sl;
}
