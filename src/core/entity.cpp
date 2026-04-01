#include "core/entity.hpp"

namespace logic::core {

Entity* Entity::identity() { return this; }

ObjectType Entity::getType() const { return ObjectType::Entity; }

std::unique_ptr<Object> Entity::clone() const {
  return std::make_unique<Entity>(*this);
}

}
