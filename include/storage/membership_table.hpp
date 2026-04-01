#pragma once
#include <cstddef>
#include <unordered_set>
#include <unordered_map>
#include <vector>

namespace logic::storage {

struct MembershipRow {
  std::size_t object_id;
  std::size_t set_id;
};

class MembershipTable {
public:
  bool insert(std::size_t object_id, std::size_t set_id);
  bool remove(std::size_t object_id, std::size_t set_id);
  bool contains(std::size_t object_id, std::size_t set_id) const;

  std::unordered_set<std::size_t> setsOf(std::size_t object_id) const;
  std::unordered_set<std::size_t> membersOf(std::size_t set_id) const;

  std::vector<MembershipRow> allRows() const;
  std::size_t size() const;

private:
  std::unordered_map<std::size_t, std::unordered_set<std::size_t>> objectToSets_;
  std::unordered_map<std::size_t, std::unordered_set<std::size_t>> setToObjects_;
};

}
