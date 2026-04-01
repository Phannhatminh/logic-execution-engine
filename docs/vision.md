# Logic Execution Engine — Vision

## The Problem

Mathematical proofs are written in natural language mixed with formal notation. They are verified by humans — readers who follow the argument step by step and convince themselves that each step is valid. This process is slow, error-prone, and does not scale.

Existing proof assistants (Coq, Lean, Isabelle) address this, but they impose their own type theories and logical foundations. They require the mathematician to work within the assistant's framework rather than expressing proofs as they naturally think about them.

## The Idea

A proof is a construction.

When a mathematician writes a proof, they are building a small, specific universe: introducing objects, declaring their properties, establishing relationships, and deriving new facts from existing ones. The argument "let n be a natural number; assume P(n); then P(n+1)" does not quantify over all of ℕ at once — it creates a single object n, attaches properties to it, and reasons about that specific object.

The domain may be infinite. The space of possible proofs may be infinite. But the proof itself — the thing actually written down — constructs a finite number of objects and derives a finite number of facts. The universe of the proof is exactly the objects and facts the proof brings into existence.

This engine takes that observation literally. It provides a world — a mathematical universe — and builds it up object by object, fact by fact, as the proof dictates.

## How It Works

A proof written in language L is fed to the engine statement by statement. Each statement becomes one or more operations on the world:

- **"Let n be a natural number"** — create an entity n; record n ∈ ℕ in the membership matrix.
- **"Assume P(n)"** — record the fact that P holds for n (as membership in the appropriate set or relation).
- **"By axiom A, Q(n) follows"** — activate the obligation encoding axiom A; the obligation's antecedent is checked against the current world state; if it holds, the consequent is materialized into the world.
- **"Therefore R"** — query the world to verify that R holds given everything constructed so far. If it does, the proof step is valid. If not, the proof is invalid at this step.

The world grows monotonically as the proof progresses. Each step either introduces new objects, materializes new facts, or verifies claims against the accumulated state.

## Obligations as Inference

The central mechanism is the **obligation**: a rule of the form "if A then B," stored as a pair of AST condition trees.

In traditional logic, inference rules are meta-level operations: modus ponens, universal instantiation, etc. In this engine, inference rules are obligations. An axiom like "for all x, if x ∈ A then x ∈ B" is stored as an obligation targeting set B with antecedent "self ∈ A."

Obligations are **lazy** — they exist in the world but do not fire until activated. When the proof invokes a rule, the engine activates the corresponding obligation: it evaluates the antecedent against the current state, and if it holds, **materializes the consequent** into the world. Materialization can write membership facts into the matrix, but it can also create new objects — entities, sets, relations, maps, tuples — bringing new things into existence, not only recording new truths about existing things. Everything materialized becomes part of the world and is visible to all subsequent reasoning.

This is how multi-step proofs work:

1. The proof states a premise. The engine records it.
2. The proof invokes a rule. The engine activates the obligation, checks its antecedent, and writes the derived fact.
3. The proof invokes another rule. The engine activates another obligation. Its antecedent may depend on the fact derived in step 2 — which is now in the matrix and visible.
4. Each step builds on the last. The world accumulates the logical consequences of the proof.

The proof is valid if every claim made along the way holds in the world at the point where it is made.

### Materialization of Compound Consequents

An obligation's consequent can be a compound expression. Materialization decomposes it into base-case operations:

- **MEMBER_OF(x, S)** — write M[x][S] = 1. The base case: x is a member of S.
- **NOT(MEMBER_OF(x, S))** — write M[x][S] = 0. The base case: x is known to not be a member of S. This is a positive fact, distinct from "unknown."
- **AND(A, B)** — materialize A, then materialize B. Both facts are written.
- **OR(A, B)** — create a new obligation encoding the disjunctive constraint. OR does not write a fact directly — instead it records the rule that if one side is negated, the other must hold. Concretely, A OR B materializes as the obligation (NOT A) → B. The obligation sits in O until the proof establishes one side, at which point activation derives the other.
- **LITERAL "true"** — write M[object][target_set] = 1, using the obligation's implicit target.

The engine does not decide which branch of an OR to take. The proof drives that choice. The engine only records the constraint and enforces it when activated.

## Membership as the Universal Primitive

Everything in the world reduces to membership.

- **Typing:** an object's type is the set it belongs to. n ∈ ℕ means n is a natural number.
- **Properties:** P(n) means n ∈ P, where P is the set of objects satisfying property P.
- **Relations:** R(a, b) means the tuple (a, b) ∈ R, where R is a set of 2-tuples.
- **Equality:** a = b under equivalence E means (a, b) ∈ E.
- **Subtyping:** A ⊆ B is an obligation: for all x, if x ∈ A then x ∈ B.

The membership matrix is the single source of truth for what holds in the world. Each cell M[x][S] has three possible states:

- **1** — x is known to be a member of S.
- **0** — x is known to not be a member of S.
- **unset** — the membership of x in S is unknown.

This is not a binary matrix. Unknown is distinct from non-membership. A proof may establish that x ∉ S (setting the cell to 0), which is a positive fact — different from simply not having mentioned x and S at all.

Obligations derive new facts from existing ones. The entire logical state of the proof is captured in this matrix.

## Finite Construction, General Truth

A proof about all natural numbers does not require infinitely many objects. It requires:

- An entity n, declared to be in ℕ.
- An entity representing n + 1 (or a successor relation).
- Obligations encoding the axioms of arithmetic.
- A finite chain of obligation activations deriving the conclusion.

The engine does not model ℕ. It models the specific objects the proof talks about and checks that the logical steps between them are valid. The generality of the theorem comes from the proof's structure — the fact that n was arbitrary and no special properties of n beyond "n ∈ ℕ" were used. The engine verifies the structure; the generality is the mathematician's claim, supported by the verified structure.

This is exactly how humans verify proofs: not by checking infinitely many cases, but by following a finite argument about specific (possibly arbitrary) objects and confirming that each step follows from the last.

## Why "Execution"

The engine **executes** logic.

A proof is a sequence of logical steps. The engine doesn't search for those steps (that's a prover) and it doesn't just check a formula against a model (that's a checker). It executes them — one by one, in order, as written by the human. Each step is an operation that mutates the world: create an object, record a fact, activate an obligation, verify a claim.

It is the same sense as a CPU executing instructions. The proof is the program. The engine runs it. And what it produces is not just a yes/no verdict — it constructs the mathematical truth that underlies the proof.

## What This Is Not

- **Not a model checker.** It does not enumerate elements of a domain and check a formula against each one. It constructs only what the proof constructs.
- **Not a theorem prover.** It does not search for proofs. The human writes the proof; the engine verifies it.
- **Not a type theory.** There is no built-in type system beyond "types are sets, subtyping is subset." The logical rules are obligations, defined by the proof or its axiom system, not hardcoded into the engine.

## A Universal Logic Engine

This is not a math engine. It is a logic engine. Mathematics is the first test case — the cleanest domain, where axioms are most agreed upon and proofs most rigorous. But the engine itself does not know it is doing math. It knows four things: objects, membership, obligations, activation. Those four operations presuppose no domain.

Every domain that reasons from rules to conclusions is a domain this engine can serve:

- **Mathematics** — axioms as obligations, theorems as verified claims.
- **Business logic** — business rules as obligations, decisions as obligation activations.
- **Legal reasoning** — statutes and precedents as obligations, legal arguments as proof chains.
- **Scientific reasoning** — hypotheses as objects, experimental results as membership facts, theories as obligation sets.
- **Probabilistic and Bayesian reasoning** — truth values as objects, belief states as sets, update rules as obligations.
- **Philosophical logic** — any axiom system, any inference rule set, any number of truth values.
- **Program verification** — program states as objects, correctness properties as sets, Hoare triples as obligations, type safety proofs as obligation chains.
- **Algorithmic reasoning** — preconditions and postconditions as obligations, algorithm execution as proof chains over program state.

The matrix does not care what the objects represent. An entity can be a number, a person, a contract, a hypothesis, a program state. A set can be a type, a category, a belief state, a legal status. An obligation can be a mathematical axiom, a business rule, a social norm, a physical law.

The engine provides the substrate for reasoning. The domain provides the content.

## Cross-Domain Reasoning at Maximum Resolution

Specialized tools — Lean for math, Z3 for verification, Drools for business rules — each trade universality for usability in their domain. For any single domain, a purpose-built tool will be more practical for human users.

But when problems span multiple domains — a legal argument that depends on mathematical reasoning that depends on empirical facts that depends on probabilistic inference — no single specialized tool covers the full chain. Each tool has its own representation, its own semantics, its own limitations.

This engine operates at the atomic level — what we call "boiling things down to ashes." Every logical move is explicit, every fact is a membership cell, every rule is an obligation. This resolution is too fine for human comfort. But it is exactly what is needed when different logics must work together across domains and you need the utmost fidelity in the reasoning chain.

## AI as the Primary User

The verbosity of the engine — which makes it impractical for human users working in a single domain — is irrelevant if the user is an AI.

An AI does not care that encoding A ⊆ B requires building an AST. It does not care that a proof takes fifty obligation activations instead of one tactic call. It can generate thousands of steps per second. What it needs is:

- **Simplicity** — four operations, no ambiguity, no hidden rules.
- **Universality** — one substrate for any domain, not five specialized tools with five different representations.
- **Transparency** — every step auditable, every conclusion grounded in atomic steps that can be verified.

The engine is built for machine reasoning. An AI writes proofs in language L. The engine executes at the membership level. Every conclusion is traceable to a chain of obligation activations over specific membership facts. Nothing is hidden.

Specialized proof assistants were built for human ergonomics. This engine is built for machine precision.

## Libraries for Human Usability

The engine operates at the ash level — the atomic logical steps that cannot be decomposed further. For human users, this is too verbose. The bridge is libraries.

A library is a collection of pre-proven theorems — packaged as reusable proof steps. A library might provide:

- "Subset transitivity" — a shortcut that, given A ⊆ B and B ⊆ C, produces A ⊆ C.
- "Modus ponens" — a shortcut that, given P and P → Q, materializes Q.
- "Peano axioms" — a set of obligations encoding natural number arithmetic.
- "Propositional logic" — the standard classical logic toolkit.

Libraries do not change the engine. They are proofs that have already been verified. Using a library theorem is an appeal to a previously completed proof — the engine trusts it because it (or a prior run) already checked every step.

The engine remains minimal — four operations. Libraries provide the vocabulary of mathematics, law, business, science. The separation is clean: the engine is the logic; libraries are the content.

## Neural Encoding

The long-term vision: encode the engine's symbolic execution into neural network computation.

The membership matrix is already a tensor — a sparse ternary structure indexed by objects and sets. Obligations are operations on that tensor. A proof is a sequence of transformations on the matrix state:

```
M₀ →(activate o₁)→ M₁ →(activate o₂)→ M₂ → ... → Mₙ
```

This is the structure of a deep network — a sequence of transformations on a state tensor. Each layer is an obligation activation. The input is the initial matrix (premises). The output is the final matrix (conclusions).

The transfer from symbolic execution to neural computation:

- **Membership matrix M** corresponds to an embedding tensor.
- **Object IDs** correspond to vector indices / embedding dimensions.
- **M[x][S] in {1, 0, unknown}** corresponds to continuous activation values.
- **Obligation (if A then B)** corresponds to a learned transformation layer.
- **Activation (check + write)** corresponds to a forward pass through a layer.
- **Proof chain** corresponds to a sequence of layers / full forward pass.
- **Verification** corresponds to output layer agreement with target.

Mathematically, this is a transfer from relational logic to tensor algebra and functional analysis. Technologically, it is finding ways to encode the engine's program execution into a neural network's latent space.

The symbolic engine and the neural encoding serve complementary roles:

- **Symbolic engine** — exact, auditable, verifiable. The ground truth. Every step is explicit and checkable.
- **Neural encoding** — fast, generalizable, uncertainty-aware. The approximation. Handles continuous values, learns patterns, scales to large problems.

The engine becomes both: the verifier that checks proofs at full resolution, and the specification for what a neural network should learn to do.

## Summary

The engine constructs the logical reality of a proof — the specific, finite universe that the proof brings into existence — and verifies that every step holds within that reality.

Four primitives: objects, membership, obligations, activation. No built-in logic. No built-in domain. Any reasoning system that can be expressed as rules over objects and membership can be executed.

Mathematics is the first test case. The real target is universal logic execution — across all domains, at atomic resolution, for machine reasoning, with neural encoding as the path to scale.
