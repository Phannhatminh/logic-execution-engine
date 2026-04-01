# Logic Execution Engine — Project Description (Resume)

## One-Liner

A high-performance formal logic execution engine in C++17 that parses, compiles, and executes machine-generated proofs over a sparse ternary membership matrix, with persistence and serialization for long-running proof sessions.

---

## Project Description

Designed and built a **formal logic execution engine** from scratch in C++17 — a systems-level runtime that takes proofs written in a formal language and executes them step-by-step, constructing a mathematical universe of objects, sets, relations, and derived facts while verifying logical correctness at every step.

Unlike theorem provers (which search for proofs) or model checkers (which enumerate state spaces), this engine **executes** proofs as programs — analogous to a CPU executing instructions, where the proof is the program and the engine is the runtime.

---

## Technical Highlights

**Core Runtime & Data Structures**

- Built a **sparse ternary incidence matrix** as the central data structure — a membership matrix indexed by object ID and set ID, where each cell encodes one of three states (member, non-member, unknown) using a two-bitset representation
- Implemented fast **bitwise column operations** (AND, OR, NOT) over the matrix for efficient logical proposition evaluation across large object/set spaces
- Designed the world model **W = (U, S, T, O)** — a self-contained logical universe holding entities, sets, all typed objects, and inference rules, with monotonically growing state

**Storage Layer**

- Engineered six persistent storage tables (object registry, membership facts, tuple structure, AST nodes, AST edges, obligation rules) with bidirectional indexing for efficient forward and reverse lookups
- Implemented a **membership matrix ↔ table sync** protocol to maintain consistency between the runtime computation structure and the persistent storage layer
- Built a **binary serialization format** for snapshotting and restoring full world state to/from disk, enabling long-running proof sessions across process restarts

**Obligation Engine (Rule System)**

- Designed a **lazy obligation system** — inference rules stored as AST condition-tree pairs (antecedent → consequent) that fire only on explicit activation, avoiding the combinatorial explosion of eager forward-chaining
- Implemented **consequent materialization** — recursive decomposition of compound logical expressions into base operations: membership writes, object creation, and chained obligation generation (e.g., OR(A, B) materializes as a new obligation NOT(A) → B)
- Built **cycle detection** via visited-set tracking during obligation activation chains to prevent infinite loops in recursive rule sets

**Parser & Language Frontend**

- Designed a **formal language L** for expressing logical constructions: entity declarations, set/relation/map definitions, membership assertions, obligation rules, and verification claims
- Built a **hand-written recursive-descent parser** (lexer → token stream → AST → execution plan) that compiles L source into sequences of world-mutating operations
- Implemented an **executor** that runs parsed operation sequences against the world, supporting the full proof lifecycle: premise introduction, rule invocation, fact derivation, and claim verification

**AST Infrastructure**

- Built a table-backed AST representation (node table + child table) supporting dynamic condition trees with 9 node types: literals, references, unary/binary operators, quantifiers, function calls, and object-creation nodes
- Implemented a recursive **AST evaluator** with three-valued logic propagation (true, false, unknown) through the expression tree

---

## Architecture

```
Source (language L)
       │
       ▼
┌─────────────┐
│    Lexer     │  tokenization
└──────┬──────┘
       │
       ▼
┌─────────────┐
│   Parser     │  recursive-descent, AST construction
└──────┬──────┘
       │
       ▼
┌─────────────┐
│  Executor    │  operation sequencing
└──────┬──────┘
       │
       ▼
┌──────────────────────────────────────────┐
│              Engine Layer                 │
│  ┌───────────┐  ┌──────────────────────┐ │
│  │   World   │  │     Evaluator        │ │
│  │ W=(U,S,T,O)  │  AST eval, obligation│ │
│  └─────┬─────┘  │  activation, cycle   │ │
│        │        │  detection           │ │
│        │        └──────────────────────┘ │
└────────┼─────────────────────────────────┘
         │
         ▼
┌──────────────────────────────────────────┐
│            Storage Layer                  │
│  ObjectTable │ MembershipTable │ ASTTables│
│  TupleElementTable │ ObligationTable     │
│  ─────────────────────────────────────── │
│         MembershipMatrix (runtime)       │
│    sparse ternary, bitwise column ops    │
└──────────────────────────────────────────┘
         │
         ▼
┌──────────────────────────────────────────┐
│           Persistence Layer               │
│     binary serialization / restore        │
└──────────────────────────────────────────┘
```

---

## Key Design Decisions

- **No built-in logic** — the engine embeds no axioms, no type theory, no inference rules. All logic is user-defined via obligations, making it domain-agnostic (mathematics, business rules, legal reasoning, program verification)
- **Three-valued knowledge** — the ternary matrix distinguishes "known non-member" from "unknown," enabling open-world reasoning where absence of proof is not proof of absence
- **Types as sets** — no separate type system; type membership is expressed through the same membership primitive, with type hierarchy (SET ⊇ REL ⊇ MAP) enforced via obligations
- **Lazy evaluation** — obligations fire only on activation, not exhaustively, keeping execution tractable for large rule sets

---

## Tech Stack

**Language & Standard:** C++17

**Build & Tooling:**
- CMake 3.10+ — cross-platform build system with out-of-source builds
- Clang / GCC — compiled and tested on both toolchains
- compile_commands.json — LSP integration for IDE tooling (clangd)

**Testing:**
- Google Test (GTest) — unit and integration test framework
- CTest — test runner via CMake integration

**C++ Standard Library (key usage):**
- `std::variant` — type-safe tagged unions for heterogeneous AST evaluation results
- `std::unordered_map` / `std::unordered_set` — hash-based storage tables with O(1) average lookup
- `std::atomic` — lock-free monotonic ID generation for object identity
- `std::vector` — contiguous memory layout for tuple elements and AST child lists
- `std::bitset` — underlying representation for ternary matrix columns (two-bitset encoding)
- `std::fstream` — binary I/O for world state serialization and persistence

**Techniques:**
- Recursive-descent parsing (hand-written lexer and parser, no generator dependencies)
- Sparse ternary matrix with bitwise column operations (AND, OR, NOT)
- AST-based expression evaluation with three-valued logic propagation
- Custom binary serialization protocol (no external serialization libraries)
- Lazy forward-chaining rule engine with cycle detection

---

## Metrics

- ~5,000 lines of C++ (implementation + tests)
- 7-phase incremental build from core primitives to full parser and persistence
- Zero external runtime dependencies beyond the C++ standard library
- Comprehensive test coverage across membership operations, obligation activation, AST evaluation, materialization, parsing, and round-trip serialization
