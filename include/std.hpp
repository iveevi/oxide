#pragma once

#include <optional>
#include <variant>

template <typename ... _Args>
struct _translate_once {
	template <typename T>
	std::variant <_Args...> operator()(const T &value) {
		return value;
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

	template <typename ... _Args>
	auto_variant <_Args...> translate(const std::variant <_Args...> &) const {
		return std::visit(_translate_once <_Args...> (), *this);
	}
};

template <typename T>
struct auto_optional : std::optional <T> {
	using std::optional <T> ::optional;

	template <typename U>
	auto_optional <U> translate() const {
		if (this->operator bool())
			return this->value();
		return std::nullopt;
	}

	template <typename F>
	auto_optional <std::invoke_result_t <F, T>> translate(F &&ftn) const {
		if (this->operator bool())
			return ftn(this->value());
		return std::nullopt;
	}

	template <typename F>
	auto inspect(F &&ftn) const {
		if (this->operator bool())
			ftn(this->value());
		return *this;
	}

	template <typename F>
	std::invoke_result_t <F, T> transform(F &&ftn) const {
		if (this->operator bool())
			return ftn(this->value());
		return std::nullopt;
	}
};
