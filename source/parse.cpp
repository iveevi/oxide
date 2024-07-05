#include <fmt/core.h>
#include <vector>
#include <deque>
#include <stack>

#include "include/action.hpp"
#include "include/format.hpp"
#include "include/lex.hpp"
#include "include/formalism.hpp"
#include "include/memory.hpp"
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
			if (top <= op || top == pbegin || top == pend)
				break;

			operators.pop();
			finish.push_back(top);
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

	void operator()(const ParenthesisBegin &) {
		operators.push(pbegin);
	}

	void operator()(const GroupEnd &) {
		while (operators.size()) {
			auto top = operators.top();
			if (top == pbegin)
				break;

			operators.pop();
			finish.push_back(top);
		}

		if (!operators.size()) {
			fmt::println("error parsing expression, no '(' found before");
			return tokens.clear();
		}

		return operators.pop();
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
// using RPE_vector = std::vector <RPE>;
struct RPE_vector {
	std::vector <RPE> rpes;
	size_t end;
};

RPE_vector rpe_vector(const std::vector <Token> &lexed, size_t pos = 0)
{
	if (pos >= lexed.size())
		return {};

	// Shunting Yards
	std::vector <RPE> finish;
	std::stack <Operation> operators;

	size_t read = 0;
	std::deque <Token> tokens {
		lexed.begin() + pos,
		lexed.end()
	};

	_rpe_vector_dispatcher rvd(finish, operators, tokens);
	while (tokens.size()) {
		auto t = tokens.front();
		tokens.pop_front();
		std::visit(rvd, t);
		read++;
	}

	while (operators.size() && operators.top() != none)
		rvd(none);

	return { finish, pos + read };
}

std::optional <std::pair <Signature, int>> signature_from_tokens(const std::vector <Token> &tokens, size_t pos = 0)
{
	Signature result;

	auto null_state = std::nullopt;
	auto fail_state = std::make_pair(result, -1);

	auto safe_get = [&](bool inc = true) -> std::optional <Token> {
		if (pos >= tokens.size())
			return std::nullopt;

		size_t p = pos;
		pos += inc;
		return tokens[p];
	};

	auto first = safe_get();
	if (!first || !first->is <SignatureBegin> ()) {
		return null_state;
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
			return fail_state;
		}

		auto in = safe_get();
		if (!in || !in->is <In> ()) {
			if (in)
				fmt::println("unexpected '{}' in signature, expected ':'", in.value());
			else
				fmt::println("expected ':' in signature");
			return fail_state;
		}

		auto domain = safe_get();
		if (!domain || !domain->is <Symbol> ()) {
			// TODO: helper function for generating messages
			if (in)
				fmt::println("unexpected '{}' in signature, expected a domain symbol", domain.value());
			else
				fmt::println("expected a domain symbol in signature");
			return fail_state;
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
			return fail_state;
		}

		if (!add_signature(result, str, dom))
			return fail_state;

		auto comma = safe_get(false);
		if (!comma) {
			fmt::println("expected ']' to close the signature");
			return fail_state;
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
		return fail_state;
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

	auto [rpev, offset] = rpe_vector(tokens, 0);
	if (rpev.empty()) {
		fmt::println("empty RPE vector");
		return std::nullopt;
	}

	const auto fallback_signature = std::make_pair(Signature(), (int) rpev.size());

	auto [sig, pos] = signature_from_tokens(tokens, rpev.size())
		.value_or(fallback_signature);

	// Indicates an error in parsing the signature
	if (pos < 0)
		return std::nullopt;

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

	auto [lhs_rpev, offset] = rpe_vector(tokens, 0);
	if (lhs_rpev.empty()) {
		fmt::println("empty RPE vector (lhs)");
		return std::nullopt;
	}

	if (offset >= tokens.size()) {
		fmt::println("statement ended before = was parsed");
		return std::nullopt;
	}

	Token middle = tokens[offset++];
	if (!middle.is <Equals> ()) {
		fmt::println("only supporting statements of equality (=)");
		return std::nullopt;
	}

	auto [rhs_rpev, _] = rpe_vector(tokens, offset);
	if (rhs_rpev.empty()) {
		fmt::println("empty RPE vector (rhs)");
		return std::nullopt;
	}

	// Optionally a domain signature at the end
	const auto fallback_signature = std::make_pair(Signature(), offset + rhs_rpev.size());

	auto [sig, pos] = signature_from_tokens(tokens, fallback_signature.second)
		.value_or(fallback_signature);

	if (pos < 0) {
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
auto_optional <Expression> TokenStreamParser::parse_symbolic_expression(const std::vector <RPE> &rpev)
{
	// Now parse the signature
	const auto fallback_signature = std::make_pair(Signature(), (int) rpev.size());

	auto [sig, pos] = signature_from_tokens(stream, rpev.size())
		.value_or(fallback_signature);

	if (rpev.empty() || pos < 0) {
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

auto_optional <Statement> TokenStreamParser::parse_symbolic_statement(const std::vector <RPE> &lhs_rpev)
{
	auto [rhs_rpev, offset] = rpe_vector(stream, pos);
	if (rhs_rpev.empty()) {
		fmt::println("empty RPE vector (rhs)");
		return std::nullopt;
	}

	// Optionally a domain signature at the end
	const auto fallback_signature = std::make_pair(Signature(), pos + rhs_rpev.size());

	auto [sig, pos] = signature_from_tokens(stream, fallback_signature.second)
		.value_or(fallback_signature);

	if (pos < 0) {
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
	auto [rpev, offset] = rpe_vector(stream, 0);
	if (rpev.empty()) {
		fmt::println("empty RPE vector (lhs)");
		return std::nullopt;
	}

	if (offset >= stream.size()) {
		return TokenStreamParser(stream, offset + 1)
			.parse_symbolic_expression(rpev)
			.translate <Symbolic> ();
	}

	Token middle = stream[--offset];
	if (middle.is <Equals> ()) {
		return TokenStreamParser(stream, offset + 1)
			.parse_symbolic_statement(rpev)
			.translate <Symbolic> ();
	}

	fmt::println("middle is not equals! {}", middle);

	return TokenStreamParser(*this)
		.parse_symbolic_expression(rpev)
		.translate <Symbolic> ();
}

auto_optional <Symbolic> TokenStreamParser::parse_symbolic()
{
	size_t og = pos;
	auto t = next();
	if (!t)
		return std::nullopt;

	auto token = t.value();
	if (!token.is <SymbolicBegin> ()) {
		pos = og;
		return std::nullopt;
	}

	// Find the end of the group
	size_t end = pos;

	size_t count = 0;
	while (end < stream.size()) {
		auto t = stream[end];

		if (t.is <SymbolicBegin> ()) {
			fmt::println("cannot nested $(...) expressions!");
			pos = og;
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
		pos = og;
		return std::nullopt;
	}

	Stream slice(stream.begin() + pos, stream.begin() + end);
	return TokenStreamParser(slice, 0)
		.parse_symbolic_scope()
		.if_valid([&](auto) {
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

auto_optional <Real> TokenStreamParser::parse_real()
{
	auto t = next();
	if (!t)
		return std::nullopt;

	auto token = t.value();
	if (!token.is <Real> ()) {
		backup();
		return std::nullopt;
	}

	return token.as <Real> ();
}

auto_optional <Integer> TokenStreamParser::parse_int()
{
	auto t = next();
	if (!t)
		return std::nullopt;

	auto token = t.value();
	if (!token.is <Integer> ()) {
		backup();
		return std::nullopt;
	}

	return token.as <Integer> ();
}

auto_optional <Truth> TokenStreamParser::parse_truth()
{
	auto t = next();
	if (!t)
		return std::nullopt;

	auto token = t.value();
	if (!token.is <Truth> ()) {
		backup();
		return std::nullopt;
	}

	return token.as <Truth> ();
}

auto_optional <Value> TokenStreamParser::parse_rvalue()
{
	if (auto z = parse_int())
		return z.translate <Value> ();

	if (auto r = parse_real())
		return r.translate <Value> ();

	if (auto t = parse_truth())
		return t.translate <Value> ();

	if (auto sym = parse_symbol())
		return sym.translate <Value> ();

	if (auto sym = parse_symbolic()) {
		return sym.translate([](const auto &sym) -> Value {
			return sym.translate(Value());
		});
	}

	if (auto tuple = parse_args())
		return tuple.translate <Value> ();

	return std::nullopt;
}

auto_optional <Tuple> TokenStreamParser::parse_args()
{
	// TODO: reset structure, returns nullopt and records original position
	// tracker(pos: int &).success(T) -> T

	auto t = next();
	if (!t)
		return {};

	auto token = t.value();
	if (!token.is <ParenthesisBegin> ())
		return std::nullopt;

	Tuple args;
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
			backup(true);
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
			.value = value.value().translate(Value())
		};

		return action;
	} else if (token.is <ParenthesisBegin> ()) {
		backup();

		auto opt_args = parse_args();
		if (!opt_args) {
			fmt::println("failed to parse args!");
			return std::nullopt;
		}

		action = Call {
			.ftn = symbol,
			.args = opt_args.value()
		};

		return action;
	} else {
		fmt::println("unexpected {}, expected a definition or a call", token);
		return std::nullopt;
	}
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

	// Option so far; defaults to boolean true
	auto action = PushOption { s.value(), true };

	// Optionally an argument list
	auto t = next(true);
	if (t && t->is <ParenthesisBegin> ()) {
		auto arg = parse_rvalue();
		if (!arg) {
			fmt::println("expected an rvalue in option");
			return std::nullopt;
		}

		// TODO: if (auto error = expect_token(...)) return error;
		auto end = next(true);
		if (!end) {
			fmt::println("expected ) to close argument to option");
			return std::nullopt;
		}

		if (!end->is <GroupEnd> ()) {
			fmt::println("unexpected {} to close argument to option, expected )", end.value());
			return std::nullopt;
		}

		action.arg = arg.value();
	} else {
		backup();
	}

	// TODO: backstepping automatically and testing it
	return action;
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
