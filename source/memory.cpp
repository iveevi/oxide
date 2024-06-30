#include "include/formalism.hpp"
#include "include/memory.hpp"

// Scoped memory management
scoped_memory_manager::~scoped_memory_manager()
{
	clear();
}

void scoped_memory_manager::drop(ETN_ref etn)
{
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
