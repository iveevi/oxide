#include "include/formalism.hpp"
#include "include/memory.hpp"
#include "include/match.hpp"
#include "include/format.hpp"

// Scoped memory management
scoped_memory_manager::~scoped_memory_manager()
{
	clear();
}

void scoped_memory_manager::transfer_to(scoped_memory_manager &smm)
{
	while (deferred.size()) {
		smm.deferred.push(deferred.front());
		deferred.pop();
	}
}

void scoped_memory_manager::drop(ETN_ref etn)
{
	// TODO: enable only on debug mode
	auto copy = deferred;
	while (copy.size()) {
		if (copy.front() == etn) {
			fmt::println("double free detected on address {}", (void *) etn);
			abort();
		}

		copy.pop();
	}

	if (etn->is <_expr_tree_op> ()) {
		auto tree = etn->as <_expr_tree_op> ();

		ETN_ref head = tree.down;
		while (head) {
			drop(head);
			head = head->next();
		}
	}

	deferred.push(etn);
}

void scoped_memory_manager::drop(const Expression &expr)
{
	drop(expr.etn);
}

void scoped_memory_manager::drop(const Statement &statement)
{
	drop(statement.lhs);
	drop(statement.rhs);
}

void scoped_memory_manager::drop(const Substitution &sub)
{
	for (auto &[sym, expr] : sub)
		drop(expr.etn);
}

void scoped_memory_manager::drop(const RValue &rv)
{
	if (rv.is <Statement> ())
		drop(rv.as <Statement> ());

	if (rv.is <Expression> ())
		drop(rv.as <Expression> ());
}

void scoped_memory_manager::clear()
{
	while (deferred.size()) {
		ETN_ref ref = deferred.front();
		deferred.pop();

		delete ref;
	}
}

// Drop methods
Expression &Expression::drop(scoped_memory_manager &smm)
{
	smm.drop(etn);
	return *this;
}

Statement &Statement::drop(scoped_memory_manager &smm)
{
	smm.drop(lhs);
	smm.drop(rhs);
	return *this;
}

Substitution &Substitution::drop(scoped_memory_manager &smm)
{
	smm.drop(*this);
	return *this;
}

// Cloning
ETN_ref clone(const ETN_ref &ref)
{
	if (ref->is <_expr_tree_op> ()) {
		auto tree = ref->as <_expr_tree_op> ();

		ETN_ref netn = new ETN(tree);

		ETN_ref head = tree.down;

		std::queue <ETN_ref> list;
		while (head) {
			list.push(clone(head));
			head = head->next();
		}

		ETN_ref nhead = list.front();
		list.pop();

		ETN_ref p = nhead;
		while (list.size()) {
			ETN_ref q = list.front();
			list.pop();

			p->next() = q;
			p = q;
		}

		netn->as <_expr_tree_op> ().down = nhead;

		return netn;
	} else {
		return new ETN(ref->as <_expr_tree_atom> ());
	}
}

ETN_ref clone_soft(const ETN_ref &ref)
{
	if (ref->is <_expr_tree_op> ())
		return new ETN(ref->as <_expr_tree_op> ());

	return new ETN(ref->as <_expr_tree_atom> ());
}
