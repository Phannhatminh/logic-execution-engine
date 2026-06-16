#pragma once
#include <cstddef>
#include <functional>

namespace logic::core {

struct ObjectId {
  std::size_t value;
  bool operator==(const ObjectId& o) const { return value == o.value; }
  bool operator!=(const ObjectId& o) const { return value != o.value; }
  bool operator<(const ObjectId& o)  const { return value <  o.value; }
};

struct AstNodeId {
  std::size_t value;
  bool operator==(const AstNodeId& o) const { return value == o.value; }
  bool operator!=(const AstNodeId& o) const { return value != o.value; }
  bool operator<(const AstNodeId& o)  const { return value <  o.value; }
};

struct ObligationId {
  std::size_t value;
  bool operator==(const ObligationId& o) const { return value == o.value; }
  bool operator!=(const ObligationId& o) const { return value != o.value; }
  bool operator<(const ObligationId& o)  const { return value <  o.value; }
};

} // namespace logic::core

namespace std {
template<> struct hash<logic::core::ObjectId> {
  std::size_t operator()(const logic::core::ObjectId& id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
  }
};
template<> struct hash<logic::core::AstNodeId> {
  std::size_t operator()(const logic::core::AstNodeId& id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
  }
};
template<> struct hash<logic::core::ObligationId> {
  std::size_t operator()(const logic::core::ObligationId& id) const noexcept {
    return std::hash<std::size_t>{}(id.value);
  }
};
} // namespace std
