#include <variant>

#include <fmt/printf.h>
#include <fmt/std.h>
#include <fmt/format.h>
#include <fmt/core.h>

#include "include/format.hpp"
#include "include/action.hpp"
#include "include/formalism.hpp"
#include "include/lex.hpp"
#include "include/types.hpp"

static constexpr const char *op_strs[] = {
	"none", "+", "-", "*", "/", "(", ")"
};

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
		ref += fmt::format("I:{}", i);
	}

	void operator()(Real r) {
		ref += fmt::format("R:{}", r);
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
		ref += "<axiom>";
	}

	void operator()(Space) {
		ref += "<space>";
	}

	template <typename T>
	void operator()(T x) {
		ref += "?";
	}
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
		ref += fmt::format("{}", i);
	}

	void operator()(Real r) {
		ref += fmt::format("{}", r);
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

std::string _etn_to_string(const ETN *etn, bool first = true)
{
	std::string result;

	_fmt_atom_dispatcher ftd(result);
	if (etn->is <_expr_tree_atom> ()) {
		auto atom = etn->as <_expr_tree_atom> ().atom;
		std::visit(ftd, atom);
	} else {
		auto tree = etn->as <_expr_tree_op> ();

		// TODO: check if binary and if its a function, etc.
		if (!first)
			result += "(";

		ETN *a = tree.down;
		ETN *b = a->next();

		result += _etn_to_string(a, false) + " ";
		ftd(tree.op);
		result += " " + _etn_to_string(b, false);

		if (!first)
			result += ")";
	}

	return result;
}

std::string format_as(const Expression &expr)
{
	// TODO: persisent options
	// return _etn_to_string(expr.etn) + " " + format_as(expr.signature);
	return _etn_to_string(expr.etn);
}

std::string format_as(const Statement &stmt)
{
	// TODO: assuming eq
	// return _etn_to_string(stmt.lhs.etn)
	// 	+ " = " + _etn_to_string(stmt.rhs.etn)
	// 	+ " " + format_as(stmt.signature);
	return _etn_to_string(stmt.lhs.etn)
		+ " = " + _etn_to_string(stmt.rhs.etn);
}

std::string format_as(const Value &v)
{
	if (v.is <Expression> ())
		return format_as(v.as <Expression> ());

	if (v.is <Statement> ())
		return format_as(v.as <Statement> ());

	if (v.is <Symbol> ())
		return v.as <Symbol> ();

	if (v.is <Truth> ())
		return fmt::format("{}", v.as <Truth> ());

	if (v.is <Integer> ())
		return fmt::format("{}", v.as <Integer> ());

	if (v.is <Real> ())
		return fmt::format("{:.2f}", v.as <Real> ());

	if (v.is <Tuple> ()) {
		auto tuple = v.as <Tuple> ();

		std::string result;
		for (size_t i = 0; i < tuple.size(); i++) {
			result += format_as(tuple[i]);
			if (i + 1 < tuple.size())
				result += ", ";
		}

		return "(" + result + ")";
	}

	if (v.is <Argument> ()) {
		auto argument = v.as <Argument> ();
		auto result = argument.result;
		std::string rstring;
		if (result.is <Symbol> ())
			rstring = result.as <Symbol> ();
		else
			rstring = format_as(result.as <Statement> ());
		return format_as(argument.predicates) + " => " + rstring;
	}

	return "?";
}

// Type string
const Symbol type_string(const Value &v)
{
	// TODO: table with # of types (auto_variant methods)
	if (v.is <Expression> ())
		return "<Expression>";
	if (v.is <Statement> ())
		return "<Statement>";
	if (v.is <Symbol> ())
		return "<Symbol>";
	if (v.is <Truth> ())
		return "<Truth>";
	if (v.is <Integer> ())
		return "<Integer>";
	if (v.is <Real> ())
		return "<Real>";
	if (v.is <Tuple> ())
		return "<Tuple>";

	return "<?>";
}
