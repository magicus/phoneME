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

#if ENABLE_JAVA_DEBUGGER

// Following classes maintain a mapping between objects and object ID's that
// are returned to the debugger.
// Every task 

class JavaDebuggerContextDesc : public MixedOopDesc {
  ObjArrayDesc* _objects_by_ref_map;
  ObjArrayDesc* _objects_by_id_map;

  friend class JavaDebuggerContext;
};

class JavaDebuggerContext : public MixedOop {
public:
  HANDLE_DEFINITION(JavaDebuggerContext, MixedOop);

  static size_t allocation_size() {
    return align_allocation_size(sizeof(JavaDebuggerContextDesc));
  }
  static size_t pointer_count() {
    return 2;
  }

  DEFINE_ACCESSOR_OBJ(JavaDebuggerContext, ObjArray, objects_by_ref_map);
  DEFINE_ACCESSOR_OBJ(JavaDebuggerContext, ObjArray, objects_by_id_map);

public:
  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS);

  ReturnOop get_object_by_id(int objectID);
  int get_object_id_by_ref(Oop *p);
  int get_object_id_by_ref_nocreate(Oop *p);
  int get_method_index(const Method *);
  jlong get_method_id(const Method *);
  jlong get_method_id(InstanceClass *, const Method *);
  ReturnOop get_method_by_id(InstanceClass *, jlong);

  // GC support
  void flush_refnodes();
  void rehash();

private:
  static int object_hash_code(Oop *);
  static int hash_id(jint);
  static ReturnOop compress_chain(RefNode *node, bool by_ref);

  enum {
    HASH_SLOT_SIZE = 127
  };
};

#endif
