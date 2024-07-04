#pragma once

#include <functional>
#include <optional>
#include <tuple>
#include <type_traits>
#include <variant>

template <typename T>
constexpr bool is_tuple = false;

template <typename T>
constexpr bool is_auto_optional = false;


template <typename T>
struct auto_optional : std::optional <T> {
	using std::optional <T> ::optional;

	// optional <T> -> optional <U>
	//
	// Transfers validity of the object
	template <typename U>
	auto_optional <U> transition(const U &opt) {
		if (this->operator bool())
			return opt;
		return std::nullopt;
	}

	template <typename U>
	auto_optional <U> transition(const auto_optional <U> &opt) {
		if (this->operator bool())
			return opt;
		return std::nullopt;
	}

	// optional <T> -> optional <U>
	//
	// Converts current value (if valid) from T to U
	template <typename U>
	auto_optional <U> translate() const {
		if (this->operator bool())
			return this->value();
		return std::nullopt;
	}

	// optional <T> -> optional <result <F>>
	//
	// Applies F to the value if present
	template <typename F>
	auto_optional <std::invoke_result_t <F, T>> translate(F &&ftn) const {
		if (this->operator bool())
			return ftn(this->value());
		return std::nullopt;
	}

	template <typename F>
	requires is_auto_optional <std::invoke_result_t <F, T>>
	std::invoke_result_t <F, T> translate(F &&ftn) const {
		if (this->operator bool())
			return ftn(this->value());
		return std::nullopt;
	}

	// optional <T> -> optional <T>
	//
	// If valid executes F
	template <typename F>
	auto if_valid(F &&ftn) const {
		if (this->operator bool())
			ftn(this->value());
		return *this;
	}

	template <typename F>
	auto if_null(F &&ftn) const {
		if (!this->operator bool())
			ftn();
		return *this;
	}

	// Combine results with short circuiting
	template <typename F, typename ... Args>
	requires is_auto_optional <std::invoke_result_t <F, Args...>>
	auto attach(F &&ftn, Args &&...args) {
		using ftype = std::invoke_result_t <F, Args...>;
		using rtype = decltype(attach(ftype(std::nullopt)));

		if (!this->operator bool())
			return rtype(std::nullopt);

		auto result = ftn(args...);
		if (!result)
			return rtype(std::nullopt);

		return attach(result);
	}

	template <typename U>
	// requires (!is_tuple <U>)
	auto_optional <std::tuple <T, U>> attach(const auto_optional <U> &other) const {
		if (this->operator bool() && other)
			return std::make_tuple(this->value(), other.value());
		return std::nullopt;
	}

	template <typename ... Args>
	auto_optional <std::tuple <T, Args...>> attach(const auto_optional <std::tuple <Args...>> &other) const {
		if (this->operator bool() && other)
			return std::tuple_cat(std::tuple <T> (this->value()), other.value());
		return std::nullopt;
	}
};

template <typename ... _Args>
struct _translate {
	using variant_type = std::variant <_Args...>;

	template <typename T>
	variant_type operator()(const T &value) {
		if constexpr (std::is_constructible_v <variant_type, T>)
			return value;

		return variant_type();
	}
};

template <typename ... Args>
struct auto_variant : std::variant <Args...> {
	using std::variant <Args...> ::variant;

	auto_variant(const std::variant <Args...> &v)
			: std::variant <Args...> (v) {}

	template <typename T>
	bool is() const {
		return std::holds_alternative <T> (*this);
	}

	template <typename T>
	T &as() {
		return std::get <T> (*this);
	}

	template <typename T>
	const T &as() const {
		return std::get <T> (*this);
	}

	template <typename T>
	auto_optional <T> maybe_as() const {
		if (is <T> ())
			return as <T> ();
		return std::nullopt;
	}

	template <typename ... _Args>
	auto_variant <_Args...> translate(const std::variant <_Args...> &) const {
		return std::visit(_translate <_Args...> (), *this);
	}

	// Passthrough with nothing
	// TODO: macro?
	template <typename F>
	auto passthrough(F &&ftn) {
		ftn();
		return *this;
	}

	template <typename F>
	auto passthrough(F &&ftn) const {
		ftn();
		return *this;
	}
};

// Specializing type trait info
template <typename ... Args>
constexpr bool is_tuple <std::tuple <Args...>> = true;

template <typename T>
constexpr bool is_auto_optional <auto_optional <T>> = true;
