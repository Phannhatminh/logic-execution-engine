#pragma once
#include <cstddef>
#include <atomic>
#include <unordered_map>
#include <vector>

namespace logic::storage {

struct ObligationRow {
  std::size_t id;
  std::size_t target_id;
  std::size_t antecedent_root_id;
  std::size_t consequent_root_id;
};

class ObligationTable {
public:
  std::size_t insert(std::size_t target_id, std::size_t antecedent_root_id, std::size_t consequent_root_id);
  void remove(std::size_t id);

  ObligationRow* find(std::size_t id);
  const ObligationRow* find(std::size_t id) const;

  std::vector<const ObligationRow*> findByTarget(std::size_t target_id) const;

  std::size_t size() const;

private:
  static std::atomic<std::size_t> nextId_;
  std::unordered_map<std::size_t, ObligationRow> rows_;
};

}
