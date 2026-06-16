#include "engine/logic_engine.hpp"

namespace logic::engine {

World::World() {
  bootstrap();
}

// 4.2 — Bootstrap type sets
void World::bootstrap() {
  // Create the five built-in type sets as objects in T.
  // Each is an Object with sys_type SET (they are sets themselves).
  for (int i = 0; i < 5; ++i) {
    auto obj = std::make_unique<core::Object>();
    core::ObjectId id = obj->getId();
    typeSetIds_[i] = id;
    objectTable_.insert(obj.get(), core::SysType::SET);
    objects_.push_back(std::move(obj));
  }

  // REL ⊆ SET: add REL type-set as member of SET type-set
  addMember(typeSetIds_[static_cast<int>(core::SysType::REL)],
            typeSetIds_[static_cast<int>(core::SysType::SET)]);

  // MAP ⊆ REL: add MAP type-set as member of REL type-set
  addMember(typeSetIds_[static_cast<int>(core::SysType::MAP)],
            typeSetIds_[static_cast<int>(core::SysType::REL)]);
}

core::ObjectId World::typeSetId(core::SysType sys_type) const {
  return typeSetIds_[static_cast<int>(sys_type)];
}

// Object creation

core::Entity* World::createEntity() {
  auto entity = std::make_unique<core::Entity>();
  core::Entity* ptr = entity.get();
  objectTable_.insert(ptr, core::SysType::ENTITY);
  // Add to the ENTITY type set
  addMember(ptr->getId(), typeSetIds_[static_cast<int>(core::SysType::ENTITY)]);
  objects_.push_back(std::move(entity));
  return ptr;
}

core::Object* World::createSet(core::SysType sys_type) {
  auto obj = std::make_unique<core::Object>();
  core::Object* ptr = obj.get();
  objectTable_.insert(ptr, sys_type);
  // Add to the appropriate type set
  addMember(ptr->getId(), typeSetIds_[static_cast<int>(sys_type)]);
  // Enforce type hierarchy: REL ⊆ SET, MAP ⊆ REL ⊆ SET
  if (sys_type == core::SysType::MAP) {
    addMember(ptr->getId(), typeSetIds_[static_cast<int>(core::SysType::REL)]);
    addMember(ptr->getId(), typeSetIds_[static_cast<int>(core::SysType::SET)]);
  } else if (sys_type == core::SysType::REL) {
    addMember(ptr->getId(), typeSetIds_[static_cast<int>(core::SysType::SET)]);
  }
  objects_.push_back(std::move(obj));
  return ptr;
}

core::Tuple* World::createTuple(std::vector<core::Object*> elements) {
  auto tuple = std::make_unique<core::Tuple>(elements);
  core::Tuple* ptr = tuple.get();
  objectTable_.insert(ptr, core::SysType::TUPLE);
  // Add to the TUPLE type set
  addMember(ptr->getId(), typeSetIds_[static_cast<int>(core::SysType::TUPLE)]);
  // Populate tuple element table
  for (std::size_t i = 0; i < elements.size(); ++i) {
    tupleElementTable_.insert(ptr->getId(), i, elements[i]->getId());
  }
  objects_.push_back(std::move(tuple));
  return ptr;
}

// Membership

bool World::addMember(core::ObjectId object_id, core::ObjectId set_id) {
  bool inserted = membershipTable_.insert(object_id, set_id);
  if (inserted) {
    membershipMatrix_.set(object_id, set_id, storage::MembershipState::MEMBER);
  }
  return inserted;
}

bool World::setNonMember(core::ObjectId object_id, core::ObjectId set_id) {
  membershipMatrix_.set(object_id, set_id, storage::MembershipState::NON_MEMBER);
  // Remove from membership table if it was previously a member
  membershipTable_.remove(object_id, set_id);
  return true;
}

bool World::removeMember(core::ObjectId object_id, core::ObjectId set_id) {
  bool removed = membershipTable_.remove(object_id, set_id);
  if (removed) {
    membershipMatrix_.set(object_id, set_id, storage::MembershipState::UNKNOWN);
  }
  return removed;
}

storage::MembershipState World::membershipState(core::ObjectId object_id, core::ObjectId set_id) const {
  return membershipMatrix_.query(object_id, set_id);
}

bool World::isMember(core::ObjectId object_id, core::ObjectId set_id) const {
  return membershipState(object_id, set_id) == storage::MembershipState::MEMBER;
}

bool World::isNonMember(core::ObjectId object_id, core::ObjectId set_id) const {
  return membershipState(object_id, set_id) == storage::MembershipState::NON_MEMBER;
}

bool World::isUnknown(core::ObjectId object_id, core::ObjectId set_id) const {
  return membershipState(object_id, set_id) == storage::MembershipState::UNKNOWN;
}

std::vector<core::ObjectId> World::membersOf(core::ObjectId set_id) const {
  return membershipMatrix_.column(set_id);
}

std::vector<core::ObjectId> World::setsOf(core::ObjectId object_id) const {
  return membershipMatrix_.row(object_id);
}

std::vector<core::ObjectId> World::intersect(core::ObjectId set_a, core::ObjectId set_b) const {
  return membershipMatrix_.intersect(set_a, set_b);
}

std::vector<core::ObjectId> World::unite(core::ObjectId set_a, core::ObjectId set_b) const {
  return membershipMatrix_.unite(set_a, set_b);
}

std::vector<core::ObjectId> World::difference(core::ObjectId set_a, core::ObjectId set_b) const {
  return membershipMatrix_.difference(set_a, set_b);
}

// Obligations

core::ObligationId World::addObligation(core::ObjectId target_id, core::AstNodeId antecedent_root_id, core::AstNodeId consequent_root_id) {
  return obligationTable_.insert(target_id, antecedent_root_id, consequent_root_id);
}

// AST construction

core::AstNodeId World::createAstNode(core::AstNodeType node_type, const std::string& value) {
  return astNodeTable_.insert(node_type, value);
}

void World::addAstChild(core::AstNodeId parent_node_id, std::size_t position, core::AstNodeId child_node_id) {
  astChildTable_.insert(parent_node_id, position, child_node_id);
}

// Lookup

core::Object* World::findObject(core::ObjectId id) {
  auto* row = objectTable_.find(id);
  return row ? row->pointer : nullptr;
}

const core::Object* World::findObject(core::ObjectId id) const {
  const auto* row = objectTable_.find(id);
  return row ? row->pointer : nullptr;
}

// Table access

const storage::ObjectTable& World::objectTable() const { return objectTable_; }
const storage::MembershipTable& World::membershipTable() const { return membershipTable_; }
const storage::MembershipMatrix& World::membershipMatrix() const { return membershipMatrix_; }
const storage::TupleElementTable& World::tupleElementTable() const { return tupleElementTable_; }
const storage::AstNodeTable& World::astNodeTable() const { return astNodeTable_; }
const storage::AstChildTable& World::astChildTable() const { return astChildTable_; }
const storage::ObligationTable& World::obligationTable() const { return obligationTable_; }

}
