#pragma once

#include <variant>

template <typename ... Args>
struct auto_variant : std::variant <Args...> {
	using std::variant <Args...> ::variant;

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
};
