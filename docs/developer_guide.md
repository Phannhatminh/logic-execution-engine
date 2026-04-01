# Logic Execution Engine — Developer Guide

## Architecture

The system has three layers. Each layer depends only on the layers below it.

```
┌──────────────────────────────────────────────┐
│  Engine Layer (engine/)                       │
│  World, Evaluator                            │
├──────────────────────────────────────────────┤
│  Storage Layer (storage/)                     │
│  ObjectTable, MembershipTable, MembershipMatrix, │
│  TupleElementTable, AstNodeTable, AstChildTable,  │
│  ObligationTable                              │
├──────────────────────────────────────────────┤
│  Core Layer (core/)                           │
│  Object, Entity, Tuple, types                │
└──────────────────────────────────────────────┘
```

**Core** — Minimal C++ types. No logic, no rules. Just identity and structure.
**Storage** — Tables and matrices. Persistent state and runtime computation structures.
**Engine** — The world model and its evaluator. All logical behavior lives here.

---

## Core Layer

### `core/types.hpp`

Three enums that classify everything in the system.

```cpp
enum class ObjectType { Object, Entity, Tuple };
```

The C++ class hierarchy tag. Every object is one of these at the C++ level.

```cpp
enum class SysType { SET, REL, MAP, ENTITY, TUPLE };
```

The logical role in the engine. A set is an Object at the C++ level but has `SysType::SET`. This exists to break a bootstrapping circularity — types are sets, but the type-sets need to exist before the matrix is populated.

```cpp
enum class AstNodeType {
  LITERAL, REFERENCE, UNARY_OP, BINARY_OP, QUANTIFIER, FUNCTION_CALL,
  CREATE_ENTITY, CREATE_TUPLE, CREATE_SET
};
```

AST node classification. The first six are for evaluation (conditions). The last three are for materialization only (object creation).

### `core/object.hpp` — Object

Base class for everything in the world.

- **`getId()`** — Unique integer ID, monotonically assigned at construction. This is the object's identity throughout the system — tables, matrix, and AST references all use this ID.
- **`getType()`** — Returns `ObjectType` enum.
- **`equals(other)`** — Identity comparison (same ID = same object).
- **`hashCode()`** — ID-based hash.
- **`clone()`** — Deep copy. Returns a new object with a new ID.
- **`toString()`** — String representation including type and ID.

ID generation uses `std::atomic<size_t>` — thread-safe, monotonically increasing, never reused.

### `core/entity.hpp` — Entity

Subclass of Object. A distinct individual with no internal structure. Two entities are the same iff they have the same ID.

- **`identity()`** — Returns pointer to self.

### `core/tuple.hpp` — Tuple

Subclass of Object. An ordered grouping of n objects.

- **`arity()`** — Number of elements.
- **`at(position)`** — Element at 0-indexed position.

Elements are stored as `Object*`. Tuples can nest — an element can itself be a tuple.

---

## Storage Layer

All tables are in-memory. Persistence (Phase 7) will serialize them to disk.

### `storage/object_table.hpp` — ObjectTable

One row per object in T. Lookup by ID, filter by SysType.

```cpp
struct ObjectRow {
  size_t id;
  ObjectType core_type;
  SysType sys_type;
  size_t hash;
  Object* pointer;    // runtime only, not persisted
};
```

- **`insert(obj, sys_type)`** — Register a new object.
- **`find(id)`** — Lookup by ID. Returns `ObjectRow*` or `nullptr`.
- **`findBySysType(type)`** — All objects of a given SysType.

### `storage/membership_table.hpp` — MembershipTable

Persistent record of positive membership facts. Each row is an `(object_id, set_id)` pair.

- **`insert(obj_id, set_id)`** — Add a membership fact. Returns false if already present.
- **`remove(obj_id, set_id)`** — Remove a membership fact.
- **`contains(obj_id, set_id)`** — Point query.
- **`setsOf(obj_id)`** — All sets this object belongs to.
- **`membersOf(set_id)`** — All objects in this set.

Backed by two `unordered_map<size_t, unordered_set<size_t>>` for bidirectional lookup.

### `storage/membership_matrix.hpp` — MembershipMatrix

Runtime ternary incidence matrix. The single source of truth for what holds in the world.

Each cell M[object_id][set_id] has three states:

- **UNKNOWN** — known bit = 0, value bit = don't care. No information.
- **MEMBER** — known bit = 1, value bit = 1. Known member.
- **NON_MEMBER** — known bit = 1, value bit = 0. Known non-member.

Implementation: two bitsets per row/column (known bits + value bits), using `vector<uint64_t>` for sparse storage. IDs are mapped to bit positions via `objectIndex_`/`setIndex_` hash maps.

- **`set(obj, set)`** — M[obj][set] = MEMBER.
- **`setNonMember(obj, set)`** — M[obj][set] = NON_MEMBER.
- **`clear(obj, set)`** — M[obj][set] = UNKNOWN.
- **`test(obj, set)`** — Returns true iff MEMBER.
- **`testNonMember(obj, set)`** — Returns true iff NON_MEMBER.
- **`query(obj, set)`** — Returns `MembershipState` enum.
- **`row(obj)`** — All sets this object is a MEMBER of.
- **`column(set)`** — All objects that are MEMBERs of this set.
- **`intersect(a, b)`** — Objects in both a AND b (bitwise AND on value bits).
- **`unite(a, b)`** — Objects in a OR b (bitwise OR on value bits).
- **`difference(a, b)`** — Objects in a but NOT b (bitwise AND-NOT on value bits).

### `storage/tuple_element_table.hpp` — TupleElementTable

Persists tuple structure: `(tuple_id, position, element_id)`.

- **`insert(tuple_id, pos, elem_id)`** — Record one element.
- **`elementsOf(tuple_id)`** — All elements, sorted by position.

### `storage/ast_tables.hpp` — AstNodeTable + AstChildTable

Two tables that together store condition trees.

**AstNodeTable** — one row per node:

```cpp
struct AstNodeRow {
  size_t node_id;
  AstNodeType node_type;
  string value;          // literal value, operator name, function name, or variable name
};
```

**AstChildTable** — tree edges:

```cpp
struct AstChildRow {
  size_t parent_node_id;
  size_t position;       // 0-indexed among siblings
  size_t child_node_id;
};
```

Node IDs use a separate atomic counter from object IDs — AST nodes are not objects in T.

### `storage/obligation_table.hpp` — ObligationTable

One row per obligation in O.

```cpp
struct ObligationRow {
  size_t id;
  size_t target_id;           // the set this obligation governs
  size_t antecedent_root_id;  // root AST node of "if" condition
  size_t consequent_root_id;  // root AST node of "then" condition
};
```

- **`insert(target, ante, consq)`** — Add an obligation. Returns its ID.
- **`findByTarget(set_id)`** — All obligations governing this set.

---

## Engine Layer

### `engine/logic_engine.hpp` — World

The top-level container: **W = (U, S, T, O)**.

Owns all objects (via `vector<unique_ptr<Object>>`), all tables, and the matrix. Everything goes through World.

**Object creation:**

- **`createEntity()`** — Creates an entity, adds to ObjectTable, registers in ENTITY type set.
- **`createSet(sys_type)`** — Creates a set/rel/map. Enforces type hierarchy: REL -> also in SET type set. MAP -> also in REL and SET.
- **`createTuple(elements)`** — Creates a tuple, populates TupleElementTable.

**Membership:**

- **`addMember(obj, set)`** — Sets M[obj][set] = MEMBER. Updates both table and matrix.
- **`setNonMember(obj, set)`** — Sets M[obj][set] = NON_MEMBER. Removes from table if present.
- **`removeMember(obj, set)`** — Sets M[obj][set] = UNKNOWN. Removes from table.
- **`isMember(obj, set)`** — True iff MEMBER.
- **`isNonMember(obj, set)`** — True iff NON_MEMBER.
- **`isUnknown(obj, set)`** — True iff UNKNOWN.

**Set operations:** `membersOf`, `setsOf`, `intersect`, `unite`, `difference` — delegate to the matrix.

**AST + Obligations:** `createAstNode`, `addAstChild`, `addObligation` — insert into the respective tables.

**Bootstrap:** On construction, World creates five type sets (SET, REL, MAP, ENTITY, TUPLE) and registers them. The type hierarchy (REL ⊆ SET, MAP ⊆ REL ⊆ SET) is established as membership facts in the matrix.

### `engine/evaluator.hpp` — Evaluator

Evaluates AST condition trees and activates obligations. Holds a mutable reference to World.

**Types:**

```cpp
using EvalResult = std::variant<bool, size_t, string>;
using Bindings = std::unordered_map<string, size_t>;
```

`EvalResult` is the return type of evaluation — a node can evaluate to a boolean, an object ID, or a string. `Bindings` maps variable names to object IDs.

**Public interface:**

- **`evaluate(node_id, bindings)`** — Evaluate an AST node. Dispatches by node type. Returns `EvalResult`.
- **`evaluateBool(node_id, bindings)`** — Evaluate and expect bool. Throws if result isn't bool.
- **`activateObligations(obj_id, set_id)`** — **The core execution method.** Finds obligations targeting `set_id`, evaluates antecedents with `self = obj_id`, and materializes consequents. Writes to the world.
- **`evaluateObligations(obj_id, set_id)`** — Read-only query: would obligations grant membership? Does not write.

**Evaluation dispatch (private):**

Each `AstNodeType` has a corresponding eval method:

- **LITERAL** (`evalLiteral`) — "true"->true, "false"->false, numeric string->size_t, else->string.
- **REFERENCE** (`evalReference`) — Look up variable name in bindings -> object ID.
- **UNARY_OP** (`evalUnaryOp`) — NOT: negate child.
- **BINARY_OP** (`evalBinaryOp`) — AND (short-circuit), OR (short-circuit), EQUALS (value comparison), IMPLIES (not-A or B).
- **QUANTIFIER** (`evalQuantifier`) — FORALL/EXISTS: iterate members of a set, bind each, evaluate body.
- **FUNCTION_CALL** (`evalFunctionCall`) — MEMBER_OF: check matrix.
- **CREATE_*** — Throws. Creation nodes cannot be evaluated, only materialized.

**Obligation activation (private):**

`activateObligationsImpl` loops through obligations targeting `set_id`:

1. Cycle detection via `visited` set of obligation IDs.
2. Bind `self = object_id`, `target = set_id`.
3. Evaluate antecedent with `evaluateBool`.
4. If true, call `materialize` on the consequent.

**Materialization (private):**

`materialize(node_id, bindings, default_obj, default_set)` walks the consequent AST:

- **LITERAL "true"** — `addMember(default_obj, default_set)`.
- **LITERAL "false"** — Return false (materialization fails).
- **MEMBER_OF(x, S)** — Evaluate x and S from bindings -> `addMember(x_id, S_id)`.
- **NOT(MEMBER_OF(x, S))** — Evaluate -> `setNonMember(x_id, S_id)`.
- **AND(A, B)** — Materialize A, then B. Bindings propagate left to right.
- **OR(A, B)** — Build NOT(A) AST node -> `addObligation(default_set, NOT_A, B)`.
- **CREATE_ENTITY("var")** — `createEntity()` -> bind new ID to "var" in bindings.
- **CREATE_TUPLE("var")** — Evaluate children -> `createTuple(elements)` -> bind to "var".
- **CREATE_SET("type")** — `createSet(sys_type)` -> bind to variable from child REFERENCE.

Bindings are mutable (`Bindings&`). Creation nodes write into them; subsequent AND siblings read from them.

---

## Data Flow

### Proof Execution Flow

```
Proof statement
      │
      ▼
  Parser (Phase 6, not yet implemented)
      │
      ▼
  World operation (create, assert, activate, verify)
      │
      ▼
  World W = (U, S, T, O)
      │
      ├─ Object Table        (identity)
      ├─ Membership Table    (persistent facts)
      ├─ Membership Matrix   (runtime truth, ternary)
      ├─ Tuple Element Table (tuple structure)
      ├─ AST Node Table      (condition nodes)
      ├─ AST Child Table     (condition edges)
      └─ Obligation Table    (rules)
```

### Obligation Activation Flow

```
activateObligations(object_id, set_id)
      │
      ├─ isMember? → true → return (already materialized)
      │
      ├─ for each obligation targeting set_id:
      │     │
      │     ├─ evaluateBool(antecedent, {self=object_id})
      │     │     │
      │     │     └─ recursive AST evaluation
      │     │           └─ MEMBER_OF → matrix.test()
      │     │
      │     └─ if true → materialize(consequent, bindings)
      │           │
      │           ├─ MEMBER_OF → addMember()
      │           ├─ NOT(MEMBER_OF) → setNonMember()
      │           ├─ AND → materialize left, then right
      │           ├─ OR → addObligation(NOT(left), right)
      │           ├─ CREATE_ENTITY → createEntity(), bind
      │           ├─ CREATE_TUPLE → createTuple(), bind
      │           └─ CREATE_SET → createSet(), bind
      │
      └─ return true/false
```

---

## Build and Test

### Build

```bash
cd build && cmake .. && make
```

Three build targets:
- `logic_engine` — main executable (`src/main.cpp`)
- `test_survey` — self-made test survey (`src/test_survey.cpp`)
- `run_tests` — Google Test suite (`test/`)

### Run Tests

```bash
# Self-made survey (64 tests)
./build/test_survey

# Google Test (26 tests)
./build/run_tests

# CTest (individual test discovery)
cd build && ctest --output-on-failure
```

### Test Organization

- **`test/test_membership.cpp`** (Membership suite) — Add/query, sets-of, members-of, set operations, type hierarchy, ternary matrix.
- **`test/test_obligations.cpp`** (Obligations suite) — Single-step, activation, chaining, AND, NOT, OR.
- **`test/test_ast.cpp`** (AstConnectives, AstQuantifiers suites) — AND, OR, NOT, IMPLIES, FORALL, EXISTS.
- **`test/test_materialization.cpp`** (Materialization suite) — CREATE_ENTITY, CREATE_TUPLE, CREATE_SET, successor pattern.

---

## Project Structure

```
logic_execution_engine/
├── CMakeLists.txt
├── docs/
│   ├── vision.md               — Why this system exists
│   ├── requirements.md         — Formal requirement specification
│   ├── design.md               — Architecture and design decisions
│   ├── data_structures.md      — All types, tables, and the matrix
│   ├── system_description.md   — Execution model
│   ├── materialization.md      — How obligation consequents become facts
│   ├── development_plan.md     — Phased development plan
│   ├── project_description_phase1-5.md — Current state summary
│   ├── user_guide.md           — This document (concepts for users)
│   └── developer_guide.md      — This document (architecture for developers)
├── include/
│   ├── core/                   — Object, Entity, Tuple, types
│   ├── storage/                — Tables and matrix
│   └── engine/                 — World and Evaluator
├── src/
│   ├── main.cpp                — Demo/examples
│   ├── test_survey.cpp         — Self-made test suite
│   ├── core/                   — Core implementations
│   ├── storage/                — Storage implementations
│   └── engine/                 — Engine implementations
└── test/
    ├── test_membership.cpp     — gtest: membership
    ├── test_obligations.cpp    — gtest: obligations
    ├── test_ast.cpp            — gtest: AST evaluation
    └── test_materialization.cpp — gtest: materialization + creation
```

---

## What's Not Implemented Yet

- **Parser (language L)** — Phase 6. Not started.
- **Persistence (serialize/load)** — Phase 7. Not started.
- **Obligation creation of objects** — Phase 5+. Implemented (CREATE_ENTITY, CREATE_TUPLE, CREATE_SET).
- **Ternary matrix** — Phase 5+. Implemented.
- **NOT materialization** — Phase 5+. Implemented.
- **OR materialization** — Phase 5+. Implemented.
