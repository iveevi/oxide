#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/lex.hpp"
#include "include/format.hpp"

int main()
{
	Scalar <Integer> scalar;
	Variable x = Variable::from <Integer> (0);

	auto tokens = lex("123 + a / x_b * 5.3").value_or(std::vector <token> {});

	fmt::println("tokens: {}", tokens);
}
