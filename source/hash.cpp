#include "include/hash.hpp"
#include "include/format.hpp"

hash_type quick_hash(const ETN_ref &tree)
{
	hash_type h0 = hhash <0> (tree);
	hash_type h1 = hhash <1> (tree);
	hash_type h2 = hhash <2> (tree);
	return std::rotr(h1, h0 % 7) | std::rotl(h2, h0 % 11);
}

hash_type quick_hash(const Expression &expr)
{
	return quick_hash(expr.etn);
}

// Displaying tables
void list_table(const ExprTable_L1 &table)
{
	fmt::println("L1 table:");

	size_t M = table.table_size;
	size_t N = table.vector_size;

	double load = 0;
	for (size_t i = 0; i < M; i++) {
		for (size_t j = 0; j < N; j++) {
			if (table.valid[i * N + j]) {
				fmt::println("  {}", table.data[i][j]);
				load++;
			}
		}
	}

	fmt::println("  load: {:03.2f}%", 100.0 * load/(M * N));
}
