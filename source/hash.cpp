#include "include/hash.hpp"

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
