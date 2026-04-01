# Logic Execution Engine

A formal logic execution engine in C++17 that executes proofs as programs.

## What is this?

Most tools in formal reasoning either **search** for proofs (theorem provers) or **check** formulas against models (model checkers). This engine does neither. It **executes** proofs — step by step, as written — the same way a CPU executes instructions.

A proof written in the engine's formal language L is fed to the engine statement by statement. Each statement becomes an operation on a mathematical universe (the "world"): create an object, record a fact, fire a rule, verify a claim. The world grows monotonically as the proof progresses, accumulating the logical reality that the proof constructs.

## Core idea

A proof is a finite construction. When a mathematician proves something about all natural numbers, they don't check infinitely many cases — they introduce a specific (arbitrary) object, reason about it through a finite chain of steps, and conclude. This engine takes that observation literally.

The engine knows exactly four operations:

1. **Create** — introduce an object into the world
2. **Assert** — record a membership fact
3. **Define** — add an obligation (an inference rule)
4. **Activate** — fire an obligation, deriving new facts

Everything else — subset relations, equality, function application, induction, contradiction — is built from these four primitives. There are no built-in axioms, no built-in type theory, no hardcoded inference rules. All logic is user-defined.

## How it works

The world **W = (U, S, T, O)** holds:
- **T** — all objects (entities, sets, relations, maps, tuples)
- **U** — all entities (individuals), a subset of T
- **S** — all sets, a subset of T
- **O** — all obligations (inference rules)

The central data structure is a **sparse ternary membership matrix** M, where each cell M[x][S] encodes one of three states:
- **Member** — x is known to belong to S
- **Non-member** — x is known to *not* belong to S
- **Unknown** — no information

This three-valued logic is key: unknown is distinct from non-membership, enabling open-world reasoning where absence of proof is not proof of absence.

**Obligations** are rules of the form "if A then B," stored as AST condition trees. They are lazy — they sit in the world until explicitly activated. When activated, the engine evaluates the antecedent against the current matrix state and, if it holds, **materializes** the consequent: writing membership facts, creating new objects, or generating further obligations.

## A universal logic engine

This is not a math engine. Mathematics is the first test case — the cleanest domain where axioms are well-defined. But the engine itself is domain-agnostic. Any system that reasons from rules to conclusions over objects and membership can be expressed:

- **Mathematics** — axioms as obligations, theorems as verified claims
- **Business logic** — business rules as obligations, decisions as activations
- **Legal reasoning** — statutes as obligations, arguments as proof chains
- **Program verification** — program states as objects, correctness properties as sets, Hoare triples as obligations

## Built for machines

The engine operates at "ash level" — the atomic logical steps that cannot be decomposed further. A paper proof says "by transitivity of subset." The engine proof shows what that actually is: two applications of the definition, chained through a shared membership fact.

This verbosity makes the engine impractical for direct human use but ideal for machine reasoning. An AI doesn't care that encoding A ⊆ B requires building an AST. It needs simplicity (four operations), universality (one substrate for any domain), and transparency (every conclusion traceable to atomic steps).

## Build

```bash
cmake -B build -S .
cmake --build build
```

## Test

```bash
# Google Test suite
./build/run_tests

# Self-made test survey
./build/test_survey

# CTest
cd build && ctest --output-on-failure
```

## Project structure

```
logic_execution_engine/
├── include/
│   ├── core/        # Object, Entity, Tuple, types
│   ├── storage/     # Tables and membership matrix
│   └── engine/      # World model and evaluator
├── src/
│   ├── core/        # Core implementations
│   ├── storage/     # Storage implementations
│   └── engine/      # Engine implementations
├── test/            # Google Test suites
└── docs/            # Design documents and vision
```

## Requirements

| Tool | Minimum version | Notes |
|------|----------------|-------|
| C++ compiler | C++17 support | GCC 7+ or Clang 5+ |
| CMake | 3.10+ | Build system |
| Google Test | any | Required for test suite (`find_package(GTest)`) |

### Installing dependencies

**macOS (Homebrew):**
```bash
brew install cmake googletest
```

**Ubuntu/Debian:**
```bash
sudo apt install cmake g++ libgtest-dev
```

**Arch Linux:**
```bash
sudo pacman -S cmake gtest
```

No external runtime dependencies — only the C++ standard library.

## Status

The core engine is implemented: world model, ternary membership matrix, obligation system with materialization, AST evaluation, and cycle detection. The parser for language L and binary persistence are in progress.

## License

All rights reserved.
