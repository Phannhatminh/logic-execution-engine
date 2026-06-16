#pragma once
#include <cstddef>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include "core/ids.hpp"
#include "storage/membership_matrix.hpp"

namespace logic::engine {

class World;

// Result of evaluating an AST node.
// Logical expressions (NOT, AND, OR, IMPLIES, FORALL, EXISTS, MEMBER_OF,
// EQUALS, and "true"/"false" literals) evaluate to MembershipState — the
// ternary truth value (MEMBER / NON_MEMBER / UNKNOWN).
using EvalResult = std::variant<storage::MembershipState, core::ObjectId, std::string>;

// Variable bindings: variable name → object ID
using Bindings = std::unordered_map<std::string, core::ObjectId>;

class Evaluator {
public:
  explicit Evaluator(World& world);

  // Evaluate an AST node, returning the result
  EvalResult evaluate(core::AstNodeId node_id, const Bindings& bindings) const;

  // Convenience: evaluate and collapse to a bool — true iff MEMBER.
  // NON_MEMBER and UNKNOWN both collapse to false. This is the binary
  // view used by obligation activation/materialization.
  bool evaluateBool(core::AstNodeId node_id, const Bindings& bindings) const;

  // Evaluate and expect a ternary MembershipState (MEMBER / NON_MEMBER / UNKNOWN).
  // This is the full three-valued (Kleene) view of logical expressions.
  storage::MembershipState evaluateState(core::AstNodeId node_id, const Bindings& bindings) const;

  // Activate obligations targeting a set for a given object.
  // If an obligation fires, the derived membership fact is materialized
  // into the world (written to the membership matrix and table).
  bool activateObligations(core::ObjectId object_id, core::ObjectId set_id);

  // Query-only: evaluate obligations without materializing.
  // Kept for cases where you want to check without mutating the world.
  bool evaluateObligations(core::ObjectId object_id, core::ObjectId set_id) const;

private:
  World& world_;

  EvalResult evalLiteral(core::AstNodeId node_id) const;
  EvalResult evalReference(core::AstNodeId node_id, const Bindings& bindings) const;
  EvalResult evalUnaryOp(core::AstNodeId node_id, const Bindings& bindings) const;
  EvalResult evalBinaryOp(core::AstNodeId node_id, const Bindings& bindings) const;
  EvalResult evalQuantifier(core::AstNodeId node_id, const Bindings& bindings) const;
  EvalResult evalFunctionCall(core::AstNodeId node_id, const Bindings& bindings) const;

  // Cycle detection for obligation evaluation
  bool evaluateObligationsImpl(core::ObjectId object_id, core::ObjectId set_id,
                               std::unordered_set<core::ObligationId>& visited) const;
  bool activateObligationsImpl(core::ObjectId object_id, core::ObjectId set_id,
                               std::unordered_set<core::ObligationId>& visited);

  // Walk a consequent AST and materialize each base-case fact into the world.
  // Bindings are mutable: creation nodes (CREATE_ENTITY, CREATE_TUPLE, CREATE_SET)
  // add new variable bindings that subsequent sibling nodes can reference.
  // Returns true if all facts were successfully materialized.
  bool materialize(core::AstNodeId node_id, Bindings& bindings,
                   core::ObjectId default_object_id, core::ObjectId default_set_id);
};

}
