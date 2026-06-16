# Logic Execution Engine — Development Plan

## Current State

The core layer has partial implementations:
- **Object** — implemented (getType, equals, hashCode, clone, toString)
- **Entity** — implemented (identity), inherits Object
- **Tuple** — header and source files exist but are empty
- **types.hpp** — defines ObjectType enum (Object, Entity, Tuple)
- **logic_engine** — header and source files exist but are empty

Everything else (logic engine layer, storage, parser) does not exist yet.

---

## Phase 1: Complete the Core Layer

Finish the foundational C++ types that everything else builds on.

### 1.1 — Object: add unique ID

Add a monotonically increasing integer ID to Object, assigned at construction. This ID is how the storage tables and membership matrix reference objects.

- Add `id` field to Object
- Add static counter for ID generation
- Add `getId()` method
- Update `hashCode()` to use ID instead of address (stable across sessions)

### 1.2 — Tuple

Implement n-ary Tuple as a subclass of Object.

- Store elements as a vector of Object pointers
- Access by position (0-indexed)
- `getType()` returns Tuple
- `toString()` returns "(elem0, elem1, ...)"
- `clone()` returns a new Tuple with cloned elements

**Note:** This implements *concrete* tuples only. Generic tuple support is a separate planned feature (see Phase 9).

### 1.3 — Extend types.hpp

Add sys_type enum alongside the existing core ObjectType.

```
enum class SysType { SET, REL, MAP, ENTITY, TUPLE };
```

---

## Phase 2: Storage Tables

Implement the persistent storage layer. These tables hold the durable state of the world.

### 2.1 — Object Table

In-memory table: id → (core_type, sys_type, hash, Object*).

- Insert on object creation
- Lookup by ID
- Scan/filter by sys_type

### 2.2 — Membership Table

In-memory table of (object_id, set_id) pairs.

- Insert/remove membership facts
- Query by object_id: "what sets is this in?"
- Query by set_id: "what objects are in this set?"

### 2.3 — Tuple Element Table

In-memory table of (tuple_id, position, element_id).

- Populated when a Tuple is created
- Query by tuple_id to reconstruct a tuple's elements

### 2.4 — AST Node Table + AST Child Table

In-memory tables for storing condition trees.

- AST Node Table: node_id → (node_type, value)
- AST Child Table: (parent_node_id, position) → child_node_id
- Insert nodes and edges when obligations are created
- Traverse from root to evaluate

### 2.5 — Obligation Table

In-memory table: id → (target_id, antecedent_root_id, consequent_root_id).

- Insert when obligations are added
- Query by target_id: "what obligations govern this set?"

---

## Phase 3: Membership Matrix

The runtime computation structure.

### 3.1 — Sparse binary incidence matrix

- Indexed by object ID (rows) and set ID (columns)
- M[i][j] = 1 iff object i is a member of set j
- Supports bitwise operations (AND, OR, NOT) over rows and columns

### 3.2 — Sync with Membership Table

- Load matrix from Membership Table at startup
- When membership changes, update both matrix and table

---

## Phase 4: World

The top-level container W = (U, S, T, O).

### 4.1 — World class

- Owns the Object Table, Membership Table, Tuple Element Table, Obligation Table, AST tables, and Membership Matrix
- Provides operations:
  - `createEntity()` → new entity in T and U
  - `createSet(sys_type)` → new set (SET, REL, or MAP) in T and S
  - `createTuple(elements)` → new tuple in T, populate Tuple Element Table
  - `addMember(object_id, set_id)` → record in Membership Table + Matrix
  - `removeMember(object_id, set_id)`
  - `isMember(object_id, set_id)` → check matrix
  - `addObligation(target_id, antecedent_ast, consequent_ast)` → store in Obligation Table + AST tables

### 4.2 — Bootstrap type sets

At world creation, create the built-in type sets (SET, REL, MAP, ENTITY, TUPLE) as objects in T. These are the sets that sys_type maps to.

---

## Phase 5: AST Evaluation

The engine that evaluates condition trees.

### 5.1 — AST node types

Implement evaluation for each node type:
- LITERAL — return the value
- REFERENCE — resolve to an object in T
- UNARY_OP (NOT) — evaluate child, negate
- BINARY_OP (AND, OR) — evaluate children, combine
- FUNCTION_CALL (MEMBER_OF) — check the membership matrix
- QUANTIFIER (FOR ALL, EXISTS) — iterate over a set, evaluate condition for each

### 5.2 — Obligation evaluation

- Given a query (e.g., "is x in S?"), look up obligations targeting S
- For each obligation: evaluate antecedent; if true, evaluate consequent
- Lazy: only evaluate when queried
- Cycle detection: track visited obligations during evaluation to prevent infinite loops

---

## Phase 6: Parser

Translate language L into world operations.

### 6.1 — Define language L

Design the syntax for:
- Declaring entities
- Defining sets, relations, maps
- Asserting membership
- Stating obligations (if A then B)
- Making claims (to be verified)

### 6.2 — Lexer

Tokenize L source files.

### 6.3 — Parser

Parse tokens into world operations. Conditions in obligations are parsed into ASTs and stored via the AST tables.

### 6.4 — Executor

Execute parsed operations against the World, sequentially.

---

## Phase 7: Persistence

Save and restore world state between sessions.

### 7.1 — Serialize tables to disk

Write Object Table, Membership Table, Tuple Element Table, AST tables, and Obligation Table to files.

### 7.2 — Load from disk

Read tables from files, reconstruct in-memory objects, rebuild the Membership Matrix.

---

## Phase 8: Standard Library

Build the standard library as pre-constructed world state — objects and obligations expressed entirely in terms of the foundational primitives.

### 8.1 — ℕ (Natural Numbers)

- Create a set object `ℕ` in the world
- Bootstrap entities for `0`, `1`, `2`, ... as needed, or define them via a successor relation
- Add Peano axioms as obligations: zero is a natural number, successor of a natural number is a natural number, no two numbers have the same successor, zero is not a successor

### 8.2 — ARITY_OF

- Create a `SysType::MAP` object `ARITY_OF` with domain `TUPLE` and codomain `ℕ`
- When a concrete tuple of arity n is created, automatically assert `(t, n) ∈ ARITY_OF`
- For generic tuples, the proof asserts this manually

### 8.3 — ELEMENT_AT

- Create a `SysType::MAP` object `ELEMENT_AT` with domain `TUPLE × ℕ` and codomain the universe
- When a concrete tuple is created, automatically assert `((t, i), eᵢ) ∈ ELEMENT_AT` for each position i
- For generic tuples, the proof asserts this manually

### 8.4 — TUPLE semantics

- Add obligations enforcing what it means to be a TUPLE: every TUPLE object has exactly one arity under `ARITY_OF`, and for each position `i < arity`, there exists exactly one element under `ELEMENT_AT`

### 8.5 — REL semantics

- Add obligations enforcing what it means to be a REL: every member must be a TUPLE, its arity must match the relation's declared arity, and each element at position i must belong to the declared domain set Sᵢ

### 8.6 — MAP semantics

- Add obligations enforcing what it means to be a MAP: it is a REL with the additional uniqueness constraint — no two members may share the same input positions

### 8.7 — Equality

- Create an equivalence relation `EQ` with obligations for reflexivity, symmetry, and transitivity
- This is the standard equality — users may define additional or alternative equality relations

### 8.8 — Library loading mechanism

- Define a mechanism for selectively loading parts of the standard library into the world at startup
- The user specifies which library modules to import; the engine bootstraps the corresponding objects and obligations before proof execution begins

---

## Phase 9: Generic Tuples

With the standard library in place — specifically `ℕ`, `ARITY_OF`, and `ELEMENT_AT` — generic tuples can now be fully implemented.

### 9.1 — Generic tuple creation

Add `World::createGenericTuple()` that registers a `SysType::TUPLE` object backed by a base `Object` (not a `Tuple`). No entries are written to the Tuple Element Table. The object's tuple-hood is expressed through its `SysType` tag and obligations in O using `ARITY_OF` and `ELEMENT_AT`.

### 9.2 — Distinguish concrete from generic at the object table level

The Object Table already stores both `core_type` and `sys_type`. A generic tuple has `core_type = OBJECT` and `sys_type = TUPLE`. A concrete tuple has `core_type = TUPLE` and `sys_type = TUPLE`. No schema change is needed — the distinction is already encodable.

### 9.3 — Equality between generic and concrete tuples

Equality is a relation — a set of 2-tuples in the membership matrix. The proof establishes `(generic_tuple, concrete_tuple) ∈ EQ` by asserting membership of the 2-tuple in the standard equality relation. The engine never modifies the backing C++ objects of either party.

---

## Dependency Order

```
Phase 1 (Core)
    │
    ▼
Phase 2 (Storage Tables)
    │
    ▼
Phase 3 (Membership Matrix)
    │
    ▼
Phase 4 (World)
    │
    ▼
Phase 5 (AST Evaluation)
    │
    ▼
Phase 6 (Parser)
    │
    ├──▶ Phase 7 (Persistence)
    │
    ▼
Phase 8 (Standard Library)
    │
    ▼
Phase 9 (Generic Tuples)
```

Each phase depends on the ones above it. Phases cannot be reordered, but work within a phase can be parallelized. Phase 8 depends on the full engine (Phases 1–6) being complete, since the standard library is expressed in language L and executed against the world. Phase 9 depends on Phase 8 because generic tuple constraints require `ℕ`, `ARITY_OF`, and `ELEMENT_AT` to exist in the world.
