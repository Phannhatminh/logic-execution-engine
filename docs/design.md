# Logic Execution Engine — Design

## Layered Architecture

The system is divided into two layers:

- **Core layer** — provides primitive building blocks: Object, Entity, Tuple.
- **Logic engine layer** — defines the world model and operates over it.

The core layer is kept minimal and free of user-defined logic. All dynamic behavior (user-defined equality, rules, conditions) lives in the logic engine layer.

---

## Core Layer

### Object

The base type for everything in the system. Provides:

- `getType()` — returns the object's type tag
- `equals()` — identity-based comparison (same address = same object)
- `hashCode()` — address-based hash, consistent with identity equality
- `clone()` — deep copy
- `toString()` — string representation including type and hashcode

### Entity

A subclass of Object. Represents a distinct individual in the world. Two entities are equal if and only if they are literally the same entity (same address). Provides:

- `identity()` — returns a pointer to itself

### Tuple

A subclass of Object. Represents an ordered grouping of objects.

---

## Logic Engine Layer

### World Model

The logic engine operates over a world **W = (U, S, T, O)** where:

- **T** (Object set) — The set of all objects that have been created.
- **U** (Universe) — The set of all entities. A subset of T.
- **S** (Set collection) — The set of all sets. A subset of T. Sets are defined at this layer, not in core.
- **O** (Obligations) — The set of all rules and conditions that govern the world.

Relationship: **U ⊆ T** and **S ⊆ T**.

### Equality as a Relation

Equality is not hardcoded into objects at the core level beyond identity. At the logic engine layer, equality is modeled as a **relation** — itself a set of 2-tuples.

A 2-tuple **(a, b)** is an element of an equality relation **E** if and only if certain conditions are satisfied. Those conditions are defined as obligations in **O**.

This means:

- Different equality relations can coexist (e.g., equality by name, equality by structure, equality by identity).
- Each equality relation is a set: **E ⊆ T x T**.
- Membership in E is determined by evaluating the associated obligation.

### Obligations

An obligation is a rule stored in **O** that encodes conditions as an **abstract syntax tree (AST)**.

Each obligation's AST represents the logical expression that must be satisfied for some fact to hold (e.g., for two objects to be considered equal under a given relation, or for an entity to be a member of a given set).

Obligations are the mechanism by which user input (commands or files) influences the behavior of the engine. User-defined conditions are parsed into ASTs and stored as obligations in O.

When an obligation is activated and its antecedent holds, its consequent is materialized into the world by decomposing it into base-case operations:

- **MEMBER_OF(x, S)** — set M[x][S] = 1.
- **NOT(MEMBER_OF(x, S))** — set M[x][S] = 0 (known non-membership, distinct from unknown).
- **AND(A, B)** — materialize both sides.
- **OR(A, B)** — create a new obligation: (NOT A) → B. The disjunction is recorded as a constraint, not resolved immediately.
- **LITERAL "true"** — set M[object][target_set] = 1.

Materialization can also create new objects — entities, sets, relations, maps, tuples — in T. The world grows in facts, objects, and obligations as the proof progresses.

---

## Summary

```
Core layer:         Object, Entity, Tuple
                    Identity-only equality. No user-defined logic.

Logic engine layer: W = (U, S, T, O)
                    Equality = relation (set of 2-tuples)
                    Conditions = obligations in O (stored as ASTs)
                    User input → parsed into ASTs → stored as obligations
```
