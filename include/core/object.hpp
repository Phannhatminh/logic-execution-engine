#pragma once
#include <atomic>
#include <cstddef>
#include <memory>
#include <string>
#include "core/ids.hpp"
#include "core/types.hpp"

namespace logic::core {

class Object {
public:
  Object();
  virtual ~Object() = default;

  ObjectId getId() const;

  virtual ObjectType getType() const;
  virtual bool equals(const Object& other) const;
  virtual std::size_t hashCode() const;
  virtual std::unique_ptr<Object> clone() const;
  virtual std::string toString() const;

protected:
  Object(ObjectId id);

private:
  static std::atomic<std::size_t> nextId_;
  ObjectId id_;
};
};
