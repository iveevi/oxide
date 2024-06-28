#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/formalism.hpp"
#include "include/lex.hpp"
#include "include/format.hpp"

#include <queue>
#include <stack>
#include <type_traits>
#include <variant>

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

// std::vector <RPE_vector> rpe_vector_sequence(const std::vector <Token> &lexed)
// {
// 	std::vector <RPE_vector> result;
//
// 	size_t pos = 0;
// 	while (true) {
// 		auto rpev = rpe_vector(lexed, pos);
// 		if (rpev.empty())
// 			break;
//
// 		result.push_back(rpev);
// 		pos += rpev.size() + 1;
// 	}
//
// 	return result;
// }

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

	return Expression { etn };
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

	return Statement { lhs, rhs, eq };
}

int main()
{
	Scalar <Integer> scalar;
	Variable x = Variable::from <Integer> (0);

	if (false) {
		auto tokens = lex("123 + a / x_b * 5.3");

		fmt::println("tokens: {}", tokens);

		auto rpev = rpe_vector(tokens);
		fmt::println("rpe vector:");
		for (auto rpe : rpev)
			fmt::println("  {}", rpe);

		auto *etn = rpes_to_etn(rpev);

		fmt::println("etn:\n{}", *etn);
	}

	{
		auto tokens = lex("a * x + b = 0");

		fmt::println("tokens: {}", tokens);

		auto expr = Expression::from("a * x + b");
		fmt::println("expr:\n{}", *expr->etn);

		auto statement = Statement::from("a * x + b = c * x + d");
		fmt::println("lhs:\n{}", *statement->lhs.etn);
		fmt::println("rhs:\n{}", *statement->rhs.etn);
	}
}
