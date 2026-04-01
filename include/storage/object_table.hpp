#pragma once
#include <cstddef>
#include <unordered_map>
#include <vector>
#include "core/types.hpp"
#include "core/object.hpp"

namespace logic::storage {

struct ObjectRow {
  std::size_t id;
  core::ObjectType core_type;
  core::SysType sys_type;
  std::size_t hash;
  core::Object* pointer;
};

class ObjectTable {
public:
  void insert(core::Object* obj, core::SysType sys_type);
  void remove(std::size_t id);

  ObjectRow* find(std::size_t id);
  const ObjectRow* find(std::size_t id) const;

  std::vector<ObjectRow*> findBySysType(core::SysType sys_type);
  std::vector<const ObjectRow*> findBySysType(core::SysType sys_type) const;

  std::size_t size() const;

private:
  std::unordered_map<std::size_t, ObjectRow> rows_;
};

}
