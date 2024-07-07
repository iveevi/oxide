# Define axiomatic rules
commutativity := $(a + b = b + a)
associativity := $(a + (b + c) = (a + b) + c)

# Define arguments with statements
T := ($(a = b), $(b = c)) => $(a = c)

relation("===")

# S := $(a === b)

# Apply transformations
# transform(E, commutativity)
# transform(E, associativity)
