#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/formalism.hpp"
#include "include/format.hpp"

int main()
{
	{
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

	{
		auto statement = Statement::from("a * x + b = c * x + d [a: R, x: Z, c: R]").value();
		fmt::println("stmt: {}", statement);
	}

	// Types of matches between statements
	// 1. Full
	// 2. Full reverse
	// 3. Half (LL)
	// 4. Half (LR)
	// 5. Half (RL)
	// 6. Half (RR)
}
