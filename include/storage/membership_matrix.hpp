#pragma once
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace logic::storage {

class MembershipTable;

// Ternary membership state
enum class MembershipState {
  UNKNOWN,      // default — no information
  MEMBER,       // known to be a member (1)
  NON_MEMBER    // known to not be a member (0)
};

class MembershipMatrix {
public:
  // Set M[object_id][set_id] = MEMBER (1)
  void set(std::size_t object_id, std::size_t set_id);

  // Set M[object_id][set_id] = NON_MEMBER (0)
  void setNonMember(std::size_t object_id, std::size_t set_id);

  // Set M[object_id][set_id] = UNKNOWN (clear both bits)
  void clear(std::size_t object_id, std::size_t set_id);

  // Test if M[object_id][set_id] == MEMBER
  bool test(std::size_t object_id, std::size_t set_id) const;

  // Test if M[object_id][set_id] == NON_MEMBER
  bool testNonMember(std::size_t object_id, std::size_t set_id) const;

  // Query the full ternary state
  MembershipState query(std::size_t object_id, std::size_t set_id) const;

  // Row operations: get the sets this object is a MEMBER of
  std::vector<std::size_t> row(std::size_t object_id) const;

  // Column operations: get the objects that are MEMBERs of this set
  std::vector<std::size_t> column(std::size_t set_id) const;

  // Bitwise operations over columns (set IDs) — operate on MEMBER bits only
  std::vector<std::size_t> intersect(std::size_t set_a, std::size_t set_b) const;
  std::vector<std::size_t> unite(std::size_t set_a, std::size_t set_b) const;
  std::vector<std::size_t> difference(std::size_t set_a, std::size_t set_b) const;

  // Load all entries from a MembershipTable (as MEMBER facts)
  void loadFrom(const MembershipTable& table);

  void reset();

private:
  // Two-bitset representation for ternary state:
  //   known bit = 1 means we have information about this cell
  //   value bit = 1 means the cell is MEMBER (only meaningful when known=1)
  //   known=0 → UNKNOWN, known=1 & value=1 → MEMBER, known=1 & value=0 → NON_MEMBER

  // Row view: object_id → bitsets over sets
  std::unordered_map<std::size_t, std::vector<uint64_t>> objectKnown_;  // known bits
  std::unordered_map<std::size_t, std::vector<uint64_t>> objectValue_;  // value bits

  // Column view: set_id → bitsets over objects
  std::unordered_map<std::size_t, std::vector<uint64_t>> setKnown_;
  std::unordered_map<std::size_t, std::vector<uint64_t>> setValue_;

  // ID ↔ bit position mappings
  std::unordered_map<std::size_t, std::size_t> objectIndex_;
  std::unordered_map<std::size_t, std::size_t> setIndex_;
  std::vector<std::size_t> objectIds_;
  std::vector<std::size_t> setIds_;

  std::size_t objectBitPos(std::size_t object_id);
  std::size_t setBitPos(std::size_t set_id);

  static void setBit(std::vector<uint64_t>& bits, std::size_t pos);
  static void clearBit(std::vector<uint64_t>& bits, std::size_t pos);
  static bool testBit(const std::vector<uint64_t>& bits, std::size_t pos);
  static void ensureSize(std::vector<uint64_t>& bits, std::size_t pos);
};

}
