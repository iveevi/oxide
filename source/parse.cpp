#include <vector>
#include <deque>
#include <stack>

#include "include/format.hpp"
#include "include/lex.hpp"
#include "include/formalism.hpp"

// Element in a reverse polish stack
struct RPE : std::variant <Atom, Operation> {
	using std::variant <Atom, Operation> ::variant;

	bool has_atom() const {
		return std::holds_alternative <Atom> (*this);
	}

	bool has_op() const {
		return std::holds_alternative <Operation> (*this);
	}

	Atom atom() {
		return std::get <0> (*this);
	}

	Operation op() {
		return std::get <1> (*this);
	}
};

struct _rpe_vector_dispatcher {
	std::vector <RPE> &finish;
	std::stack <Operation> &operators;
	std::deque <Token> &tokens;

	template <typename T>
	requires std::is_constructible_v <Atom, T>
	void operator()(const T &t) {
		finish.push_back(t);
	}

	void operator()(const Operation &op) {
		while (operators.size()) {
			auto top = operators.top();
			if (top <= op)
				break;

			operators.pop();
			finish.push_back(top);

			fmt::println("popped operation: {}", (int) top);
		}

		operators.push(op);
	}

	// TODO: if equals, then clear tokens and proceed to final stage...
	void operator()(const Equals &) {
		// Stop parsing, but leave output queue and operators intact
		tokens.clear();
	}

	template <typename T>
	void operator()(const T &t) {
		fmt::println("unexpected token encountered: {}", t);

		// Stop parsing
		tokens.clear();
		finish.clear();

		while (operators.size())
			operators.pop();
	}
};

// TODO: supply the domain specification for function lookups
using RPE_vector = std::vector <RPE>;

RPE_vector rpe_vector(const std::vector <Token> &lexed, size_t pos = 0)
{
	if (pos >= lexed.size())
		return {};

	// Shunting Yards
	std::vector <RPE> finish;
	std::stack <Operation> operators;

	std::deque <Token> tokens {
		lexed.begin() + pos,
		lexed.end()
	};

	_rpe_vector_dispatcher rvd(finish, operators, tokens);
	while (tokens.size()) {
		auto t = tokens.front();
		tokens.pop_front();
		std::visit(rvd, t);
	}

	while (operators.size() && operators.top() != none)
		rvd(none);

	return finish;
}

// TODO: custom allocator for more coherent trees
ETN_ref rpes_to_etn(const std::vector <RPE> &rpes)
{
	std::stack <ETN> operands;

	std::deque <RPE> queued {
		rpes.begin(),
		rpes.end()
	};

	while (queued.size()) {
		auto r = queued.front();
		queued.pop_front();

		fmt::println("operand stack: {}", operands.size());

		if (r.has_atom()) {
			operands.push(r.atom());

			fmt::println("constructed ETN (atom) for: {}", r.atom());
		} else {
			if (operands.size() < 2) {
				fmt::println("expected at least two operands for operation: {}", Token(r.op()));
				return nullptr;
			}

			// Assume binary for now
			ETN_ref rhs = new ETN(operands.top());
			operands.pop();

			ETN_ref lhs = new ETN(operands.top());
			operands.pop();

			lhs->next() = rhs;

			ETN opt = expr_tree_op {
				.op = r.op(),
				.down = lhs,
				.next = nullptr
			};

			operands.push(opt);

			fmt::println("constructed ETN (tree) for operation: {}", Token(r.op()));
		}
	}

	auto root = operands.top();
	return new ETN(root);
}

std::optional <Expression> Expression::from(const std::string &s)
{
	auto tokens = lex(s);

	RPE_vector rpev = rpe_vector(tokens, 0);
	if (rpev.empty()) {
		fmt::println("empty RPE vector");
		return std::nullopt;
	}

	if (rpev.empty() || rpev.size() != tokens.size()) {
		fmt::println("failed to fully parse expression");
		return std::nullopt;
	}

	ETN_ref etn = rpes_to_etn(rpev);
	if (!etn) {
		fmt::println("error in constructing ETN");
		return std::nullopt;
	}

	return Expression {
		.etn = etn,
		.signature = default_signature(*etn)
	};
}

std::optional <Statement> Statement::from(const std::string &s)
{
	auto tokens = lex(s);

	RPE_vector lhs_rpev = rpe_vector(tokens, 0);
	if (lhs_rpev.empty()) {
		fmt::println("empty RPE vector (lhs)");
		return std::nullopt;
	}

	size_t offset = lhs_rpev.size();

	Token middle = tokens[offset++];
	if (!std::holds_alternative <Equals> (middle)) {
		fmt::println("only supporting statements of equality (=)");
		return std::nullopt;
	}

	RPE_vector rhs_rpev = rpe_vector(tokens, offset);
	if (rhs_rpev.empty()) {
		fmt::println("empty RPE vector (rhs)");
		return std::nullopt;
	}

	ETN_ref lhs = rpes_to_etn(lhs_rpev);
	if (!lhs) {
		fmt::println("error in constructing ETN (lhs)");
		return std::nullopt;
	}

	ETN_ref rhs = rpes_to_etn(rhs_rpev);
	if (!rhs) {
		fmt::println("error in constructing ETN (rhs)");
		return std::nullopt;
	}

	// TODO: safe join
	auto sl = default_signature(*lhs);
	auto sr = default_signature(*rhs);

	return Statement {
		.lhs = Expression { lhs, sl },
		.rhs = Expression { rhs, sr },
		.cmp = eq,
		.signature = join(sl, sr).value()
	};
}
