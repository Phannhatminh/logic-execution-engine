#pragma once
#include <cstddef>
#include <memory>
#include <vector>
#include "core/types.hpp"
#include "core/object.hpp"
#include "core/entity.hpp"
#include "core/tuple.hpp"
#include "storage/object_table.hpp"
#include "storage/membership_table.hpp"
#include "storage/membership_matrix.hpp"
#include "storage/tuple_element_table.hpp"
#include "storage/ast_tables.hpp"
#include "storage/obligation_table.hpp"

namespace logic::engine {

class World {
public:
  World();

  // Object creation
  core::Entity* createEntity();
  core::Object* createSet(core::SysType sys_type = core::SysType::SET);
  core::Tuple* createTuple(std::vector<core::Object*> elements);

  // Membership
  bool addMember(std::size_t object_id, std::size_t set_id);
  bool setNonMember(std::size_t object_id, std::size_t set_id);
  bool removeMember(std::size_t object_id, std::size_t set_id);
  bool isMember(std::size_t object_id, std::size_t set_id) const;
  bool isNonMember(std::size_t object_id, std::size_t set_id) const;
  bool isUnknown(std::size_t object_id, std::size_t set_id) const;

  // Membership matrix queries
  std::vector<std::size_t> membersOf(std::size_t set_id) const;
  std::vector<std::size_t> setsOf(std::size_t object_id) const;
  std::vector<std::size_t> intersect(std::size_t set_a, std::size_t set_b) const;
  std::vector<std::size_t> unite(std::size_t set_a, std::size_t set_b) const;
  std::vector<std::size_t> difference(std::size_t set_a, std::size_t set_b) const;

  // Obligations
  std::size_t addObligation(std::size_t target_id, std::size_t antecedent_root_id, std::size_t consequent_root_id);

  // AST construction
  std::size_t createAstNode(core::AstNodeType node_type, const std::string& value = "");
  void addAstChild(std::size_t parent_node_id, std::size_t position, std::size_t child_node_id);

  // Lookup
  core::Object* findObject(std::size_t id);
  const core::Object* findObject(std::size_t id) const;

  // Bootstrap type set IDs
  std::size_t typeSetId(core::SysType sys_type) const;

  // Table access
  const storage::ObjectTable& objectTable() const;
  const storage::MembershipTable& membershipTable() const;
  const storage::MembershipMatrix& membershipMatrix() const;
  const storage::TupleElementTable& tupleElementTable() const;
  const storage::AstNodeTable& astNodeTable() const;
  const storage::AstChildTable& astChildTable() const;
  const storage::ObligationTable& obligationTable() const;

private:
  // Object ownership
  std::vector<std::unique_ptr<core::Object>> objects_;

  // Storage tables
  storage::ObjectTable objectTable_;
  storage::MembershipTable membershipTable_;
  storage::MembershipMatrix membershipMatrix_;
  storage::TupleElementTable tupleElementTable_;
  storage::AstNodeTable astNodeTable_;
  storage::AstChildTable astChildTable_;
  storage::ObligationTable obligationTable_;

  // Bootstrap type set IDs
  std::size_t typeSetIds_[5]; // indexed by SysType enum

  void bootstrap();
};

}
