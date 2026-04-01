# Logic Execution Engine — Project Description

## What It Is

A C++17 engine that executes formal mathematical proofs. It takes proofs written in a formal language L, parses them into operations on a mathematical world, and evaluates whether the proofs hold.

## Architecture

```
Source file (language L)  →  Parser  →  World operations  →  World W = (U, S, T, O)
```

The system has three layers:

### Core Layer (`include/core/`, `src/core/`)

Primitive building blocks. Minimal, no user-defined logic.

- **`types.hpp`**
  - `ObjectType` — Core type enum: OBJECT, ENTITY, TUPLE.
  - `SysType` — System type enum: SET, REL, MAP, ENTITY, TUPLE.
  - `AstNodeType` — AST node types: LITERAL, REFERENCE, UNARY_OP, BINARY_OP, QUANTIFIER, FUNCTION_CALL, CREATE_ENTITY, CREATE_TUPLE, CREATE_SET.
- **`object.hpp/cpp`** — `Object`. Base type. Has unique ID, identity equality, ID-based hash, toString.
- **`entity.hpp/cpp`** — `Entity`. Distinct individual. Identity is the only equality.
- **`tuple.hpp/cpp`** — `Tuple`. N-ary ordered grouping of objects.

### Storage Layer (`include/storage/`, `src/storage/`)

Persistent tables and runtime computation structures.

- **`object_table`** — `ObjectTable`. id -> (core_type, sys_type, hash, pointer). Lookup by ID, filter by sys_type.
- **`membership_table`** — `MembershipTable`. (object_id, set_id) pairs. Bidirectional: setsOf(obj), membersOf(set).
- **`tuple_element_table`** — `TupleElementTable`. (tuple_id, position, element_id). Persists tuple structure.
- **`ast_tables`**
  - `AstNodeTable` — node_id -> (node_type, value). Stores condition tree nodes.
  - `AstChildTable` — (parent_node_id, position) -> child_node_id. Stores tree edges.
- **`obligation_table`** — `ObligationTable`. id -> (target_id, antecedent_root_id, consequent_root_id). Rules as A -> B.
- **`membership_matrix`** — `MembershipMatrix`. Sparse ternary incidence matrix (1=member, 0=known non-member, unset=unknown). Supports bitwise AND/OR/NOT over columns for fast logical proposition evaluation.

### Engine Layer (`include/engine/`, `src/engine/`)

The world and its evaluator.

- **`logic_engine`** — `World`. Top-level container for W = (U, S, T, O). Owns all tables, matrix, and object lifetime. Bootstraps type sets on creation.
- **`evaluator`** — `Evaluator`. Evaluates AST condition trees, activates obligations with materialization (including object creation), and evaluates obligations lazily against the world.

## Key Concepts

**Types are sets.** An object's type is the set it belongs to. The type hierarchy SET ⊇ REL ⊇ MAP is expressed as set membership in the membership matrix.

**Membership is the primitive relation.** All relationships in the system — typing, subtyping, relations, equality — reduce to "object x is a member of set S." This is stored in a sparse ternary incidence matrix (1 = member, 0 = known non-member, unset = unknown) for fast bitwise computation, backed by a membership table for persistence. Known non-membership (0) is a positive fact distinct from the unknown/unset default.

**Obligations are lazy.** An obligation is a rule A → B (if A then B), where A and B are AST condition trees. Obligations only fire when explicitly activated by the proof — they are not exhaustively evaluated. When activated, the antecedent is checked and, if it holds, the consequent is materialized into the world by decomposing it into base-case operations: positive membership (M[x][S] = 1), known non-membership (M[x][S] = 0), new objects in T, or new obligations in O (for disjunctive consequents where OR(A, B) becomes the obligation (NOT A) → B). Activation can chain: facts materialized by one obligation become visible to subsequent activations.

**ASTs are dynamically constructed condition trees.** Each node evaluates based on its type (AND, OR, NOT, MEMBER_OF, FORALL, EXISTS, etc.) and delegates to its children. The parser will construct these trees from user input at runtime.

## Current State

Phases 1–5 of the development plan are complete:

- [x] Phase 1: Core layer (Object with ID, Entity, Tuple, SysType enum)
- [x] Phase 2: Storage tables (6 tables)
- [x] Phase 3: Membership matrix (sparse bitset, bitwise operations)
- [x] Phase 4: World (top-level container, bootstrap, all operations)
- [x] Phase 5: AST evaluation (all node types, obligation evaluation with cycle detection)
- [ ] Phase 6: Parser (language L definition, lexer, parser, executor)
- [ ] Phase 7: Persistence (serialize/load tables to/from disk)

## Project Structure

```
logic_execution_engine/
├── CMakeLists.txt
├── docs/
│   ├── design.md
│   ├── data_structures.md
│   ├── system_description.md
│   ├── development_plan.md
│   └── project_description.md
├── include/
│   ├── core/
│   │   ├── types.hpp
│   │   ├── object.hpp
│   │   ├── entity.hpp
│   │   └── tuple.hpp
│   ├── storage/
│   │   ├── object_table.hpp
│   │   ├── membership_table.hpp
│   │   ├── membership_matrix.hpp
│   │   ├── tuple_element_table.hpp
│   │   ├── ast_tables.hpp
│   │   └── obligation_table.hpp
│   └── engine/
│       ├── logic_engine.hpp
│       └── evaluator.hpp
└── src/
    ├── main.cpp
    ├── core/
    │   ├── object.cpp
    │   ├── entity.cpp
    │   └── tuple.cpp
    ├── storage/
    │   ├── object_table.cpp
    │   ├── membership_table.cpp
    │   ├── membership_matrix.cpp
    │   ├── tuple_element_table.cpp
    │   ├── ast_tables.cpp
    │   └── obligation_table.cpp
    └── engine/
        ├── logic_engine.cpp
        └── evaluator.cpp
```
