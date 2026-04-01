#include "core/tuple.hpp"
#include <stdexcept>

namespace logic::core {

Tuple::Tuple(std::vector<Object*> elements) : elements_(std::move(elements)) {}

std::size_t Tuple::arity() const { return elements_.size(); }

Object* Tuple::at(std::size_t position) const {
  if (position >= elements_.size()) {
    throw std::out_of_range("Tuple index out of range");
  }
  return elements_[position];
}

ObjectType Tuple::getType() const { return ObjectType::Tuple; }

std::unique_ptr<Object> Tuple::clone() const {
  return std::make_unique<Tuple>(*this);
}

std::string Tuple::toString() const {
  std::string result = "(";
  for (std::size_t i = 0; i < elements_.size(); ++i) {
    if (i > 0) result += ", ";
    result += elements_[i]->toString();
  }
  result += ")";
  return result;
}

}
