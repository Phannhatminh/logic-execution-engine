#pragma once
#include <cstddef>
#include <map>
#include <vector>
#include "core/ids.hpp"

namespace logic::storage {

struct TupleElementRow {
  core::ObjectId tuple_id;
  std::size_t position;
  core::ObjectId element_id;
};

class TupleElementTable {
public:
  void insert(core::ObjectId tuple_id, std::size_t position, core::ObjectId element_id);
  void removeByTuple(core::ObjectId tuple_id);

  std::vector<TupleElementRow> elementsOf(core::ObjectId tuple_id) const;

  std::vector<TupleElementRow> allRows() const;
  std::size_t size() const;

private:
  // tuple_id → sorted map of position → element_id
  std::map<core::ObjectId, std::map<std::size_t, core::ObjectId>> rows_;
};

}
