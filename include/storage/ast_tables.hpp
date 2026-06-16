#pragma once
#include <cstddef>
#include <atomic>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
#include "core/ids.hpp"
#include "core/types.hpp"

namespace logic::storage {

struct AstNodeRow {
  core::AstNodeId node_id;
  core::AstNodeType node_type;
  std::string value;
};

struct AstChildRow {
  core::AstNodeId parent_node_id;
  std::size_t position;
  core::AstNodeId child_node_id;
};

class AstNodeTable {
public:
  core::AstNodeId insert(core::AstNodeType node_type, const std::string& value = "");
  void remove(core::AstNodeId node_id);

  AstNodeRow* find(core::AstNodeId node_id);
  const AstNodeRow* find(core::AstNodeId node_id) const;

  std::size_t size() const;

private:
  static std::atomic<std::size_t> nextNodeId_;
  std::unordered_map<core::AstNodeId, AstNodeRow> rows_;
};

class AstChildTable {
public:
  void insert(core::AstNodeId parent_node_id, std::size_t position, core::AstNodeId child_node_id);
  void removeByParent(core::AstNodeId parent_node_id);

  std::vector<AstChildRow> childrenOf(core::AstNodeId parent_node_id) const;

  std::size_t size() const;

private:
  // parent_node_id → sorted map of position → child_node_id
  std::map<core::AstNodeId, std::map<std::size_t, core::AstNodeId>> rows_;
};

}
