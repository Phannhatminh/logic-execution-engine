# Logic Execution Engine — Requirement Specification

## 1. Purpose

The engine executes formal mathematical proofs written in language L. It constructs the finite mathematical universe that a proof brings into existence and verifies that every logical step holds within that universe.

---

## 2. World

### R-W1: World State

The engine shall maintain a world W = (U, S, T, O) where:

- **T** is the set of all objects that exist.
- **U** is the set of all entities (individuals). U ⊆ T.
- **S** is the set of all sets, relations, and maps. S ⊆ T.
- **O** is the set of all obligations. O is meta — obligations are not objects in T.

### R-W2: Monotonic Growth

The world shall grow monotonically during proof execution. Objects and facts may be added but not removed. The world at step N contains everything from steps 1 through N.

### R-W3: World as Source of Truth

The membership matrix shall be the single source of truth for what holds in the world at any point during execution. All queries about the state of the proof shall be answered by consulting the matrix.

---

## 3. Objects

### R-O1: Object Identity

Every object shall have a unique integer ID assigned at creation. This ID is the object's identity within the world.

### R-O2: Object Types

Objects are classified by core type (Object, Entity, Tuple) and system type (SET, REL, MAP, ENTITY, TUPLE).

### R-O3: Entity

An entity is a distinct individual in the universe. Two entities are equal if and only if they are the same entity (same ID). An entity has no internal structure beyond its identity.

### R-O4: Tuple

A tuple is an ordered grouping of n objects (n ≥ 0). Tuples may be nested — an element of a tuple may itself be a tuple. Tuples are the structural unit for relations: a binary relation contains 2-tuples, an n-ary relation contains n-tuples.

### R-O4a: Concrete Tuple

A concrete tuple is one whose elements are fully specified at creation time. It is backed by a C++ `Tuple` object and its elements are stored in the Tuple Element Table.

### R-O4b: Generic Tuple

A generic (schematic) tuple is one introduced without fully specified elements — for example, "let t be an n-ary tuple over S." It is registered in the world with `SysType::TUPLE` but backed by a base `Object` at the C++ level. Its properties (arity, domain membership) are encoded as obligations. The engine shall not require the backing object to be upgraded when the tuple is later instantiated.

### R-O4c: Tuple Equality

A generic tuple and a concrete tuple may be declared equal via an equality relation — a set of 2-tuples in the membership matrix. This is the mechanism by which a generic tuple is instantiated: the proof establishes `(generic, concrete) ∈ E` for some equality relation E, without modifying the backing C++ objects of either.

### R-O5: Set

A set is an object in which other objects can be members. Sets are objects and can themselves be members of other sets.

### R-O6: Relation

A relation is a set (sys_type REL) whose members are n-tuples. A relation on domains S1 × S2 × ... × Sn only accepts n-tuples where the element at position i is a member of Si. This constraint is enforced by obligations.

### R-O7: Map

A map is a relation (sys_type MAP) with the additional constraint that each combination of input positions (all positions except the last) maps to exactly one output. This constraint is enforced by obligations.

### R-O8: Type Hierarchy

SET ⊇ REL ⊇ MAP. Every map is a relation, and every relation is a set. Types are sets — an object's type is the set it belongs to. Subtyping is the subset relation.

---

## 4. Membership

### R-M1: Membership as Primitive

Membership (x ∈ S) is the fundamental relation in the system. All facts — typing, properties, relations, equality — are encoded as membership.

### R-M2: Membership Matrix

The engine shall maintain a sparse ternary incidence matrix M where each cell M[i][j] has three possible states:

- **1** — object i is known to be a member of set j.
- **0** — object i is known to not be a member of set j.
- **unset** — the membership of object i in set j is unknown.

Unknown is the default state. It is distinct from known non-membership (0). A cell set to 0 represents a positive fact established by the proof — "x is not in S" — not merely the absence of information.

The matrix shall support bitwise operations (AND, OR, NOT) over rows and columns.

### R-M3: Membership Table

The engine shall maintain a persistent membership table recording all membership facts as (object_id, set_id) pairs. The membership matrix is the runtime representation; the membership table is the persistent representation. Both shall be kept in sync.

### R-M4: Add Membership

The engine shall support adding a positive membership fact (x ∈ S), setting M[x][S] = 1, and updating both the membership table and the membership matrix.

### R-M4a: Add Non-Membership

The engine shall support adding a negative membership fact (x ∉ S), setting M[x][S] = 0. This records that x is known to not be a member of S — distinct from the unknown/unset state.

### R-M5: Query Membership

The engine shall support querying:

- Is x ∈ S? (point query)
- What are the members of S? (members-of query)
- What sets is x a member of? (sets-of query)

### R-M6: Set Operations

The engine shall support computing intersect, union, and difference of two sets via bitwise operations on the membership matrix.

---

## 5. Obligations

### R-OB1: Obligation Structure

An obligation is a rule of the form A → B (if A then B), where:

- **Target** — the set this obligation governs.
- **Antecedent (A)** — an AST condition tree encoding the "if" condition.
- **Consequent (B)** — an AST condition tree encoding the "then" condition.

### R-OB2: Obligations Are Meta

Obligations belong to O, not T. They are not objects in the world being reasoned about. They observe and constrain; they do not participate as members of sets.

### R-OB3: Obligations Are Lazy

Obligations shall not fire automatically. They exist in the world but remain inert until explicitly activated by the proof.

### R-OB4: Obligation Activation

When the proof invokes a rule, the engine shall activate the corresponding obligation:

1. Evaluate the antecedent against the current world state.
2. If the antecedent holds, evaluate the consequent.
3. If the consequent holds, **materialize the results** into the world by decomposing the consequent AST into base-case operations:
   - **MEMBER_OF(x, S)** — set M[x][S] = 1 (positive membership).
   - **NOT(MEMBER_OF(x, S))** — set M[x][S] = 0 (known non-membership).
   - **AND(A, B)** — materialize A, then materialize B.
   - **OR(A, B)** — create a new obligation encoding the disjunctive constraint: (NOT A) → B. The engine does not choose which side holds — the proof drives that. The obligation is recorded in O and enforced when the proof later activates it.
   - **LITERAL "true"** — set M[object_id][target_set_id] = 1, using the obligation's implicit target.
   - Create new objects — entities, sets, relations, maps, tuples — into T.
   - Establish memberships for newly created objects.

All materialized objects, facts, and obligations become part of the world and are visible to all subsequent operations.

### R-OB5: Multi-Step Chaining

Obligation activation shall support chaining. A fact materialized by activating obligation 1 shall be visible when evaluating the antecedent of obligation 2. This is achieved by writing to the matrix before the next activation, not by recursive obligation-aware evaluation.

### R-OB6: Cycle Detection

The engine shall detect cycles during obligation evaluation and terminate gracefully rather than entering an infinite loop.

---

## 6. AST Condition Trees

### R-AST1: Dynamic Construction

Condition trees shall be constructed dynamically at runtime from parsed proof input. Different proofs produce different trees.

### R-AST2: Node Types

The engine shall support the following AST node types:

- **LITERAL** — A constant value: boolean, integer, or string.
- **REFERENCE** — A reference to a bound variable, resolved from bindings.
- **UNARY_OP** — Single-operand operation. Supported: NOT.
- **BINARY_OP** — Two-operand operation. Supported: AND, OR, EQUALS, IMPLIES.
- **QUANTIFIER** — Quantification over a finite set. Supported: FORALL, EXISTS.
- **FUNCTION_CALL** — Application of a named function. Supported: MEMBER_OF.
- **CREATE_ENTITY** — Materialization only. Create a new entity, bind to variable name in value.
- **CREATE_TUPLE** — Materialization only. Create a new tuple from children, bind to variable name in value.
- **CREATE_SET** — Materialization only. Create a new set/rel/map (type in value), bind via child REFERENCE.

### R-AST3: Evaluation

Each AST node shall be evaluatable given a set of variable bindings. Evaluation proceeds recursively from the root, with each node type determining how it combines its children's results.

### R-AST4: Short-Circuit Evaluation

AND shall short-circuit on false (do not evaluate the right operand if the left is false). OR shall short-circuit on true. IMPLIES (A → B) shall short-circuit when A is false (the implication holds vacuously).

### R-AST5: Quantifier Evaluation

FORALL and EXISTS shall iterate over the members of a specified set, binding each member to a variable and evaluating the body. FORALL returns true iff the body holds for every member. EXISTS returns true iff the body holds for at least one member.

### R-AST6: Storage

AST nodes shall be stored in an AST Node Table (node_id → node_type, value). Tree structure shall be stored in an AST Child Table (parent_node_id, position → child_node_id).

---

## 7. Proof Execution

### R-PE1: Sequential Execution

The engine shall execute proof statements sequentially, in the order they appear in the source file. Each statement translates to one or more world operations.

### R-PE2: Proof Operations

The engine shall support the following categories of proof operations:

- **Introduce** — Create a new object (entity, set, relation, map, tuple) and add it to T.
- **Assert membership** — Record x ∈ S in the membership matrix.
- **Assume** — Record a fact as given (membership or relation).
- **Define obligation** — Add a rule A → B to O, targeting a specific set.
- **Activate obligation** — Evaluate an obligation's antecedent; if it holds, materialize the consequent.
- **Verify claim** — Query the world to check whether a statement holds. If it does not, the proof is invalid at this step.

### R-PE3: Proof Validity

A proof is valid if and only if every verification step succeeds — every claim holds in the world at the point where it is made. If any verification fails, execution halts and the engine reports the failing step.

### R-PE4: Error Reporting

When a proof step fails, the engine shall report:

- Which step failed.
- What claim was made.
- What the relevant world state was (which facts were present, which were missing).

---

## 8. Language L

### R-L1: Formal Language

The engine shall accept proofs written in a formal language L. Language L shall have syntax and semantics for expressing:

- Entity declarations
- Set, relation, and map definitions
- Membership assertions
- Assumptions
- Obligation definitions (axioms, inference rules)
- Obligation activations (invoking a rule)
- Claims (to be verified)

### R-L2: Parser

The engine shall include a parser that translates language L source files into a sequence of world operations. The parser shall process the source file statement by statement.

### R-L3: AST Construction

Conditions appearing in obligations and claims shall be parsed into AST condition trees and stored in the AST tables.

---

## 9. Storage

### R-S1: Tables

The engine shall maintain the following persistent tables:

- **Object Table** — Key: object_id. Contents: core_type, sys_type, hash.
- **Membership Table** — Key: (object_id, set_id). Contents: membership facts.
- **Tuple Element Table** — Key: (tuple_id, position). Contents: element_id.
- **AST Node Table** — Key: node_id. Contents: node_type, value.
- **AST Child Table** — Key: (parent_node_id, position). Contents: child_node_id.
- **Obligation Table** — Key: obligation_id. Contents: target_id, antecedent_root_id, consequent_root_id.

### R-S2: Persistence

The engine shall be able to serialize all tables to disk and restore them, including rebuilding the membership matrix from the membership table.

---

## 10. Standard Library

### R-SL1: Two-Level Architecture

The engine shall maintain a strict separation between two levels:

- **Foundation** — entities, sets, and membership. These are engine primitives and cannot be redefined.
- **Standard library** — all concepts built on top of the foundation (tuples, relations, maps, natural numbers, arity, element access, equality). These are pre-constructed objects and obligations, not engine primitives.

### R-SL2: Standard Library as World State

Each standard library concept shall be expressed entirely as objects and obligations in the world — using only entities, sets, and membership as primitives. The engine shall contain no hardcoded knowledge of what a tuple, relation, map, or natural number is beyond their `SysType` bootstrap tag.

### R-SL3: Standard Library Contents

The engine shall ship with a standard library providing at minimum:

- **TUPLE** — an object representing an ordered n-tuple, with arity and positional element access defined as obligations.
- **REL** — a set whose members are tuples constrained by domain sets, defined as obligations.
- **MAP** — a relation with a uniqueness constraint on inputs, defined as obligations.
- **ℕ** — natural numbers as entities with Peano axioms as obligations.
- **ARITY_OF** — a map from tuples to ℕ, populated automatically for concrete tuples at creation time.
- **ELEMENT_AT** — a map from (tuple × ℕ) to objects, populated automatically for concrete tuples at creation time.
- **Equality** — an equivalence relation with reflexivity, symmetry, and transitivity as obligations.

### R-SL4: Dependency Order

Standard library concepts shall be constructed in dependency order:

```
Foundation (entity, set, membership)
    │
    ▼
ℕ
    │
    ▼
ARITY_OF, ELEMENT_AT
    │
    ▼
TUPLE semantics (arity and element obligations)
    │
    ▼
REL semantics (domain constraints)
    │
    ▼
MAP semantics (uniqueness constraint)
```

### R-SL5: User Optionality

The standard library shall be optional. The user may choose to import any subset of it, or none at all, and define their own concepts from foundational primitives. The engine shall not impose standard library definitions on the user's world.

### R-SL6: Automatic Population for Concrete Objects

When a concrete tuple is created, the engine shall automatically assert the corresponding facts in `ARITY_OF` and `ELEMENT_AT` — recording `(t, n) ∈ ARITY_OF` and `(t, i, eᵢ) ∈ ELEMENT_AT` for each element — provided those maps have been loaded from the standard library. If they have not been loaded, no automatic population occurs.

---

## 11. Constraints

### R-C1: No Built-In Arithmetic

The engine does not provide built-in arithmetic operations. Arithmetic (successor, addition, etc.) is modeled through relations and obligations defined by the proof's axiom system, not hardcoded into the engine.

### R-C2: No Proof Search

The engine does not search for proofs. It executes proofs provided by the human. If a step is missing or incorrect, the engine rejects — it does not attempt to fill gaps.

### R-C3: No Built-In Type System

Beyond the bootstrap types (SET, REL, MAP, ENTITY, TUPLE) and the sys_type mechanism that breaks the circularity of self-typing, the engine imposes no type system. Types are sets. Subtyping is subset. Type constraints are obligations.

### R-C4: Finite Construction

The engine constructs only what the proof constructs. It does not enumerate or model infinite domains. The finiteness of the constructed universe is inherent to the model, not a limitation.
