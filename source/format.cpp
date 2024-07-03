#include <variant>

#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/format.hpp"
#include "include/formalism.hpp"
#include "include/lex.hpp"

std::string format_as(const Domain &dom)
{
	switch (dom) {
	case integer:
		return "Z";
	case real:
		return "R";
	default:
		break;
	}

	return "?";
}

struct _fmt_token_dispatcher {
	std::string &ref;

	void operator()(Integer i) {
		ref += fmt::format("I:{}", i.value);
	}

	void operator()(Real r) {
		ref += fmt::format("R:{}", r.value);
	}

	void operator()(Symbol symbol) {
		ref += "sym:";
		ref += symbol;
	}

	void operator()(Operation op) {
		ref += "op:";
		ref += op_strs[op];
	}

	void operator()(Equals) {
		ref += "eq";
	}

	void operator()(Comma) {
		ref += "comma";
	}

	void operator()(In) {
		ref += "in";
	}

	void operator()(Define) {
		ref += "define";
	}

	void operator()(Implies) {
		ref += "implies";
	}

	void operator()(At) {
		ref += "at";
	}

	void operator()(Semicolon) {
		ref += "semicolon";
	}

	void operator()(SymbolicBegin) {
		ref += "<symbolic-begin>";
	}

	void operator()(ParenthesisBegin) {
		ref += "<paren-begin>";
	}

	void operator()(GroupEnd) {
		ref += "<group-end>";
	}

	void operator()(SignatureBegin) {
		ref += "<sig-begin>";
	}

	void operator()(SignatureEnd) {
		ref += "<sig-end>";
	}

	void operator()(Axiom) {
		ref += "axiom";
	}

	template <typename T>
	void operator()(T x) {
		ref += "?";
	}

	static constexpr const char *op_strs[] = {
		"none", "+", "-", "*", "/"
	};
};

std::string format_as(const Atom &atom)
{
	std::string result;
	_fmt_token_dispatcher ftd(result);
	std::visit(ftd, atom);
	return result;
}

std::string format_as(const Token &t)
{
	std::string result;
	std::visit(_fmt_token_dispatcher(result), t);
	return result;
}

std::string format_as(const std::vector <Token> &tokens)
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

struct _fmt_atom_dispatcher {
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

std::string format_as(const ETN &etn, int indent)
{
	std::string result;

	if (indent > 0) {
		result += std::string((indent - 1) << 2, ' ');
		result += " └──";
	}

	_fmt_atom_dispatcher ftd(result);
	if (etn.is <_expr_tree_atom> ()) {
		auto atom = etn.as <_expr_tree_atom> ().atom;

		result += "[";
			std::visit(ftd, atom);
		result += "]";

		// TODO: on debug only
		result += fmt::format(" ({})", (void *) &etn);
	} else {
		auto tree = etn.as <_expr_tree_op> ();

		result += "[";
			ftd(tree.op);
		result += "]";

		// TODO: on debug only
		result += fmt::format(" ({})", (void *) &etn);

		ETN *d = tree.down;
		while (d) {
			result += "\n" + format_as(*d, indent + 1);
			d = d->next();
		}
	}

	return result;
}

std::string format_as(const Signature &sig)
{
	std::string result;

	for (auto it = sig.begin(); it != sig.end(); it++) {
		result += it->first + " ∈ " + format_as(it->second);
		if (std::next(it) != sig.end())
			result += ", ";
	}

	return "[" + result + "]";
}

std::string _etn_to_string(const ETN *etn)
{
	std::string result;

	_fmt_atom_dispatcher ftd(result);
	if (etn->is <_expr_tree_atom> ()) {
		auto atom = etn->as <_expr_tree_atom> ().atom;
		std::visit(ftd, atom);
	} else {
		auto tree = etn->as <_expr_tree_op> ();

		// TODO: check if binary
		result += "(";

		ETN *a = tree.down;
		ETN *b = a->next();

		result += _etn_to_string(a) + " ";
		ftd(tree.op);
		result += " " + _etn_to_string(b);

		result += ")";
	}

	return result;
}

std::string format_as(const Expression &expr)
{
	return _etn_to_string(expr.etn) + " " + format_as(expr.signature);
}

std::string format_as(const Statement &stmt)
{
	// TODO: assuming eq
	return _etn_to_string(stmt.lhs.etn)
		+ " = " + _etn_to_string(stmt.rhs.etn)
		+ " " + format_as(stmt.signature);
}

std::string format_as(const RValue &rv)
{
	if (rv.is <Expression> ())
		return format_as(rv.as <Expression> ());

	if (rv.is <Statement> ())
		return format_as(rv.as <Statement> ());

	if (rv.is <Symbol> ())
		return rv.as <Symbol> ();

	return "<?>";
}
