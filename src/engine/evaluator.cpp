#include "engine/evaluator.hpp"
#include "engine/logic_engine.hpp"
#include <stdexcept>

namespace logic::engine {

Evaluator::Evaluator(World& world) : world_(world) {}

// Main dispatch

EvalResult Evaluator::evaluate(std::size_t node_id, const Bindings& bindings) const {
  const auto* node = world_.astNodeTable().find(node_id);
  if (!node) {
    throw std::runtime_error("AST node not found: " + std::to_string(node_id));
  }

  switch (node->node_type) {
    case core::AstNodeType::LITERAL:       return evalLiteral(node_id);
    case core::AstNodeType::REFERENCE:     return evalReference(node_id, bindings);
    case core::AstNodeType::UNARY_OP:      return evalUnaryOp(node_id, bindings);
    case core::AstNodeType::BINARY_OP:     return evalBinaryOp(node_id, bindings);
    case core::AstNodeType::QUANTIFIER:    return evalQuantifier(node_id, bindings);
    case core::AstNodeType::FUNCTION_CALL: return evalFunctionCall(node_id, bindings);
    case core::AstNodeType::CREATE_ENTITY:
    case core::AstNodeType::CREATE_TUPLE:
    case core::AstNodeType::CREATE_SET:
      throw std::runtime_error("Creation nodes cannot be evaluated — they can only be materialized");
  }
  throw std::runtime_error("Unknown AST node type");
}

bool Evaluator::evaluateBool(std::size_t node_id, const Bindings& bindings) const {
  EvalResult result = evaluate(node_id, bindings);
  if (auto* b = std::get_if<bool>(&result)) {
    return *b;
  }
  throw std::runtime_error("Expected bool result from AST node " + std::to_string(node_id));
}

// LITERAL: value is stored as string in the node
// "true"/"false" → bool, numeric string → size_t, otherwise → string
EvalResult Evaluator::evalLiteral(std::size_t node_id) const {
  const auto* node = world_.astNodeTable().find(node_id);
  const std::string& val = node->value;

  if (val == "true") return true;
  if (val == "false") return false;

  // Try numeric
  try {
    std::size_t pos;
    std::size_t num = std::stoull(val, &pos);
    if (pos == val.size()) return num;
  } catch (...) {}

  return val;
}

// REFERENCE: resolve variable name from bindings → object ID
EvalResult Evaluator::evalReference(std::size_t node_id, const Bindings& bindings) const {
  const auto* node = world_.astNodeTable().find(node_id);
  const std::string& name = node->value;

  auto it = bindings.find(name);
  if (it == bindings.end()) {
    throw std::runtime_error("Unbound variable: " + name);
  }
  return it->second;
}

// UNARY_OP: value is the operator ("NOT")
// child at position 0 is the operand
EvalResult Evaluator::evalUnaryOp(std::size_t node_id, const Bindings& bindings) const {
  const auto* node = world_.astNodeTable().find(node_id);
  auto children = world_.astChildTable().childrenOf(node_id);

  if (children.empty()) {
    throw std::runtime_error("Unary operator has no operand");
  }

  if (node->value == "NOT") {
    return !evaluateBool(children[0].child_node_id, bindings);
  }

  throw std::runtime_error("Unknown unary operator: " + node->value);
}

// BINARY_OP: value is the operator ("AND", "OR", "EQUALS")
// children at position 0 and 1 are the operands
EvalResult Evaluator::evalBinaryOp(std::size_t node_id, const Bindings& bindings) const {
  const auto* node = world_.astNodeTable().find(node_id);
  auto children = world_.astChildTable().childrenOf(node_id);

  if (children.size() < 2) {
    throw std::runtime_error("Binary operator needs 2 operands");
  }

  std::size_t left_id = children[0].child_node_id;
  std::size_t right_id = children[1].child_node_id;

  if (node->value == "AND") {
    // Short-circuit: if left is false, don't evaluate right
    if (!evaluateBool(left_id, bindings)) return false;
    return evaluateBool(right_id, bindings);
  }

  if (node->value == "OR") {
    // Short-circuit: if left is true, don't evaluate right
    if (evaluateBool(left_id, bindings)) return true;
    return evaluateBool(right_id, bindings);
  }

  if (node->value == "EQUALS") {
    EvalResult left = evaluate(left_id, bindings);
    EvalResult right = evaluate(right_id, bindings);
    return left == right;
  }

  if (node->value == "IMPLIES") {
    // A → B is equivalent to ¬A ∨ B
    if (!evaluateBool(left_id, bindings)) return true;
    return evaluateBool(right_id, bindings);
  }

  throw std::runtime_error("Unknown binary operator: " + node->value);
}

// QUANTIFIER: value is "FORALL" or "EXISTS"
// child 0: REFERENCE node (the bound variable name)
// child 1: REFERENCE node (the set to iterate over, resolved to set ID)
// child 2: the body condition
EvalResult Evaluator::evalQuantifier(std::size_t node_id, const Bindings& bindings) const {
  const auto* node = world_.astNodeTable().find(node_id);
  auto children = world_.astChildTable().childrenOf(node_id);

  if (children.size() < 3) {
    throw std::runtime_error("Quantifier needs variable, set, and body");
  }

  // Get bound variable name
  const auto* varNode = world_.astNodeTable().find(children[0].child_node_id);
  std::string varName = varNode->value;

  // Get set ID to iterate over
  EvalResult setResult = evaluate(children[1].child_node_id, bindings);
  std::size_t set_id = std::get<std::size_t>(setResult);

  // Get members of the set
  std::vector<std::size_t> members = world_.membersOf(set_id);

  std::size_t body_id = children[2].child_node_id;

  if (node->value == "FORALL") {
    for (std::size_t member_id : members) {
      Bindings newBindings = bindings;
      newBindings[varName] = member_id;
      if (!evaluateBool(body_id, newBindings)) {
        return false;
      }
    }
    return true;
  }

  if (node->value == "EXISTS") {
    for (std::size_t member_id : members) {
      Bindings newBindings = bindings;
      newBindings[varName] = member_id;
      if (evaluateBool(body_id, newBindings)) {
        return true;
      }
    }
    return false;
  }

  throw std::runtime_error("Unknown quantifier: " + node->value);
}

// FUNCTION_CALL: value is the function name
// "MEMBER_OF": child 0 is the object, child 1 is the set
EvalResult Evaluator::evalFunctionCall(std::size_t node_id, const Bindings& bindings) const {
  const auto* node = world_.astNodeTable().find(node_id);
  auto children = world_.astChildTable().childrenOf(node_id);

  if (node->value == "MEMBER_OF") {
    if (children.size() < 2) {
      throw std::runtime_error("MEMBER_OF needs object and set arguments");
    }
    EvalResult objResult = evaluate(children[0].child_node_id, bindings);
    EvalResult setResult = evaluate(children[1].child_node_id, bindings);

    std::size_t obj_id = std::get<std::size_t>(objResult);
    std::size_t set_id = std::get<std::size_t>(setResult);

    return world_.isMember(obj_id, set_id);
  }

  throw std::runtime_error("Unknown function: " + node->value);
}

// 5.2 — Obligation evaluation

bool Evaluator::evaluateObligations(std::size_t object_id, std::size_t set_id) const {
  // First check the matrix directly
  if (world_.isMember(object_id, set_id)) {
    return true;
  }

  // Then check obligations lazily
  std::unordered_set<std::size_t> visited;
  return evaluateObligationsImpl(object_id, set_id, visited);
}

bool Evaluator::evaluateObligationsImpl(std::size_t object_id, std::size_t set_id,
                                        std::unordered_set<std::size_t>& visited) const {
  auto obligations = world_.obligationTable().findByTarget(set_id);

  for (const auto* obligation : obligations) {
    // Cycle detection
    if (!visited.insert(obligation->id).second) {
      continue;
    }

    // Set up bindings with the object being tested
    Bindings bindings;
    bindings["self"] = object_id;
    bindings["target"] = set_id;

    // Evaluate antecedent: if true, evaluate consequent
    if (evaluateBool(obligation->antecedent_root_id, bindings)) {
      if (evaluateBool(obligation->consequent_root_id, bindings)) {
        return true;
      }
    }
  }

  return false;
}

// 5.3 — Obligation activation (materializing)

bool Evaluator::activateObligations(std::size_t object_id, std::size_t set_id) {
  // Already a direct member — nothing to activate
  if (world_.isMember(object_id, set_id)) {
    return true;
  }

  std::unordered_set<std::size_t> visited;
  return activateObligationsImpl(object_id, set_id, visited);
}

bool Evaluator::activateObligationsImpl(std::size_t object_id, std::size_t set_id,
                                        std::unordered_set<std::size_t>& visited) {
  auto obligations = world_.obligationTable().findByTarget(set_id);

  for (const auto* obligation : obligations) {
    // Cycle detection
    if (!visited.insert(obligation->id).second) {
      continue;
    }

    // Set up bindings with the object being tested
    Bindings bindings;
    bindings["self"] = object_id;
    bindings["target"] = set_id;

    // Evaluate antecedent: if true, materialize the consequent
    if (evaluateBool(obligation->antecedent_root_id, bindings)) {
      if (materialize(obligation->consequent_root_id, bindings, object_id, set_id)) {
        return true;
      }
    }
  }

  return false;
}

// 5.4 — Consequent materialization
//
// Walk a consequent AST and decompose it into base-case world operations.
//
// Base cases:
//   LITERAL "true"                  → addMember(default_object_id, default_set_id)
//   LITERAL "false"                 → materialization fails
//   FUNCTION_CALL MEMBER_OF(x, S)   → addMember(x, S)
//
// NOT case:
//   UNARY_OP NOT(MEMBER_OF(x, S))   → setNonMember(x, S)
//   UNARY_OP NOT(LITERAL "true")    → setNonMember(default_object_id, default_set_id)
//
// Compound cases:
//   BINARY_OP AND(A, B)             → materialize A, then materialize B
//   BINARY_OP OR(A, B)              → create obligation: (NOT A) → B
//
// Returns true if all materializations succeeded.

bool Evaluator::materialize(std::size_t node_id, Bindings& bindings,
                            std::size_t default_object_id, std::size_t default_set_id) {
  const auto* node = world_.astNodeTable().find(node_id);
  if (!node) {
    throw std::runtime_error("AST node not found during materialization: " + std::to_string(node_id));
  }

  // Base case: LITERAL "true" — materialize the default fact (object ∈ target set)
  if (node->node_type == core::AstNodeType::LITERAL) {
    if (node->value == "true") {
      world_.addMember(default_object_id, default_set_id);
      return true;
    }
    if (node->value == "false") {
      return false;
    }
    throw std::runtime_error("Cannot materialize literal: " + node->value);
  }

  // Base case: FUNCTION_CALL MEMBER_OF(x, S) — materialize x ∈ S
  if (node->node_type == core::AstNodeType::FUNCTION_CALL && node->value == "MEMBER_OF") {
    auto children = world_.astChildTable().childrenOf(node_id);
    if (children.size() < 2) {
      throw std::runtime_error("MEMBER_OF materialization needs object and set arguments");
    }
    EvalResult objResult = evaluate(children[0].child_node_id, bindings);
    EvalResult setResult = evaluate(children[1].child_node_id, bindings);

    std::size_t obj_id = std::get<std::size_t>(objResult);
    std::size_t set_id = std::get<std::size_t>(setResult);

    world_.addMember(obj_id, set_id);
    return true;
  }

  // NOT case: NOT(X) — materialize the negation of X
  if (node->node_type == core::AstNodeType::UNARY_OP && node->value == "NOT") {
    auto children = world_.astChildTable().childrenOf(node_id);
    if (children.empty()) {
      throw std::runtime_error("NOT materialization needs an operand");
    }

    std::size_t child_id = children[0].child_node_id;
    const auto* child = world_.astNodeTable().find(child_id);

    // NOT(MEMBER_OF(x, S)) → setNonMember(x, S)
    if (child->node_type == core::AstNodeType::FUNCTION_CALL && child->value == "MEMBER_OF") {
      auto grandchildren = world_.astChildTable().childrenOf(child_id);
      if (grandchildren.size() < 2) {
        throw std::runtime_error("NOT(MEMBER_OF) materialization needs object and set arguments");
      }
      EvalResult objResult = evaluate(grandchildren[0].child_node_id, bindings);
      EvalResult setResult = evaluate(grandchildren[1].child_node_id, bindings);

      std::size_t obj_id = std::get<std::size_t>(objResult);
      std::size_t set_id = std::get<std::size_t>(setResult);

      world_.setNonMember(obj_id, set_id);
      return true;
    }

    // NOT(LITERAL "true") → setNonMember on the default target
    if (child->node_type == core::AstNodeType::LITERAL && child->value == "true") {
      world_.setNonMember(default_object_id, default_set_id);
      return true;
    }

    throw std::runtime_error("Cannot materialize NOT of node with value: " + child->value);
  }

  // Compound case: AND(A, B) — materialize both
  if (node->node_type == core::AstNodeType::BINARY_OP && node->value == "AND") {
    auto children = world_.astChildTable().childrenOf(node_id);
    if (children.size() < 2) {
      throw std::runtime_error("AND materialization needs 2 operands");
    }
    bool left = materialize(children[0].child_node_id, bindings, default_object_id, default_set_id);
    bool right = materialize(children[1].child_node_id, bindings, default_object_id, default_set_id);
    return left && right;
  }

  // Compound case: OR(A, B) — create obligation (NOT A) → B
  if (node->node_type == core::AstNodeType::BINARY_OP && node->value == "OR") {
    auto children = world_.astChildTable().childrenOf(node_id);
    if (children.size() < 2) {
      throw std::runtime_error("OR materialization needs 2 operands");
    }

    std::size_t left_id = children[0].child_node_id;
    std::size_t right_id = children[1].child_node_id;

    // Build antecedent: NOT(A)
    auto not_a = world_.createAstNode(core::AstNodeType::UNARY_OP, "NOT");
    world_.addAstChild(not_a, 0, left_id);

    // Create obligation: (NOT A) → B, targeting the default set
    world_.addObligation(default_set_id, not_a, right_id);

    return true;
  }

  // Creation case: CREATE_ENTITY("varname") — create a new entity, bind to varname
  if (node->node_type == core::AstNodeType::CREATE_ENTITY) {
    auto* entity = world_.createEntity();
    if (!node->value.empty()) {
      bindings[node->value] = entity->getId();
    }
    return true;
  }

  // Creation case: CREATE_TUPLE("varname") — children are elements, create tuple and bind
  if (node->node_type == core::AstNodeType::CREATE_TUPLE) {
    auto children = world_.astChildTable().childrenOf(node_id);
    std::vector<core::Object*> elements;
    for (const auto& child : children) {
      EvalResult result = evaluate(child.child_node_id, bindings);
      std::size_t elem_id = std::get<std::size_t>(result);
      core::Object* obj = world_.findObject(elem_id);
      if (!obj) {
        throw std::runtime_error("CREATE_TUPLE: element not found: " + std::to_string(elem_id));
      }
      elements.push_back(obj);
    }
    auto* tuple = world_.createTuple(elements);
    if (!node->value.empty()) {
      bindings[node->value] = tuple->getId();
    }
    return true;
  }

  // Creation case: CREATE_SET("SET"/"REL"/"MAP") — child 0 is REFERENCE with varname
  if (node->node_type == core::AstNodeType::CREATE_SET) {
    core::SysType sys_type = core::SysType::SET;
    if (node->value == "REL") sys_type = core::SysType::REL;
    else if (node->value == "MAP") sys_type = core::SysType::MAP;

    auto* set = world_.createSet(sys_type);

    // If there's a child at position 0, it's a REFERENCE node with the variable name
    auto children = world_.astChildTable().childrenOf(node_id);
    if (!children.empty()) {
      const auto* varNode = world_.astNodeTable().find(children[0].child_node_id);
      if (varNode) {
        bindings[varNode->value] = set->getId();
      }
    }
    return true;
  }

  throw std::runtime_error("Cannot materialize AST node type with value: " + node->value);
}

}
