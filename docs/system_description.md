# Logic Execution Engine — System Description

## Overview

The Logic Execution Engine is a system that executes formal mathematical proofs. It consists of two components: a **parser** and a **world**.

The parser reads an input file written in a formal language **L** and translates it into a sequence of operations on the world. The world is the mathematical universe in which the proof takes place — it holds the objects, the structures between them, and the rules that govern them.

## Components

### Parser

The parser takes a source file in language L and produces a sequence of world operations. Language L is a formal proof language — its syntax and semantics are designed for expressing mathematical definitions, constructions, and proofs.

The parser translates L into:
- Creation of entities (individuals in the universe)
- Definition of sets, relations, and maps
- Establishment of membership facts
- Construction of tuples
- Addition of obligations (logical rules encoded as ASTs)
- Queries against the world (membership checks, condition evaluation)

### World — W = (U, S, T, O)

The world is the state of the mathematical universe being reasoned about.

- **T** (Object set) — All objects that exist.
- **U** (Universe) — All entities (individuals). U ⊆ T.
- **S** (Set collection) — All sets, relations, and maps. S ⊆ T.
- **O** (Obligations) — All logical rules. O is meta — outside the world being reasoned about.

The world's state changes as the parser feeds it operations derived from the proof. Each statement in L corresponds to one or more mutations or queries on W.

## Execution Model

```
Source file (language L)
        │
        ▼
      Parser
        │
        ▼
  World operations
        │
        ▼
   World W = (U, S, T, O)
        │
        ├── Object Table       — all objects with identity and type
        ├── Membership Matrix   — ternary incidence matrix (1=member, 0=known non-member, unset=unknown)
        ├── Membership Table    — persistent membership facts
        ├── Tuple Element Table — tuple structure
        ├── AST Node Table      — condition tree nodes
        ├── AST Child Table     — condition tree edges
        └── Obligation Table    — rules (A → B)
```

1. The parser reads the source file line by line (or statement by statement).
2. Each statement is parsed into one or more world operations.
3. The engine executes each operation against W — creating objects, recording memberships, adding obligations.
4. When the proof invokes a rule, the engine activates the corresponding obligation — evaluating its antecedent and, if it holds, materializing the consequent into the world. Materialization decomposes the consequent into base-case operations: writing positive membership (M[x][S] = 1), writing known non-membership (M[x][S] = 0), creating new objects in T, or creating new obligations in O (for disjunctive consequents where OR(A, B) becomes the obligation (NOT A) → B).
5. When the proof makes a claim, the engine evaluates it by querying the world — checking membership in the matrix.
6. If a claim holds, execution continues. If it does not, the proof is invalid at that step.

## Purpose

The engine provides a computational foundation for verifying formal mathematical proofs. Language L allows mathematicians to express definitions, axioms, theorems, and proofs. The engine translates these into concrete operations on a world of objects, sets, and logical rules, and determines whether the proof holds.
