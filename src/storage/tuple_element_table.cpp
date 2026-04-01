#include "storage/tuple_element_table.hpp"

namespace logic::storage {

void TupleElementTable::insert(std::size_t tuple_id, std::size_t position, std::size_t element_id) {
  rows_[tuple_id][position] = element_id;
}

void TupleElementTable::removeByTuple(std::size_t tuple_id) {
  rows_.erase(tuple_id);
}

std::vector<TupleElementRow> TupleElementTable::elementsOf(std::size_t tuple_id) const {
  std::vector<TupleElementRow> result;
  auto it = rows_.find(tuple_id);
  if (it == rows_.end()) return result;

  for (const auto& [pos, elem_id] : it->second) {
    result.push_back({tuple_id, pos, elem_id});
  }
  return result;
}

std::vector<TupleElementRow> TupleElementTable::allRows() const {
  std::vector<TupleElementRow> result;
  for (const auto& [tuple_id, positions] : rows_) {
    for (const auto& [pos, elem_id] : positions) {
      result.push_back({tuple_id, pos, elem_id});
    }
  }
  return result;
}

std::size_t TupleElementTable::size() const {
  std::size_t count = 0;
  for (const auto& [tuple_id, positions] : rows_) {
    count += positions.size();
  }
  return count;
}

}
