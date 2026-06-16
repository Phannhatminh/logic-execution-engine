#pragma once
#include <cstddef>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "core/ids.hpp"

namespace logic::storage {

struct MembershipRow {
  core::ObjectId object_id;
  core::ObjectId set_id;
};

class MembershipTable {
public:
  bool insert(core::ObjectId object_id, core::ObjectId set_id);
  bool remove(core::ObjectId object_id, core::ObjectId set_id);
  bool contains(core::ObjectId object_id, core::ObjectId set_id) const;

  std::unordered_set<core::ObjectId> setsOf(core::ObjectId object_id) const;
  std::unordered_set<core::ObjectId> membersOf(core::ObjectId set_id) const;

  std::vector<MembershipRow> allRows() const;
  std::size_t size() const;

private:
  std::unordered_map<core::ObjectId, std::unordered_set<core::ObjectId>> objectToSets_;
  std::unordered_map<core::ObjectId, std::unordered_set<core::ObjectId>> setToObjects_;
};

}
