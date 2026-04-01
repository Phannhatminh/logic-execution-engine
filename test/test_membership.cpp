#include <gtest/gtest.h>
#include "engine/logic_engine.hpp"
#include "engine/evaluator.hpp"

using namespace logic::core;
using namespace logic::engine;

// ---------------------------------------------------------------------------
// R-M1/R-M4/R-M5: Basic membership
// ---------------------------------------------------------------------------

TEST(Membership, AddAndQuery) {
  World w;
  auto* s = w.createSet();
  auto* a = w.createEntity();
  auto* b = w.createEntity();

  w.addMember(a->getId(), s->getId());

  EXPECT_TRUE(w.isMember(a->getId(), s->getId()));
  EXPECT_FALSE(w.isMember(b->getId(), s->getId()));
}

TEST(Membership, SetsOfQuery) {
  World w;
  auto* s1 = w.createSet();
  auto* s2 = w.createSet();
  auto* a = w.createEntity();

  w.addMember(a->getId(), s1->getId());
  w.addMember(a->getId(), s2->getId());

  auto sets = w.setsOf(a->getId());
  bool found_s1 = false, found_s2 = false;
  for (auto id : sets) {
    if (id == s1->getId()) found_s1 = true;
    if (id == s2->getId()) found_s2 = true;
  }
  EXPECT_TRUE(found_s1);
  EXPECT_TRUE(found_s2);
}

TEST(Membership, MembersOfQuery) {
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
  EXPECT_TRUE(found_a);
  EXPECT_TRUE(found_b);
  EXPECT_FALSE(found_c);
}

// ---------------------------------------------------------------------------
// R-M6: Set operations
// ---------------------------------------------------------------------------

TEST(Membership, Intersect) {
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
  ASSERT_EQ(inter.size(), 1u);
  EXPECT_EQ(inter[0], y->getId());
}

TEST(Membership, Union) {
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

  auto uni = w.unite(s1->getId(), s2->getId());
  EXPECT_EQ(uni.size(), 3u);
}

TEST(Membership, Difference) {
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

  auto diff = w.difference(s1->getId(), s2->getId());
  ASSERT_EQ(diff.size(), 1u);
  EXPECT_EQ(diff[0], x->getId());
}

// ---------------------------------------------------------------------------
// R-O8/R-M1: Types are sets
// ---------------------------------------------------------------------------

TEST(Membership, TypesAreSets) {
  World w;
  auto* e = w.createEntity();
  auto* s = w.createSet();
  auto* r = w.createSet(SysType::REL);

  EXPECT_TRUE(w.isMember(e->getId(), w.typeSetId(SysType::ENTITY)));
  EXPECT_TRUE(w.isMember(s->getId(), w.typeSetId(SysType::SET)));
  EXPECT_TRUE(w.isMember(r->getId(), w.typeSetId(SysType::REL)));
  EXPECT_TRUE(w.isMember(r->getId(), w.typeSetId(SysType::SET)));
}

// ---------------------------------------------------------------------------
// R-M2: Ternary membership matrix
// ---------------------------------------------------------------------------

TEST(Membership, TernaryMatrix) {
  World w;
  auto* s = w.createSet();
  auto* a = w.createEntity();
  auto* b = w.createEntity();
  auto* c = w.createEntity();

  w.addMember(a->getId(), s->getId());
  w.setNonMember(b->getId(), s->getId());

  // a: member
  EXPECT_TRUE(w.isMember(a->getId(), s->getId()));
  EXPECT_FALSE(w.isNonMember(a->getId(), s->getId()));
  EXPECT_FALSE(w.isUnknown(a->getId(), s->getId()));

  // b: known non-member
  EXPECT_FALSE(w.isMember(b->getId(), s->getId()));
  EXPECT_TRUE(w.isNonMember(b->getId(), s->getId()));
  EXPECT_FALSE(w.isUnknown(b->getId(), s->getId()));

  // c: unknown
  EXPECT_FALSE(w.isMember(c->getId(), s->getId()));
  EXPECT_FALSE(w.isNonMember(c->getId(), s->getId()));
  EXPECT_TRUE(w.isUnknown(c->getId(), s->getId()));
}
