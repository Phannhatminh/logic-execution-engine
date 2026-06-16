#include "core/object.hpp"

namespace logic::core {

std::atomic<std::size_t> Object::nextId_{0};

Object::Object() : id_(ObjectId{nextId_++}) {}

Object::Object(ObjectId id) : id_(id) {}

ObjectId Object::getId() const { return id_; }

ObjectType Object::getType() const { return ObjectType::Object; }

bool Object::equals(const Object& other) const { return this == &other; }

std::size_t Object::hashCode() const { return id_.value; }

std::unique_ptr<Object> Object::clone() const {
  return std::make_unique<Object>(*this);
}

std::string Object::toString() const {
  static const char* typeNames[] = {"Object", "Entity", "Tuple"};
  return std::string(typeNames[static_cast<int>(getType())]) + "@" +
         std::to_string(hashCode());
}

}
