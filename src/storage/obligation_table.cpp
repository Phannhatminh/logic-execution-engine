#include "storage/obligation_table.hpp"

namespace logic::storage {

std::atomic<std::size_t> ObligationTable::nextId_{0};

std::size_t ObligationTable::insert(std::size_t target_id, std::size_t antecedent_root_id, std::size_t consequent_root_id) {
  std::size_t id = nextId_++;
  rows_.emplace(id, ObligationRow{id, target_id, antecedent_root_id, consequent_root_id});
  return id;
}

void ObligationTable::remove(std::size_t id) {
  rows_.erase(id);
}

ObligationRow* ObligationTable::find(std::size_t id) {
  auto it = rows_.find(id);
  return it != rows_.end() ? &it->second : nullptr;
}

const ObligationRow* ObligationTable::find(std::size_t id) const {
  auto it = rows_.find(id);
  return it != rows_.end() ? &it->second : nullptr;
}

std::vector<const ObligationRow*> ObligationTable::findByTarget(std::size_t target_id) const {
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
