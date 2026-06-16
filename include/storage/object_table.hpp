#pragma once
#include <cstddef>
#include <unordered_map>
#include <vector>
#include "core/ids.hpp"
#include "core/types.hpp"
#include "core/object.hpp"

namespace logic::storage {

struct ObjectRow {
  core::ObjectId id;
  core::ObjectType core_type;
  core::SysType sys_type;
  std::size_t hash;
  core::Object* pointer;
};

class ObjectTable {
public:
  void insert(core::Object* obj, core::SysType sys_type);
  void remove(core::ObjectId id);

  ObjectRow* find(core::ObjectId id);
  const ObjectRow* find(core::ObjectId id) const;

  std::vector<ObjectRow*> findBySysType(core::SysType sys_type);
  std::vector<const ObjectRow*> findBySysType(core::SysType sys_type) const;

  std::size_t size() const;

private:
  std::unordered_map<core::ObjectId, ObjectRow> rows_;
};

}
