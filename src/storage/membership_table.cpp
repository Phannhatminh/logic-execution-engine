#include "storage/membership_table.hpp"

namespace logic::storage {

bool MembershipTable::insert(core::ObjectId object_id, core::ObjectId set_id) {
  bool inserted = objectToSets_[object_id].insert(set_id).second;
  if (inserted) {
    setToObjects_[set_id].insert(object_id);
  }
  return inserted;
}

bool MembershipTable::remove(core::ObjectId object_id, core::ObjectId set_id) {
  auto objIt = objectToSets_.find(object_id);
  if (objIt == objectToSets_.end()) return false;

  bool removed = objIt->second.erase(set_id) > 0;
  if (removed) {
    auto setIt = setToObjects_.find(set_id);
    if (setIt != setToObjects_.end()) {
      setIt->second.erase(object_id);
      if (setIt->second.empty()) setToObjects_.erase(setIt);
    }
    if (objIt->second.empty()) objectToSets_.erase(objIt);
  }
  return removed;
}

bool MembershipTable::contains(core::ObjectId object_id, core::ObjectId set_id) const {
  auto it = objectToSets_.find(object_id);
  if (it == objectToSets_.end()) return false;
  return it->second.count(set_id) > 0;
}

std::unordered_set<core::ObjectId> MembershipTable::setsOf(core::ObjectId object_id) const {
  auto it = objectToSets_.find(object_id);
  if (it == objectToSets_.end()) return {};
  return it->second;
}

std::unordered_set<core::ObjectId> MembershipTable::membersOf(core::ObjectId set_id) const {
  auto it = setToObjects_.find(set_id);
  if (it == setToObjects_.end()) return {};
  return it->second;
}

std::vector<MembershipRow> MembershipTable::allRows() const {
  std::vector<MembershipRow> result;
  for (const auto& [obj_id, sets] : objectToSets_) {
    for (const core::ObjectId& set_id : sets) {
      result.push_back({obj_id, set_id});
    }
  }
  return result;
}

std::size_t MembershipTable::size() const {
  std::size_t count = 0;
  for (const auto& [obj_id, sets] : objectToSets_) {
    count += sets.size();
  }
  return count;
}

}
