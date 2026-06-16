#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include "storage/membership_matrix.hpp"

namespace logic::engine {

class World;

// Result of evaluating an AST node.
// Logical expressions (NOT, AND, OR, IMPLIES, FORALL, EXISTS, MEMBER_OF,
// EQUALS, and "true"/"false" literals) evaluate to MembershipState — the
// ternary truth value (MEMBER / NON_MEMBER / UNKNOWN).
using EvalResult = std::variant<storage::MembershipState, std::size_t, std::string>;

// Variable bindings: variable name → object ID
using Bindings = std::unordered_map<std::string, std::size_t>;

class Evaluator {
public:
  explicit Evaluator(World& world);

  // Evaluate an AST node, returning the result
  EvalResult evaluate(std::size_t node_id, const Bindings& bindings) const;

  // Convenience: evaluate and collapse to a bool — true iff MEMBER.
  // NON_MEMBER and UNKNOWN both collapse to false. This is the binary
  // view used by obligation activation/materialization.
  bool evaluateBool(std::size_t node_id, const Bindings& bindings) const;

  // Evaluate and expect a ternary MembershipState (MEMBER / NON_MEMBER / UNKNOWN).
  // This is the full three-valued (Kleene) view of logical expressions.
  storage::MembershipState evaluateState(std::size_t node_id, const Bindings& bindings) const;

  // Activate obligations targeting a set for a given object.
  // If an obligation fires, the derived membership fact is materialized
  // into the world (written to the membership matrix and table).
  bool activateObligations(std::size_t object_id, std::size_t set_id);

  // Query-only: evaluate obligations without materializing.
  // Kept for cases where you want to check without mutating the world.
  bool evaluateObligations(std::size_t object_id, std::size_t set_id) const;

private:
  World& world_;

  EvalResult evalLiteral(std::size_t node_id) const;
  EvalResult evalReference(std::size_t node_id, const Bindings& bindings) const;
  EvalResult evalUnaryOp(std::size_t node_id, const Bindings& bindings) const;
  EvalResult evalBinaryOp(std::size_t node_id, const Bindings& bindings) const;
  EvalResult evalQuantifier(std::size_t node_id, const Bindings& bindings) const;
  EvalResult evalFunctionCall(std::size_t node_id, const Bindings& bindings) const;

  // Cycle detection for obligation evaluation
  bool evaluateObligationsImpl(std::size_t object_id, std::size_t set_id,
                               std::unordered_set<std::size_t>& visited) const;
  bool activateObligationsImpl(std::size_t object_id, std::size_t set_id,
                               std::unordered_set<std::size_t>& visited);

  // Walk a consequent AST and materialize each base-case fact into the world.
  // Bindings are mutable: creation nodes (CREATE_ENTITY, CREATE_TUPLE, CREATE_SET)
  // add new variable bindings that subsequent sibling nodes can reference.
  // Returns true if all facts were successfully materialized.
  bool materialize(std::size_t node_id, Bindings& bindings,
                   std::size_t default_object_id, std::size_t default_set_id);
};

}
