#pragma once

#include <queue>

struct ETN;
struct Expression;
struct Statement;

using ETN_ref = ETN *;

struct scoped_memory_manager {
	std::queue <ETN_ref> deferred;

	~scoped_memory_manager();

	void drop(ETN_ref);
	void drop(const Expression &);
	void drop(const Statement &);

	void clear();

	// TODO: drop(smm) for Expressions and Statements (maybe even ETNs)
};
