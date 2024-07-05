#pragma once

#include "include/action.hpp"
#include "include/format.hpp"

using Options = std::unordered_map <Symbol, RValue>;

using Function = std::function <Result (const std::vector <RValue> &, const Options &)>;

template <size_t N, typename T, typename ... Args>
auto_optional <std::tuple <T, Args...>>
_overload(const std::vector <RValue> &args)
{
	if (args.size() != 1 + N + sizeof...(Args))
		return std::nullopt;

	if constexpr (sizeof...(Args) == 0) {
		return args[N].maybe_as <T> ()
			.template translate <std::tuple <T>> ();
	} else {
		return args[N].maybe_as <T> ()
			.attach(_overload <N + 1, Args...>, args);
	}
}

// TODO: structure with some more metadata
template <typename T, typename ... Args>
auto_optional <std::tuple <T, Args...>>
overload(const std::vector <RValue> &args)
{
	return _overload <0, T, Args...> (args);
}

template <typename T>
T check_option(const Options &options, const Symbol &opt, T def)
{
	if (!options.contains(opt))
		return def;

	const auto &rv = options.at(opt);
	if (!rv.is <T> ()) {
		// TODO: type string
		fmt::println("wrong type for option {}", opt);
		return def;
	}

	return rv.as <T> ();
}
