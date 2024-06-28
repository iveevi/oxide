#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/formalism.hpp"
#include "include/lex.hpp"
#include "include/format.hpp"

int main()
{
	{
		auto expr = Expression::from("a * x + b").value();
		fmt::println("expr: {}", expr);

		auto statement = Statement::from("a * x + b = c * x + d").value();
		fmt::println("stmt: {}", statement);
	}
}
