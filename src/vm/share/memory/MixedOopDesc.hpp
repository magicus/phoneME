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

class MixedOopDesc: public OopDesc {
public:
  void variable_oops_do(void do_oop(OopDesc**));
  size_t object_size() const {
    return (size_t)_size;
  }
  void initialize(int type, size_t size, int pointer_count) {
    GUARANTEE(Type_BufferedFile <= type && type <= _last_type, "size limit");
    GUARANTEE(size <= 0xffff && pointer_count <= 0x7f, "size limit");

    _size = (jushort)size;
    _type = (jubyte)type;
    _pointer_count = (jubyte)pointer_count;
  }

  void reinitialize(OopDesc* klass, int type, size_t size, int pointer_count)
  {
    OopDesc::reinitialize(klass);
    initialize(type, size, pointer_count);
  }

// The MIXED_OOP_TYPES_DO macro is used by many places that need to
// iterate over all possible types of mixed oop.

#define MIXED_OOP_TYPES_DO(template) \
  template(BufferedFile,            buffered_file)             \
  template(ClassParserState,        class_parser_state)        \
  template(FileDecoder,             file_decoder)              \
  template(Inflater,                inflater)                  \
  template(JarFileParser,           jar_file_parser)           \
  template(PacketDataBuffer,        packet_data_buffer)        \
  template(PendingLink,             pending_link)              \
  template(ROMizerHashEntry,        romizer_hashentry)         \
  template(ROMLookupTable,          rom_lookup_table)          \
  template(ROMVector,               rom_vector)                \
  template(MetaObjType,             meta_obj_type)             \
  template(Thread,                  thread)                    \
  template(VirtualStackFrame,       virtual_stack_frame)       \
  template(Task,                    task)                      \
  template(Transport,               transport)                 \
  template(WTKThreadRecord,         profiler_info)             \
  template(VMEvent,                 vm_event)                  \
  template(VMEventModifier,         vm_event_modifier)         \
  template(ROMProfile,              rom_profile)         

#define MIXED_OOP_ENUM_DECLARE(x, y) \
  Type_ ## x,

  enum {
    MIXED_OOP_TYPES_DO(MIXED_OOP_ENUM_DECLARE)
    _last_type
  };

// The following constructs things such as
// bool is_task() {return (_type == Type_Task);}

#define MIXED_OOP_TYPE_CHECKER(x, y) \
  bool is_ ## y() {return (_type == Type_ ## x);}

MIXED_OOP_TYPES_DO(MIXED_OOP_TYPE_CHECKER)

protected:
  // size of this MixedOopDesc (number of bytes)
  jushort                _size;

  // type of mixed oop (used for debugging only)
  jubyte                 _type;

  // Number of object pointers (that follow the _pointer_count field)
  jubyte                _pointer_count;

  friend class MixedOop;
  friend class Universe;
  friend class OopDesc;
};
