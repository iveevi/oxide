#pragma once

#include <cstdint>
#include <cstring>
#include <concepts>

#include "include/formalism.hpp"

// Hierarchical function(s) split by depth
using hash_type = uint64_t;

template <typename T>
requires std::is_constructible_v <Atom, T>
hash_type ahash(const T &)
{
	return 0;
}

template <size_t N>
hash_type hhash(const ETN_ref tree)
{
	if (tree->is <_expr_tree_op> ()) {
		auto trop = tree->as <_expr_tree_op> ();

		// TODO: inline hash of ops...
		int64_t seed = trop.op;
		if constexpr (N == 0) {
			return seed;
		} else {
			// TODO: ETN_ref->inspect_operands(...);
			ETN_ref head = trop.down;
			while (head) {
				seed ^= hhash <N - 1> (head);
				head = head->next();
			}

			return seed;
		}
	} else {
		auto atom = tree->as <_expr_tree_atom> ().atom;
		return (N == 0) ? ahash(atom) : 0;
	}
}

template <size_t N = 0>
hash_type hhash(const Expression &expr)
{
	return hhash <N> (expr.etn);
}

// TODO: analyze this function with a corpus of many distinct expressions
hash_type quick_hash(const ETN_ref &);
hash_type quick_hash(const Expression &);

// Hash table using the quick hash
template <size_t M, size_t N>
struct ExpressionTable {
	// M is table size
	// N is vector size (collision list)
	ETN_ref data[M][N];

	size_t unique;

	// Local memory manager, useful
	// for transporting tables on the fly
	scoped_memory_manager smm;

	ExpressionTable() : unique(0) {
		std::memset(data, 0, sizeof(data));
	}

	// No copies
	ExpressionTable(const ExpressionTable &) = delete;
	ExpressionTable &operator=(const ExpressionTable &) = delete;

	bool push(const ETN_ref &tree) {
		hash_type h = quick_hash(tree) % M;
		for (size_t i = 0; i < N; i++) {
			if (!data[h][i]) {
				data[h][i] = tree;
				unique++;
				return true;
			}

			// Check if the same
			if (equal(data[h][i], tree))
				return true;
		}

		return false;
	}
};

// TODO: hierarchy of tabling (last one is a list of tables)
// but raw variadics are expensive since everything is stacked
using ExprTable_L1 = ExpressionTable <41, 4>;
