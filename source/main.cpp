#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/lex.hpp"
#include "include/format.hpp"

#include <queue>
#include <stack>
#include <type_traits>
#include <variant>

struct _rpe_vector_dispatcher {
	std::vector <RPE> &finish;
	std::stack <Operation> &operators;

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

	template <typename T>
	void operator()(const T &t) {
		fmt::println("unexpected token encountered: {}", t);
	}
};

// TODO: supply the domain specification for function lookups
std::vector <RPE> rpe_vector(const std::vector <token> &lexed)
{
	// Shunting Yards
	std::vector <RPE> finish;
	std::stack <Operation> operators;

	std::deque <token> tokens {
		lexed.begin(),
		lexed.end()
	};

	_rpe_vector_dispatcher rvd(finish, operators);
	while (tokens.size()) {
		auto t = tokens.front();
		tokens.pop_front();
		std::visit(rvd, t);
	}

	while (operators.size() && operators.top() != none)
		rvd(none);

	return finish;
}

struct _rpe_to_etn_dispatcher {
	std::stack <ETN> &operands;

	// template <typename T>
	// requires std::is_constructible_v <Atom, T>
	// void operator()(const T &t) {
	// }

	// void operator()(const Operation &op) {
	// }

	template <typename T>
	void operator()(const T &t) {
		fmt::println("unexpected token encountered: {}", t);
	}
};

// TODO: custom allocator for more coherent trees
ETN_ref rpes_to_etn(const std::vector <RPE> &rpes)
{
	std::stack <ETN> operands;

	std::deque <RPE> queued {
		rpes.begin(),
		rpes.end()
	};

	_rpe_to_etn_dispatcher red(operands);
	while (queued.size()) {
		auto r = queued.front();
		queued.pop_front();

		fmt::println("operand stack: {}", operands.size());

		if (r.has_atom()) {
			operands.push(r.atom());

			fmt::println("constructed ETN (atom) for: {}", r.atom());
		} else {
			if (operands.size() < 2) {
				fmt::println("expected at least two operands for operation: {}", token(r.op()));
				return nullptr;
			}

			// Assume binary for now
			ETN_ref lhs = new ETN(operands.top());
			operands.pop();

			ETN *rhs = new ETN(operands.top());
			operands.pop();

			lhs->next() = rhs;

			ETN opt = expr_tree_op {
				.op = r.op(),
				.down = lhs,
				.next = nullptr
			};

			operands.push(opt);

			fmt::println("constructed ETN (tree) for operation: {}", token(r.op()));
		}
	}

	auto root = operands.top();
	return new ETN(root);
}

int main()
{
	Scalar <Integer> scalar;
	Variable x = Variable::from <Integer> (0);

	auto tokens = lex("123 + a / x_b * 5.3").value_or(std::vector <token> {});

	fmt::println("tokens: {}", tokens);

	auto rpes = rpe_vector(tokens);
	fmt::println("rpe vector:");
	for (auto rpe : rpes)
		fmt::println("  {}", rpe);

	rpes_to_etn(rpes);
}
