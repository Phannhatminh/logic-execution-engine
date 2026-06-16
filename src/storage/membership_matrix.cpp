#include "storage/membership_matrix.hpp"
#include "storage/membership_table.hpp"
#include <cassert>

namespace logic::storage {

// Bit manipulation helpers

void MembershipMatrix::ensureSize(std::vector<uint64_t>& bits, std::size_t pos) {
  std::size_t word = pos / 64;
  if (bits.size() <= word) {
    bits.resize(word + 1, 0);
  }
}

void MembershipMatrix::setBit(std::vector<uint64_t>& bits, std::size_t pos) {
  ensureSize(bits, pos);
  bits[pos / 64] |= (uint64_t(1) << (pos % 64));
}

void MembershipMatrix::clearBit(std::vector<uint64_t>& bits, std::size_t pos) {
  std::size_t word = pos / 64;
  if (word < bits.size()) {
    bits[word] &= ~(uint64_t(1) << (pos % 64));
  }
}

bool MembershipMatrix::testBit(const std::vector<uint64_t>& bits, std::size_t pos) {
  std::size_t word = pos / 64;
  if (word >= bits.size()) return false;
  return (bits[word] & (uint64_t(1) << (pos % 64))) != 0;
}

// Index management

std::size_t MembershipMatrix::objectBitPos(core::ObjectId object_id) {
  auto it = objectIndex_.find(object_id);
  if (it != objectIndex_.end()) return it->second;

  std::size_t pos = objectIds_.size();
  objectIndex_[object_id] = pos;
  objectIds_.push_back(object_id);
  return pos;
}

std::size_t MembershipMatrix::setBitPos(core::ObjectId set_id) {
  auto it = setIndex_.find(set_id);
  if (it != setIndex_.end()) return it->second;

  std::size_t pos = setIds_.size();
  setIndex_[set_id] = pos;
  setIds_.push_back(set_id);
  return pos;
}

// Core operations

void MembershipMatrix::set(core::ObjectId object_id, core::ObjectId set_id, MembershipState state) {
  if (state == MembershipState::UNKNOWN) {
    auto setIt = setIndex_.find(set_id);
    auto objIt = objectIndex_.find(object_id);
    if (setIt == setIndex_.end() || objIt == objectIndex_.end()) return;
    clearBit(objectKnown_[object_id], setIt->second);
    clearBit(objectValue_[object_id], setIt->second);
    clearBit(setKnown_[set_id], objIt->second);
    clearBit(setValue_[set_id], objIt->second);
    return;
  }

  std::size_t obj_pos = objectBitPos(object_id);
  std::size_t set_pos = setBitPos(set_id);

  setBit(objectKnown_[object_id], set_pos);
  setBit(setKnown_[set_id], obj_pos);

  if (state == MembershipState::MEMBER) {
    setBit(objectValue_[object_id], set_pos);
    setBit(setValue_[set_id], obj_pos);
  } else {
    clearBit(objectValue_[object_id], set_pos);
    clearBit(setValue_[set_id], obj_pos);
  }
}

bool MembershipMatrix::test(core::ObjectId object_id, core::ObjectId set_id) const {
  return query(object_id, set_id) == MembershipState::MEMBER;
}

bool MembershipMatrix::testNonMember(core::ObjectId object_id, core::ObjectId set_id) const {
  return query(object_id, set_id) == MembershipState::NON_MEMBER;
}

MembershipState MembershipMatrix::query(core::ObjectId object_id, core::ObjectId set_id) const {
  auto setIt = setIndex_.find(set_id);
  if (setIt == setIndex_.end()) return MembershipState::UNKNOWN;

  auto knownIt = objectKnown_.find(object_id);
  if (knownIt == objectKnown_.end()) return MembershipState::UNKNOWN;

  if (!testBit(knownIt->second, setIt->second)) return MembershipState::UNKNOWN;

  assert(objectValue_.count(object_id) > 0);
  auto valIt = objectValue_.find(object_id);
  return testBit(valIt->second, setIt->second) ? MembershipState::MEMBER : MembershipState::NON_MEMBER;
}

// Row: which sets does this object belong to? (MEMBER only)
std::vector<core::ObjectId> MembershipMatrix::row(core::ObjectId object_id) const {
  std::vector<core::ObjectId> result;
  auto valIt = objectValue_.find(object_id);
  if (valIt == objectValue_.end()) return result;

  const auto& bits = valIt->second;
  for (std::size_t w = 0; w < bits.size(); ++w) {
    uint64_t word = bits[w];
    while (word) {
      std::size_t bit = __builtin_ctzll(word);
      std::size_t pos = w * 64 + bit;
      if (pos < setIds_.size()) {
        result.push_back(setIds_[pos]);
      }
      word &= word - 1;
    }
  }
  return result;
}

// Column: which objects are in this set? (MEMBER only)
std::vector<core::ObjectId> MembershipMatrix::column(core::ObjectId set_id) const {
  std::vector<core::ObjectId> result;
  auto valIt = setValue_.find(set_id);
  if (valIt == setValue_.end()) return result;

  const auto& bits = valIt->second;
  for (std::size_t w = 0; w < bits.size(); ++w) {
    uint64_t word = bits[w];
    while (word) {
      std::size_t bit = __builtin_ctzll(word);
      std::size_t pos = w * 64 + bit;
      if (pos < objectIds_.size()) {
        result.push_back(objectIds_[pos]);
      }
      word &= word - 1;
    }
  }
  return result;
}

// Bitwise: objects in BOTH set_a AND set_b (MEMBER bits)
std::vector<core::ObjectId> MembershipMatrix::intersect(core::ObjectId set_a, core::ObjectId set_b) const {
  std::vector<core::ObjectId> result;
  auto aIt = setValue_.find(set_a);
  auto bIt = setValue_.find(set_b);
  if (aIt == setValue_.end() || bIt == setValue_.end()) return result;

  const auto& a = aIt->second;
  const auto& b = bIt->second;
  std::size_t len = std::min(a.size(), b.size());

  for (std::size_t w = 0; w < len; ++w) {
    uint64_t word = a[w] & b[w];
    while (word) {
      std::size_t bit = __builtin_ctzll(word);
      std::size_t pos = w * 64 + bit;
      if (pos < objectIds_.size()) {
        result.push_back(objectIds_[pos]);
      }
      word &= word - 1;
    }
  }
  return result;
}

// Bitwise: objects in set_a OR set_b (MEMBER bits)
std::vector<core::ObjectId> MembershipMatrix::unite(core::ObjectId set_a, core::ObjectId set_b) const {
  std::vector<core::ObjectId> result;
  auto aIt = setValue_.find(set_a);
  auto bIt = setValue_.find(set_b);

  const std::vector<uint64_t>* a = aIt != setValue_.end() ? &aIt->second : nullptr;
  const std::vector<uint64_t>* b = bIt != setValue_.end() ? &bIt->second : nullptr;

  std::size_t len = 0;
  if (a) len = std::max(len, a->size());
  if (b) len = std::max(len, b->size());

  for (std::size_t w = 0; w < len; ++w) {
    uint64_t wa = (a && w < a->size()) ? (*a)[w] : 0;
    uint64_t wb = (b && w < b->size()) ? (*b)[w] : 0;
    uint64_t word = wa | wb;
    while (word) {
      std::size_t bit = __builtin_ctzll(word);
      std::size_t pos = w * 64 + bit;
      if (pos < objectIds_.size()) {
        result.push_back(objectIds_[pos]);
      }
      word &= word - 1;
    }
  }
  return result;
}

// Bitwise: objects in set_a but NOT set_b (MEMBER bits)
std::vector<core::ObjectId> MembershipMatrix::difference(core::ObjectId set_a, core::ObjectId set_b) const {
  std::vector<core::ObjectId> result;
  auto aIt = setValue_.find(set_a);
  if (aIt == setValue_.end()) return result;

  auto bIt = setValue_.find(set_b);
  const auto& a = aIt->second;

  for (std::size_t w = 0; w < a.size(); ++w) {
    uint64_t wb = (bIt != setValue_.end() && w < bIt->second.size()) ? bIt->second[w] : 0;
    uint64_t word = a[w] & ~wb;
    while (word) {
      std::size_t bit = __builtin_ctzll(word);
      std::size_t pos = w * 64 + bit;
      if (pos < objectIds_.size()) {
        result.push_back(objectIds_[pos]);
      }
      word &= word - 1;
    }
  }
  return result;
}

// Load from table

void MembershipMatrix::loadFrom(const MembershipTable& table) {
  reset();
  for (const auto& row : table.allRows()) {
    set(row.object_id, row.set_id, MembershipState::MEMBER);
  }
}

void MembershipMatrix::reset() {
  objectKnown_.clear();
  objectValue_.clear();
  setKnown_.clear();
  setValue_.clear();
  objectIndex_.clear();
  setIndex_.clear();
  objectIds_.clear();
  setIds_.clear();
}

}
