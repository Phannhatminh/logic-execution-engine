#pragma once
#include <cstddef>
#include <memory>
#include <vector>
#include "core/ids.hpp"
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
  bool addMember(core::ObjectId object_id, core::ObjectId set_id);
  bool setNonMember(core::ObjectId object_id, core::ObjectId set_id);
  bool removeMember(core::ObjectId object_id, core::ObjectId set_id);
  bool isMember(core::ObjectId object_id, core::ObjectId set_id) const;
  bool isNonMember(core::ObjectId object_id, core::ObjectId set_id) const;
  bool isUnknown(core::ObjectId object_id, core::ObjectId set_id) const;
  storage::MembershipState membershipState(core::ObjectId object_id, core::ObjectId set_id) const;

  // Membership matrix queries
  std::vector<core::ObjectId> membersOf(core::ObjectId set_id) const;
  std::vector<core::ObjectId> setsOf(core::ObjectId object_id) const;
  std::vector<core::ObjectId> intersect(core::ObjectId set_a, core::ObjectId set_b) const;
  std::vector<core::ObjectId> unite(core::ObjectId set_a, core::ObjectId set_b) const;
  std::vector<core::ObjectId> difference(core::ObjectId set_a, core::ObjectId set_b) const;

  // Obligations
  core::ObligationId addObligation(core::ObjectId target_id, core::AstNodeId antecedent_root_id, core::AstNodeId consequent_root_id);

  // AST construction
  core::AstNodeId createAstNode(core::AstNodeType node_type, const std::string& value = "");
  void addAstChild(core::AstNodeId parent_node_id, std::size_t position, core::AstNodeId child_node_id);

  // Lookup
  core::Object* findObject(core::ObjectId id);
  const core::Object* findObject(core::ObjectId id) const;

  // Bootstrap type set IDs
  core::ObjectId typeSetId(core::SysType sys_type) const;

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
  core::ObjectId typeSetIds_[5]; // indexed by SysType enum

  void bootstrap();
};

}
