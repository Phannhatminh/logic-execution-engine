#pragma once
#include <cstddef>
#include <atomic>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/types.hpp"

namespace logic::storage {

struct AstNodeRow {
  std::size_t node_id;
  core::AstNodeType node_type;
  std::string value;
};

struct AstChildRow {
  std::size_t parent_node_id;
  std::size_t position;
  std::size_t child_node_id;
};

class AstNodeTable {
public:
  std::size_t insert(core::AstNodeType node_type, const std::string& value = "");
  void remove(std::size_t node_id);

  AstNodeRow* find(std::size_t node_id);
  const AstNodeRow* find(std::size_t node_id) const;

  std::size_t size() const;

private:
  static std::atomic<std::size_t> nextNodeId_;
  std::unordered_map<std::size_t, AstNodeRow> rows_;
};

class AstChildTable {
public:
  void insert(std::size_t parent_node_id, std::size_t position, std::size_t child_node_id);
  void removeByParent(std::size_t parent_node_id);

  std::vector<AstChildRow> childrenOf(std::size_t parent_node_id) const;

  std::size_t size() const;

private:
  // parent_node_id → sorted map of position → child_node_id
  std::map<std::size_t, std::map<std::size_t, std::size_t>> rows_;
};

}
