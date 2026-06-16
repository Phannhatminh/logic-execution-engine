#include "storage/obligation_table.hpp"

namespace logic::storage {

std::atomic<std::size_t> ObligationTable::nextId_{0};

core::ObligationId ObligationTable::insert(core::ObjectId target_id, core::AstNodeId antecedent_root_id, core::AstNodeId consequent_root_id) {
  core::ObligationId id{nextId_++};
  rows_.emplace(id, ObligationRow{id, target_id, antecedent_root_id, consequent_root_id});
  return id;
}

void ObligationTable::remove(core::ObligationId id) {
  rows_.erase(id);
}

ObligationRow* ObligationTable::find(core::ObligationId id) {
  auto it = rows_.find(id);
  return it != rows_.end() ? &it->second : nullptr;
}

const ObligationRow* ObligationTable::find(core::ObligationId id) const {
  auto it = rows_.find(id);
  return it != rows_.end() ? &it->second : nullptr;
}

std::vector<const ObligationRow*> ObligationTable::findByTarget(core::ObjectId target_id) const {
  std::vector<const ObligationRow*> result;
  for (const auto& [id, row] : rows_) {
    if (row.target_id == target_id) {
      result.push_back(&row);
    }
  }
  return result;
}

std::size_t ObligationTable::size() const { return rows_.size(); }

}
