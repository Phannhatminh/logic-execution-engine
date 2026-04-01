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
    ▼
Phase 7 (Persistence)
```

Each phase depends on the ones above it. Phases cannot be reordered, but work within a phase can be parallelized.
