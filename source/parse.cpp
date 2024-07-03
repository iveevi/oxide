#include <fmt/core.h>
#include <vector>
#include <deque>
#include <stack>

#include "include/action.hpp"
#include "include/format.hpp"
#include "include/lex.hpp"
#include "include/formalism.hpp"
#include "include/std.hpp"
#include "include/parse.hpp"
#include "include/types.hpp"

// Element in a reverse polish stack
struct RPE : std::variant <Atom, Operation> {
	using std::variant <Atom, Operation> ::variant;

	bool has_atom() const {
		return std::holds_alternative <Atom> (*this);
	}

	bool has_op() const {
		return std::holds_alternative <Operation> (*this);
	}

	Atom atom() {
		return std::get <0> (*this);
	}

	Operation op() {
		return std::get <1> (*this);
	}
};

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

			// fmt::println("popped operation: {}", (int) top);
		}

		operators.push(op);
	}

	// End parsing expressions when encountering:
	// - equals (-> statement)
	// - [      (-> signature)
	void operator()(const Equals &) {
		// Stop parsing, but leave output queue and operators intact
		tokens.clear();
	}

	void operator()(const SignatureBegin &) {
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

std::optional <std::pair <Signature, size_t>> signature_from_tokens(const std::vector <Token> &tokens, size_t pos = 0)
{
	Signature result;

	auto safe_get = [&](bool inc = true) -> std::optional <Token> {
		if (pos >= tokens.size())
			return std::nullopt;

		size_t p = pos;
		pos += inc;
		return tokens[p];
	};

	auto first = safe_get();
	if (!first || !first->is <SignatureBegin> ()) {
		return std::nullopt;
	}

	// TODO: easier error handling... (<clause>, <expected message>, <expected>)
	while (true) {
		// <symbol> <in> <symbol: domain> <comma> ...
		auto symbol = safe_get();
		if (!symbol || !symbol->is <Symbol> ()) {
			if (symbol)
				fmt::println("unexpected '{}' in signature, expected a symbol", symbol.value());
			else
				fmt::println("expected a symbol in signature");
			return std::nullopt;
		}

		auto in = safe_get();
		if (!in || !in->is <In> ()) {
			if (in)
				fmt::println("unexpected '{}' in signature, expected ':'", in.value());
			else
				fmt::println("expected ':' in signature");
			return std::nullopt;
		}

		auto domain = safe_get();
		if (!domain || !domain->is <Symbol> ()) {
			if (in)
				fmt::println("unexpected '{}' in signature, expected a domain symbol", domain.value());
			else
				fmt::println("expected a domain symbol in signature");
			return std::nullopt;
		}

		std::string str = symbol->as <Symbol> ();
		std::string dstr = domain->as <Symbol> ();

		Domain dom;
		if (dstr == "R") {
			dom = real;
		} else if (dstr == "Z") {
			dom = integer;
		} else {
			fmt::println("invalid domain symbol '{}'", dstr);
			return std::nullopt;
		}

		if (!add_signature(result, str, dom)) {
			return std::nullopt;
		}

		auto comma = safe_get(false);
		if (!comma) {
			fmt::println("expected ']' to close the signature");
			return std::nullopt;
		}

		if (!comma->is <Comma> ())
			break;

		pos++;
	}

	auto end = safe_get();
	if (!end || !end->is <SignatureEnd> ()) {
		if (end)
			fmt::println("unexpected '{}' in signature, expected ']' to close", end.value());
		else
			fmt::println("expected ']' to close the signature");
		return std::nullopt;
	}

	return std::make_pair(result, pos);
}

// TODO: custom allocator for more coherent trees
ETN_ref rpes_to_etn(const std::vector <RPE> &rpes)
{
	std::stack <ETN_ref> operands;

	std::deque <RPE> queued {
		rpes.begin(),
		rpes.end()
	};

	while (queued.size()) {
		auto r = queued.front();
		queued.pop_front();

		// fmt::println("operand stack: {}", operands.size());

		if (r.has_atom()) {
			ETN_ref atom = new ETN(_expr_tree_atom(r.atom()));
			operands.push(atom);
			// fmt::println("constructed ETN (atom) for: {}", r.atom());
		} else {
			if (operands.size() < 2) {
				fmt::println("expected at least two operands for operation: {}", Token(r.op()));
				return nullptr;
			}

			// Assume binary for now
			ETN_ref rhs = operands.top();
			operands.pop();

			ETN_ref lhs = operands.top();
			operands.pop();

			lhs->next() = rhs;

			ETN_ref opt = new ETN(_expr_tree_op {
				.op = r.op(),
				.down = lhs,
				.next = nullptr
			});

			operands.push(opt);

			// fmt::println("constructed ETN (tree) for operation: {}", Token(r.op()));
		}
	}

	// TODO: some checking here...
	return operands.top();
}

std::optional <Expression> Expression::from(const std::string &s)
{
	auto tokens = lex(s);

	RPE_vector rpev = rpe_vector(tokens, 0);
	if (rpev.empty()) {
		fmt::println("empty RPE vector");
		return std::nullopt;
	}

	const auto fallback_signature = std::make_pair(Signature(), rpev.size());

	auto [sig, pos] = signature_from_tokens(tokens, rpev.size())
		.value_or(fallback_signature);

	if (rpev.empty() || pos != tokens.size()) {
		fmt::println("failed to fully parse expression");
		return std::nullopt;
	}

	ETN_ref etn = rpes_to_etn(rpev);
	if (!etn) {
		fmt::println("error in constructing ETN");
		return std::nullopt;
	}

	return Expression {
		.etn = etn,
		.signature = default_signature(sig, *etn)
	};
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
	if (offset >= tokens.size()) {
		fmt::println("statement ended before = was parsed");
		return std::nullopt;
	}

	Token middle = tokens[offset++];
	if (!middle.is <Equals> ()) {
		fmt::println("only supporting statements of equality (=)");
		return std::nullopt;
	}

	RPE_vector rhs_rpev = rpe_vector(tokens, offset);
	if (rhs_rpev.empty()) {
		fmt::println("empty RPE vector (rhs)");
		return std::nullopt;
	}

	// Optionally a domain signature at the end
	const auto fallback_signature = std::make_pair(Signature(), offset + rhs_rpev.size());

	auto [sig, pos] = signature_from_tokens(tokens, fallback_signature.second)
		.value_or(fallback_signature);

	if (pos != tokens.size()) {
		fmt::println("failed to fully parse statement");
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

	// TODO: safe join
	auto sl = default_signature(sig, *lhs);
	auto sr = default_signature(sig, *rhs);

	return Statement {
		.lhs = Expression { lhs, sl },
		.rhs = Expression { rhs, sr },
		.cmp = eq,
		.signature = join(sl, sr).value()
	};
}

// Parsing more general programs
auto_optional <Token> TokenStreamParser::end_statement()
{
	auto end = next();
	if (!end) {
		fmt::println("every statement must end with a semicolon");
		return std::nullopt;
	}

	if (!end->is <Semicolon> ()) {
		fmt::println("unexpected {}, expected ';'", end.value());
		return std::nullopt;
	}

	return end;
}

auto_optional <Expression> TokenStreamParser::parse_symbolic_expression(const RPE_vector &rpev)
{
	// Now parse the signature
	const auto fallback_signature = std::make_pair(Signature(), rpev.size());

	auto [sig, pos] = signature_from_tokens(stream, rpev.size())
		.value_or(fallback_signature);

	if (rpev.empty() || pos != stream.size()) {
		fmt::println("failed to fully parse expression");
		return std::nullopt;
	}

	ETN_ref etn = rpes_to_etn(rpev);
	if (!etn) {
		fmt::println("error in constructing ETN");
		return std::nullopt;
	}

	// TODO: for signatures, make sure everything is in the set of symbols...
	return Expression {
		.etn = etn,
		.signature = default_signature(sig, *etn)
	};
}

auto_optional <Statement> TokenStreamParser::parse_symbolic_statement(const RPE_vector &lhs_rpev)
{
	RPE_vector rhs_rpev = rpe_vector(stream, pos);
	if (rhs_rpev.empty()) {
		fmt::println("empty RPE vector (rhs)");
		return std::nullopt;
	}

	// Optionally a domain signature at the end
	const auto fallback_signature = std::make_pair(Signature(), pos + rhs_rpev.size());

	auto [sig, pos] = signature_from_tokens(stream, fallback_signature.second)
		.value_or(fallback_signature);

	if (pos != stream.size()) {
		fmt::println("failed to fully parse statement");
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

	auto sl = default_signature(sig, *lhs);
	auto sr = default_signature(sig, *rhs);

	return Statement {
		.lhs = Expression { lhs, sl },
		.rhs = Expression { rhs, sr },
		.cmp = eq,
		.signature = join(sl, sr).value()
	};
}

auto_optional <Symbolic> TokenStreamParser::parse_symbolic_scope()
{
	// At least one expression
	RPE_vector rpev = rpe_vector(stream, 0);
	if (rpev.empty()) {
		fmt::println("empty RPE vector (lhs)");
		return std::nullopt;
	}

	size_t offset = rpev.size();
	if (offset >= stream.size()) {
		return TokenStreamParser(stream, offset + 1)
			.parse_symbolic_expression(rpev)
			.translate <Symbolic> ();
	}

	Token middle = stream[offset];
	if (middle.is <Equals> ()) {
		return TokenStreamParser(stream, offset + 1)
			.parse_symbolic_statement(rpev)
			.translate <Symbolic> ();
	}

	return TokenStreamParser(*this)
		.parse_symbolic_expression(rpev)
		.translate <Symbolic> ();
}

auto_optional <Symbolic> TokenStreamParser::parse_symbolic()
{
	auto t = next();
	if (!t)
		return std::nullopt;

	auto token = t.value();
	if (!token.is <SymbolicBegin> ()) {
		fmt::println("no symbolic begin");
		return std::nullopt;
	}

	// Find the end of the group
	size_t end = pos;

	size_t count = 0;
	while (end < stream.size()) {
		auto t = stream[end];

		if (t.is <SymbolicBegin> ()) {
			fmt::println("cannot nested $(...) expressions!");
			return std::nullopt;
		}

		if (t.is <ParenthesisBegin> ())
			count++;

		if (t.is <GroupEnd> () && ((count--) == 0))
			break;
		end++;
	}

	if (!stream[end].is <GroupEnd> ()) {
		fmt::println("unclosed $(...) expression");
		return std::nullopt;
	}

	Stream slice(stream.begin() + pos, stream.begin() + end);
	return TokenStreamParser(slice, 0)
		.parse_symbolic_scope()
		.inspect([&](auto) {
			pos = end + 1;
		});
}

auto_optional <Symbol> TokenStreamParser::parse_symbol()
{
	if (pos >= stream.size())
		return std::nullopt;

	Symbol symbol;
	while (auto t = next()) {
		if (!t->is <Symbol> ())
			break;

		symbol += t->as <Symbol> ();
	}

	backup();

	return symbol.size() ? auto_optional <Symbol> (symbol) : std::nullopt;
}

auto_optional <RValue> TokenStreamParser::parse_rvalue()
{
	if (auto sym = parse_symbol())
		return sym.translate <RValue> ();

	if (auto sym = parse_symbolic()) {
		return sym.translate([](const auto &sym) -> RValue {
			return sym.translate(RValue());
		});
	}

	return std::nullopt;
}

auto_optional <TokenStreamParser::RValue_vec> TokenStreamParser::parse_args()
{
	// TODO: reset structure, returns nullopt and records original position

	auto t = next();
	if (!t)
		return {};

	auto token = t.value();
	if (!token.is <ParenthesisBegin> ())
		return std::nullopt;

	RValue_vec args;
	while (true) {
		auto opt_arg = parse_rvalue();
		if (!opt_arg) {
			if (args.size()) {
				fmt::println("expected another rvalue");
				return std::nullopt;
			}

			break;
		}

		args.push_back(opt_arg.value());

		auto opt_comma = next();
		if (!opt_comma || !opt_comma->is <Comma> ()) {
			backup();
			break;
		}
	}

	// TODO: null_or_not <...> ()?
	t = next();
	if (!t)
		return std::nullopt;

	token = t.value();
	if (!token.is <GroupEnd> ())
		return std::nullopt;

	return args;
}

auto_optional <Action> TokenStreamParser::parse_statement_from_symbol(const Symbol &symbol)
{
	// Either define or call
	Action action;

	auto t = next();
	if (!t) {
		fmt::println("expected a defintion or a call");
		return std::nullopt;
	}

	auto token = t.value();
	if (token.is <Define> ()) {
		auto value = parse_rvalue();
		if (!value) {
			fmt::println("failed to parse RValue");
			return std::nullopt;
		}

		action = DefineSymbol {
			.identifier = symbol,
			.value = value.value().translate(RValue())
		};
	} else if (token.is <ParenthesisBegin> ()) {
		backup();

		auto opt_args = parse_args();
		if (!opt_args)
			return std::nullopt;

		action = Call {
			.ftn = symbol,
			.args = opt_args.value()
		};
	} else {
		fmt::println("unexpected {}, expected a definition or a call", token);
		return std::nullopt;
	}

	return end_statement().transition(action);
}

auto_optional <Action> TokenStreamParser::parse_statement_from_at()
{
	// Assume that we were on @ to begin with
	next();

	// Need a symbol to begin with
	auto s = parse_symbol();
	if (!s) {
		// fmt::println("expected option, got {} instead", ...);
		return std::nullopt;
	}

	// TODO: optionally an argument list

	// TODO: backstepping automatically and testing it
	auto action = PushOption { s.value() };
	return end_statement().transition(action);
}

auto_optional <Action> TokenStreamParser::parse_statement()
{
	auto t = next(false);
	if (!t)
		return std::nullopt;

	auto token = t.value();
	if (token.is <Symbol> ()) {
		auto s = parse_symbol();
		if (!s) {
			fmt::println("fatal error...");
			return std::nullopt;
		}

		return parse_statement_from_symbol(s.value());
	}

	if (token.is <At> ())
		return parse_statement_from_at();

	fmt::println("unexpected token {}", token);
	return std::nullopt;
}

std::vector <Action> TokenStreamParser::parse()
{
	// TODO: sink for diagnostics
	std::vector <Action> actions;
	while (auto action = parse_statement())
		actions.push_back(action.value());
	return actions;
}
