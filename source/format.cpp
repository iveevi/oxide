#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>
#include <variant>

#include "include/format.hpp"

struct _fmt_token_dispatcher {
	std::string &ref;

	void operator()(Integer i) {
		ref += fmt::format("{}", i.value);
	}

	void operator()(Real r) {
		ref += fmt::format("{}", r.value);
	}

	void operator()(Symbol symbol) {
		ref += symbol;
	}

	void operator()(Operation op) {
		ref += op_strs[op];
	}

	template <typename T>
	void operator()(T x) {
		ref += "?";
	}

	static constexpr const char *op_strs[] = {
		"none", "+", "-", "*", "/"
	};
};

std::string format_as(const RPE &rpe)
{
	std::string result;

	_fmt_token_dispatcher ftd(result);
	if (std::holds_alternative <Atom> (rpe))
		std::visit(ftd, std::get <0> (rpe));
	else
		ftd(std::get <1> (rpe));

	return result;
}

std::string format_as(const token &t)
{
	std::string result;
	std::visit(_fmt_token_dispatcher(result), t);
	return result;
}

std::string format_as(const std::vector <token> &tokens)
{
	std::string result;

	_fmt_token_dispatcher ftd(result);
	for (size_t i = 0; i < tokens.size(); i++) {
		std::visit(ftd, tokens[i]);
		if (i + 1 < tokens.size())
			result += ", ";
	}

	return "[" + result + "]";
}
