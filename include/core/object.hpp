#pragma once
#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include "core/types.hpp"

namespace logic::core {

class Object {
public:
  Object();
  virtual ~Object() = default;

  std::size_t getId() const;

  virtual ObjectType getType() const;
  virtual bool equals(const Object& other) const;
  virtual std::size_t hashCode() const;
  virtual std::unique_ptr<Object> clone() const;
  virtual std::string toString() const;

protected:
  Object(std::size_t id);

private:
  static std::atomic<std::size_t> nextId_;
  std::size_t id_;
};
};