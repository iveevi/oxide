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
		auto A = Expression::from("a + b")
			.value()
			.drop(gsmm);

		auto B = Expression::from("k + 2 * x")
			.value()
			.drop(gsmm);

		auto C = Expression::from("a * b")
			.value()
			.drop(gsmm);

		auto sub = match(A, B)
			.value()
			.drop(gsmm);

		fmt::println("A := {}", A);
		fmt::println("B := {}", B);
		fmt::println("C := {}", C);

		fmt::println("substitution:");
		for (auto &[sym, expr] : sub)
			fmt::println("  sym: {} -> expr: {}", sym, expr);

		auto D = sub.apply(C).drop(gsmm);

		fmt::println("D := {}", D);
	}
}
