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
		finish.push_back(RPE(t));
	}

	void operator()(const Operation &op) {
		while (operators.size()) {
			auto top = operators.top();
			if (top <= op)
				break;

			operators.pop();
			finish.push_back(RPE(top));

			fmt::println("popped operation: {}", (int) top);
		}

		operators.push(op);
	}

	template <typename T>
	void operator()(const T &t) {
		fmt::println("unexpected token encountered: {}", t);
	}
};

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
}
