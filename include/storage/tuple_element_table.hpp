#pragma once
#include <cstddef>
#include <map>
#include <vector>

namespace logic::storage {

struct TupleElementRow {
  std::size_t tuple_id;
  std::size_t position;
  std::size_t element_id;
};

class TupleElementTable {
public:
  void insert(std::size_t tuple_id, std::size_t position, std::size_t element_id);
  void removeByTuple(std::size_t tuple_id);

  std::vector<TupleElementRow> elementsOf(std::size_t tuple_id) const;

  std::vector<TupleElementRow> allRows() const;
  std::size_t size() const;

private:
  // tuple_id → sorted map of position → element_id
  std::map<std::size_t, std::map<std::size_t, std::size_t>> rows_;
};

}
