#include "storage/ast_tables.hpp"

namespace logic::storage {

// AstNodeTable

std::atomic<std::size_t> AstNodeTable::nextNodeId_{0};

std::size_t AstNodeTable::insert(core::AstNodeType node_type, const std::string& value) {
  std::size_t id = nextNodeId_++;
  rows_.emplace(id, AstNodeRow{id, node_type, value});
  return id;
}

void AstNodeTable::remove(std::size_t node_id) {
  rows_.erase(node_id);
}

AstNodeRow* AstNodeTable::find(std::size_t node_id) {
  auto it = rows_.find(node_id);
  return it != rows_.end() ? &it->second : nullptr;
}

const AstNodeRow* AstNodeTable::find(std::size_t node_id) const {
  auto it = rows_.find(node_id);
  return it != rows_.end() ? &it->second : nullptr;
}

std::size_t AstNodeTable::size() const { return rows_.size(); }

// AstChildTable

void AstChildTable::insert(std::size_t parent_node_id, std::size_t position, std::size_t child_node_id) {
  rows_[parent_node_id][position] = child_node_id;
}

void AstChildTable::removeByParent(std::size_t parent_node_id) {
  rows_.erase(parent_node_id);
}

std::vector<AstChildRow> AstChildTable::childrenOf(std::size_t parent_node_id) const {
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
