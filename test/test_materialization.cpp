#include <gtest/gtest.h>
#include "engine/logic_engine.hpp"
#include "engine/evaluator.hpp"

using namespace logic::core;
using namespace logic::engine;

// Helper: build antecedent MEMBER_OF(self, set_id)
static AstNodeId makeAntecedent(World& w, ObjectId set_id) {
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_set = w.createAstNode(AstNodeType::LITERAL, std::to_string(set_id.value));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_set);
  return ante;
}

// ---------------------------------------------------------------------------
// CREATE_ENTITY
// ---------------------------------------------------------------------------

TEST(Materialization, CreateEntity) {
  World w;
  Evaluator eval(w);

  auto* trigger = w.createSet();
  auto* target = w.createSet();
  auto* x = w.createEntity();
  w.addMember(x->getId(), trigger->getId());

  // Consequent: CREATE_ENTITY("newobj") AND MEMBER_OF(newobj, target)
  auto create = w.createAstNode(AstNodeType::CREATE_ENTITY, "newobj");
  auto ref_new = w.createAstNode(AstNodeType::REFERENCE, "newobj");
  auto lit_target = w.createAstNode(AstNodeType::LITERAL, std::to_string(target->getId().value));
  auto mem = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem, 0, ref_new);
  w.addAstChild(mem, 1, lit_target);
  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(consq, 0, create);
  w.addAstChild(consq, 1, mem);

  auto ante = makeAntecedent(w, trigger->getId());
  w.addObligation(target->getId(), ante, consq);

  EXPECT_TRUE(w.membersOf(target->getId()).empty());

  EXPECT_TRUE(eval.activateObligations(x->getId(), target->getId()));

  auto members = w.membersOf(target->getId());
  ASSERT_EQ(members.size(), 1u);
  EXPECT_NE(members[0], x->getId());
}

// ---------------------------------------------------------------------------
// CREATE_TUPLE
// ---------------------------------------------------------------------------

TEST(Materialization, CreateTuple) {
  World w;
  Evaluator eval(w);

  auto* domain = w.createSet();
  auto* rel = w.createSet(SysType::REL);
  auto* a = w.createEntity();
  auto* b = w.createEntity();
  w.addMember(a->getId(), domain->getId());

  // Consequent: CREATE_TUPLE("pair", self, b) AND MEMBER_OF(pair, rel)
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_b = w.createAstNode(AstNodeType::LITERAL, std::to_string(b->getId().value));
  auto create_tuple = w.createAstNode(AstNodeType::CREATE_TUPLE, "pair");
  w.addAstChild(create_tuple, 0, ref_self);
  w.addAstChild(create_tuple, 1, lit_b);

  auto ref_pair = w.createAstNode(AstNodeType::REFERENCE, "pair");
  auto lit_rel = w.createAstNode(AstNodeType::LITERAL, std::to_string(rel->getId().value));
  auto mem = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem, 0, ref_pair);
  w.addAstChild(mem, 1, lit_rel);

  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(consq, 0, create_tuple);
  w.addAstChild(consq, 1, mem);

  auto ante = makeAntecedent(w, domain->getId());
  w.addObligation(rel->getId(), ante, consq);

  EXPECT_TRUE(w.membersOf(rel->getId()).empty());

  EXPECT_TRUE(eval.activateObligations(a->getId(), rel->getId()));

  auto rel_members = w.membersOf(rel->getId());
  ASSERT_EQ(rel_members.size(), 1u);
  EXPECT_TRUE(w.isMember(rel_members[0], w.typeSetId(SysType::TUPLE)));
}

// ---------------------------------------------------------------------------
// CREATE_SET
// ---------------------------------------------------------------------------

TEST(Materialization, CreateSet) {
  World w;
  Evaluator eval(w);

  auto* trigger = w.createSet();
  auto* container = w.createSet();
  auto* x = w.createEntity();
  w.addMember(x->getId(), trigger->getId());

  // Consequent: CREATE_SET("SET", var="newset") AND MEMBER_OF(newset, container)
  auto var_ref = w.createAstNode(AstNodeType::REFERENCE, "newset");
  auto create_set = w.createAstNode(AstNodeType::CREATE_SET, "SET");
  w.addAstChild(create_set, 0, var_ref);

  auto ref_newset = w.createAstNode(AstNodeType::REFERENCE, "newset");
  auto lit_container = w.createAstNode(AstNodeType::LITERAL, std::to_string(container->getId().value));
  auto mem = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem, 0, ref_newset);
  w.addAstChild(mem, 1, lit_container);

  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(consq, 0, create_set);
  w.addAstChild(consq, 1, mem);

  auto ante = makeAntecedent(w, trigger->getId());
  w.addObligation(container->getId(), ante, consq);

  EXPECT_TRUE(w.membersOf(container->getId()).empty());

  EXPECT_TRUE(eval.activateObligations(x->getId(), container->getId()));

  auto members = w.membersOf(container->getId());
  ASSERT_EQ(members.size(), 1u);
  EXPECT_TRUE(w.isMember(members[0], w.typeSetId(SysType::SET)));
}

// ---------------------------------------------------------------------------
// Multi-step creation: entity + tuple + relation membership
// ---------------------------------------------------------------------------

TEST(Materialization, SuccessorPattern) {
  World w;
  Evaluator eval(w);

  auto* nat = w.createSet();
  auto* succ = w.createSet(SysType::REL);
  auto* n = w.createEntity();
  w.addMember(n->getId(), nat->getId());

  // Obligation on succ: if self ∈ nat →
  //   CREATE_ENTITY("m") AND MEMBER_OF(m, nat) AND
  //   CREATE_TUPLE("t", self, m) AND MEMBER_OF(t, succ)
  //
  // The obligation targets succ (the relation), triggered by nat membership.
  // The proof activates it for n against succ.
  auto ante = makeAntecedent(w, nat->getId());

  auto create_m = w.createAstNode(AstNodeType::CREATE_ENTITY, "m");
  auto mem_m_nat_ref = w.createAstNode(AstNodeType::REFERENCE, "m");
  auto mem_m_nat_lit = w.createAstNode(AstNodeType::LITERAL, std::to_string(nat->getId().value));
  auto mem_m_nat = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem_m_nat, 0, mem_m_nat_ref);
  w.addAstChild(mem_m_nat, 1, mem_m_nat_lit);

  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto ref_m = w.createAstNode(AstNodeType::REFERENCE, "m");
  auto create_t = w.createAstNode(AstNodeType::CREATE_TUPLE, "t");
  w.addAstChild(create_t, 0, ref_self);
  w.addAstChild(create_t, 1, ref_m);

  auto ref_t = w.createAstNode(AstNodeType::REFERENCE, "t");
  auto lit_succ = w.createAstNode(AstNodeType::LITERAL, std::to_string(succ->getId().value));
  auto mem_t_succ = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem_t_succ, 0, ref_t);
  w.addAstChild(mem_t_succ, 1, lit_succ);

  // Build AND chain: create_m AND (mem_m_nat AND (create_t AND mem_t_succ))
  auto and_inner = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(and_inner, 0, create_t);
  w.addAstChild(and_inner, 1, mem_t_succ);

  auto and_mid = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(and_mid, 0, mem_m_nat);
  w.addAstChild(and_mid, 1, and_inner);

  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(consq, 0, create_m);
  w.addAstChild(consq, 1, and_mid);

  w.addObligation(succ->getId(), ante, consq);

  // Before: nat has n, succ is empty
  EXPECT_EQ(w.membersOf(nat->getId()).size(), 1u);
  EXPECT_TRUE(w.membersOf(succ->getId()).empty());

  // Activate for n against succ: should create m, add m to nat, create (n, m), add to succ
  EXPECT_TRUE(eval.activateObligations(n->getId(), succ->getId()));

  // After: nat has n and m (the newly created successor)
  auto nat_members = w.membersOf(nat->getId());
  EXPECT_EQ(nat_members.size(), 2u);

  // succ has one tuple (n, m)
  auto succ_members = w.membersOf(succ->getId());
  ASSERT_EQ(succ_members.size(), 1u);
  EXPECT_TRUE(w.isMember(succ_members[0], w.typeSetId(SysType::TUPLE)));
}
