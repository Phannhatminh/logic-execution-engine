#pragma once

namespace logic::core {

enum class ObjectType {
  Object,
  Entity,
  Tuple
};

enum class SysType {
  SET,
  REL,
  MAP,
  ENTITY,
  TUPLE
};

enum class AstNodeType {
  LITERAL,
  REFERENCE,
  UNARY_OP,
  BINARY_OP,
  QUANTIFIER,
  FUNCTION_CALL,
  CREATE_ENTITY,    // Materialization: create a new entity, bind to variable named in value
  CREATE_TUPLE,     // Materialization: create a new tuple from children, bind to variable named in value
  CREATE_SET        // Materialization: create a new set/rel/map (sys_type in value), bind to variable named in child 0
};

}
