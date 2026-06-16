# Logic Execution Engine — Data Structures

## Core Layer

### Object

Base type for all things in the system. Every item that exists in the world is an Object.

- Provides identity-based equality and hashing by default.
- Each object has a unique integer ID assigned at creation time.
- Subclassed by Entity and Tuple.

### Entity

Subclass of Object. Represents a distinct individual. Identity is the only equality — two Entity pointers are equal if and only if they point to the same address.

### Tuple

Subclass of Object. An ordered grouping of Objects.

- An n-tuple groups n objects by position (0-indexed).
- A 2-tuple (a, b) is the fundamental unit for binary relations.
- N-tuples of arbitrary arity are supported.

**Note:** `Tuple` (C++ class) represents only *concrete* tuples — those whose elements are fully known at creation time. Generic or schematic tuples (e.g. "an n-ary tuple over S") are not backed by this class. See the logic engine layer section on TUPLE for the full picture.

---

## Logic Engine Layer

Everything at this layer is an object. Objects are classified by **system type** (sys_type), a built-in property on each object (via `getType()`). sys_type exists because of a bootstrapping problem: if types were only represented as sets in the membership matrix, the type-sets would need to be typed via the matrix before the matrix is populated — a circular dependency. sys_type breaks that circularity.

### Types as Sets

In this system, **types are sets**. An object's type is the set it belongs to. Subtyping is the subset relation:

> A ⊆ B iff ∀x ∈ A, x ∈ B

This constraint is itself an obligation in O.

### Type Hierarchy: SET ⊇ REL ⊇ MAP

The system defines three fundamental logical types, forming a subtype chain:

```
SET ⊇ REL ⊇ MAP
```

Each level adds membership constraints, enforced as obligations:

- **SET** — No restriction. Any object can be a member.
- **REL** — Members must be n-tuples with elements drawn from specified domain sets. An n-ary relation on S1 x S2 x ... x Sn only accepts n-tuples where position i draws from Si.
- **MAP** — A REL with the additional constraint that each input (all positions except the last) maps to exactly one output.

Since REL ⊆ SET and MAP ⊆ REL, every MAP is a REL and every REL is a SET. Storage is uniform — all are sets in the membership matrix. The type determines which obligations guard membership.

### TUPLE: Concrete vs Generic

`SysType::TUPLE` marks an object as a tuple *in the mathematical world*. But the backing C++ representation depends on whether the tuple is concrete or generic:

**Concrete tuple** — elements are fully known at creation time. The world registers a `SysType::TUPLE` object backed by a C++ `Tuple` instance. Elements are stored positionally in the `TupleElementTable`.

**Generic (schematic) tuple** — the user introduces a tuple of some arity n without specifying concrete elements (e.g. "let t be an n-ary tuple over S"). The world registers a `SysType::TUPLE` object backed by a base `Object` instance — no element structure exists at the C++ level. The tuple's properties (arity, domain constraints) are encoded entirely as obligations in O.

The two can be declared **equal** via an equality relation — a set of 2-tuples `(generic, concrete)` in the membership matrix. This is the standard mechanism for instantiating a generic tuple: the proof establishes equality between the generic object and a concrete tuple without the engine ever needing to "upgrade" the backing C++ object.

> **Status: planned (Phase 9).** Currently `World::createTuple` always produces a concrete tuple backed by `ObjectType::Tuple`. Support for generic tuples (backed by `ObjectType::Object`) and the associated equality mechanism depends on the standard library (Phase 8) providing `ℕ`, `ARITY_OF`, and `ELEMENT_AT` first.

---

### Set

A set is an object in which other objects can be members, or not.

- Sets are objects and can themselves be members of other sets.
- A **relation** is a typed set (REL) whose members are n-tuples constrained by domain sets.
- A **map/function** is a typed set (MAP) that is a relation with a uniqueness constraint on inputs.

### AST (Abstract Syntax Tree) — Condition Trees

A condition is a dynamically constructed tree of evaluatable nodes. Each node is an object (outside the system — not in T) with a strategy: evaluate its children, combine the results. The tree structure itself *is* the strategy — no separate strategy pattern is needed on top of it.

The AST is dynamic: the parser reads user input and constructs the tree at runtime. Different input produces different trees.

**How evaluation works:**

A condition A is true if B AND C are true. B is true if x ∈ X. C is true if y ∈ Y. This forms:

```
       A (AND)
      /       \
   B (x ∈ X)   C (y ∈ Y)
```

The AND node evaluates B and C, returns true if both are true. The leaf nodes check membership in the matrix. Each node type (AND, OR, NOT, MEMBER_OF, etc.) determines how that node evaluates, and the tree structure determines the composition of conditions.

**Node types:**

- **Literal** — A constant value (number, string, boolean). Example: `42`, `"Alice"`, `true`.
- **Reference** — A reference to an object, variable, or field. Example: `x`, `x.name`.
- **Unary operator** — Single-operand operation. Example: `NOT p`.
- **Binary operator** — Two-operand operation. Example: `a AND b`, `x = y`, `x > 5`.
- **Quantifier** — Universal or existential quantification over a set. Example: `FOR ALL x IN S: P(x)`.
- **Function call** — Application of a named function or predicate. Example: `member(x, S)`.
- **Create entity** — Materialization only. Creates a new entity and binds it. Example: `CREATE_ENTITY("m")`.
- **Create tuple** — Materialization only. Creates a tuple from children and binds it. Example: `CREATE_TUPLE("t", a, b)`.
- **Create set** — Materialization only. Creates a new set/rel/map and binds it. Example: `CREATE_SET("REL", "r")`.

Each node holds:
- A node type tag — determines the evaluation strategy.
- Zero or more child nodes — the sub-conditions it delegates to.
- An optional value (for literals, operator types, function names).

### Obligation

A rule of the form **if A then B** (A → B). Stored in O. Obligations are **not** part of the world being reasoned about — O is meta. Objects in T do not have membership relations with obligations. Obligations observe and constrain, they don't participate.

Obligations are **lazy** — they only activate when queried. The engine does not exhaustively evaluate all obligations. When a query comes in (e.g., "is x a member of S?"), the engine looks up obligations targeting S and evaluates them on demand.

Structure:
- **Target** — the set this obligation governs.
- **Antecedent (A)** — an AST encoding the "if" condition.
- **Consequent (B)** — an AST encoding the "then" condition.

A and B are both arbitrary compositions of conditions (AND, OR, NOT of other conditions). Evaluating A or B may trigger evaluation of other obligations lazily, forming chains.

When an obligation is activated and its antecedent holds, the consequent is materialized into the world by decomposing it into base-case operations:

- **MEMBER_OF(x, S)** — set M[x][S] = 1. Positive membership.
- **NOT(MEMBER_OF(x, S))** — set M[x][S] = 0. Known non-membership.
- **AND(A, B)** — materialize A, then materialize B. Both facts are written.
- **OR(A, B)** — create a new obligation: (NOT A) → B. The disjunction is not resolved immediately — the engine records the constraint, and the proof later activates it once one side is established.
- **LITERAL "true"** — set M[object][target_set] = 1, using the obligation's implicit target.

Materialization can also create new objects — entities, sets, relations, maps, tuples — in T. The world grows in both facts and objects as obligations fire.

Example: `(x ∈ S1) AND (x ∈ S2)` is stored as:

```
        AND
       /   \
   x ∈ S1   x ∈ S2
```

Each node in the tree is a row in the AST Node Table. Each parent-child edge is a row in the AST Child Table. The obligation points to the root nodes of A and B.

### World

The top-level container representing the state of the system. Holds W = (U, S, T, O).

---

## Storage

### Tables (Persistent)

Three tables store the durable state of the system.

**Table 1: Object Table**

One row per object in T. Stores metadata and identity.

- **id** (integer, PK) — Unique integer ID, assigned monotonically at creation.
- **core_type** (enum: OBJECT, ENTITY, TUPLE) — C++ class of the object.
- **sys_type** (enum: SET, REL, MAP, ENTITY, TUPLE) — Logical role in the engine.
- **hash** (integer) — Hash code of the object.
- **pointer** (Object*, runtime only) — Not persisted — reconstructed at load time.

**Table 2: Membership Table**

Persistent record of all membership facts. Each row represents one membership: object is a member of set.

- **object_id** (integer, FK to Object Table) — ID of the member object.
- **set_id** (integer, FK to Object Table) — ID of the set it belongs to.

Composite PK: (object_id, set_id). This is the durable source of truth for membership. The in-memory membership matrix is rebuilt from this table at startup.

**Table 3: Tuple Element Table**

Persists the structure of tuples — which objects a tuple contains and in what order. Nested tuples are handled naturally: if an element is itself a tuple, its element_id points to another tuple in the Object Table.

- **tuple_id** (integer, FK to Object Table) — ID of the tuple object.
- **position** (integer) — 0-indexed position within the tuple.
- **element_id** (integer, FK to Object Table) — ID of the element at this position.

Composite PK: (tuple_id, position).

Example: tuple #5 = (obj#1, obj#2, obj#3), tuple #7 = (obj#1, tuple#5):

- Tuple #5, position 0: element #1
- Tuple #5, position 1: element #2
- Tuple #5, position 2: element #3
- Tuple #7, position 0: element #1
- Tuple #7, position 1: element #5 (which is tuple #5)

**Table 4: AST Node Table**

One row per node in any AST. Stores the node's type and optional value.

- **node_id** (integer, PK) — Unique node ID.
- **node_type** (enum: LITERAL, REFERENCE, UNARY_OP, BINARY_OP, QUANTIFIER, FUNCTION_CALL, CREATE_ENTITY, CREATE_TUPLE, CREATE_SET) — What kind of node.
- **value** (optional) — Literal value, operator type, function name, etc.

**Table 5: AST Child Table**

One row per parent-child edge in an AST. Stores the tree structure.

- **parent_node_id** (integer, FK to AST Node Table) — Parent node.
- **position** (integer) — 0-indexed position among siblings.
- **child_node_id** (integer, FK to AST Node Table) — Child node.

Composite PK: (parent_node_id, position).

Example: the expression `(x ∈ S1) AND (x ∈ S2)`:

AST Node Table entries:

- Node #10: node_type = BINARY_OP, value = AND
- Node #11: node_type = FUNCTION_CALL, value = MEMBER_OF
- Node #12: node_type = FUNCTION_CALL, value = MEMBER_OF

AST Child Table entries:

- Parent #10, position 0: child #11
- Parent #10, position 1: child #12

**Table 6: Obligation Table**

One row per obligation in O. Separate from the object table because O is meta — outside the world being reasoned about. Each obligation is A → B, pointing to the root nodes of its antecedent and consequent ASTs.

- **id** (integer, PK) — Unique obligation ID.
- **target_id** (integer, FK to Object Table) — The set this obligation governs.
- **antecedent_root_id** (integer, FK to AST Node Table) — Root node of the "if" condition.
- **consequent_root_id** (integer, FK to AST Node Table) — Root node of the "then" condition.

### Membership Matrix (Runtime)

In-memory sparse ternary incidence matrix, optimized for fast logical proposition evaluation. Indexed by object ID.

```
             Set_1  Set_2  Set_3  ...
  Obj_0      [1     0      _         ]
  Obj_1      [_     1      0         ]
  Obj_2      [1     1      _         ]
  ...
```

Each cell M[i][j] has three possible states:

- **1** — object i is known to be a member of set j.
- **0** — object i is known to not be a member of set j.
- **_ (unset)** — the membership of object i in set j is unknown.

Unknown is the default state. It is distinct from known non-membership (0). A cell set to 0 is a positive fact — "x is not in S" — established by the proof or by materializing a NOT consequent.

- Supports bitwise operations (AND, OR, NOT) over rows and columns for bulk evaluation of logical propositions.
- Loaded from the Membership Table at startup.
- When membership changes, both the matrix and the Membership Table are updated.

All relations (equality, ordering, user-defined) except membership itself are expressed as sets of tuples. Membership of a tuple in a relation set is recorded in this matrix. For example, if E is the equality relation and (a, b) is a 2-tuple, then M[(a,b)][E] = 1 means a and b are equal under E.

---

## Summary

```
Core:           Object (with unique ID), Entity, Tuple

Logic engine:   Set         — an object that other objects can be members of
                AST Node    — expression tree node for conditions
                Obligation  — rule = target + condition (AST) + kind
                World       — container for W = (U, S, T, O)

Persistent:     Object Table        — id, core_type, sys_type, hash
                Membership Table    — object_id, set_id
                Tuple Element Table — tuple_id, position, element_id
                AST Node Table      — node_id, node_type, value
                AST Child Table     — parent_node_id, position, child_node_id
                Obligation Table    — id, target_id, antecedent_root_id, consequent_root_id

Runtime:        Membership Matrix — binary incidence matrix for fast
                                    bitwise evaluation of logical propositions
                                    (loaded from Membership Table at startup)
```
