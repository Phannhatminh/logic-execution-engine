#pragma once
#include "core/object.hpp"

namespace logic::core {

class Entity : public Object {
public:
  Entity* identity();

  ObjectType getType() const override;
  std::unique_ptr<Object> clone() const override;
};

}