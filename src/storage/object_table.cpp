#include "storage/object_table.hpp"

namespace logic::storage {

void ObjectTable::insert(core::Object* obj, core::SysType sys_type) {
  ObjectRow row{obj->getId(), obj->getType(), sys_type, obj->hashCode(), obj};
  rows_.emplace(row.id, row);
}

void ObjectTable::remove(std::size_t id) {
  rows_.erase(id);
}

ObjectRow* ObjectTable::find(std::size_t id) {
  auto it = rows_.find(id);
  return it != rows_.end() ? &it->second : nullptr;
}

const ObjectRow* ObjectTable::find(std::size_t id) const {
  auto it = rows_.find(id);
  return it != rows_.end() ? &it->second : nullptr;
}

std::vector<ObjectRow*> ObjectTable::findBySysType(core::SysType sys_type) {
  std::vector<ObjectRow*> result;
  for (auto& [id, row] : rows_) {
    if (row.sys_type == sys_type) {
      result.push_back(&row);
    }
  }
  return result;
}

std::vector<const ObjectRow*> ObjectTable::findBySysType(core::SysType sys_type) const {
  std::vector<const ObjectRow*> result;
  for (const auto& [id, row] : rows_) {
    if (row.sys_type == sys_type) {
      result.push_back(&row);
    }
  }
  return result;
}

std::size_t ObjectTable::size() const { return rows_.size(); }

}
