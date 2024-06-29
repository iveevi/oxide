#include <queue>

#include "include/format.hpp"
#include "include/formalism.hpp"

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
bool ETN::has_atom() const
{
	return std::holds_alternative <expr_tree_atom> (*this);
}

bool ETN::has_op() const
{
	return std::holds_alternative <expr_tree_op> (*this);
}

std::vector <Symbol> ETN::symbols() const
{
	using ref_type = std::reference_wrapper <const ETN>;

	std::vector <Symbol> symbols;
	std::queue <ref_type> refs;

	refs.push(std::cref(*this));
	while (refs.size()) {
		auto e = refs.front().get();
		refs.pop();

		if (e.has_op()) {
			auto etop = std::get <1> (e);

			ETN_ref head = etop.down;
			while (head) {
				refs.push(std::cref(*head));
				head = head->next();
			}
		} else {
			auto eta = std::get <0> (e).atom;
			if (std::holds_alternative <Symbol> (eta))
				symbols.push_back(std::get <Symbol> (eta));
		}
	}

	return symbols;
}

ETN_ref &ETN::next()
{
	if (std::holds_alternative <expr_tree_atom> (*this))
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
