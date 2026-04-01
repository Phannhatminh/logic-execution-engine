#include <gtest/gtest.h>
#include "engine/logic_engine.hpp"
#include "engine/evaluator.hpp"

using namespace logic::core;
using namespace logic::engine;

// Helper: build an antecedent AST "MEMBER_OF(self, set_id)"
static std::size_t makeAntecedent(World& w, std::size_t set_id) {
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_set = w.createAstNode(AstNodeType::LITERAL, std::to_string(set_id));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_set);
  return ante;
}

// Helper: build a MEMBER_OF(varname, set_id) AST node
static std::size_t makeMemberOf(World& w, const std::string& varname, std::size_t set_id) {
  auto ref = w.createAstNode(AstNodeType::REFERENCE, varname);
  auto lit = w.createAstNode(AstNodeType::LITERAL, std::to_string(set_id));
  auto mem = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem, 0, ref);
  w.addAstChild(mem, 1, lit);
  return mem;
}

// ---------------------------------------------------------------------------
// R-OB1/R-OB3: Single-step obligation evaluation
// ---------------------------------------------------------------------------

TEST(Obligations, SingleStepEvaluation) {
  World w;
  Evaluator eval(w);

  auto* dogs = w.createSet();
  auto* mammals = w.createSet();
  auto* rex = w.createEntity();
  auto* rock = w.createEntity();

  w.addMember(rex->getId(), dogs->getId());

  auto ante = makeAntecedent(w, dogs->getId());
  auto consq = w.createAstNode(AstNodeType::LITERAL, "true");
  w.addObligation(mammals->getId(), ante, consq);

  EXPECT_TRUE(eval.evaluateObligations(rex->getId(), mammals->getId()));
  EXPECT_FALSE(eval.evaluateObligations(rock->getId(), mammals->getId()));
}

// ---------------------------------------------------------------------------
// R-OB4: Obligation activation materializes facts
// ---------------------------------------------------------------------------

TEST(Obligations, ActivationMaterializesFacts) {
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  auto ante = makeAntecedent(w, A->getId());
  auto consq = w.createAstNode(AstNodeType::LITERAL, "true");
  w.addObligation(B->getId(), ante, consq);

  EXPECT_FALSE(w.isMember(x->getId(), B->getId()));

  EXPECT_TRUE(eval.activateObligations(x->getId(), B->getId()));
  EXPECT_TRUE(w.isMember(x->getId(), B->getId()));
}

// ---------------------------------------------------------------------------
// R-OB5: Multi-step obligation chaining
// ---------------------------------------------------------------------------

TEST(Obligations, Chaining) {
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* C = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  auto ante1 = makeAntecedent(w, A->getId());
  auto consq1 = w.createAstNode(AstNodeType::LITERAL, "true");
  w.addObligation(B->getId(), ante1, consq1);

  auto ante2 = makeAntecedent(w, B->getId());
  auto consq2 = w.createAstNode(AstNodeType::LITERAL, "true");
  w.addObligation(C->getId(), ante2, consq2);

  EXPECT_TRUE(eval.activateObligations(x->getId(), B->getId()));
  EXPECT_TRUE(w.isMember(x->getId(), B->getId()));

  EXPECT_TRUE(eval.activateObligations(x->getId(), C->getId()));
  EXPECT_TRUE(w.isMember(x->getId(), C->getId()));
}

// ---------------------------------------------------------------------------
// R-OB4: Compound consequent (AND)
// ---------------------------------------------------------------------------

TEST(Obligations, CompoundAND) {
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* C = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  auto ante = makeAntecedent(w, A->getId());
  auto mem_B = makeMemberOf(w, "self", B->getId());
  auto mem_C = makeMemberOf(w, "self", C->getId());
  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(consq, 0, mem_B);
  w.addAstChild(consq, 1, mem_C);

  w.addObligation(B->getId(), ante, consq);

  EXPECT_TRUE(eval.activateObligations(x->getId(), B->getId()));
  EXPECT_TRUE(w.isMember(x->getId(), B->getId()));
  EXPECT_TRUE(w.isMember(x->getId(), C->getId()));
}

// ---------------------------------------------------------------------------
// R-OB4: NOT materialization
// ---------------------------------------------------------------------------

TEST(Obligations, NotMaterialization) {
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  auto ante = makeAntecedent(w, A->getId());
  auto mem_B = makeMemberOf(w, "self", B->getId());
  auto not_mem = w.createAstNode(AstNodeType::UNARY_OP, "NOT");
  w.addAstChild(not_mem, 0, mem_B);

  w.addObligation(B->getId(), ante, not_mem);

  EXPECT_TRUE(w.isUnknown(x->getId(), B->getId()));

  EXPECT_TRUE(eval.activateObligations(x->getId(), B->getId()));
  EXPECT_TRUE(w.isNonMember(x->getId(), B->getId()));
  EXPECT_FALSE(w.isMember(x->getId(), B->getId()));
}

// ---------------------------------------------------------------------------
// R-OB4: OR materialization
// ---------------------------------------------------------------------------

TEST(Obligations, OrMaterialization) {
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* C = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  auto ante = makeAntecedent(w, A->getId());
  auto mem_B = makeMemberOf(w, "self", B->getId());
  auto mem_C = makeMemberOf(w, "self", C->getId());
  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "OR");
  w.addAstChild(consq, 0, mem_B);
  w.addAstChild(consq, 1, mem_C);

  w.addObligation(B->getId(), ante, consq);

  // OR creates a constraint, doesn't pick a side
  EXPECT_TRUE(eval.activateObligations(x->getId(), B->getId()));
  EXPECT_FALSE(w.isMember(x->getId(), B->getId()));
  EXPECT_FALSE(w.isMember(x->getId(), C->getId()));

  // Proof establishes NOT(x ∈ B)
  w.setNonMember(x->getId(), B->getId());

  // Derived obligation fires: NOT(x ∈ B) → x ∈ C
  EXPECT_TRUE(eval.activateObligations(x->getId(), B->getId()));
  EXPECT_TRUE(w.isMember(x->getId(), C->getId()));
}
