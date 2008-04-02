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

/** \class LoaderContext
    Holds the class loading state during the class loading process
*/

enum ResolveMode {
 FullResolve = 0,
 SemiResolve = 1
};

class LoaderContext {
#define FIELD_ACCESSORS(type, name) \
private: \
  type _##name; \
public: \
  type name() {return _##name ;} \
  void set_##name(type name) {_##name = name ;}

  FIELD_ACCESSORS(Symbol*, class_name)
  FIELD_ACCESSORS(FailureMode, fail_mode)
  FIELD_ACCESSORS(ResolveMode, resolve_mode)
  FIELD_ACCESSORS(Buffer*, buf)
  FIELD_ACCESSORS(bool, is_system_class)  
#undef FIELD_ACCESSORS

public:
  LoaderContext(Symbol* class_name,
                FailureMode fail_mode = ExceptionOnFailure,
                ResolveMode resolve_mode = FullResolve,
                Buffer *bufp = NULL) {
    _class_name = class_name;
    _fail_mode = fail_mode;
    _resolve_mode = resolve_mode;
    _buf = bufp;
    _is_system_class = false;
  }
  LoaderContext(Symbol* class_name, FailureMode fail_mode, 
                LoaderContext *copy_from) {
    _class_name = class_name;
    _fail_mode = fail_mode;
    _resolve_mode = copy_from->_resolve_mode;
    _buf = NULL;
  }
#ifndef PRODUCT
  void print_on(Stream *st);
  void p() {
    print_on(tty);
  }
#endif
};

/** \class SystemDictionary
    Manages the list of loaded classes in the VM.
    Used to load a class file and parse it via a call to ClassFileParser.
*/
class SystemDictionary : public AllStatic {
 public:
  // Returns a class with a given class name and class loader.  Loads
  // the class if needed. If not found a NoClassDefFoundError or a
  // ClassNotFoundException is thrown, depending on the value on the
  // fail_mode flag.  For most uses the fail_mode argument
  // should be set to ErrorOnFailure.
  inline static ReturnOop resolve(Symbol* class_name, FailureMode fail_mode 
                                  JVM_TRAPS) {
    LoaderContext loader_ctx(class_name, fail_mode);
    return resolve(&loader_ctx JVM_NO_CHECK_AT_BOTTOM);
  }
  inline static ReturnOop resolve(Symbol* class_name, FailureMode fail_mode,
                                  Buffer *buf  JVM_TRAPS) {
    LoaderContext loader_ctx(class_name, fail_mode, FullResolve, buf);
    return resolve(&loader_ctx JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop resolve(LoaderContext *loader_ctx JVM_TRAPS);
  inline static ReturnOop semi_resolve(Symbol* class_name JVM_TRAPS) {
    LoaderContext loader_ctx(class_name, ExceptionOnFailure, SemiResolve);
    return resolve(&loader_ctx JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop resolve_array_class(LoaderContext *loader_ctx JVM_TRAPS);
  static ReturnOop find(LoaderContext *loader_ctx JVM_TRAPS) {
    return find(loader_ctx, 
                /*lookup_only=*/ false,
                /*check_only=*/ true JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop find(LoaderContext *loader_ctx, 
                        bool lookup_only, bool check_only JVM_TRAPS);
  static ReturnOop find_class_in_dictionary(ObjArray *dictionary,
                                            LoaderContext *loader_ctx, 
                                            bool lookup_only);
  static ReturnOop find_class_or_null(Symbol *class_name);

#if ENABLE_ISOLATES
  static ReturnOop bucket_for_array_class(ObjArray *sd, juint hash_value);

  static void insert_array_class(jushort class_id, ObjArrayClass *ac);

  static ReturnOop find_array_class(jushort class_id);
#endif

#ifndef PRODUCT
  static char _last_class_loaded[];
#endif
 private:
  static void insert(LoaderContext *loader_ctx, InstanceClass* instance_class
                     JVM_TRAPS);
  static ReturnOop load_system_class(LoaderContext *loader_ctx JVM_TRAPS);

  static ReturnOop fetch_buffer(LoaderContext *loader_ctx JVM_TRAPS);

  static ReturnOop bucket_for(ObjArray *sd, juint hash_value);
  static void set_bucket_for(ObjArray *sd, juint hash_value, InstanceClass* bucket);
  static void set_bucket_for(ObjArray *sd, juint hash_value, ObjArrayClass* bucket);
  static ReturnOop create_fake_class(LoaderContext *loader_ctx JVM_TRAPS);
  static void update_fake_class(InstanceClass *real_cls, 
                                InstanceClass *fake_cls JVM_TRAPS);

friend class Universe; // need to use insert()
};
