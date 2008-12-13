/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

# include "incls/_precompiled.incl"
# include "incls/_InstanceClass.cpp.incl"

HANDLE_CHECK(InstanceClass, is_instance_class())

#if ENABLE_ROM_GENERATOR
ReturnOop InstanceClass::package_name(JVM_SINGLE_ARG_TRAPS) {  
  UsingFastOops fast_oops;
  Symbol::Fast symbol_class_name = original_name();

  int len = symbol_class_name().strrchr('/');
  if (len <= 0) {
    len = symbol_class_name().length();
  }
  TypeArray::Fast byte_array = Universe::new_byte_array(len JVM_CHECK_0);  

  for (int i = 0; i < len; i++) {
    byte_array().byte_at_put(i, symbol_class_name().byte_at(i));
  }
  GUARANTEE(SymbolTable::current()->not_null(), 
    "JavaClass::package_name() requires SymbolTable");
  return SymbolTable::symbol_for(&byte_array JVM_NO_CHECK_AT_BOTTOM_0);
}
#endif

#if !ROMIZED_PRODUCT || ENABLE_ISOLATES
void InstanceClass::bootstrap_initialize(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
#ifndef PRODUCT
  Method::Fast init = find_local_method(Symbols::class_initializer_name(),
                                        Symbols::void_signature());
  GUARANTEE(init.is_null(), "cannot have class initializer");
  AZZERT_ONLY_VAR(init);
#endif
  set_initialized();
#if USE_EMBEDDED_VTABLE_BITMAP
  update_vtable_bitmaps(JVM_SINGLE_ARG_CHECK);
#endif
  verify(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}
#endif

void InstanceClass::set_verified() {
  ClassInfo::Raw info = class_info();
  info().set_is_verified();
#if !ENABLE_ISOLATES
  JavaClassObj::Raw mirror = java_mirror();
  mirror().set_verified();
#endif
}

bool InstanceClass::is_verified() {
  ClassInfo::Raw info = class_info();
  bool result = info().is_verified();

#if !ENABLE_ISOLATES && defined(AZZERT)
  // Verify that the flag in ClassInfo is in-sync with the mirror object 
  JavaClassObj::Raw mirror = java_mirror();
  GUARANTEE(mirror().is_verified() == result, "sanity");
#endif

  return result;
}

#if ENABLE_ISOLATES
// Initialize a class on behalf of a task.
// When entering here, the class may either be not initialized at all, or it 
// may be being initialized. 
//
// If not initialized, a task mirror is created.
// setup_java_mirror set the task mirror pointer in the task mirror table
// of the class in "initializing" state.
// set_initialized sets the task mirror pointer in "initialized" state.
ReturnOop InstanceClass::initialize_for_task(JVM_SINGLE_ARG_TRAPS){
  UsingFastOops fast_oops;
  TaskMirror::Fast tm; 
  tm = task_mirror_desc();
  if (TaskMirrorDesc::is_being_initialized_mirror((TaskMirrorDesc*)tm.obj())) {
    tm = TaskMirror::clinit_list_lookup(this);
    if (tm.is_null()) {
      // allocate Task mirror
      tm = setup_task_mirror(static_field_size(), vtable_length(), 
                             true JVM_CHECK_0);
      initialize_static_fields(&tm);
    }
  } else {
    return tm.obj();
  }
  GUARANTEE(!tm.is_null(), "null task mirror");
  JavaClassObj::Fast m = tm().real_java_mirror();
  initialize_internal(Thread::current(), &m JVM_CHECK_0);
  return tm.obj();
}

void InstanceClass::set_initialized() {
 TaskMirror::Raw tm;
 tm = task_mirror_desc();
 GUARANTEE(tm.not_null(), "task mirror must not be null");
 if (TaskMirrorDesc::is_being_initialized_mirror((TaskMirrorDesc*)tm.obj())){
   tm = TaskMirror::clinit_list_lookup(this);
   GUARANTEE(tm.not_null(), "task mirror must not be null");
   tm = TaskMirror::clinit_list_remove(this);
 }
 GUARANTEE(tm.not_null(), "task mirror must not be null");
 JavaClassObj::Raw mirror = tm().real_java_mirror();
 GUARANTEE(mirror.not_null(), "mirror must not be null");
 if (mirror().status() == JavaClassObj::ERROR_FLAG) {
         return;
 }
 mirror().set_initialized();
 set_task_mirror(&tm);
}

bool InstanceClass::is_initialized() {
  return TaskMirrorDesc::is_initialized_mirror(task_mirror_desc());
}

void InstanceClass::initialize(JVM_SINGLE_ARG_TRAPS) {
  initialize_for_task(JVM_SINGLE_ARG_CHECK);
}

#else
// !ENABLE_ISOLATES

void InstanceClass::set_initialized() {
  JavaClassObj::Raw mirror = java_mirror();
  mirror().set_initialized();
}

bool InstanceClass::is_initialized() {
  JavaClassObj::Raw mirror = java_mirror();
  return mirror().is_initialized();
}

void InstanceClass::initialize(JVM_SINGLE_ARG_TRAPS) {
  JavaClassObj::Raw mirror = java_mirror();
  if (mirror().is_initialized()) {
    return;
  }
  initialize_internal(Thread::current(), &mirror JVM_CHECK);
}

#endif //ENABLE_ISOLATES

void InstanceClass::initialize_internal(Thread *thread, Oop *m JVM_TRAPS) {
#ifdef AZZERT
  Symbol::Raw class_name = name();
  if (!class_name.equals(Symbols::java_lang_NoClassDefFoundError()) &&
      !class_name.equals(Symbols::java_lang_ClassNotFoundException())) {
    GUARANTEE(!Compiler::is_active(), 
              "Compiler should not initialize any classes");
  }
#endif

  UsingFastOops fast_oops;
  JavaClassObj::Fast mirror = m->obj();
  if (mirror().in_progress()) {
    ThreadObj::Raw initializing_thread = mirror().thread();
    ThreadObj::Raw thread_obj = thread->thread_obj();
    if (initializing_thread().equals(&thread_obj)) {
      return;
    }
  }

#if ENABLE_TTY_TRACE
  if (TraceClassInitialization) {
    tty->print("class initializing: ");
    print_name_on(tty);
    tty->cr();
  }
#endif

  // Execute the method java.lang.Class.initialize. Note that if another
  // thread is already executing Class.initialize(), this thread would
  // be blocked until the other thread completes.
  Method::Fast method = Universe::java_lang_Class_class()
      ->find_local_method(Symbols::initialize_name(),
                          Symbols::void_signature());
  GUARANTEE(!method.is_null(),"We need the method java.lang.Class.initialize");

  {
    EntryActivation::Raw entry = Universe::new_entry_activation(&method, 1 
                                                                JVM_CHECK);
    entry().obj_at_put(0, &mirror);
    thread->append_pending_entry(&entry);
  }
}

void InstanceClass::clinit(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  // Find the method to invoke
  Method::Fast init = find_local_method(Symbols::class_initializer_name(),
                                        Symbols::void_signature());

  if (!init.is_null()) {
    EntryActivation::Raw entry =
        Universe::new_entry_activation(&init, 0 JVM_NO_CHECK);
    if (entry.not_null()) {
      Thread::current()->append_pending_entry(&entry);
    }
  }
}

void InstanceClass::verify(JVM_SINGLE_ARG_TRAPS) {
  if (get_UseVerifier() && !is_verified()) {
    Verifier::verify_class(this JVM_NO_CHECK);

#if ENABLE_ROM_GENERATOR
    if (GenerateROMImage) {
      if (CURRENT_HAS_PENDING_EXCEPTION) {
        tty->print("Failed to verify class ");
        this->print_name_on(tty);
        tty->cr();
        Oop::Raw oome = Universe::out_of_memory_error_instance();
        if (!oome.equals(Thread::current_pending_exception()) && 
            PostponeErrorsUntilRuntime) {
          // Do not abort on verification failure. Instead a valid image is 
          // produced and the verification error is reported at run-time.
          Thread::clear_current_pending_exception();
    
          // We disable ROM optimizations to be able to report the failure 
          // properly at run-time.
          set_is_non_optimizable();
          return;
        }
      }
    }
#endif

    if (!CURRENT_HAS_PENDING_EXCEPTION) {
      UsingFastOops fast_oops;
      ObjArray::Fast array = this->methods();
      Method::Fast m;
      int len = array().length();
      for (int i=0; i<len; i++) {
        m = array().obj_at(i);
#if ENABLE_CPU_VARIANT && ENABLE_ARM11_JAZELLE_DLOAD_BUG_WORKAROUND
        m().fix_long_operations(); //see CR 6486596
#endif
        if (m().code_size() == 5) {
          m().set_fast_accessor_entry(JVM_SINGLE_ARG_NO_CHECK);
          GUARANTEE(!CURRENT_HAS_PENDING_EXCEPTION, "sanity");
        }
      }
    }
  }
}

ReturnOop InstanceClass::new_instance(FailureMode fail_mode JVM_TRAPS) {
  if (is_abstract() || equals(Universe::java_lang_Class_class())) {
    Throw::instantiation(fail_mode JVM_THROW_0);
  }

  // ensure that the class gets initialized before instantiation
  initialize(JVM_SINGLE_ARG_CHECK_0);
  return Universe::new_instance(this JVM_NO_CHECK_AT_BOTTOM);
}

size_t InstanceClass::first_static_map_offset() const {
  size_t map_offset = first_nonstatic_map_offset();
  while (oop_map_at(map_offset) != OopMapSentinel) {
    map_offset++;
  }
  return map_offset + 1;
}

size_t InstanceClass::nonstatic_map_size() const {
  return first_static_map_offset() - first_nonstatic_map_offset();
}

size_t InstanceClass::last_nonstatic_oop_offset() const {
  size_t map_offset = first_nonstatic_map_offset();
  size_t oop_offset = 0;
  jubyte value;
  while ((value = oop_map_at(map_offset++)) != OopMapSentinel) {
    oop_offset += value*oopSize;
  }
  return oop_offset;
}

size_t InstanceClass::static_map_size() const {
  size_t map_start = first_static_map_offset();
  size_t map_offset = map_start;
  while (oop_map_at(map_offset) != OopMapSentinel) {
    map_offset++;
  }
  return map_offset + 1 - map_start;
}

#if USE_EMBEDDED_VTABLE_BITMAP
void InstanceClass::set_is_method_overridden(int vtable_index) {
  GUARANTEE(0 <= vtable_index && vtable_index < vtable_length(),
            "Bound check");
  ClassInfo::Raw info = class_info();    
  Method::Raw method = info().vtable_method_at(vtable_index);
  InstanceClass::Raw holder = method().holder();
  holder().set_vtable_bitmap_bit(vtable_index);
}

bool InstanceClass::is_method_overridden(int vtable_index) const {
  GUARANTEE(0 <= vtable_index && vtable_index < vtable_length(),
            "Bound check");
  ClassInfo::Raw info = class_info();    
  Method::Raw method = info().vtable_method_at(vtable_index);
  InstanceClass::Raw holder = method().holder();
  if (holder().is_vtable_bitmap_installed()) {
    return holder().vtable_bitmap_bit(vtable_index);
  } else {
    return false;
  }
}
#endif

/// Find a method with matching name and signature (regardless of access
/// flags). We start by searching the current class, and recursively walk
/// up the class hierarchy.
ReturnOop InstanceClass::lookup_method(Symbol* name, Symbol* signature, bool non_static_only) {
  InstanceClass::Raw ic = this->obj();
  Method::Raw m;          // If non-null, a method with matching name+sig
  InstanceClass::Raw m_holder; // Holder of <m>

  // (1) Recursively search the method table (and walk up the class hierarchy)
  //     until we find a matching method.
  while (!ic.is_null()) {
    m = ic().find_local_method(name, signature, non_static_only);
    if (!m.is_null()) {
      break;
    }
    ic = ic().super();
  }

  if (m.not_null()) {
    m_holder = ic.obj();
    if (m_holder == this->obj()) {
      // This is a common case -- method is already declared in this class. 
      // We can't find a more suitable method in the vtable.
      return m.obj();
    }
  }

  // (2) Find a matching method in the vtable -- this is necessary for
  //     ROMized classes -- their virtual method are removed from the method
  //     table to save space.
  ClassInfo::Raw info = class_info();
  int vtable_length = info().vtable_length();
  for (int i = 0; i < vtable_length; i++) {
    Method::Raw m2 = info().vtable_method_at(i);
    if (m2.not_null() && (!m2.equals(&m)) && m2().match(name, signature)) {
      InstanceClass::Raw ic2 = m2().holder();

      if (m.is_null()) {
        m = m2.obj();
        m_holder = ic2.obj();
      } else {
        // Check if ic2 is a sub-class of m_holder
        while (ic2.not_null()) {
          if (ic2.equals(&m_holder)) {
            // One of ic2's super class is m_holder, so m2 was declared by
            // a subclass of m_holder
            m = m2.obj();
            m_holder = ic2.obj();
            break;
          }
          ic2 = ic2().super();
        }
      }
    }
  }

  return m.obj();
}

ReturnOop InstanceClass::find_local_method(Symbol* name, Symbol* signature, bool non_static_only) {
  ObjArray::Raw array = methods();
  return find_method(&array, name, signature, non_static_only);
}

/// Adds miranda methods to a class's methods array. For an interface I and
/// a class C that declares to implement I, a miranda method I.m is a method
/// declared in I but is not declared in C.
///
/// Miranda methods are needed for bytecodes like "invokevirtual C.m" to
/// work properly.
///
/// super -- super class of C
/// methods -- methods that C declares locally
/// interfaces -- interfaces that C declares locally to implement
ReturnOop InstanceClass::add_miranda_methods(InstanceClass* super,
                                             ObjArray* methods, 
                                             TypeArray* interfaces,
                                             Symbol *class_name JVM_TRAPS)
{
  UsingFastOops fast_oops;
  InstanceClass::Fast interface_class, interface_class2, s;
  ObjArray::Fast interface_methods, interface_methods2;
  ObjArray::Fast new_methods;

  // No need for miranda methods for these simple cases
  if (super->is_null()) {
    if (interfaces->length() != 0) {
      JVM_FATAL(java_lang_object_cannot_implement_interfaces);
    }
    return methods->obj();
  }
  if (interfaces->length() == 0) {
    return methods->obj();
  }

  ClassInfo::Fast sinfo = super->class_info();
  int super_vtable_length = sinfo().vtable_length();
  int num_interfaces = interfaces->length();
  int num_methods = methods->length();

  int pass;      // outer-most loop
  int i;         // iterate over all the interfaces
  int im_index;  // iterate over all methods in interface_class
  int im2_index; // iterate over all methods in interface_class2

  int vt_index;  // iterate over the itable methods in <super>
  int m_index;   // iterate over the <methods> parameter

  Method::Fast im;      // a method declared by an interface
  Symbol::Fast im_name; // name of <im>
  Symbol::Fast im_sig;  // signature of <im>
  Method::Fast miranda_method;

  int num_mirandas_found = 0, num_mirandas_added = 0;
  int j;

  // Do this in two passes:
  //   1st pass: count the number of miranda methods
  //   2nd pass: add miranda methods
  for (pass=0; pass<2; pass++) {
    for (i=0; i<num_interfaces; i++) {
      interface_class = Universe::class_from_id(interfaces->ushort_at(i));
      interface_methods = interface_class().methods();
      int num_interface_methods = interface_methods().length();

      for (im_index=0; im_index<num_interface_methods; im_index++) {
        im = interface_methods().obj_at(im_index);
        if (im.is_null()) {
          continue;
        }
        if (im().is_static()) {
          continue;
        }

        im_name = im().name();
        im_sig  = im().signature();

        // An interface method <im> is NOT a miranda method if any of the
        // following (a), (b) or (c) is true

        // (a) the class declares <im> locally -- this is the most common
        //     case, so do it first
        for (m_index = 0; m_index < num_methods; m_index++) {
          Method::Raw m = methods->obj_at(m_index);
          if (m.is_null()) {
            continue;
          }
          if (m().match(&im_name, &im_sig)) {
            goto not_miranda;
          }
        }

        // (b.1) the method is already in the super class's vtable
        for (vt_index = 0; vt_index < super_vtable_length; vt_index++) {
          Method::Raw m = sinfo().vtable_method_at(vt_index);
          if (m.not_null() && m().match(&im_name, &im_sig) && m().is_public()) {
            goto not_miranda;
          }
        }

        // (b.2) the method is a final method in a super class
        for (s = super->obj(); s.not_null(); s = s().super()) {
          ObjArray::Raw super_methods = s().methods();
          int n = super_methods().length();
          for (j=0; j<n; j++) {
            Method::Raw m = super_methods().obj_at(j);
            if (m.not_null()
                && m().match(&im_name, &im_sig) && m().is_final()) {
              goto not_miranda;
            }
          }
        }

        // (c) the method is already declared by another interface I2, which
        //     precedes I in interfaces.
        for (j=0; j<i; j++) {
          interface_class2 = Universe::class_from_id(interfaces->ushort_at(j));
          interface_methods2 = interface_class2().methods();
          int num_interface_methods2 = interface_methods2().length();

          for (im2_index=0; im2_index<num_interface_methods2; im2_index++) {
            Method::Raw m = interface_methods2().obj_at(im2_index);
            if (m.is_null()) {
              continue;
            }
            if (m().match(&im_name, &im_sig)) {
              goto not_miranda;
            }
          }
        }

        // im is a miranda method.
        if (pass == 0) {
          num_mirandas_found ++;
        } else {
          miranda_method = ClassFileParser::new_lazy_error_method(&im,
                     (address) Java_abstract_method_execution JVM_CHECK_0);
          int idx = num_methods + num_mirandas_added;
          new_methods().obj_at_put(idx, &miranda_method);
          num_mirandas_added ++;

#if ENABLE_TTY_TRACE
          if (TraceMirandaMethods) {
            TTY_TRACE(("Miranda method in class: "));
            class_name->print_symbol_on(tty);
            tty->cr();
            TTY_TRACE(("  -> added "));
            im().print_name_on(tty);
            tty->cr();
            tty->cr();
          }
#else
          (void)class_name;
#endif
        }
        continue;

      not_miranda:
        continue;
      }
    }

    if (num_mirandas_found == 0) {
      GUARANTEE(pass == 0, "sanity");
      return methods->obj();
    }
    if (pass == 0) {
      int num_new_methods = num_methods + num_mirandas_found;
      new_methods = Universe::new_obj_array(num_new_methods JVM_CHECK_0);
      ObjArray::array_copy(methods, 0, &new_methods, 0, num_methods 
                           JVM_MUST_SUCCEED);
    }
  }

  GUARANTEE(num_mirandas_added == num_mirandas_found, "sanity");
  GUARANTEE(new_methods.not_null(), "sanity");

  return new_methods.obj();
}


/*
 * [1] What's the "super class" of my interface?
 *
 *     First of all, although in Java you write
 *
 *         interface Foo extends A {...}
 *
 *     internally we always treat Foo as a subclass of
 *     java.lang.Object, not a subclass of A. This is because an
 *     interface may "extend" multiple interfaces. E.g.,
 *
 *         interface Foo extends A, B {...}
 *
 *     We record A and B in Foo's local_interfaces() array.
 */

ReturnOop
InstanceClass::lookup_method_in_all_interfaces(Symbol* name, 
                                               Symbol* signature,
                                               int& interface_class_id,
                                               int& itable_index) {
  Method::Raw method = find_local_method(name, signature);
  if (method.not_null()) {
    interface_class_id = class_id();         // returns to caller
    itable_index = method().itable_index();  // returns to caller
    return method.obj();
  }

  InstanceClass::Raw interface_class;
  TypeArray::Raw interfaces = local_interfaces();
  int n_interfaces = interfaces().length();

  for (int i = 0; i < n_interfaces; ++i) {
    interface_class = Universe::class_from_id(interfaces().ushort_at(i));
    method = interface_class().lookup_method_in_all_interfaces(name, signature,
                                            interface_class_id, itable_index);
    if (method.not_null()) {
      return method.obj();
    }
  }
  return NULL;
}

ReturnOop InstanceClass::interface_method_at(jint itable_index) {
  GUARANTEE(is_interface(), "sanity");

  ObjArray::Raw my_methods = methods();
  return my_methods().obj_at(itable_index);
}

ReturnOop InstanceClass::find_method(ObjArray* class_methods, Symbol* name,
                                     Symbol* signature, bool non_static_only) {
  AllocationDisabler raw_pointers_used_in_this_function;

  OopDesc *name_obj = name->obj();
  OopDesc *sig_obj = signature->obj();
  MethodDesc **ptr = (MethodDesc**)class_methods->base_address();
  MethodDesc **end = ptr + class_methods->length();

  while (ptr < end) {
    MethodDesc *m = *ptr++;
    if (m != NULL && m->match(name_obj, sig_obj)) {
      if (!non_static_only) {
        return m;
      } else {
        Method::Raw method = m;
        if (!method().is_static()) {
          return m;
        }
      }
    }
  }
  return NULL;
}

void InstanceClass::remove_clinit() {
  ObjArray::Raw class_methods(methods());

  if (ROM::in_any_loaded_readonly_bundle(class_methods.obj())) {
    // Can't remove clinit methods that are in ROM
    return;
  }

  if (is_interface()) {
    // IMPL_NOTE: itable calculations assumes that this->methods()
    // contains no NULL pointers
    return;
  }

  Symbol *name = Symbols::class_initializer_name();
  Symbol *signature = Symbols::void_signature();

  int length = class_methods().length();
  Method::Raw m; 
  for (int index = 0; index < length; index++) {
    m = class_methods().obj_at(index);
    GUARANTEE(!m.is_null(), "no methods could have been removed yet");
    if (m().match(name, signature)) {
      class_methods().obj_at_clear(index);
      return;
    }
  }
}

bool InstanceClass::itable_contains(InstanceClass* instance_class) {
  ClassInfo::Raw info = class_info();
  for (int index = 0; index < info().itable_length(); index++) {
    InstanceClass::Raw element = info().itable_interface_at(index);
    if (element.equals(instance_class)) {
      return true;
    }
  }
  return false;
}

bool InstanceClass::is_same_class_package(Symbol* other_class_name) {
  Symbol::Raw this_class_name = name();
  return this_class_name().is_same_class_package(other_class_name);
}

bool InstanceClass::is_same_class_package(InstanceClass* other_class) {
  Symbol::Raw other_class_name = other_class->name();
  return is_same_class_package(&other_class_name);
}

// See JVMS 5.4.4
bool InstanceClass::check_access_by(InstanceClass* sender_class, 
                                    FailureMode fail_mode JVM_TRAPS) {
  if (sender_class->access_flags().is_preloaded()) {
    // We're checking access on a non-quickened bytecode that lives in
    // ROM. The check was already done during romization. No need to
    // repeat. In addition, the check may fail if the sender_class was
    // renamed.
    return true;
  }

  // access to hidden classes is prohibited to user classes,
  // system ones are covered by previous check
  if (is_hidden()) {
    UsingFastOops fast_oops;
    Symbol::Fast class_name = name();        
    Throw::class_not_found(&class_name, fail_mode JVM_THROW_0);
  }

  // We come to here in the Monet case if there method reference is invalid
  // possibly an illegal access. Monet should not rename these two classes
  // so that the necessary checks will be performed at run-time.
  GUARANTEE(!sender_class->is_renamed(), "must not be renamed by converter");
  GUARANTEE(!is_renamed(), "must not be renamed by converter");

  if (is_public() || is_same_class_package(sender_class)) {
    return true;
  }
  Throw::illegal_access(fail_mode JVM_THROW_0);
}

void InstanceClass::update_vtable(int super_vtable_length) {
  InstanceClass::Raw superclass    = super();
  Symbol::Raw        classname     = name();
  ObjArray::Raw      class_methods = methods();
  ClassInfo::Raw     info          = class_info();
  Method::Raw        method;

  int first_free = super_vtable_length;

  for (int index = 0; index < class_methods().length(); index++) {
    method = class_methods().obj_at(index);
    bool updating = needs_new_vtable_entry(&method, &superclass, 
                                           &classname,
                                           access_flags(), &info, true);
    if (updating) {
      info().vtable_at_put(first_free, &method);
      first_free++;
    }
  }

  // In class hierarchies where the accessibility is not increasing (i.e.,
  // going from private -> package_private -> public_protected), the vtable
  // might actually be smaller than our initial calculation.
  GUARANTEE(first_free <= info().vtable_length(),
            "vtable initialization failed");
  for(; first_free < info().vtable_length(); first_free++) {
    info().vtable_at_clear(first_free);
  }
}

// The following is specifically a static method so that it can be called
// from JavaVTable before we actually have an InstanceClass.
bool InstanceClass::needs_new_vtable_entry(Method* method, InstanceClass*super,
                                           Symbol* classname,
                                           const AccessFlags access_flags, 
                                           ClassInfo* info, //don't need if update_entries == false
                                           bool update_entries)
{
  bool allocate_new = true;

  if (method->is_null()) {
    // a removed <clinit> method
    return false;
  }

  // Static and <init> methods are never in the vtable
  if (method->is_static() || method->is_object_initializer()) {
    return false;
  }

  if ((access_flags.is_final() || method->is_final())) {
    // a final method never needs a new entry; final methods can be statically
    // resolved and they have to be present in the vtable only if they override
    // a super's method, in which case they re-use its entry
    allocate_new = false;
  }

  // we need a new entry if there is no superclass
  if (super->is_null()) {
    return allocate_new;
  }

  // private methods always have a new entry in the vtable
  if (method->is_private()) {
    return allocate_new;
  }

  // search through the vtable and update overridden entries
  OopDesc *myname = method->name();
  OopDesc *mysig  = method->signature();
  Method::Raw match;
  InstanceClass::Raw holder;

  ClassInfo::Raw sinfo = super->class_info();
  int super_length = sinfo().vtable_length();
  for(int index = 0; index < super_length; index++) {
    match = sinfo().vtable_method_at(index);
    
    if (match.is_null()) {
      continue;
    }

    // Check if method name matches
    if (myname == match().name() && mysig == match().signature()) {
      holder = match().holder();
      // Check if the match_method is accessible from current class

      bool same_package_init = false;
      bool same_package_flag = false;
      bool simple_match = match().is_public()  || match().is_protected();
      if (!simple_match) {
        same_package_init = true;
        same_package_flag = holder().is_same_class_package(classname);
        simple_match = match().is_package_private() && same_package_flag;
      }
      // A simple form of this statement is:
      // if ( (match_method->is_public()  || match_method->is_protected()) ||
      //      (!match_method->is_private() && 
      //       holder->is_same_class_package(klass->class_loader(), 
      //       klass->name())) )
      //
      // The complexity is introduced it avoid recomputing
      // 'is_same_class_package' which is expensive.
      if (simple_match) {
        // Check if target_method and match_method has same 
        // (or target_method has weaker )level of
        // accessibility. The accessibility of the match method is the
        // "most-general" visibility of all entries at it's particular
        // vtable index for all superclasses. This check must be done
        // before we override the current entry in the vtable.
        JavaClass::AccessType at = super->vtable_accessibility_at(index);

        if (  (at == JavaClass::acc_publicprotected &&
               (method->is_public() || method->is_protected()))
           || (at == JavaClass::acc_package_private && 
               (!method->is_private() &&
                (( same_package_init && same_package_flag) ||
                 (!same_package_init && 
                  holder().is_same_class_package(classname)))))
           ) {
          // target and match has same accessibility - share entry
          if (update_entries) {
            info->vtable_at_put(index, method);
          }
          allocate_new = false;
        }
      }
    }
  }

  return allocate_new;
}

void InstanceClass::itable_copy_down(InstanceClass* ic, int& index,
                                     int& method_offset JVM_TRAPS) {
  UsingFastOops fast_oops;
  ClassInfo::Fast info = class_info();
  info().itable_interface_at_put(index, ic->class_id());
  info().itable_offset_at_put(index, method_offset);
  index++;

  // Copy methods
  ObjArray::Fast methods = ic->methods();
  Method::Fast interface_method;
  Method::Fast method;

  for (int i = 0; i < methods().length(); i++) {
    interface_method = methods().obj_at(i);
    if (interface_method.is_null()) {
      // This class implements this interface but during ROMization
      // this method was probably declared dead and removed
      // Nothing we can do about it except skip it
      continue;
    }

    {
      Symbol::Raw name      = interface_method().name();
      Symbol::Raw signature = interface_method().signature();
      // Find the real method in class
      method = lookup_method(&name, &signature, true);
#ifdef AZZERT
      if (method.not_null()) {
        //this is very important check. Failing it could lead 
        //to security hole
        //see CR 6539767
        GUARANTEE(!method().is_static(), "check");
      }
#endif
    }

    if (!access_flags().is_abstract()) {
      // We patch broken method entries to get lazy error throwing.
      // Please note we have taken care of the abstract methods
      if (method.is_null() || method().is_static()) {
        method = ClassFileParser::new_lazy_error_method(&interface_method,
                     (address) Java_incompatible_method_execution JVM_CHECK);
      } else if (!method().is_public()) {
        method = ClassFileParser::new_lazy_error_method(&interface_method,
                     (address) Java_illegal_method_execution JVM_CHECK);
      }
    }

    info().obj_field_put(method_offset, &method);
    method_offset += sizeof(jobject);
  }
}

void InstanceClass::update_local_itables(InstanceClass* intf, int& index,
                                         int& method_offset JVM_TRAPS) {
  UsingFastOops fast_oops;

  InstanceClass::Fast s = super();
  TypeArray::Fast ifs = intf->local_interfaces();
  InstanceClass::Fast super_intf;
  for (int j = 0; j < ifs().length(); j++) {
    super_intf = Universe::class_from_id(ifs().ushort_at(j));
    if (s.is_null() || !s().itable_contains(&super_intf)) {
      itable_copy_down(&super_intf, index, method_offset JVM_CHECK);
    }
    update_local_itables(&super_intf, index, method_offset JVM_CHECK);
  }
}

void InstanceClass::check_and_initialize_itable(JVM_SINGLE_ARG_TRAPS) {
  if (Universe::is_bootstrapping()) {
#ifndef PRODUCT
    TypeArray::Raw lifs = local_interfaces();
    GUARANTEE(lifs().length() == 0, 
              "bootstrap classes cannot implement interface");
    AZZERT_ONLY_VAR(lifs);
#endif
    return;
  }

  UsingFastOops fast_oops;

  // first copy down the itable from the super class
  ClassInfo::Fast info = class_info();
  InstanceClass::Fast s = super();
  TypeArray::Fast lifs;
  InstanceClass::Fast ic, other_interface;
  ClassInfo::Fast sinfo;

  int index = 0;
  int method_offset = info().itable_methods_offset();
  if (!s.is_null()) {
    lifs = local_interfaces();
    sinfo = s().class_info();
    int super_length = sinfo().itable_length();
    for (int i = 0; i < super_length; i++) {
      ic = sinfo().itable_interface_at(i);
      for (int j = 0; j < lifs().length(); j++) {
        other_interface = Universe::class_from_id(lifs().ushort_at(j));
        if (other_interface.equals(this)) {
          Throw::class_format_error(circular_interfaces JVM_THROW);
        }
      }
      itable_copy_down(&ic, index, method_offset JVM_CHECK);
    }
  }

  // update interfaces implemented by locally define interfaces
  update_local_itables(this, index, method_offset JVM_NO_CHECK_AT_BOTTOM);
}

#if ENABLE_ISOLATES
// Called only when barrier is not set for this class in the current task.
void InstanceClass::initialize_static_fields() {
  if (Universe::java_lang_Class_class()->is_null()) {
    return;
  } else {
    TaskMirror::Raw statics_holder = task_mirror();
    initialize_static_fields(&statics_holder);
  }
}

#else
void InstanceClass::initialize_static_fields() {
  initialize_static_fields(this);
}
#endif

void InstanceClass::initialize_static_fields(Oop *statics_holder) {
  AllocationDisabler raw_pointers_used_in_this_function;

  TypeArray::Raw field_array = fields();
  ConstantPool::Raw cp = constants();

  jushort *field = field_array().ushort_base_address();
  jushort *field_end = field + field_array().length();

  for (; field < field_end; field += Field::NUMBER_OF_SLOTS) {
    int initval_index = field[Field::INITVAL_OFFSET];
    int access_flags  = field[Field::ACCESS_FLAGS_OFFSET];

    if ((initval_index != 0) && (access_flags & JVM_ACC_STATIC)) {
      int type_index = field[Field::SIGNATURE_OFFSET];
      int offset     = field[Field::OFFSET_OFFSET];
      FieldType::Raw type = cp().symbol_at(type_index);

      switch (type().basic_type()) {
      case T_BYTE:
      case T_CHAR:
      case T_SHORT:
      case T_BOOLEAN:
      case T_INT:
        statics_holder->int_field_put(offset, 
                                      cp().int_at(initval_index));
        break;
      case T_FLOAT:
        statics_holder->float_field_put(offset, 
                                      cp().float_at(initval_index));
        break;
      case T_DOUBLE:
        statics_holder->double_field_put(offset,
                                      cp().double_at(initval_index));
        break;
      case T_LONG:
        statics_holder->long_field_put(offset,
                                      cp().long_at(initval_index));
        break;
      case T_OBJECT:
        {
          String::Raw string = cp().resolved_string_at(initval_index);
          statics_holder->obj_field_put(offset, &string);
        }
        break;
      default:
        SHOULD_NOT_REACH_HERE();
      }
    }
  }
}

#if ENABLE_COMPILER && ENABLE_INLINE
void InstanceClass::update_vtable_bitmaps(JVM_SINGLE_ARG_TRAPS) const {
  if (is_interface()) {
    return;
  }

  UsingFastOops fast_oops;
  InstanceClass::Fast super_class = this->super();

  if (super_class.not_null()) {
    UsingFastOops fast_oops_2;
    ClassInfo::Fast this_class_info = this->class_info();
    ClassInfo::Fast super_class_info = super_class().class_info();
    const int super_vtable_length = super_class_info().vtable_length();

    GUARANTEE(super_vtable_length <= this_class_info().vtable_length(),
              "Sanity");

    for (int vtable_index = 0; vtable_index < super_vtable_length; 
         vtable_index++) {
      Method::Raw this_method = 
        this_class_info().vtable_method_at(vtable_index);
      if (this_method.not_null()) {
        InstanceClass::Raw holder = this_method().holder();
        if (holder.equals(this)) {
          if (!super_class().is_method_overridden(vtable_index)) {
            Method::Raw super_method = 
              super_class_info().vtable_method_at(vtable_index);

            GUARANTEE(!this_method.equals(&super_method), 
                      "Cannot be equal: must have different holders");
            
            if (TraceMethodInlining) {
              tty->print("Method ");
              this_method().print_name_on_tty();
              tty->print(" overrides ");
              super_method().print_name_on_tty();
              tty->cr();
            }

            // If the loaded class overrides this method in super_class,
            // unlink all methods that has this method inlined and mark 
            // the method as overriden in the inline table.
            super_method().unlink_direct_callers();

            super_class().set_is_method_overridden(vtable_index);
          } else {
#ifdef AZZERT
            Method::Raw super_method = 
              super_class_info().vtable_method_at(vtable_index);

            GUARANTEE(!this_method.equals(&super_method), 
                      "Cannot be equal: must have different holders");
            
            Method::DirectCallerStream reader(&super_method);

            GUARANTEE(!reader.has_next(), 
                      "Overridden method must not have direct callers");
#endif
          }
        }
      }
    }
  }
}
#endif

bool InstanceClass::compute_is_subtype_of(JavaClass* other_class) {
  if (other_class->is_interface()) {
    return equals(other_class) || itable_contains((InstanceClass*)other_class);
  } else {
    return is_subclass_of(other_class);
  }
}

bool InstanceClass::is_renamed() {
  Symbol::Raw n = name();
  if (n.equals(Symbols::unknown())) {
    return true;
  } else {
    return false;
  }
}

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR \
     ||ENABLE_JVMPI_PROFILE
// Returns the original set of fields before romizer has renamed them.
ReturnOop InstanceClass::original_fields() {
  ReturnOop orig = ROM::get_original_fields(this);
  if (orig != NULL) {
    return orig;
  } else {
    return fields();
  }
}

// Returns the original name that has been changed by romizer optimization
ReturnOop InstanceClass::original_name() {
  Symbol::Raw n = name();
  if (n.equals(Symbols::unknown())) {
    ClassInfo::Raw info = class_info();
    return ROM::get_original_class_name(&info);
  } else {
    return n.obj();
  }
}
#endif //!PRODUCT || USE_PRODUCT_BINARY_IMAGE_GENERATOR || ENABLE_JVMPI_PROFILE

#ifndef PRODUCT

void InstanceClass::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  st->print("InstanceClass ");
  print_name_on(st);
#endif
}

void InstanceClass::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  JavaClass::iterate(visitor);
  {
    NamedField id("next", true);
    visitor->do_oop(&id, next_offset(), true);
  }

  if (!is_fake_class()) {
#if !ENABLE_ISOLATES 
    iterate_static_fields(visitor);
#endif

    iterate_oop_maps(visitor);
    iterate_non_static_fields(visitor);
  }
#endif
}

void InstanceClass::iterate_oopmaps(oopmaps_doer do_map, void* param) {
#if USE_OOP_VISITOR
  JavaClass::iterate_oopmaps(do_map, param);
  OOPMAP_ENTRY_4(do_map, param, T_OBJECT, next);
#endif
}


void InstanceClass::iterate_static_fields(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  bool has_renamed_fields = (ROM::get_original_fields(this) != NULL);

  // Print static fields defined by local class.
  // Static fields are already sorted by offset.
  if (has_renamed_fields) {
    visitor->do_comment("Static fields (++: renamed fields)");
  } else {
    visitor->do_comment("Static fields");
  }

  TypeArray f = original_fields();
  int index=0;
  for(index = 0; index < f.length(); index += 5) {
    OriginalField f(this, index);
    if (f.is_static()) {
      Symbol name = f.name();
      SymbolField field(&name, false);
      field.set_renamed(f.is_renamed());

      // Note: 8- and 16-bit values are stored in sign-extended 32-bit
      // slots.
      switch(f.type()) {
      case T_BOOLEAN: visitor->do_uint(&field, f.offset(), true);   break;
      case T_CHAR:    visitor->do_uint(&field, f.offset(), true);   break;
      case T_FLOAT :  visitor->do_float(&field, f.offset(), true);  break;
      case T_DOUBLE:  visitor->do_double(&field, f.offset(), true); break;
      case T_BYTE:    visitor->do_int(&field, f.offset(), true);    break;
      case T_SHORT:   visitor->do_int(&field, f.offset(), true);    break;
      case T_INT:     visitor->do_int(&field, f.offset(), true);    break;
      case T_LONG:    visitor->do_long(&field, f.offset(), true);   break;
      case T_OBJECT:  visitor->do_oop(&field, f.offset(), true);    break;
      case T_ARRAY:   visitor->do_oop(&field, f.offset(), true);    break;
      }
    }
  }
#endif
}

void InstanceClass::iterate_oop_maps(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  size_t offset;
  NamedField id("delta (words)", true);
  jubyte map;
  
  // Print non-static oop map
  offset = first_nonstatic_map_offset();
  visitor->do_comment("Instance oop map");
  do {
    map = oop_map_at(offset);
    visitor->do_ubyte(&id, offset, true);
    offset++;
  } while (map != OopMapSentinel);
  
  // Print static oop map
  offset = first_static_map_offset();
  visitor->do_comment("Static oop map");
  do {
    map = oop_map_at(offset);
    visitor->do_ubyte(&id, offset, true);
    offset++;
  } while (map != OopMapSentinel);
#endif
}

void InstanceClass::iterate_non_static_fields(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  bool has_renamed_fields = (ROM::get_original_fields(this) != NULL);

  // Print non-static fields defined by instances of this class
  // Static fields are not sorted sorted by offset in fields, let's
  // sort them for better readability.
  if (has_renamed_fields) {
    visitor->do_comment("Non-static fields (++: renamed fields)");
  } else {
    visitor->do_comment("Non-static fields");
  }
  TypeArray f = original_fields();
  int last_field_offset = -1;
  for (;;) {
    int next_field_index = -1;
    int min_offset = 0x7fffffff;

    for (int index = 0; index < f.length(); index += 5) {
      OriginalField f(this, index);
      if (!f.is_static() &&
          f.offset() > last_field_offset && 
          f.offset() < min_offset) {
        next_field_index = index;
        min_offset = f.offset();
      }
    }
    
    last_field_offset = min_offset;
    if (next_field_index < 0) {
      break;
    }

    OriginalField f(this, next_field_index);
    Symbol name = f.name();
    SymbolField field(&name, false);
    field.set_renamed(f.is_renamed());

    switch(f.type()) {
    case T_BOOLEAN: visitor->do_bool(&field, f.offset(), false);   break;
    case T_CHAR:    visitor->do_char(&field, f.offset(), false);   break;
    case T_FLOAT :  visitor->do_float(&field, f.offset(), false);  break;
    case T_DOUBLE:  visitor->do_double(&field, f.offset(), false); break;
    case T_BYTE:    visitor->do_byte(&field, f.offset(), false);   break;
    case T_SHORT:   visitor->do_short(&field, f.offset(), false);  break;
    case T_INT:     visitor->do_int(&field, f.offset(), false);    break;
    case T_LONG:    visitor->do_long(&field, f.offset(), false);   break;
    case T_OBJECT:  visitor->do_oop(&field, f.offset(), false);    break;
    case T_ARRAY:   visitor->do_oop(&field, f.offset(), false);    break;
    }
  }
#endif
}

void InstanceClass::internal_error_hint() {
#if USE_DEBUG_PRINTING
  tty->print_cr("  Your classes.zip file probably contains .class files that");
  tty->print_cr("  were copied from a different version of the VM, or were");
  tty->print_cr("  compiled with different values of ENABLE flags. For this");
  tty->print_cr("  VM executable, you must have");
  tty->print_cr("  + ENABLE_CLDC_11 = %s", (ENABLE_CLDC_11 ? "true":"false"));
  tty->print_cr("  + ENABLE_FLOAT   = %s", (ENABLE_FLOAT   ? "true":"false"));
#endif
}

void InstanceClass::internal_class_error() {
#if USE_DEBUG_PRINTING
  tty->print("Failed to load internal class ");
  print_name_on(tty);
  tty->print_cr(":");
  internal_error_hint();
#endif
}

void InstanceClass::internal_field_error(Symbol *field_name, Symbol *field_sig)
{
#if USE_DEBUG_PRINTING
  tty->print("Failed to load internal class ");
  print_name_on(tty);
  tty->print_cr(":");
  tty->print("-> Fatal error in field ");
  field_name->print_symbol_on(tty);
  tty->print(":");
  field_sig->print_symbol_on(tty);
  tty->print_cr("");
  internal_error_hint();
#endif
}

void InstanceClass::verify_instance_size(size_t size) {
  // Verify that size matches the instance size
  InstanceSize i_size = instance_size();
  if (!i_size.is_fixed()) {
    internal_class_error();
    JVM_FATAL(internal_class_must_have_fixed_size);
  }
  if (size != i_size.fixed_value()) {
    internal_class_error();
    TTY_TRACE_CR(("Expected size of %d but got %d", 
                  size, i_size.fixed_value()));
    JVM_FATAL(internal_class_size_mismatch);
  }
}

void InstanceClass::verify_instance_field(const char* name, 
                                          const char* signature,
                                          jint field_offset) {
  verify_field(name, signature, field_offset, false);
}

void InstanceClass::verify_static_field(const char* name, 
                                        const char* signature, 
                                        jint field_offset) {
  verify_field(name, signature, field_offset, true);
}

// Verify that a field has the right offset.
void InstanceClass::verify_field(const char* name, const char* signature,
                                 jint field_offset, bool is_static) {
  if (!LoadROMDebugSymbols || !ENABLE_ROM_DEBUG_SYMBOLS) {
    return;
  }

  SETUP_ERROR_CHECKER_ARG;
  UsingFastOops fast_oops;
  Symbol::Fast field_name = SymbolTable::symbol_for(name JVM_NO_CHECK);
  Symbol::Fast field_signature = TypeSymbol::parse(signature JVM_NO_CHECK);

  // IMPL_NOTE: GUARANTEE(field_signature().no_fake_class(), 
  //                  "all classes must be loaded");

  if (field_name.not_null() && field_signature.not_null()) {
    OriginalField f(this, &field_name, &field_signature);
    if (!f.is_valid()) {
      internal_field_error(&field_name, &field_signature);
      JVM_FATAL(internal_field_must_be_valid);
    }
    if (!is_static && f.is_static()) {
      internal_field_error(&field_name, &field_signature);
      JVM_FATAL(internal_field_must_be_non_static);
    }
    if (is_static && !f.is_static()) {
      internal_field_error(&field_name, &field_signature);
      JVM_FATAL(internal_field_must_be_static);
    }
    if (f.offset() != field_offset) {
      internal_field_error(&field_name, &field_signature);
      TTY_TRACE_CR(("Expected offset of %d but got %d", 
                    field_offset, f.offset()));
      JVM_FATAL(internal_field_offset_mismatch);
    }
  } else {
    JVM_FATAL(allocation_cannot_fail_during_bootstrap);
  }
}


#endif //!PRODUCT

#if ENABLE_ROM_GENERATOR || !defined(PRODUCT)
SystemClassStream::SystemClassStream(bool do_all) {
  if (do_all) {
    _search_class_id = 0;
  } else {
    _search_class_id = _rom_number_of_java_classes;
  }
#ifdef AZZERT
  _found_class_id = -1;
#endif
}

bool SystemClassStream::has_next(bool only_optimizable, bool include_fake) {
  int max = Universe::number_of_java_classes();
  for (; _search_class_id < max; _search_class_id++) {
    JavaClass::Raw klass = Universe::class_from_id(_search_class_id);
    if (klass.not_null() && (!klass().is_fake_class() || include_fake) && 
        klass.is_instance_class() &&
        (!only_optimizable || klass().is_optimizable())) {
      _found_class_id = _search_class_id;
      _search_class_id ++;
      return true;
    }
  }

  return false;
}
#endif
