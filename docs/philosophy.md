# Logic Execution Engine — Philosophy

## A Universal Logic Engine

This is not a math engine. It is a logic engine. Mathematics is the first application — the cleanest domain, where axioms are most agreed upon and proofs most rigorous. But the engine itself does not know it is doing math. It knows four things: objects, membership, obligations, activation. Those four operations do not presuppose any domain.

Every domain that reasons from rules to conclusions is a domain this engine can serve:

- **Mathematics** — axioms as obligations, theorems as verified claims.
- **Business logic** — business rules as obligations, decisions as obligation activations.
- **Legal reasoning** — statutes and precedents as obligations, legal arguments as proof chains.
- **Scientific reasoning** — hypotheses as objects, experimental results as membership facts, theories as obligation sets.
- **Probabilistic and Bayesian reasoning** — truth values as objects, belief states as sets, update rules as obligations.
- **Philosophical logic** — any axiom system, any inference rule set, any number of truth values.

The matrix does not care what the objects represent. An entity can be a number, a person, a contract, a hypothesis. A set can be a type, a category, a belief state, a legal status. An obligation can be a mathematical axiom, a business rule, a social norm, a physical law.

The engine provides the substrate for reasoning. The domain provides the content.

## Boil Down to Definitions

The engine operates at the level of definitions — the atomic logical steps that cannot be decomposed further. There are no built-in theorems, no built-in inference shortcuts, no convenience operations that skip steps. Every logical move is explicit.

A ⊆ B is not a primitive. It is an obligation encoding the definition: "for any x, if x ∈ A then x ∈ B." Transitivity of subset is not a primitive. It is a chain of obligation activations on specific objects — modus ponens applied twice. The engine does not know what "subset" means. It knows membership, obligations, and activation. Subset is a pattern built from those.

This is deliberate. The purpose of the engine is to express the core structure of logic — the fundamental moves that all reasoning reduces to. When a proof says "A ⊆ B and B ⊆ C, therefore A ⊆ C," there is hidden structure: the proof must introduce an arbitrary x, derive x ∈ C from x ∈ A through two applications of the subset definition, and then generalize. The engine makes that hidden structure visible and explicit.

## No Shortcuts at the Engine Level

The engine provides exactly four operations:

1. **Create** — introduce an object into the world.
2. **Assert** — record a membership fact.
3. **Define** — add an obligation (a rule).
4. **Activate** — fire an obligation, materializing its consequent.

Everything else — subset, equality, function application, induction, contradiction, universal generalization — is built from these four. The engine does not provide a "prove subset transitivity" button. It provides the machinery to execute the proof step by step, at the level where each step is a single membership fact or a single obligation activation.

This means proofs in the engine are longer and more explicit than proofs written on paper. That is the point. The paper proof says "by transitivity of subset." The engine proof shows what transitivity of subset actually is: two applications of the definition, chained through a shared membership fact.

## Libraries as a Layer on Top

Once the engine can verify proofs at the definition level, a library system can be built on top. A library is a collection of pre-proven theorems — packaged as reusable proof steps. A library might provide:

- "Subset transitivity" — a proven shortcut that, given obligations encoding A ⊆ B and B ⊆ C, produces the obligation A ⊆ C.
- "Modus ponens" — a proven shortcut that, given P and P → Q, materializes Q.
- "Peano axioms" — a set of obligations encoding the axioms of natural number arithmetic.

Libraries do not change the engine. They are proofs that have already been verified. Using a library theorem is an appeal to a previously completed proof — the engine trusts it because it (or a prior run) already checked every step.

The engine itself remains minimal. It knows four operations. Libraries provide the vocabulary of mathematics. The separation is clean: the engine is the logic; libraries are the mathematics.

## Why This Matters

Most proof assistants embed mathematical knowledge into the system. They have built-in notions of equality, natural numbers, induction, type hierarchies. This makes them powerful but opaque — the user must trust that the system's built-in rules are correct.

This engine embeds nothing. Every rule is an obligation written by the proof or imported from a library. The engine's correctness reduces to: does it correctly evaluate AST conditions, and does it correctly materialize consequents? Those are simple, auditable operations. The mathematical content lives entirely in the obligations, which are data — inspectable, replaceable, and verifiable.

The cost is verbosity. The benefit is transparency. Every logical step is visible. Nothing is hidden behind a built-in rule. If the proof is valid, you can see exactly why — it's in the matrix.
