#pragma once

#include <cstdint>
#include <cstring>
#include <stack>
#include <bitset>
#include <concepts>

#include "include/formalism.hpp"

// Hierarchical function(s) split by depth
using hash_type = uint64_t;

// Atom hashes
template <typename T>
requires std::is_constructible_v <Atom, T>
hash_type ahash(const T &)
{
	return 0;
}

template <>
inline hash_type ahash(const Symbol &sym)
{
	return std::hash <Symbol> {} (sym);
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
				seed++;
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
using push_marker = std::vector <size_t>;

template <size_t M, size_t N>
struct ExpressionTable {
	std::bitset <M * N> valid;

	// M is table size
	// N is vector size (collision list)
	Expression data[M][N];

	// TODO: shadow table with higher level hashes (h1, h2, ...)

	size_t unique;
	const size_t table_size = M;
	const size_t vector_size = N;

	// Local memory manager, useful
	// for transporting tables on the fly
	scoped_memory_manager smm;

	// TODO: next pointer for larger sizes?

	ExpressionTable() : valid(0), unique(0) {
		std::memset(data, 0, sizeof(data));
	}

	// No copies
	ExpressionTable(const ExpressionTable &) = delete;
	ExpressionTable &operator=(const ExpressionTable &) = delete;

	const Expression &flat_at(size_t i) const {
		// TODO: error outside?
		return data[i / N][i % N];
	}

	bool push(const Expression &expr, push_marker &pm) {
		hash_type h = quick_hash(expr) % M;
		for (size_t i = 0; i < N; i++) {
			size_t j = h * N + i;
			if (!valid[j]) {
				data[h][i] = expr;
				valid[j] = true;
				pm.push_back(j);
				unique++;
				return true;
			}

			// Check if the same
			if (equal(data[h][i], expr))
				return true;
		}

		return false;
	}

	bool push(const Expression &expr) {
		hash_type h = quick_hash(expr) % M;
		for (size_t i = 0; i < N; i++) {
			size_t j = h * N + i;
			if (!valid[j]) {
				data[h][i] = expr;
				valid[j] = true;
				unique++;
				return true;
			}

			// Check if the same
			if (equal(data[h][i], expr))
				return true;
		}

		return false;
	}

	void clear(const push_marker &pm) {
		// TODO: can free immediately as well
		for (size_t i : pm) {
			unique -= valid[i];
			valid[i] = false;
		}
	}
};

// TODO: hierarchy of tabling (last one is a list of tables)
// but raw variadics are expensive since everything is stacked
using ExprTable_L1 = ExpressionTable <41, 4>;

// TODO: msb index for page sized array of validity bits (uint64_t):
// first check == 0xff..., otherwise we know there is a valid bit/slot
// the use intrinsic
