#include <gtest/gtest.h>
#include "engine/logic_engine.hpp"
#include "engine/evaluator.hpp"

using namespace logic::core;
using namespace logic::engine;

// Helper: build MEMBER_OF(var, set_id)
static AstNodeId makeMemberCheck(World& w, const std::string& var, ObjectId set_id) {
  auto ref = w.createAstNode(AstNodeType::REFERENCE, var);
  auto lit = w.createAstNode(AstNodeType::LITERAL, std::to_string(set_id));
  auto call = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(call, 0, ref);
  w.addAstChild(call, 1, lit);
  return call;
}

// ---------------------------------------------------------------------------
// R-AST2: Logical connectives
// ---------------------------------------------------------------------------

class AstConnectives : public ::testing::Test {
protected:
  World w;
  Evaluator eval{w};
  Object* s;
  Entity* a;
  Entity* b;
  Bindings bind;

  void SetUp() override {
    s = w.createSet();
    a = w.createEntity();
    b = w.createEntity();
    w.addMember(a->getId(), s->getId());
    // b is NOT in s
    bind["a"] = a->getId();
    bind["b"] = b->getId();
  }
};

TEST_F(AstConnectives, AND) {
  auto node = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(node, 0, makeMemberCheck(w, "a", s->getId()));
  w.addAstChild(node, 1, makeMemberCheck(w, "b", s->getId()));
  EXPECT_FALSE(eval.evaluateBool(node, bind));
}

TEST_F(AstConnectives, OR) {
  auto node = w.createAstNode(AstNodeType::BINARY_OP, "OR");
  w.addAstChild(node, 0, makeMemberCheck(w, "a", s->getId()));
  w.addAstChild(node, 1, makeMemberCheck(w, "b", s->getId()));
  EXPECT_TRUE(eval.evaluateBool(node, bind));
}

TEST_F(AstConnectives, NOT) {
  auto node = w.createAstNode(AstNodeType::UNARY_OP, "NOT");
  w.addAstChild(node, 0, makeMemberCheck(w, "a", s->getId()));
  EXPECT_FALSE(eval.evaluateBool(node, bind));
}

TEST_F(AstConnectives, IMPLIES_VacuouslyTrue) {
  // false → true = true
  auto node = w.createAstNode(AstNodeType::BINARY_OP, "IMPLIES");
  w.addAstChild(node, 0, makeMemberCheck(w, "b", s->getId()));
  w.addAstChild(node, 1, makeMemberCheck(w, "a", s->getId()));
  EXPECT_TRUE(eval.evaluateBool(node, bind));
}

TEST_F(AstConnectives, IMPLIES_TrueToFalse) {
  // true → false = false
  auto node = w.createAstNode(AstNodeType::BINARY_OP, "IMPLIES");
  w.addAstChild(node, 0, makeMemberCheck(w, "a", s->getId()));
  w.addAstChild(node, 1, makeMemberCheck(w, "b", s->getId()));
  EXPECT_FALSE(eval.evaluateBool(node, bind));
}

// ---------------------------------------------------------------------------
// R-AST5: Quantifiers
// ---------------------------------------------------------------------------

TEST(AstQuantifiers, ForAllFalse) {
  World w;
  Evaluator eval(w);

  auto* domain = w.createSet();
  auto* target = w.createSet();
  auto* x = w.createEntity();
  auto* y = w.createEntity();
  auto* z = w.createEntity();

  w.addMember(x->getId(), domain->getId());
  w.addMember(y->getId(), domain->getId());
  w.addMember(z->getId(), domain->getId());
  w.addMember(x->getId(), target->getId());
  w.addMember(y->getId(), target->getId());
  // z NOT in target

  auto var = w.createAstNode(AstNodeType::REFERENCE, "e");
  auto dom = w.createAstNode(AstNodeType::LITERAL, std::to_string(domain->getId()));
  auto body = makeMemberCheck(w, "e", target->getId());
  auto forall = w.createAstNode(AstNodeType::QUANTIFIER, "FORALL");
  w.addAstChild(forall, 0, var);
  w.addAstChild(forall, 1, dom);
  w.addAstChild(forall, 2, body);

  Bindings bind;
  EXPECT_FALSE(eval.evaluateBool(forall, bind));
}

TEST(AstQuantifiers, ForAllTrue) {
  World w;
  Evaluator eval(w);

  auto* domain = w.createSet();
  auto* target = w.createSet();
  auto* x = w.createEntity();
  auto* y = w.createEntity();

  w.addMember(x->getId(), domain->getId());
  w.addMember(y->getId(), domain->getId());
  w.addMember(x->getId(), target->getId());
  w.addMember(y->getId(), target->getId());

  auto var = w.createAstNode(AstNodeType::REFERENCE, "e");
  auto dom = w.createAstNode(AstNodeType::LITERAL, std::to_string(domain->getId()));
  auto body = makeMemberCheck(w, "e", target->getId());
  auto forall = w.createAstNode(AstNodeType::QUANTIFIER, "FORALL");
  w.addAstChild(forall, 0, var);
  w.addAstChild(forall, 1, dom);
  w.addAstChild(forall, 2, body);

  Bindings bind;
  EXPECT_TRUE(eval.evaluateBool(forall, bind));
}

TEST(AstQuantifiers, Exists) {
  World w;
  Evaluator eval(w);

  auto* domain = w.createSet();
  auto* target = w.createSet();
  auto* x = w.createEntity();
  auto* y = w.createEntity();

  w.addMember(x->getId(), domain->getId());
  w.addMember(y->getId(), domain->getId());
  w.addMember(x->getId(), target->getId());
  // y NOT in target

  auto var = w.createAstNode(AstNodeType::REFERENCE, "e");
  auto dom = w.createAstNode(AstNodeType::LITERAL, std::to_string(domain->getId()));
  auto body = makeMemberCheck(w, "e", target->getId());
  auto exists = w.createAstNode(AstNodeType::QUANTIFIER, "EXISTS");
  w.addAstChild(exists, 0, var);
  w.addAstChild(exists, 1, dom);
  w.addAstChild(exists, 2, body);

  Bindings bind;
  EXPECT_TRUE(eval.evaluateBool(exists, bind));
}
