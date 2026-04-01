# Logic Execution Engine — User Guide

## What Is This?

The Logic Execution Engine executes formal mathematical proofs. You write a proof in a formal language, and the engine constructs the mathematical universe your proof describes — object by object, fact by fact — verifying that every step is valid.

The engine does not search for proofs. It does not guess what you mean. You provide the reasoning; it builds the reality and checks that it holds.

---

## Core Concepts

### The World

When the engine runs your proof, it maintains a **world** — the mathematical universe your proof constructs. The world contains:

- **Objects** — the things that exist (entities, sets, relations, maps, tuples)
- **Membership facts** — which objects belong to which sets
- **Obligations** — the logical rules that govern derivation

Everything your proof talks about lives in the world. If you say "let n be a natural number," the engine creates an entity n and records that n belongs to the set of natural numbers. If you invoke an axiom, the engine activates the corresponding obligation and writes the derived fact into the world.

### Objects

There are five kinds of objects:

- **Entity** — A distinct individual. Has no internal structure — only identity. Example: a natural number n, a point P.
- **Set** — A collection. Other objects can be members of it, or not. Example: the set of natural numbers N.
- **Relation** — A set whose members are tuples, constrained by domain sets. Example: the less-than relation < on N.
- **Map** — A relation where each input maps to exactly one output. Example: the successor function S: N -> N.
- **Tuple** — An ordered grouping of objects. Example: the pair (n, S(n)).

Every object has a unique ID. Objects are created as the proof introduces them and persist for the remainder of execution.

### Types Are Sets

There is no separate type system. An object's type is simply the set it belongs to. When you say "n is a natural number," you are saying n ∈ ℕ — a membership fact, nothing more.

Subtyping is the subset relation. If every element of A is also an element of B, then A is a subtype of B. This is enforced by obligations, not by the engine.

The built-in type hierarchy is:

```
SET ⊇ REL ⊇ MAP
```

Every map is a relation, and every relation is a set.

### Membership — The Universal Primitive

Everything in the world is expressed as membership:

- **n ∈ ℕ** — n is a natural number.
- **P(n)** — n has property P (n ∈ P).
- **R(a, b)** — a and b are related by R (the tuple (a,b) ∈ R).
- **a = b** — a equals b under equivalence E (the tuple (a,b) ∈ E).
- **A ⊆ B** — A is a subset of B (for all x ∈ A, x ∈ B).

The **membership matrix** is the single source of truth. Each cell M[x][S] has three states:

- **1 (member)** — x is known to be in S.
- **0 (non-member)** — x is known to NOT be in S.
- **unset (unknown)** — No information about x and S.

Non-membership is a fact, not the absence of information. If you prove that n ∉ S, the engine records 0, which is different from never having mentioned n and S together.

### Obligations — The Rules

An obligation is a rule of the form **"if A then B"** — an antecedent and a consequent, both expressed as condition trees.

Obligations encode:
- Axioms ("for all x, if x ∈ ℕ then x has a successor")
- Inference rules ("if P and P → Q, then Q")
- Definitions ("x ∈ EvenNumbers iff ∃k ∈ ℕ such that x = 2k")
- Constraints ("if x ∈ Relation, then x must be a tuple")

Obligations are **lazy**. They sit in the world and do nothing until your proof explicitly activates them. The engine is a bookkeeper — it records what you tell it and executes what you command. It does not reason forward on its own.

### Activation — Making Things Happen

When your proof invokes a rule, the engine **activates** the corresponding obligation:

1. Check the antecedent against the current world state.
2. If it holds, **materialize** the consequent — decompose it into concrete world operations.
3. The derived facts and objects become part of the world, visible to all subsequent steps.

Materialization handles compound consequents by decomposition:

- **x ∈ S** — Record the membership fact.
- **NOT(x ∈ S)** — Record that x is known to not be in S.
- **A AND B** — Materialize A, then materialize B.
- **A OR B** — Create a new obligation: (NOT A) → B. The engine doesn't pick a side — your proof does.
- **CREATE_ENTITY** — Create a new entity in the world.
- **CREATE_TUPLE** — Create a new tuple from specified elements.
- **CREATE_SET** — Create a new set (or relation, or map).

### Proof Execution

Your proof is a sequence of steps. Each step is one of:

- **Introduce** — Create a new object and declare its type.
- **Assert** — Record a membership fact.
- **Assume** — Record a fact as given.
- **Define obligation** — Add a rule to the world.
- **Activate obligation** — Invoke a rule — evaluate antecedent, materialize consequent.
- **Verify** — Check that a claim holds in the current world.

The proof is valid if every verification succeeds. If any check fails, the proof is invalid at that step.

---

## How Proofs Work — By Example

### Example 1: Simple Syllogism

> All dogs are mammals. Rex is a dog. Therefore Rex is a mammal.

Steps:
1. **Introduce** set Dogs, set Mammals
2. **Introduce** entity Rex
3. **Assert** Rex ∈ Dogs
4. **Define obligation** on Mammals: if self ∈ Dogs → self ∈ Mammals
5. **Activate** the obligation for Rex against Mammals
6. **Verify** Rex ∈ Mammals — **passes**

After step 5, the engine has materialized Rex ∈ Mammals in the matrix. Step 6 reads the matrix and confirms.

### Example 2: Chained Reasoning

> A ⊆ B ⊆ C. x ∈ A. Therefore x ∈ C.

Steps:
1. **Introduce** sets A, B, C and entity x
2. **Assert** x ∈ A
3. **Define obligation** on B: if self ∈ A → self ∈ B
4. **Define obligation** on C: if self ∈ B → self ∈ C
5. **Activate** obligation on B for x — materializes x ∈ B
6. **Activate** obligation on C for x — sees x ∈ B (from step 5), materializes x ∈ C
7. **Verify** x ∈ C — **passes**

Each activation builds on what the previous one materialized.

### Example 3: Creating a Successor

> n ∈ ℕ. By the successor axiom, there exists m ∈ ℕ such that (n, m) ∈ Succ.

Steps:
1. **Introduce** set ℕ, relation Succ, entity n
2. **Assert** n ∈ ℕ
3. **Define obligation** on Succ: if self ∈ ℕ → CREATE_ENTITY("m") AND m ∈ ℕ AND CREATE_TUPLE("t", self, m) AND t ∈ Succ
4. **Activate** the obligation for n against Succ

After activation, the world contains:
- The original entity n ∈ ℕ
- A new entity m ∈ ℕ
- A new tuple (n, m) ∈ Succ

The obligation created both m and the tuple, and established all their memberships, in a single activation.

---

## Key Principles

1. **The proof drives.** The engine executes what you tell it. It does not reason, search, or fill gaps.
2. **Everything is membership.** Types, properties, relations, equality — all x ∈ S.
3. **Obligations are lazy.** Rules exist but do nothing until activated.
4. **The world grows monotonically.** Facts and objects accumulate. Nothing is removed.
5. **Unknown ≠ false.** The matrix distinguishes "not in S" from "we don't know."
6. **Finite construction, general truth.** A proof about ℕ doesn't need infinitely many objects — just the ones the proof talks about.
