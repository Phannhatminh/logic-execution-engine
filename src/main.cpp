#include <iostream>
#include <string>
#include "engine/logic_engine.hpp"
#include "engine/evaluator.hpp"

using namespace logic::core;
using namespace logic::engine;

// ---------------------------------------------------------------------------
// Helper: build MEMBER_OF(varname, set_id) AST node
// ---------------------------------------------------------------------------
static std::size_t makeMemberOf(World& w, const std::string& var, std::size_t set_id) {
  auto ref = w.createAstNode(AstNodeType::REFERENCE, var);
  auto lit = w.createAstNode(AstNodeType::LITERAL, std::to_string(set_id));
  auto call = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(call, 0, ref);
  w.addAstChild(call, 1, lit);
  return call;
}

// ---------------------------------------------------------------------------
// Proof: Transitivity of Subset Inclusion
//
// Theorem. If A ⊆ B and B ⊆ C, and x ∈ A, then x ∈ C.
//
// Proof.
//   1. Let A, B, C be sets.                       [introduce]
//   2. Let x be an entity.                        [introduce]
//   3. Assume x ∈ A.                              [premise]
//   4. Assume A ⊆ B.                              [premise, as obligation]
//   5. Assume B ⊆ C.                              [premise, as obligation]
//   6. Since x ∈ A and A ⊆ B, we have x ∈ B.     [activate obligation from 4]
//   7. Since x ∈ B and B ⊆ C, we have x ∈ C.     [activate obligation from 5]
//   8. Therefore x ∈ C.                           [verify]  QED.
// ---------------------------------------------------------------------------

static bool proof_subset_transitivity() {
  std::cout << "=== Theorem: Transitivity of Subset Inclusion ===" << std::endl;
  std::cout << "If A ⊆ B and B ⊆ C, and x ∈ A, then x ∈ C." << std::endl;
  std::cout << std::endl;

  World w;
  Evaluator eval(w);

  // Step 1. Let A, B, C be sets.
  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* C = w.createSet();
  std::cout << "1. Let A, B, C be sets." << std::endl;

  // Step 2. Let x be an entity.
  auto* x = w.createEntity();
  std::cout << "2. Let x be an entity." << std::endl;

  // Step 3. Assume x ∈ A.
  w.addMember(x->getId(), A->getId());
  std::cout << "3. Assume x ∈ A.                              [x ∈ A recorded]" << std::endl;

  // Step 4. Assume A ⊆ B.
  //   Encoded as obligation on B: if self ∈ A → self ∈ B
  auto ante_AB = makeMemberOf(w, "self", A->getId());
  auto consq_AB = makeMemberOf(w, "self", B->getId());
  w.addObligation(B->getId(), ante_AB, consq_AB);
  std::cout << "4. Assume A ⊆ B.                              [obligation: self ∈ A → self ∈ B]" << std::endl;

  // Step 5. Assume B ⊆ C.
  //   Encoded as obligation on C: if self ∈ B → self ∈ C
  auto ante_BC = makeMemberOf(w, "self", B->getId());
  auto consq_BC = makeMemberOf(w, "self", C->getId());
  w.addObligation(C->getId(), ante_BC, consq_BC);
  std::cout << "5. Assume B ⊆ C.                              [obligation: self ∈ B → self ∈ C]" << std::endl;

  // Step 6. Since x ∈ A and A ⊆ B, we have x ∈ B.
  //   Activate the A ⊆ B obligation for x against B.
  //   Antecedent: x ∈ A? Yes (step 3). Consequent: materialize x ∈ B.
  bool step6 = eval.activateObligations(x->getId(), B->getId());
  std::cout << "6. Since x ∈ A and A ⊆ B, x ∈ B.             [activate: "
            << (step6 ? "OK" : "FAILED") << ", x ∈ B = "
            << w.isMember(x->getId(), B->getId()) << "]" << std::endl;

  // Step 7. Since x ∈ B and B ⊆ C, we have x ∈ C.
  //   Activate the B ⊆ C obligation for x against C.
  //   Antecedent: x ∈ B? Yes (step 6 materialized it). Consequent: materialize x ∈ C.
  bool step7 = eval.activateObligations(x->getId(), C->getId());
  std::cout << "7. Since x ∈ B and B ⊆ C, x ∈ C.             [activate: "
            << (step7 ? "OK" : "FAILED") << ", x ∈ C = "
            << w.isMember(x->getId(), C->getId()) << "]" << std::endl;

  // Step 8. Verify: x ∈ C.
  bool qed = w.isMember(x->getId(), C->getId());
  std::cout << "8. Therefore x ∈ C.                           [verify: "
            << (qed ? "QED" : "INVALID") << "]" << std::endl;

  return qed;
}

// ---------------------------------------------------------------------------

int main() {
  bool valid = proof_subset_transitivity();
  std::cout << std::endl;
  std::cout << "Proof " << (valid ? "VALID" : "INVALID") << "." << std::endl;
  return valid ? 0 : 1;
}
