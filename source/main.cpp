#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>
#include <type_traits>

#include "include/formalism.hpp"
#include "include/format.hpp"
#include "include/match.hpp"
#include "include/memory.hpp"

int main()
{
	scoped_memory_manager gsmm;

	{
		scoped_memory_manager smm;

		auto tokens = lex("a * x + b [a: Z, b: Z]");
		fmt::println("tokens:");
		for (const auto &t : tokens)
			fmt::print("{} ", t);
		fmt::println("");

		auto expr = Expression::from("a * x + b [a: Z, b: Z]").value();
		fmt::println("expr: {}", expr);

		smm.drop(expr);

		auto statement = Statement::from("a * x + b = c * x + d").value();
		fmt::println("stmt: {}", statement);

		smm.drop(statement);
	}

	{
		scoped_memory_manager smm;

		auto statement = Statement::from("a * x + b = c * x + d [a: R, x: Z, c: R]").value();
		fmt::println("stmt: {}", statement);

		smm.drop(statement);
	}

	{
		auto A = Expression::from("a + b")
			.value()
			.drop(gsmm);

		auto B = Expression::from("k + 2 * x")
			.value()
			.drop(gsmm);

		auto sub = match(A, B)
			.value();
			// .drop(gsmm);

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
