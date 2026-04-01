# Logic Execution Engine — Materialization

## Overview

Materialization is the process by which obligation consequents become concrete facts and objects in the world. When an obligation is activated and its antecedent holds, the engine walks the consequent AST and decomposes it into base-case world operations.

The consequent AST serves two roles depending on context:

- **Evaluation mode** — the AST is a condition tree that evaluates to a boolean. Used by `evaluateBool` and `evaluateObligations`.
- **Materialization mode** — the AST is a description of what to produce. Used by `activateObligations` via `materialize`.

The same AST, two interpretations. Evaluation asks "is this true?" Materialization says "make this true."

---

## Base Cases

### LITERAL "true"

The simplest consequent. Materializes the implicit fact: the tested object belongs to the obligation's target set.

```
Consequent:  true
Effect:      addMember(object_id, target_set_id)
```

### LITERAL "false"

Materialization fails. The obligation does not produce any fact.

### MEMBER_OF(x, S)

An explicit membership fact. The engine evaluates x and S using the current bindings to resolve them to object IDs, then writes the fact.

```
Consequent:  MEMBER_OF(self, B)
Effect:      addMember(self_id, B_id)
```

### NOT(MEMBER_OF(x, S))

A known non-membership fact. Sets M[x][S] = 0 in the ternary matrix — distinct from unknown.

```
Consequent:  NOT(MEMBER_OF(self, B))
Effect:      setNonMember(self_id, B_id)
```

---

## Compound Cases

### AND(A, B)

Materialize both sides. A is materialized first, then B. If A creates new objects and binds them to variable names, those bindings are visible to B.

```
Consequent:  MEMBER_OF(self, B) AND MEMBER_OF(self, C)
Effect:      addMember(self_id, B_id)
             addMember(self_id, C_id)
```

### OR(A, B)

Does not write a fact directly. Instead, creates a new obligation encoding the disjunctive constraint: (NOT A) → B. The obligation is added to O and sits inert until the proof activates it.

```
Consequent:  MEMBER_OF(self, B) OR MEMBER_OF(self, C)
Effect:      addObligation(target, NOT(MEMBER_OF(self, B)), MEMBER_OF(self, C))
```

The engine does not decide which side of the OR holds. The proof drives that choice. When the proof later establishes NOT(MEMBER_OF(self, B)) and activates the derived obligation, the engine materializes MEMBER_OF(self, C).

---

## Creation Cases

Creation nodes are AST node types that exist exclusively for materialization. They cannot be evaluated as conditions — attempting to evaluate them throws an error.

### CREATE_ENTITY

Creates a new entity in T and binds its ID to the variable name stored in the node's `value` field.

```
Node:    CREATE_ENTITY("m")
Effect:  entity = world.createEntity()
         bindings["m"] = entity.getId()
```

The variable name is available to subsequent sibling nodes in the same AND chain. This is how a consequent like "create entity m, then add m to set S" works:

```
         AND
        /   \
CREATE_ENTITY("m")   MEMBER_OF(m, S)
```

CREATE_ENTITY materializes first (AND processes left to right), binds m. Then MEMBER_OF resolves m from bindings and writes the membership fact.

### CREATE_TUPLE

Creates a new tuple from its children (which are evaluated as references or literals to resolve to object IDs) and binds the tuple's ID to the variable name in the node's `value` field.

```
Node:    CREATE_TUPLE("pair")
         children: REFERENCE("a"), REFERENCE("b")
Effect:  tuple = world.createTuple({a, b})
         bindings["pair"] = tuple.getId()
```

This is the mechanism for deriving relation facts. A consequent that says "create tuple (a, b) and add it to relation R":

```
            AND
           /   \
CREATE_TUPLE("t")   MEMBER_OF(t, R)
  /       \
REF("a")  REF("b")
```

### CREATE_SET

Creates a new set (or relation, or map) and binds its ID. The node's `value` field holds the system type: "SET", "REL", or "MAP". The variable name comes from a REFERENCE child at position 0.

```
Node:    CREATE_SET("REL")
         child 0: REFERENCE("newrel")
Effect:  rel = world.createSet(SysType::REL)
         bindings["newrel"] = rel.getId()
```

---

## Binding Propagation in AND Chains

The key design choice that makes creation work is that `materialize` takes mutable bindings. When AND materializes its left child, any new bindings created (by CREATE_ENTITY, CREATE_TUPLE, or CREATE_SET) are visible to the right child.

This allows a single consequent tree to express multi-step constructions:

```
              AND
             /   \
            AND   MEMBER_OF(t, Succ)
           /   \
CREATE_ENTITY("m")  AND
                   /   \
        MEMBER_OF(m, ℕ)  CREATE_TUPLE("t")
                           /        \
                       REF("self")  REF("m")
```

This consequent:
1. Creates a new entity m
2. Adds m to ℕ
3. Creates a tuple (self, m)
4. Adds the tuple to the successor relation

Each step builds on the bindings established by prior steps. The AND tree is processed left to right, depth first.

---

## Summary

- **LITERAL "true"** — Mode: Materialize. Effect: addMember(object, target).
- **LITERAL "false"** — Mode: Materialize. Effect: Fails.
- **MEMBER_OF(x, S)** — Mode: Both. Eval: returns bool. Materialize: addMember(x, S).
- **NOT(MEMBER_OF(x, S))** — Mode: Both. Eval: returns bool. Materialize: setNonMember(x, S).
- **AND(A, B)** — Mode: Both. Eval: short-circuit &&. Materialize: materialize A then B.
- **OR(A, B)** — Mode: Both. Eval: short-circuit ||. Materialize: create obligation (NOT A) -> B.
- **CREATE_ENTITY("var")** — Mode: Materialize only. Effect: createEntity(), bind to var.
- **CREATE_TUPLE("var")** — Mode: Materialize only. Effect: createTuple(children), bind to var.
- **CREATE_SET("type")** — Mode: Materialize only. Effect: createSet(type), bind to var from child.
