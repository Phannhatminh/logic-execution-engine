# Logic Execution Engine — Comparisons

## Overview

This document compares the Logic Execution Engine to existing systems that share some of its goals or mechanisms. No single existing system occupies the same design point. The engine's combination of features — a ternary membership matrix as the entire mutable state, lazy proof-driven obligation activation, object creation from consequents, and domain-agnostic execution — appears to be unique.

---

## Proof Assistants

### Lean / Coq / Agda

Proof assistants built on the Calculus of Inductive Constructions (CIC), a dependent type theory.

**Shared:** Both verify formal proofs. Both can express mathematical reasoning.

**Different:**

- **Foundation** — Lean/Coq/Agda: CIC (dependent type theory). This engine: none — user-provided obligations.
- **Primitive** — Lean/Coq/Agda: type judgment (Gamma |- t : T). This engine: membership (x in S).
- **Proof representation** — Lean/Coq/Agda: a term that type-checks. This engine: a sequence of operations that mutate a world.
- **Equality** — Lean/Coq/Agda: built-in definitional + propositional equality. This engine: modeled as a relation (set of tuples).
- **Computation** — Lean/Coq/Agda: built-in beta/delta reduction. This engine: none — the world is inert data.
- **Induction** — Lean/Coq/Agda: built-in inductive types. This engine: encoded as obligations.
- **Automation** — Lean/Coq/Agda: tactic framework, proof search. This engine: none — the proof drives every step.
- **Logic** — Lean/Coq/Agda: fixed (CIC). This engine: any logic the user defines.

Lean embeds a specific, powerful logic. This engine embeds none. Lean is more practical for mathematics. This engine is more general.

### Mizar

A proof system based on Tarski-Grothendieck set theory.

**Shared:** Set-theoretic foundation. Membership as a core concept. Proofs verified step by step.

**Different:**

- **Foundation** — Mizar: Tarski-Grothendieck set theory. This engine: none.
- **Inference** — Mizar: built-in inference rules. This engine: all inference is user-defined obligations.
- **Logic** — Mizar: classical FOL, fixed. This engine: any logic.
- **State representation** — Mizar: proof environment. This engine: ternary membership matrix.

Mizar is the closest existing system in foundation (sets, membership) but it has built-in inference and a fixed logic. This engine has neither.

---

## Logical Frameworks

### Metamath

A minimal proof verifier with no built-in logic.

**Shared:** Logic-agnostic. Minimal kernel. Explicit proofs with no hidden steps. User provides all axioms and rules.

**Different:**

- **Primitive** — Metamath: string substitution. This engine: set membership.
- **State** — Metamath: stateless — list of proven statements. This engine: mutable world — ternary membership matrix.
- **Execution model** — Metamath: verify each step against axiom set. This engine: execute each step, mutating the world.
- **Object creation** — Metamath: not applicable. This engine: obligations can create new objects.
- **Ternary logic** — Metamath: no. This engine: yes (member / non-member / unknown).

Metamath is the closest system in philosophy — both commit to minimalism and logic-agnosticism. The key difference is statefulness: Metamath verifies, this engine constructs.

### LF / Twelf

The Edinburgh Logical Framework — a meta-language for specifying logics.

**Shared:** Logic-agnostic. Designed to encode different logics within a single framework.

**Different:**

- **Encoding mechanism** — LF/Twelf: dependent types. This engine: membership + obligations.
- **Primitive** — LF/Twelf: type judgment. This engine: membership.
- **Proof checking** — LF/Twelf: type checking. This engine: obligation activation + matrix query.
- **State** — LF/Twelf: static type context. This engine: mutable membership matrix.

LF uses dependent types as its universal encoding mechanism. This engine uses membership and obligations.

### MMT / OmDoc

Meta-Meta-Theory — a framework for representing and managing mathematical knowledge across foundations.

**Shared:** Foundation-independent. Logic-agnostic. Can encode arbitrary logical systems.

**Different:**

- **Purpose** — MMT: represent and organize knowledge. This engine: execute proofs.
- **Core feature** — MMT: theory morphisms (translate between logics). This engine: obligation activation (derive new facts).
- **Execution** — MMT: does not execute proofs. This engine: executes proofs step by step.
- **State** — MMT: static theory graph. This engine: mutable membership matrix.
- **Interoperability** — MMT: core goal. This engine: not a goal.

MMT organizes mathematical knowledge. This engine constructs mathematical (and logical) reality. MMT says "here is how these logics relate." This engine says "here is what this proof builds, fact by fact."

### Isabelle / Pure

A generic proof assistant with a minimal meta-logic on which object logics are built.

**Shared:** Logic-agnostic substrate. Object logics defined on top.

**Different:**

- **Meta-logic** — Isabelle/Pure: higher-order natural deduction. This engine: four operations (create, assert, define, activate).
- **Mechanism** — Isabelle/Pure: higher-order unification. This engine: membership matrix + obligation activation.
- **Automation** — Isabelle/Pure: built-in simplifier, tactics. This engine: none.

Isabelle's meta-logic is richer — higher-order unification is powerful. This engine's meta-logic is simpler — membership and obligations only.

---

## Automated Reasoning

### Z3 / SMT Solvers

Satisfiability Modulo Theories — automated solvers that determine whether logical formulas are satisfiable.

**Shared:** Can encode logical constraints and check whether they hold.

**Different:**

- **Mode** — Z3/SMT: automatic search for satisfying assignments. This engine: proof-driven execution.
- **User role** — Z3/SMT: provide constraints, solver finds solution. This engine: provide the proof, engine verifies it.
- **State** — Z3/SMT: internal solver state (opaque). This engine: membership matrix (transparent).
- **Completeness** — Z3/SMT: aims to be complete for supported theories. This engine: no search — rejects if a step fails.

Z3 searches. This engine executes. Z3 answers "is this satisfiable?" This engine answers "is this proof valid?"

### Prolog

Logic programming language with automatic backtracking search.

**Shared:** Facts and rules as the basis for reasoning.

**Different:**

- **Execution** — Prolog: automatic goal-directed search with backtracking. This engine: proof-driven, no search.
- **Rules** — Prolog: Horn clauses, automatic unification. This engine: obligations, manual activation.
- **State** — Prolog: implicit in the search tree. This engine: explicit membership matrix.
- **Negation** — Prolog: negation as failure. This engine: explicit non-membership (ternary matrix).

Prolog's engine reasons automatically. This engine only does what the proof tells it to.

### Datalog

Declarative logic query language over relations.

**Shared:** Facts + rules -> derived facts. Relational data model.

**Different:**

- **Evaluation** — Datalog: automatic fixpoint — all rules fire exhaustively. This engine: lazy — obligations fire only when activated.
- **Derivation** — Datalog: bottom-up, compute all consequences. This engine: proof-driven, compute only what the proof requests.
- **State** — Datalog: relational tables. This engine: ternary membership matrix.
- **Object creation** — Datalog: typically not supported. This engine: supported (CREATE_ENTITY, CREATE_TUPLE, CREATE_SET).

Datalog computes everything that follows from the rules. This engine computes only what the proof asks for. Datalog is eager; this engine is lazy.

---

## Knowledge Systems

### Knowledge Graphs (Neo4j, RDF, OWL)

Graph databases storing entities, relations, and properties.

**Shared:** Objects and relations as the core data model. Membership/classification of entities.

**Different:**

- **Purpose** — Knowledge Graphs: store and query knowledge. This engine: execute logical reasoning.
- **Rules** — Knowledge Graphs: optional (RDFS/OWL inference, usually limited). This engine: core mechanism (obligations).
- **Execution** — Knowledge Graphs: query/traversal. This engine: proof-driven state mutation.
- **Proof** — Knowledge Graphs: not a concept. This engine: the central concept.
- **Ternary state** — Knowledge Graphs: no (triples are present or absent). This engine: yes (member / non-member / unknown).

A knowledge graph is a snapshot of the membership matrix at one point in time, with the obligations and execution history thrown away. The data without the reasoning.

### Knowledge Graph Embeddings (TransE, RotatE, ComplEx)

Neural encodings of relational data — entities as vectors, relations as transformations.

**Shared:** Tensor representation of entity-relation facts. The long-term neural encoding vision connects here.

**Different:**

- **Foundation** — KG Embeddings: statistical patterns over static graphs. This engine: symbolic execution over a mutable world.
- **Reasoning** — KG Embeddings: learned (approximate, probabilistic). This engine: explicit (exact, auditable).
- **Rules** — KG Embeddings: implicit in learned parameters. This engine: explicit obligations.
- **Proof** — KG Embeddings: not a concept. This engine: the central concept.

KG embeddings learn patterns. This engine executes proofs. The neural encoding vision would bridge the two — the symbolic engine as ground truth, the neural encoding as learned approximation.

---

## Rule Engines

### Business Rule Engines (Drools, OPA, Clara)

Domain-specific systems for evaluating business rules.

**Shared:** Rules that derive conclusions from facts. Forward-chaining inference.

**Different:**

- **Domain** — Business Rule Engines: business logic. This engine: any domain.
- **Evaluation** — Business Rule Engines: automatic — all applicable rules fire. This engine: lazy — proof activates specific obligations.
- **Logic** — Business Rule Engines: domain-specific, typically classical. This engine: any logic the user defines.
- **Proof structure** — Business Rule Engines: not a concept. This engine: the central concept.
- **Transparency** — Business Rule Engines: varies. This engine: full — every step in the matrix.

Business rule engines are specialized for their domain and evaluate eagerly. This engine is universal and evaluates lazily.

---

## Program Verification

### Hoare Logic / Separation Logic

Formal frameworks for reasoning about program correctness.

**Shared:** Preconditions -> postconditions via rules. Proof-based verification.

**Different:**

- **Domain** — Hoare/Separation Logic: program correctness. This engine: any domain.
- **Encoding** — Hoare/Separation Logic: specialized (triples, frames, footprints). This engine: universal (membership + obligations).
- **Implementation** — Hoare/Separation Logic: usually embedded in a proof assistant. This engine: standalone engine.

Hoare logic could be encoded in this engine as a set of obligations over program-state objects.

---

## What This Engine Uniquely Combines

Each of the following exists in some system. No existing system combines all of them:

1. **Membership matrix as the entire mutable state.** Not terms (Lean), not strings (Metamath), not theory graphs (MMT), not relational tables (Datalog). A single ternary incidence structure that *is* the proof's reality.

2. **Ternary state (member / non-member / unknown).** Not binary. Known non-membership is a positive fact. Unknown is distinct from false.

3. **Lazy obligations activated by the proof.** Not automatic fixpoint (Datalog), not automatic search (Prolog, Z3), not automatic rule firing (Drools). The proof drives every step.

4. **Object creation from obligation consequents.** Rules can bring new entities, tuples, sets, relations, and maps into existence — not just derive new facts about existing objects.

5. **Domain-agnostic.** No built-in logic (unlike Lean, Coq, Mizar). No built-in domain (unlike Drools, Hoare logic). No built-in computation (unlike Lean's reduction). The engine provides the substrate; the user provides everything else.

6. **Execution, not representation.** The proof is a program. The engine runs it. The world grows. Facts materialize. This is not a static proof object that gets checked — it is a dynamic construction that gets built.
