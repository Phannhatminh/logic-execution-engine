#include <iostream>
#include <string>
#include "engine/logic_engine.hpp"
#include "engine/evaluator.hpp"

using namespace logic::core;
using namespace logic::engine;

static int passed = 0;
static int failed = 0;

static void check(const std::string& name, bool condition) {
  if (condition) {
    std::cout << "  PASS: " << name << std::endl;
    passed++;
  } else {
    std::cout << "  FAIL: " << name << std::endl;
    failed++;
  }
}

// ---------------------------------------------------------------------------
// 1. MEMBERSHIP AS UNIVERSAL PRIMITIVE
// ---------------------------------------------------------------------------

static void test_membership_basic() {
  std::cout << "\n--- R-M1/R-M4/R-M5: Basic membership ---" << std::endl;
  World w;

  auto* s = w.createSet();
  auto* a = w.createEntity();
  auto* b = w.createEntity();

  w.addMember(a->getId(), s->getId());

  check("entity added to set is a member", w.isMember(a->getId(), s->getId()));
  check("entity not added is not a member", !w.isMember(b->getId(), s->getId()));
}

static void test_membership_sets_of() {
  std::cout << "\n--- R-M5: sets-of query ---" << std::endl;
  World w;

  auto* s1 = w.createSet();
  auto* s2 = w.createSet();
  auto* a = w.createEntity();

  w.addMember(a->getId(), s1->getId());
  w.addMember(a->getId(), s2->getId());

  auto sets = w.setsOf(a->getId());
  // a is also in the ENTITY type set from bootstrap, so check s1 and s2 are present
  bool found_s1 = false, found_s2 = false;
  for (auto id : sets) {
    if (id == s1->getId()) found_s1 = true;
    if (id == s2->getId()) found_s2 = true;
  }
  check("setsOf returns all sets an entity belongs to", found_s1 && found_s2);
}

static void test_membership_members_of() {
  std::cout << "\n--- R-M5: members-of query ---" << std::endl;
  World w;

  auto* s = w.createSet();
  auto* a = w.createEntity();
  auto* b = w.createEntity();
  auto* c = w.createEntity();

  w.addMember(a->getId(), s->getId());
  w.addMember(b->getId(), s->getId());

  auto members = w.membersOf(s->getId());
  bool found_a = false, found_b = false, found_c = false;
  for (auto id : members) {
    if (id == a->getId()) found_a = true;
    if (id == b->getId()) found_b = true;
    if (id == c->getId()) found_c = true;
  }
  check("membersOf returns added members", found_a && found_b);
  check("membersOf does not return non-members", !found_c);
}

static void test_set_operations() {
  std::cout << "\n--- R-M6: Set operations ---" << std::endl;
  World w;

  auto* s1 = w.createSet();
  auto* s2 = w.createSet();
  auto* x = w.createEntity();
  auto* y = w.createEntity();
  auto* z = w.createEntity();

  w.addMember(x->getId(), s1->getId());
  w.addMember(y->getId(), s1->getId());
  w.addMember(y->getId(), s2->getId());
  w.addMember(z->getId(), s2->getId());

  auto inter = w.intersect(s1->getId(), s2->getId());
  check("intersect: only shared member", inter.size() == 1 && inter[0] == y->getId());

  auto uni = w.unite(s1->getId(), s2->getId());
  check("union: all members from both sets", uni.size() == 3);

  auto diff = w.difference(s1->getId(), s2->getId());
  check("difference: members in first but not second", diff.size() == 1 && diff[0] == x->getId());
}

static void test_typing_as_membership() {
  std::cout << "\n--- R-O8/R-M1: Types are sets ---" << std::endl;
  World w;

  auto* e = w.createEntity();
  auto* s = w.createSet();
  auto* r = w.createSet(SysType::REL);

  check("entity is member of ENTITY type set", w.isMember(e->getId(), w.typeSetId(SysType::ENTITY)));
  check("set is member of SET type set", w.isMember(s->getId(), w.typeSetId(SysType::SET)));
  check("relation is member of SET type set (REL ⊆ SET)", w.isMember(r->getId(), w.typeSetId(SysType::SET)));
  check("relation is member of REL type set", w.isMember(r->getId(), w.typeSetId(SysType::REL)));
}

// ---------------------------------------------------------------------------
// 2. OBLIGATIONS AS INFERENCE
// ---------------------------------------------------------------------------

static void test_obligation_single_step() {
  std::cout << "\n--- R-OB1/R-OB3: Single-step obligation evaluation ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* dogs = w.createSet();
  auto* mammals = w.createSet();
  auto* rex = w.createEntity();
  auto* rock = w.createEntity();

  w.addMember(rex->getId(), dogs->getId());

  // Obligation on mammals: if self ∈ dogs → true
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_dogs = w.createAstNode(AstNodeType::LITERAL, std::to_string(dogs->getId()));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_dogs);
  auto consq = w.createAstNode(AstNodeType::LITERAL, "true");

  w.addObligation(mammals->getId(), ante, consq);

  check("obligation fires for entity satisfying antecedent",
        eval.evaluateObligations(rex->getId(), mammals->getId()));
  check("obligation does not fire for entity not satisfying antecedent",
        !eval.evaluateObligations(rock->getId(), mammals->getId()));
}

static void test_ast_connectives() {
  std::cout << "\n--- R-AST2: Logical connectives (AND, OR, NOT, IMPLIES) ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* s = w.createSet();
  auto* a = w.createEntity();
  auto* b = w.createEntity();
  w.addMember(a->getId(), s->getId());

  // Helper: build MEMBER_OF(var, s) node
  auto make_member_check = [&](const std::string& var) -> std::size_t {
    auto ref = w.createAstNode(AstNodeType::REFERENCE, var);
    auto set_lit = w.createAstNode(AstNodeType::LITERAL, std::to_string(s->getId()));
    auto call = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
    w.addAstChild(call, 0, ref);
    w.addAstChild(call, 1, set_lit);
    return call;
  };

  Bindings bind;
  bind["a"] = a->getId();
  bind["b"] = b->getId();

  // AND: (a ∈ S) AND (b ∈ S) → false
  auto and_node = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(and_node, 0, make_member_check("a"));
  w.addAstChild(and_node, 1, make_member_check("b"));
  check("AND: true AND false = false", !eval.evaluateBool(and_node, bind));

  // OR: (a ∈ S) OR (b ∈ S) → true
  auto or_node = w.createAstNode(AstNodeType::BINARY_OP, "OR");
  w.addAstChild(or_node, 0, make_member_check("a"));
  w.addAstChild(or_node, 1, make_member_check("b"));
  check("OR: true OR false = true", eval.evaluateBool(or_node, bind));

  // NOT: NOT(a ∈ S) → false
  auto not_node = w.createAstNode(AstNodeType::UNARY_OP, "NOT");
  w.addAstChild(not_node, 0, make_member_check("a"));
  check("NOT: NOT true = false", !eval.evaluateBool(not_node, bind));

  // IMPLIES: (b ∈ S) → (a ∈ S) → true (false implies anything)
  auto imp_node = w.createAstNode(AstNodeType::BINARY_OP, "IMPLIES");
  w.addAstChild(imp_node, 0, make_member_check("b"));
  w.addAstChild(imp_node, 1, make_member_check("a"));
  check("IMPLIES: false → true = true (vacuous)", eval.evaluateBool(imp_node, bind));
}

static void test_quantifiers() {
  std::cout << "\n--- R-AST5: Quantifiers (FORALL, EXISTS) ---" << std::endl;
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
  // z is NOT in target

  // FORALL e IN domain: e ∈ target
  auto var_node = w.createAstNode(AstNodeType::REFERENCE, "e");
  auto domain_ref = w.createAstNode(AstNodeType::LITERAL, std::to_string(domain->getId()));
  auto ref_e = w.createAstNode(AstNodeType::REFERENCE, "e");
  auto target_lit = w.createAstNode(AstNodeType::LITERAL, std::to_string(target->getId()));
  auto body = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(body, 0, ref_e);
  w.addAstChild(body, 1, target_lit);

  auto forall = w.createAstNode(AstNodeType::QUANTIFIER, "FORALL");
  w.addAstChild(forall, 0, var_node);
  w.addAstChild(forall, 1, domain_ref);
  w.addAstChild(forall, 2, body);

  Bindings bind;
  check("FORALL: false when not all members satisfy body", !eval.evaluateBool(forall, bind));

  // Now add z to target — FORALL should become true
  w.addMember(z->getId(), target->getId());

  check("FORALL: true when all members satisfy body", eval.evaluateBool(forall, bind));

  // EXISTS e IN domain: e ∈ target (already true since x,y,z all in target)
  auto var_node2 = w.createAstNode(AstNodeType::REFERENCE, "e");
  auto domain_ref2 = w.createAstNode(AstNodeType::LITERAL, std::to_string(domain->getId()));
  auto ref_e2 = w.createAstNode(AstNodeType::REFERENCE, "e");
  auto target_lit2 = w.createAstNode(AstNodeType::LITERAL, std::to_string(target->getId()));
  auto body2 = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(body2, 0, ref_e2);
  w.addAstChild(body2, 1, target_lit2);

  auto exists = w.createAstNode(AstNodeType::QUANTIFIER, "EXISTS");
  w.addAstChild(exists, 0, var_node2);
  w.addAstChild(exists, 1, domain_ref2);
  w.addAstChild(exists, 2, body2);

  check("EXISTS: true when at least one member satisfies body", eval.evaluateBool(exists, bind));
}

// ---------------------------------------------------------------------------
// 3. ACTIVATION AS EXECUTION (materialization)
// ---------------------------------------------------------------------------

static void test_obligation_materialization() {
  std::cout << "\n--- R-OB4: Obligation activation materializes facts ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  // Obligation on B: if self ∈ A → grant membership
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_A = w.createAstNode(AstNodeType::LITERAL, std::to_string(A->getId()));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_A);
  auto consq = w.createAstNode(AstNodeType::LITERAL, "true");

  w.addObligation(B->getId(), ante, consq);

  // Before activation: x should NOT be in B directly
  check("before activation: x not directly in B", !w.isMember(x->getId(), B->getId()));

  // Activation: activate obligation and materialize the fact
  bool fires = eval.activateObligations(x->getId(), B->getId());
  check("obligation activates to true", fires);

  // After activation: x SHOULD be in B in the matrix
  check("after activation: x is directly in B (materialized)",
        w.isMember(x->getId(), B->getId()));
}

static void test_obligation_chaining() {
  std::cout << "\n--- R-OB5: Multi-step obligation chaining ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* C = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  // Obligation on B: if self ∈ A → true
  auto ref_self1 = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_A = w.createAstNode(AstNodeType::LITERAL, std::to_string(A->getId()));
  auto ante1 = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante1, 0, ref_self1);
  w.addAstChild(ante1, 1, lit_A);
  auto consq1 = w.createAstNode(AstNodeType::LITERAL, "true");
  w.addObligation(B->getId(), ante1, consq1);

  // Obligation on C: if self ∈ B → true
  auto ref_self2 = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_B = w.createAstNode(AstNodeType::LITERAL, std::to_string(B->getId()));
  auto ante2 = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante2, 0, ref_self2);
  w.addAstChild(ante2, 1, lit_B);
  auto consq2 = w.createAstNode(AstNodeType::LITERAL, "true");
  w.addObligation(C->getId(), ante2, consq2);

  // Step 1: activate obligation on B — should materialize x ∈ B
  bool step1 = eval.activateObligations(x->getId(), B->getId());
  check("step 1: obligation on B fires (x ∈ A)", step1);
  check("step 1: x is now directly in B", w.isMember(x->getId(), B->getId()));

  // Step 2: activate obligation on C — should see x ∈ B and materialize x ∈ C
  bool step2 = eval.activateObligations(x->getId(), C->getId());
  check("step 2: obligation on C fires (x ∈ B from step 1)", step2);

  // Final: x should be directly in C
  check("x is directly in C after chained activation", w.isMember(x->getId(), C->getId()));
}

static void test_compound_materialization() {
  std::cout << "\n--- R-OB4: Compound consequent materializes multiple facts ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* C = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  // Obligation on B: if self ∈ A → MEMBER_OF(self, B) AND MEMBER_OF(self, C)
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_A = w.createAstNode(AstNodeType::LITERAL, std::to_string(A->getId()));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_A);

  // Consequent: MEMBER_OF(self, B) AND MEMBER_OF(self, C)
  auto ref_self2 = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_B = w.createAstNode(AstNodeType::LITERAL, std::to_string(B->getId()));
  auto mem_B = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem_B, 0, ref_self2);
  w.addAstChild(mem_B, 1, lit_B);

  auto ref_self3 = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_C = w.createAstNode(AstNodeType::LITERAL, std::to_string(C->getId()));
  auto mem_C = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem_C, 0, ref_self3);
  w.addAstChild(mem_C, 1, lit_C);

  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(consq, 0, mem_B);
  w.addAstChild(consq, 1, mem_C);

  w.addObligation(B->getId(), ante, consq);

  check("before: x not in B", !w.isMember(x->getId(), B->getId()));
  check("before: x not in C", !w.isMember(x->getId(), C->getId()));

  bool fires = eval.activateObligations(x->getId(), B->getId());
  check("compound obligation fires", fires);
  check("after: x is in B", w.isMember(x->getId(), B->getId()));
  check("after: x is also in C", w.isMember(x->getId(), C->getId()));
}

static void test_ternary_matrix() {
  std::cout << "\n--- R-M2: Ternary membership matrix ---" << std::endl;
  World w;

  auto* s = w.createSet();
  auto* a = w.createEntity();
  auto* b = w.createEntity();
  auto* c = w.createEntity();

  w.addMember(a->getId(), s->getId());
  w.setNonMember(b->getId(), s->getId());
  // c is not mentioned — should be unknown

  check("a ∈ S: isMember = true", w.isMember(a->getId(), s->getId()));
  check("a ∈ S: isNonMember = false", !w.isNonMember(a->getId(), s->getId()));
  check("a ∈ S: isUnknown = false", !w.isUnknown(a->getId(), s->getId()));

  check("b ∉ S: isMember = false", !w.isMember(b->getId(), s->getId()));
  check("b ∉ S: isNonMember = true", w.isNonMember(b->getId(), s->getId()));
  check("b ∉ S: isUnknown = false", !w.isUnknown(b->getId(), s->getId()));

  check("c ? S: isMember = false", !w.isMember(c->getId(), s->getId()));
  check("c ? S: isNonMember = false", !w.isNonMember(c->getId(), s->getId()));
  check("c ? S: isUnknown = true", w.isUnknown(c->getId(), s->getId()));
}

static void test_not_materialization() {
  std::cout << "\n--- R-OB4: NOT materialization sets non-membership ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  // Obligation on B: if self ∈ A → NOT(MEMBER_OF(self, B))
  // This means: if x is in A, then x is known to NOT be in B
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_A = w.createAstNode(AstNodeType::LITERAL, std::to_string(A->getId()));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_A);

  // Consequent: NOT(MEMBER_OF(self, B))
  auto ref_self2 = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_B = w.createAstNode(AstNodeType::LITERAL, std::to_string(B->getId()));
  auto mem_B = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem_B, 0, ref_self2);
  w.addAstChild(mem_B, 1, lit_B);
  auto not_mem = w.createAstNode(AstNodeType::UNARY_OP, "NOT");
  w.addAstChild(not_mem, 0, mem_B);

  w.addObligation(B->getId(), ante, not_mem);

  check("before: x membership in B is unknown", w.isUnknown(x->getId(), B->getId()));

  bool fires = eval.activateObligations(x->getId(), B->getId());
  check("NOT obligation fires", fires);
  check("after: x is known non-member of B", w.isNonMember(x->getId(), B->getId()));
  check("after: x is not a member of B", !w.isMember(x->getId(), B->getId()));
}

static void test_or_materialization() {
  std::cout << "\n--- R-OB4: OR materialization creates obligation ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* A = w.createSet();
  auto* B = w.createSet();
  auto* C = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), A->getId());

  // Obligation on B: if self ∈ A → MEMBER_OF(self, B) OR MEMBER_OF(self, C)
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_A = w.createAstNode(AstNodeType::LITERAL, std::to_string(A->getId()));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_A);

  // Consequent: MEMBER_OF(self, B) OR MEMBER_OF(self, C)
  auto ref_self2 = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_B = w.createAstNode(AstNodeType::LITERAL, std::to_string(B->getId()));
  auto mem_B = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem_B, 0, ref_self2);
  w.addAstChild(mem_B, 1, lit_B);

  auto ref_self3 = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_C = w.createAstNode(AstNodeType::LITERAL, std::to_string(C->getId()));
  auto mem_C = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem_C, 0, ref_self3);
  w.addAstChild(mem_C, 1, lit_C);

  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "OR");
  w.addAstChild(consq, 0, mem_B);
  w.addAstChild(consq, 1, mem_C);

  w.addObligation(B->getId(), ante, consq);

  // Activate: should NOT directly write x into B or C
  // Instead it should create an obligation: NOT(MEMBER_OF(self, B)) → MEMBER_OF(self, C)
  bool fires = eval.activateObligations(x->getId(), B->getId());
  check("OR obligation fires (creates constraint)", fires);
  check("x is NOT directly in B (OR doesn't pick a side)", !w.isMember(x->getId(), B->getId()));
  check("x is NOT directly in C (OR doesn't pick a side)", !w.isMember(x->getId(), C->getId()));

  // Now the proof establishes that x is NOT in B
  w.setNonMember(x->getId(), B->getId());

  // Activate the derived obligation on B: NOT(MEMBER_OF(self, B)) → MEMBER_OF(self, C)
  // The antecedent checks NOT(MEMBER_OF(self, B)) — which evaluates self ∈ B → false,
  // then NOT(false) → true. So the obligation fires and materializes MEMBER_OF(self, C).
  bool derived = eval.activateObligations(x->getId(), B->getId());
  check("derived obligation fires after NOT(x ∈ B) established", derived);
  check("x is now in C (derived from OR + NOT B)", w.isMember(x->getId(), C->getId()));
}

static void test_create_entity_materialization() {
  std::cout << "\n--- R-OB4: CREATE_ENTITY materialization ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* trigger = w.createSet();
  auto* target = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), trigger->getId());

  // Obligation on target: if self ∈ trigger →
  //   CREATE_ENTITY("newobj") AND MEMBER_OF(newobj, target)
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_trigger = w.createAstNode(AstNodeType::LITERAL, std::to_string(trigger->getId()));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_trigger);

  // Consequent: CREATE_ENTITY("newobj") AND MEMBER_OF(newobj, target)
  auto create = w.createAstNode(AstNodeType::CREATE_ENTITY, "newobj");

  auto ref_new = w.createAstNode(AstNodeType::REFERENCE, "newobj");
  auto lit_target = w.createAstNode(AstNodeType::LITERAL, std::to_string(target->getId()));
  auto mem = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem, 0, ref_new);
  w.addAstChild(mem, 1, lit_target);

  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(consq, 0, create);
  w.addAstChild(consq, 1, mem);

  w.addObligation(target->getId(), ante, consq);

  auto members_before = w.membersOf(target->getId());
  check("before: target set is empty", members_before.empty());

  bool fires = eval.activateObligations(x->getId(), target->getId());
  check("obligation fires", fires);

  auto members_after = w.membersOf(target->getId());
  check("after: target set has one new member", members_after.size() == 1);
  check("after: new member is not x", !members_after.empty() && members_after[0] != x->getId());
}

static void test_create_tuple_materialization() {
  std::cout << "\n--- R-OB4: CREATE_TUPLE materialization ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* domain = w.createSet();
  auto* rel = w.createSet(SysType::REL);
  auto* a = w.createEntity();
  auto* b = w.createEntity();

  w.addMember(a->getId(), domain->getId());

  // Obligation on rel: if self ∈ domain →
  //   CREATE_TUPLE("pair", children: self, b_literal) AND MEMBER_OF(pair, rel)
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_domain = w.createAstNode(AstNodeType::LITERAL, std::to_string(domain->getId()));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_domain);

  // Consequent: CREATE_TUPLE("pair") with elements (self, b) AND MEMBER_OF(pair, rel)
  auto ref_self2 = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_b = w.createAstNode(AstNodeType::LITERAL, std::to_string(b->getId()));
  auto create_tuple = w.createAstNode(AstNodeType::CREATE_TUPLE, "pair");
  w.addAstChild(create_tuple, 0, ref_self2);
  w.addAstChild(create_tuple, 1, lit_b);

  auto ref_pair = w.createAstNode(AstNodeType::REFERENCE, "pair");
  auto lit_rel = w.createAstNode(AstNodeType::LITERAL, std::to_string(rel->getId()));
  auto mem = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem, 0, ref_pair);
  w.addAstChild(mem, 1, lit_rel);

  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(consq, 0, create_tuple);
  w.addAstChild(consq, 1, mem);

  w.addObligation(rel->getId(), ante, consq);

  check("before: rel is empty", w.membersOf(rel->getId()).empty());

  bool fires = eval.activateObligations(a->getId(), rel->getId());
  check("obligation fires", fires);

  auto rel_members = w.membersOf(rel->getId());
  check("after: rel has one tuple", rel_members.size() == 1);

  // Verify the tuple contains (a, b)
  if (!rel_members.empty()) {
    auto* obj = w.findObject(rel_members[0]);
    check("after: tuple exists", obj != nullptr);
    check("after: tuple is in TUPLE type set",
          w.isMember(rel_members[0], w.typeSetId(SysType::TUPLE)));
  }
}

static void test_create_set_materialization() {
  std::cout << "\n--- R-OB4: CREATE_SET materialization ---" << std::endl;
  World w;
  Evaluator eval(w);

  auto* trigger = w.createSet();
  auto* container = w.createSet();
  auto* x = w.createEntity();

  w.addMember(x->getId(), trigger->getId());

  // Obligation on container: if self ∈ trigger →
  //   CREATE_SET("SET", varname="newset") AND MEMBER_OF(newset, container)
  auto ref_self = w.createAstNode(AstNodeType::REFERENCE, "self");
  auto lit_trigger = w.createAstNode(AstNodeType::LITERAL, std::to_string(trigger->getId()));
  auto ante = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(ante, 0, ref_self);
  w.addAstChild(ante, 1, lit_trigger);

  // Consequent: CREATE_SET("SET") with child REFERENCE("newset") AND MEMBER_OF(newset, container)
  auto var_ref = w.createAstNode(AstNodeType::REFERENCE, "newset");
  auto create_set = w.createAstNode(AstNodeType::CREATE_SET, "SET");
  w.addAstChild(create_set, 0, var_ref);

  auto ref_newset = w.createAstNode(AstNodeType::REFERENCE, "newset");
  auto lit_container = w.createAstNode(AstNodeType::LITERAL, std::to_string(container->getId()));
  auto mem = w.createAstNode(AstNodeType::FUNCTION_CALL, "MEMBER_OF");
  w.addAstChild(mem, 0, ref_newset);
  w.addAstChild(mem, 1, lit_container);

  auto consq = w.createAstNode(AstNodeType::BINARY_OP, "AND");
  w.addAstChild(consq, 0, create_set);
  w.addAstChild(consq, 1, mem);

  w.addObligation(container->getId(), ante, consq);

  check("before: container is empty", w.membersOf(container->getId()).empty());

  bool fires = eval.activateObligations(x->getId(), container->getId());
  check("obligation fires", fires);

  auto members = w.membersOf(container->getId());
  check("after: container has one new set", members.size() == 1);

  if (!members.empty()) {
    check("after: new set is in SET type set",
          w.isMember(members[0], w.typeSetId(SysType::SET)));
  }
}

// ---------------------------------------------------------------------------

int main() {
  std::cout << "=== Logic Execution Engine — System Survey ===" << std::endl;

  // 1. Membership as universal primitive
  std::cout << "\n====== CORE 1: MEMBERSHIP AS UNIVERSAL PRIMITIVE ======" << std::endl;
  test_membership_basic();
  test_membership_sets_of();
  test_membership_members_of();
  test_set_operations();
  test_typing_as_membership();

  // 2. Obligations as inference
  std::cout << "\n====== CORE 2: OBLIGATIONS AS INFERENCE ======" << std::endl;
  test_obligation_single_step();
  test_ast_connectives();
  test_quantifiers();

  // 3. Activation as execution
  std::cout << "\n====== CORE 3: ACTIVATION AS EXECUTION ======" << std::endl;
  test_obligation_materialization();
  test_obligation_chaining();
  test_compound_materialization();
  test_ternary_matrix();
  test_not_materialization();
  test_or_materialization();
  test_create_entity_materialization();
  test_create_tuple_materialization();
  test_create_set_materialization();

  // Summary
  std::cout << "\n=== SUMMARY ===" << std::endl;
  std::cout << "Passed: " << passed << std::endl;
  std::cout << "Failed: " << failed << std::endl;
  std::cout << "Total:  " << passed + failed << std::endl;

  return failed > 0 ? 1 : 0;
}
