#include <queue>

#include "include/format.hpp"
#include "include/formalism.hpp"

// Signature
std::optional <Signature> join(const Signature &A, const Signature &B)
{
	Signature result = A;
	for (const auto &[s, dom] : B) {
		if (result.count(s) && result[s] != dom) {
			fmt::println("conflicting domain signature for symbol '{}' between {} and {}", s, result[s], dom);
			return std::nullopt;
		}

		result[s] = dom;
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
