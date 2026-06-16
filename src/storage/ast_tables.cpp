#include "storage/ast_tables.hpp"

namespace logic::storage {

// AstNodeTable

std::atomic<std::size_t> AstNodeTable::nextNodeId_{0};

core::AstNodeId AstNodeTable::insert(core::AstNodeType node_type, const std::string& value) {
  core::AstNodeId id{nextNodeId_++};
  rows_.emplace(id, AstNodeRow{id, node_type, value});
  return id;
}

void AstNodeTable::remove(core::AstNodeId node_id) {
  rows_.erase(node_id);
}

AstNodeRow* AstNodeTable::find(core::AstNodeId node_id) {
  auto it = rows_.find(node_id);
  return it != rows_.end() ? &it->second : nullptr;
}

const AstNodeRow* AstNodeTable::find(core::AstNodeId node_id) const {
  auto it = rows_.find(node_id);
  return it != rows_.end() ? &it->second : nullptr;
}

std::size_t AstNodeTable::size() const { return rows_.size(); }

// AstChildTable

void AstChildTable::insert(core::AstNodeId parent_node_id, std::size_t position, core::AstNodeId child_node_id) {
  rows_[parent_node_id][position] = child_node_id;
}

void AstChildTable::removeByParent(core::AstNodeId parent_node_id) {
  rows_.erase(parent_node_id);
}

std::vector<AstChildRow> AstChildTable::childrenOf(core::AstNodeId parent_node_id) const {
  std::vector<AstChildRow> result;
  auto it = rows_.find(parent_node_id);
  if (it == rows_.end()) return result;

  for (const auto& [pos, child_id] : it->second) {
    result.push_back({parent_node_id, pos, child_id});
  }
  return result;
}

std::size_t AstChildTable::size() const {
  std::size_t count = 0;
  for (const auto& [parent_id, positions] : rows_) {
    count += positions.size();
  }
  return count;
}

}
