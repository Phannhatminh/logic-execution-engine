#pragma once
#include <cstddef>
#include <atomic>
#include <unordered_map>
#include <vector>
#include "core/ids.hpp"

namespace logic::storage {

struct ObligationRow {
  core::ObligationId id;
  core::ObjectId target_id;
  core::AstNodeId antecedent_root_id;
  core::AstNodeId consequent_root_id;
};

class ObligationTable {
public:
  core::ObligationId insert(core::ObjectId target_id, core::AstNodeId antecedent_root_id, core::AstNodeId consequent_root_id);
  void remove(core::ObligationId id);

  ObligationRow* find(core::ObligationId id);
  const ObligationRow* find(core::ObligationId id) const;

  std::vector<const ObligationRow*> findByTarget(core::ObjectId target_id) const;

  std::size_t size() const;

private:
  static std::atomic<std::size_t> nextId_;
  std::unordered_map<core::ObligationId, ObligationRow> rows_;
};

}
