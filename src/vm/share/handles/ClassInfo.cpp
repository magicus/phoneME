/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 */

# include "incls/_precompiled.incl"
# include "incls/_ClassInfo.cpp.incl"

int ClassInfo::itable_methods_offset() {
  return itable_offset_from_index(itable_length());
}

jint ClassInfo::itable_size() {
  int nof_methods = 0;
  for (int index = 0; index < itable_length(); index++) {
    InstanceClass::Raw ic = itable_interface_at(index);
    ObjArray::Raw methods = ic().methods();
    nof_methods += methods().length();
  }
  return itable_size(itable_length(), nof_methods);
}

jint ClassInfo::itable_size(int nof_interfaces, int nof_methods) {
  return nof_interfaces * (sizeof(jint) + sizeof(int))
       + nof_methods    * sizeof(jobject);
}

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_TTY_TRACE ||\
                         ENABLE_PERFORMANCE_COUNTERS
void ClassInfo::print_name_on(Stream* st) {
#if ENABLE_TTY_TRACE
  if (!check_valid_for_print(st)) {
    return;
  }
  Symbol::Raw n = name();
  if (n.equals(Symbols::unknown())) {
    n = ROM::get_original_class_name(this);
  }
  if (TraceGC) {
    st->print("[id %d] ", class_id());
  }
  if (n.not_null()) {
    n().print_symbol_on(st);
  } else {
    st->print("NULL");
  }
#endif
}
#endif

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR || ENABLE_TTY_TRACE
void ClassInfo::print_value_on(Stream* st) {
#if ENABLE_TTY_TRACE
  st->print("ClassInfo ");

  ReturnOop raw_name = obj_field(name_offset());
  if (raw_name == Symbols::unknown()->obj()) {
    st->print("(renamed) ");
  }

  print_name_on(st);
#endif
}
#endif

#if !defined(PRODUCT)

void ClassInfo::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);

  {
    NamedField id("object_size", true);
    visitor->do_ushort(&id, object_size_offset(), true);
  }

  {
    NamedField id("vtable_length", true);
    visitor->do_ushort(&id, vtable_length_offset(), true);
  }

  {
    NamedField id("itable_length", true);
    visitor->do_ushort(&id, itable_length_offset(), true);
  }

  {
    NamedField id("class_id", true);
    visitor->do_ushort(&id, class_id_offset(), true);
  }

  {
    NamedField id("name", true);
    visitor->do_oop(&id, name_offset(), true);
  }

  {
    char buff[1024];
    access_flags().print_to_buffer(buff, AccessFlags::CLASS_FLAGS);
    visitor->do_comment(buff);
    NamedField id("access_flags", true);
    id.set_hex_output(true);
    visitor->do_uint(&id, access_flags_offset(), true);
  }

  if (is_array()) {
    {
      NamedField id("type", true);
      visitor->do_int(&id, type_offset(), true);
    }
    { 
      NamedField id("scale", true);
      visitor->do_int(&id, scale_offset(), true);
    }
  } else {
    {
      NamedField id("methods", true);
      visitor->do_oop(&id, methods_offset(), true);
    }
    {
      NamedField id("fields", true);
      visitor->do_oop(&id, fields_offset(), true);
    }
    {
      NamedField id("local_interfaces", true);
      visitor->do_oop(&id, local_interfaces_offset(), true);
    }
#if ENABLE_REFLECTION
    {
      NamedField id("inner_classes", true);
      visitor->do_oop(&id, inner_classes_offset(), true);
    }
#endif
    {
      NamedField id("constants", true);
      visitor->do_oop(&id, constants_offset(), true);
    }
  }

  iterate_tables(visitor);
#endif
}

void ClassInfo::iterate_oopmaps(oopmaps_doer do_map, void* param) {
#if USE_OOP_VISITOR
  OOPMAP_ENTRY_4(do_map, param, T_SHORT, object_size);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT, vtable_length);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT, itable_length);
  OOPMAP_ENTRY_4(do_map, param, T_SHORT, class_id);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT,name);
  OOPMAP_ENTRY_4(do_map, param, T_INT,   access_flags);
  OOPMAP_ENTRY_5(do_map, param, T_OBJECT,methods,         OOPMAP_VARIABLE_OBJ);
  OOPMAP_ENTRY_5(do_map, param, T_OBJECT,fields,          OOPMAP_VARIABLE_OBJ);
#if ENABLE_ISOLATES
  OOPMAP_ENTRY_4(do_map, param, T_INT,   static_field_end);
#endif
  OOPMAP_ENTRY_5(do_map, param, T_OBJECT,local_interfaces,OOPMAP_VARIABLE_OBJ);
#if ENABLE_REFLECTION
  OOPMAP_ENTRY_5(do_map, param, T_OBJECT,inner_classes,   OOPMAP_VARIABLE_OBJ);
#endif
  OOPMAP_ENTRY_5(do_map, param, T_OBJECT,constants,       OOPMAP_VARIABLE_OBJ);

  // alias OOPMAP_ENTRY_4(do_map, param, T_SHORT,  type);
  // alias OOPMAP_ENTRY_4(do_map, param, T_SHORT,  scale);
#endif
}

#endif /* #ifndef PRODUCT*/

#if USE_OOP_VISITOR || USE_BINARY_IMAGE_GENERATOR || ENABLE_TTY_TRACE
void ClassInfo::iterate_tables(OopROMVisitor* visitor) {
  int index = 0;
  if (vtable_length() > 0) {
    visitor->do_comment("Virtual dispatch table");
    for (index = 0; index < vtable_length(); index++) {
      IndexableField field(index, true);
      visitor->do_oop(&field, vtable_offset_from_index(index), true);
    }
  }

  if (itable_length() > 0){
    visitor->do_comment("Interface dispatch table");
    for (index = 0; index < itable_length(); index++) {
      {
        InstanceClass ic = itable_interface_at(index);
        Symbol name = ic.name();
        char buffer[1024];
        jvm_sprintf(buffer, "interface klass_index ");
        name.string_copy(buffer + jvm_strlen(buffer), 
                         sizeof(buffer) - jvm_strlen(buffer));

        NamedField field(buffer, true);
        visitor->do_int(&field, itable_offset_from_index(index), true);
      }
      {
        NamedField field("offset", true);
        visitor->do_int(&field, 
                        itable_offset_from_index(index) + sizeof(jint),
                        true);
      }
    }
    for (index = 0; index < itable_length(); index++) {
      int offset = itable_offset_at(index);
      // Some ROM's interfaces that implement other interfaces set
      // offset=0, since this information is actually needed by
      // anyone.
      if (offset > 0) {
        InstanceClass ic = itable_interface_at(index);
        Symbol name = ic.name();
        ObjArray methods = ic.methods();
        const int buffer_size = 256;
        char buffer[256];
        jvm_sprintf(buffer, "Table for interface #%d: ", index);
        name.string_copy(buffer + jvm_strlen(buffer), buffer_size- jvm_strlen(buffer));
        visitor->do_comment(buffer);
        for (int i = 0; i < methods.length(); i ++) {
          IndexableField field(i, true);
          visitor->do_oop(&field, offset + i * sizeof(jobject), true);
        }
      }
    }
  }
}
#endif // USE_OOP_VISITOR || USE_BINARY_IMAGE_GENERATOR

#if ENABLE_ROM_GENERATOR

// generate a map of all the field types in this object
int ClassInfo::generate_fieldmap(TypeArray* field_map) {
  int map_index = 0;
  
  // (1) map the generic header

  // Obj Near
  map_index = Near::generate_fieldmap(field_map);

  //_object_size
  field_map->byte_at_put(map_index++, T_SHORT);
  //_vtable_length
  field_map->byte_at_put(map_index++, T_SHORT);
  //_itable_length
  field_map->byte_at_put(map_index++, T_SHORT);
  //_klass_index
  field_map->byte_at_put(map_index++, T_SHORT);
  //_name
  field_map->byte_at_put(map_index++, T_OBJECT);
  //_access_flags
  field_map->byte_at_put(map_index++, T_INT);

  // (2) Map the fields specific to array or instance
  int offset = generic_header_size();
  if (is_array()) {
    //_type
    field_map->byte_at_put(map_index++, T_INT);
    offset += sizeof(int);
    //_scale;
    field_map->byte_at_put(map_index++, T_INT);
    offset += sizeof(int);
    // 2 dummies
    field_map->byte_at_put(map_index++, T_INT);
    field_map->byte_at_put(map_index++, T_INT);
    offset += 2 * sizeof(int);
#if ENABLE_ISOLATES
    // 1 more dummy
    field_map->byte_at_put(map_index++, T_INT);
    offset += sizeof(int);
#endif
  } else {
    // _methods
    field_map->byte_at_put(map_index++, T_OBJECT);
    offset += sizeof(jobject);
    // _fields
    field_map->byte_at_put(map_index++, T_OBJECT);
    offset += sizeof(jobject);
#if ENABLE_ISOLATES
    // static_field size
    field_map->byte_at_put(map_index++, T_INT);
    offset += sizeof(int);
#endif
    // _local_interfaces
    field_map->byte_at_put(map_index++, T_OBJECT);
    offset += sizeof(jobject);
#if ENABLE_REFLECTION
    // _inner_classes
    field_map->byte_at_put(map_index++, T_OBJECT);
    offset += sizeof(jobject);
#endif
    // _constants
    field_map->byte_at_put(map_index++, T_OBJECT);
    offset += sizeof(jobject);
  }

  while (offset < header_size()) {
    // C++ compiler may have added struct padding
    field_map->byte_at_put(map_index++, T_INT);
    offset += sizeof(jint);
  }

  // (3) Map the itable and vtable
  int i;
  // vtable
  for (i = 0; i < vtable_length(); i++) {
    field_map->byte_at_put(map_index++, T_OBJECT);
    offset += sizeof(jobject);
  }
  // itable
  for (i = 0; i < itable_length(); i++) {
    field_map->byte_at_put(map_index++, T_INT); // interface klass_index
    offset += sizeof(jint);
    field_map->byte_at_put(map_index++, T_INT); // method index
    offset += sizeof(int);
  }
  while (offset < itable_end_offset()) {
    field_map->byte_at_put(map_index++, T_OBJECT); // interface method
    offset += sizeof(jobject);
  }

  return map_index;
}
#endif
