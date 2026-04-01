#include "engine/logic_engine.hpp"

namespace logic::engine {

World::World() {
  bootstrap();
}

// 4.2 — Bootstrap type sets
void World::bootstrap() {
  // Create the five built-in type sets as objects in T.
  // Each is an Object with sys_type SET (they are sets themselves).
  core::SysType types[] = {
    core::SysType::SET,
    core::SysType::REL,
    core::SysType::MAP,
    core::SysType::ENTITY,
    core::SysType::TUPLE
  };

  for (int i = 0; i < 5; ++i) {
    auto obj = std::make_unique<core::Object>();
    std::size_t id = obj->getId();
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

std::size_t World::typeSetId(core::SysType sys_type) const {
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

bool World::addMember(std::size_t object_id, std::size_t set_id) {
  bool inserted = membershipTable_.insert(object_id, set_id);
  if (inserted) {
    membershipMatrix_.set(object_id, set_id);
  }
  return inserted;
}

bool World::setNonMember(std::size_t object_id, std::size_t set_id) {
  membershipMatrix_.setNonMember(object_id, set_id);
  // Remove from membership table if it was previously a member
  membershipTable_.remove(object_id, set_id);
  return true;
}

bool World::removeMember(std::size_t object_id, std::size_t set_id) {
  bool removed = membershipTable_.remove(object_id, set_id);
  if (removed) {
    membershipMatrix_.clear(object_id, set_id);
  }
  return removed;
}

bool World::isMember(std::size_t object_id, std::size_t set_id) const {
  return membershipMatrix_.test(object_id, set_id);
}

bool World::isNonMember(std::size_t object_id, std::size_t set_id) const {
  return membershipMatrix_.testNonMember(object_id, set_id);
}

bool World::isUnknown(std::size_t object_id, std::size_t set_id) const {
  return membershipMatrix_.query(object_id, set_id) == storage::MembershipState::UNKNOWN;
}

std::vector<std::size_t> World::membersOf(std::size_t set_id) const {
  return membershipMatrix_.column(set_id);
}

std::vector<std::size_t> World::setsOf(std::size_t object_id) const {
  return membershipMatrix_.row(object_id);
}

std::vector<std::size_t> World::intersect(std::size_t set_a, std::size_t set_b) const {
  return membershipMatrix_.intersect(set_a, set_b);
}

std::vector<std::size_t> World::unite(std::size_t set_a, std::size_t set_b) const {
  return membershipMatrix_.unite(set_a, set_b);
}

std::vector<std::size_t> World::difference(std::size_t set_a, std::size_t set_b) const {
  return membershipMatrix_.difference(set_a, set_b);
}

// Obligations

std::size_t World::addObligation(std::size_t target_id, std::size_t antecedent_root_id, std::size_t consequent_root_id) {
  return obligationTable_.insert(target_id, antecedent_root_id, consequent_root_id);
}

// AST construction

std::size_t World::createAstNode(core::AstNodeType node_type, const std::string& value) {
  return astNodeTable_.insert(node_type, value);
}

void World::addAstChild(std::size_t parent_node_id, std::size_t position, std::size_t child_node_id) {
  astChildTable_.insert(parent_node_id, position, child_node_id);
}

// Lookup

core::Object* World::findObject(std::size_t id) {
  auto* row = objectTable_.find(id);
  return row ? row->pointer : nullptr;
}

const core::Object* World::findObject(std::size_t id) const {
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
