#pragma once
#include <vector>
#include "core/object.hpp"

namespace logic::core {

class Tuple : public Object {
public:
  explicit Tuple(std::vector<Object*> elements);

  std::size_t arity() const;
  Object* at(std::size_t position) const;

  ObjectType getType() const override;
  std::unique_ptr<Object> clone() const override;
  std::string toString() const override;

private:
  std::vector<Object*> elements_;
};

}
