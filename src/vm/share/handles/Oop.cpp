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
# include "incls/_Oop.cpp.incl"

#ifdef AZZERT

Oop* last_raw_handle = NULL;

bool BasicOop::_disable_on_stack_check = false;

bool BasicOop::is_bootstrapping() {
  // Unfortunately, this can't be inlined because Universe.hpp has to
  // be declared after Oop.hpp.
  return Universe::is_bootstrapping();
}

void BasicOop::enable_on_stack_check() {
  _disable_on_stack_check = false;
}
void BasicOop::disable_on_stack_check() {
  _disable_on_stack_check = true;
}

void LinkedBasicOop::pre_push_handle_verification() {
  // Check this and top handle are not in object heap
  GUARANTEE(BasicOop::on_stack_check_disabled() ||
            !ObjectHeap::contains((OopDesc*) _last_handle),
            "handle should be on stack");
  GUARANTEE(BasicOop::on_stack_check_disabled() ||
            !ObjectHeap::contains((OopDesc*) this),
            "handle should be on stack");
#ifdef NOTCURRENTLYUSED
  GUARANTEE(!ObjectHeap::is_gc_active(),
            "shouldn't create handles during GC");
#endif
  // Make sure that there loops in the handle stack.
  // This could happen from bad usage of ::Fast.
  Oop* handle = _last_handle;
  for (int i = 0; handle != NULL && i < 5;
                  handle = handle->previous(), i++) {
    GUARANTEE((address)handle != (address)this, "Loop in handles");
  }
}

void LinkedBasicOop::pre_pop_handle_verification() {
  GUARANTEE(this == _last_handle, "wrong handle stack ordering");
  GUARANTEE(BasicOop::on_stack_check_disabled() ||
            !ObjectHeap::contains((OopDesc*) previous()),
            "handle should be on stack");
}

/* 
 * Verifies that this oop is a unique handle to its obj, i.e. no other Oop on
 * the chain refences its obj.
 */
void LinkedBasicOop::handle_uniqueness_verification() {
  if (not_null()) {
    ForAllHandles( handle ) {
      GUARANTEE(handle == this || handle->obj() != obj(), "Non-unique handle");
    }
  }
}

void UsingFastOops::pre_fast_oops_verification(void) const {
  GUARANTEE(BasicOop::on_stack_check_disabled() ||
            !ObjectHeap::contains((OopDesc*) _last_handle),
            "handle should be on stack");
  GUARANTEE(BasicOop::on_stack_check_disabled() ||
            !ObjectHeap::contains((OopDesc*) this),
            "handle should be on stack");
#ifdef NOTCURRENTLYUSED
  GUARANTEE(!ObjectHeap::is_gc_active(),
            "shouldn't create handles during GC");
#endif
}

void UsingFastOops::post_fast_oops_verification(void) const {
  int i = 0;
  ForAllHandles( handle ) {
    GUARANTEE(i < 100, "Sanity");
    if (handle == _previous) return;
    i++;
  }
  GUARANTEE(_previous == NULL, "handle not on stack");
}

#endif

#if ENABLE_OOP_TAG

int BasicOop::current_task_id() {
  return TaskContext::current_task_id();
}
int BasicOop::current_task_seq() {
  return Task::current_task_seq(Universe::current_task_id());
}
bool BasicOop::rom_contains(OopDesc *p) {
  return ROM::in_any_loaded_bundle(p);
}
#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

#if USE_OOP_VISITOR
class OopDispatcher : public StackObj {
 public:
  OopDispatcher(BasicOop* obj) {
    this->obj = (Oop*)obj;      // it's Oop for historical reasons.
  }
  void dispatch();

 private:
  Oop* obj;

  virtual void do_null() JVM_PURE_VIRTUAL;
  virtual void do_generic(Oop* obj, const char* type) {
    JVM_PURE_VIRTUAL_2_PARAM(obj, type);
  }
  virtual void do_java_oop(JavaOop* obj) {
    do_generic(obj, "Java Object");
  }
  virtual void do_instance(Instance* obj) {
    do_java_oop(obj);
  }
  virtual void do_type_array(TypeArray* obj) {
    do_java_oop(obj);
  }
  virtual void do_obj_array(ObjArray* obj) {
    do_java_oop(obj);
  }
  virtual void do_constant_pool(ConstantPool* obj) {
    do_generic(obj, "Constant Pool");
  }
  virtual void do_method(Method* obj) {
    do_generic(obj, "Method");
  }
  virtual void do_near(Near* obj) {
    do_generic(obj, "Near");
  }
  virtual void do_obj_near(ObjNear* obj) {
    do_near(obj);
  }
  virtual void do_java_near(JavaNear* obj) {
    do_near(obj);
  }
  virtual void do_symbol(Symbol* obj) {
    do_generic(obj, "Symbol");
  }
  virtual void do_far_class(FarClass* obj) {
    do_generic(obj, "Far Class");
  }
  virtual void do_far_class_class(FarClass* obj) {
    do_far_class(obj);
  }
  virtual void do_generic_near_class(NearClass* obj) {
    do_far_class(obj);
  }
  virtual void do_obj_near_class(NearClass* obj) {
    do_far_class(obj);
  }
  virtual void do_java_near_class(NearClass* obj) {
    do_far_class(obj);
  }

  virtual void do_symbol_class(FarClass* obj) {
    do_far_class(obj);
  }
  virtual void do_method_class(FarClass* obj) {
    do_far_class(obj);
  }
  virtual void do_java_class(JavaClass* obj) {
    do_far_class(obj);
  }
  virtual void do_instance_class(InstanceClass* obj) {
    do_java_class(obj);
  }
  virtual void do_array_class(ArrayClass* obj) {
    do_java_class(obj);
  }
  virtual void do_obj_array_class(ObjArrayClass* obj) {
    do_array_class(obj);
  }
  virtual void do_type_array_class(TypeArrayClass* obj) {
    do_array_class(obj);
  }
  virtual void do_constant_pool_class(FarClass* obj) {
    do_far_class(obj);
  }
  virtual void do_instance_class_class(FarClass* obj) {
    do_far_class_class(obj);
  }
  virtual void do_type_array_class_class(FarClass* obj) {
    do_far_class_class(obj);
  }
  virtual void do_obj_array_class_class(FarClass* obj) {
    do_far_class_class(obj);
  }
  virtual void do_meta_class(FarClass* obj) {
    do_far_class_class(obj);
  }
  virtual void do_string(String* obj) {
    do_instance(obj);
  }
  virtual void do_java_lang_Class(JavaClassObj* obj) {
    do_instance(obj);
  }
#if USE_COMPILER_STRUCTURES
  virtual void do_compiled_method(CompiledMethod* obj) {
    do_generic(obj, "Compiled Method");
  }
#endif
#if 0 && ENABLE_COMPILER
  virtual void do_entry(Entry* obj) {
    do_generic(obj, "Entry");
  }
#endif
  virtual void do_mixed_oop(MixedOop* obj) {
    do_generic(obj, obj->type_string());
  }
  virtual void do_compiled_method_class(FarClass* obj) {
    do_far_class(obj);
  }
  virtual void do_entry_activation(EntryActivation* obj) {
    do_generic(obj, "Entry Activation");
  }
  virtual void do_execution_stack(ExecutionStack* obj) {
    do_generic(obj, "Execution stack");
  }
  virtual void do_class_info(ClassInfo* obj) {
    do_generic(obj, "Class Info");
  }
#if ENABLE_ISOLATES
  virtual void do_task(Task* obj) {
    do_generic(obj, "Task");
  }
  virtual void do_task_mirror(TaskMirror* obj) {
    do_generic(obj, "Task Mirror");
  }
#endif
  virtual void do_stackmap_list(StackmapList* obj) {
    do_generic(obj, "StackmapList");
  }
  virtual void do_thread(Thread* obj) {
    do_generic(obj, "Thread");
  }
};

void OopDispatcher::dispatch() {
  if (obj->is_null()) {
    do_null();
    return;
  }

  FarClass fc = obj->blueprint();
  switch(fc.instance_size_as_jint()) {
  default:
    // object is an instance. Some are treated as special cases.
    if (fc.instance_size_as_jint() > 0) {
      if (fc.equals(Universe::string_class())) {
         String it = obj;
         do_string(&it);
      } else if (fc.equals(Universe::java_lang_Class_class())) {
        JavaClassObj it = obj;
        do_java_lang_Class(&it);
      } else {
        Instance it = obj;
        do_instance(&it);
      }
    } else {
      do_generic(obj, "Random heap object");
    }
    return;

  case InstanceSize::size_obj_array:
    { ObjArray it = obj;  do_obj_array(&it); return; }

  case InstanceSize::size_type_array_1:
  case InstanceSize::size_type_array_2:
  case InstanceSize::size_type_array_4:
  case InstanceSize::size_type_array_8:
    { TypeArray it = obj; do_type_array(&it); return;}


  case InstanceSize::size_method:
    { Method it = obj; do_method(&it); return; }

#if USE_COMPILER_STRUCTURES
  case InstanceSize::size_compiled_method:
    { CompiledMethod it = obj; do_compiled_method(&it); return; }
#endif
  case InstanceSize::size_mixed_oop:
    { MixedOop it = obj; do_mixed_oop(&it); return; }

  case InstanceSize::size_constant_pool:
    { ConstantPool it = obj; do_constant_pool(&it); return; }

  case InstanceSize::size_symbol:
    { Symbol it = obj; do_symbol(&it); return; }

  case InstanceSize::size_entry_activation:
    { EntryActivation it = obj; do_entry_activation(&it); return; }

  case InstanceSize::size_execution_stack:
    { ExecutionStack it = obj; do_execution_stack(&it); return; }

  case InstanceSize::size_class_info:
    { ClassInfo it = obj; do_class_info(&it); return; }

  case InstanceSize::size_stackmap_list:
    { StackmapList it = obj; do_stackmap_list(&it); return; }

  case InstanceSize::size_instance_class:
    { InstanceClass it = obj; do_instance_class(&it); return; }

  case InstanceSize::size_obj_array_class:
    { ObjArrayClass it = obj; do_obj_array_class(&it); return; }

  case InstanceSize::size_type_array_class:
    { TypeArrayClass it = obj;  do_type_array_class(&it); return; }

  case InstanceSize::size_generic_near:
    { Near it = obj; do_near(&it); return; }

  case InstanceSize::size_java_near:
    { JavaNear it = obj; do_java_near(&it); return; }

  case InstanceSize::size_obj_near:
    { ObjNear it = obj; do_obj_near(&it); return; }

#if ENABLE_ISOLATES
  case InstanceSize::size_task_mirror:
    { TaskMirror it = obj; do_task_mirror(&it); return; }

  case InstanceSize::size_boundary:
    { do_generic(obj, "allocation boundary"); return; }
#endif

  case InstanceSize::size_far_class: {
    // The meta class is the blueprint for lots of individual classes.  But
    // we can tell what we have by looking at their own instance size field.
    FarClass me = obj;
    switch(me.instance_size_as_jint()) {
      case InstanceSize::size_compiled_method:
        { FarClass it = obj; do_compiled_method_class(&it); return; }
      case InstanceSize::size_method:
        { FarClass it = obj; do_method_class(&it); return; }
      case InstanceSize::size_constant_pool:
        { FarClass it = obj; do_constant_pool_class(&it); return; }
      case InstanceSize::size_symbol:
        { FarClass it = obj;  do_symbol_class(&it); return; }
      case InstanceSize::size_instance_class:
        { FarClass it = obj; do_instance_class_class(&it); return; }
      case InstanceSize::size_type_array_class:
        { FarClass it = obj; do_type_array_class_class(&it); return; }
      case InstanceSize::size_obj_array_class:
        { FarClass it = obj; do_obj_array_class_class(&it); return; }
      case InstanceSize::size_generic_near:
        { NearClass it = obj; do_generic_near_class(&it); return; }
      case InstanceSize::size_java_near:
        { NearClass it = obj; do_java_near_class(&it); return; }
      case InstanceSize::size_obj_near:
        { NearClass it = obj; do_obj_near_class(&it); return; }
      case InstanceSize::size_far_class:
        // Object is its own meta class. . .
#if ENABLE_HEAP_NEARS_IN_HEAP
        GUARANTEE(me.equals(Universe::meta_class()) ||
            me.equals(Universe::rom_meta_class()), "Sanity");
#else
        GUARANTEE(me.equals(Universe::meta_class()), "Sanity");
#endif
        { FarClass it = obj; do_meta_class(&it); return; }
      default:
        { FarClass it = obj; do_far_class(&it); return; }
    }
  }

  }
}
#endif

#if USE_DEBUG_PRINTING
class PrintDispatcher : public OopDispatcher {
 public:
  PrintDispatcher(BasicOop* obj, Stream* st) : OopDispatcher(obj) {
    this->st = st;
  }

 private:
  Stream* st;
  virtual void do_null() {
    st->print("null");
  }
  virtual void do_generic(Oop* obj, const char *type) {
    (void)obj;
    st->print("%s", type);
  }
  virtual void do_instance(Instance* obj) {
    obj->print_value_on(st);
  }
  virtual void do_type_array(TypeArray* obj) {
    obj->print_value_on(st);
  }
  virtual void do_obj_array(ObjArray* obj) {
    obj->print_value_on(st);
  }
  virtual void do_constant_pool(ConstantPool* obj) {
    obj->print_value_on(st);
  }
  virtual void do_method(Method* obj) {
    st->print("Method ");
    obj->print_name_on(st);
  }
  virtual void do_symbol(Symbol* obj) {
    obj->print_value_on(st);
  }
  virtual void do_symbol_class(FarClass* obj) {
    do_generic(obj, "The Symbol Class");
  }
  virtual void do_method_class(FarClass* obj) {
    do_generic(obj, "The Method Class");;
  }
  virtual void do_instance_class(InstanceClass* obj) {
    obj->print_value_on(st);
  }
  virtual void do_obj_array_class(ObjArrayClass* obj) {
    obj->print_value_on(st);
  }
  virtual void do_type_array_class(TypeArrayClass* obj) {
    obj->print_value_on(st);
  }
  virtual void do_constant_pool_class(FarClass* obj) {
    do_generic(obj, "The ConstantPool Class");
  }
  virtual void do_near(Near* obj) {
    obj->print_value_on(st);
  }
  virtual void do_obj_near(ObjNear* obj) {
    obj->print_value_on(st);
  }
  virtual void do_java_near(JavaNear* obj) {
    obj->print_value_on(st);
  }
  virtual void do_generic_near_class(NearClass* obj) {
    obj->print_value_on(st);
  }
  virtual void do_java_near_class(NearClass* obj) {
    obj->print_value_on(st);
  }
  virtual void do_obj_near_class(NearClass* obj) {
    obj->print_value_on(st);
  }
  virtual void do_instance_class_class(FarClass* obj) {
    do_generic(obj, "The Instance ClassClass");
  }
  virtual void do_type_array_class_class(FarClass* obj) {
    do_generic(obj, "The Type Array ClassClass");
  }
  virtual void do_obj_array_class_class(FarClass* obj) {
    do_generic(obj, "The Object Array ClassClass");
  }
  virtual void do_meta_class(FarClass* obj) {
    do_generic(obj, "The MetaClass");
  }
  virtual void do_string(String* obj) {
    obj->print_value_on(st);
  }
  virtual void do_java_lang_Class(JavaClassObj* obj) {
    obj->print_value_on(st);
  }
#if USE_COMPILER_STRUCTURES
  virtual void do_compiled_method(CompiledMethod* obj) {
    obj->print_value_on(st);
  }
#endif
  virtual void do_compiled_method_class(FarClass* obj) {
    do_generic(obj, "The CompiledMethod Class");
  }
  virtual void do_entry_activation(EntryActivation* obj) {
    obj->print_value_on(st);
  }
  virtual void do_execution_stack(ExecutionStack* obj) {
    obj->print_value_on(st);
  }
  virtual void do_class_info(ClassInfo* obj) {
    obj->print_value_on(st);
  }
};
#endif // USE_DEBUG_PRINTING

void BasicOop::print_value_on(Stream* st) {
#if USE_DEBUG_PRINTING
  DebugHandleMarker debug_handle_marker;

  if (is_null()) {
    st->print("null");
    return;
  }
  if (Verbose && VerbosePointers) {
    st->print("(0x%lx) ", obj());
  }
  if (!check_valid_for_print(st)) {
    return;
  }
  if (klass() == NULL) {
    st->print("Unknown object (klass == NULL)");
    return;
  }
  if (blueprint() == NULL) {
    st->print("Unknown object (blueprint == NULL)");
    return;
  }
  PrintDispatcher pd(this, st);
  pd.dispatch();
#endif
}

bool BasicOop::check_valid_for_print(Stream* st) {
  if (!ObjectHeap::contains_live(obj()) && !ROM::in_any_loaded_bundle(obj())) {
    st->print("[Invalid object 0x%08x]", obj());
    return false;
  } else {
    return true;
  }
}

#endif

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

void BasicOop::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  // klass
  if (ROM::system_text_contains(obj())) {
    // Don't have a header, let's just print garbage
    NamedField id("klass (skipped)", true);
    visitor->do_int(&id, klass_offset(), true);
  } else {
    NamedField id("klass", true);
    visitor->do_oop(&id, klass_offset(), true);
  }
#endif
}

void BasicOop::iterate_one_oopmap_entry(BasicType type, void *param,
                                        const char *name, size_t offset, 
                                        int flags)
{
#if USE_OOP_VISITOR
  (void)flags;
  OopVisitor* visitor = (OopVisitor*)param;
  NamedField id(name, true);

  switch (type) {
  case T_BOOLEAN:
    visitor->do_bool(&id, offset, true);
    break;
  case T_CHAR:
    visitor->do_byte(&id, offset, true);
    break;
  case T_FLOAT:
    visitor->do_float(&id, offset, true);
    break;
  case T_DOUBLE:
    visitor->do_double(&id, offset, true);
    break;
  case T_BYTE:
    visitor->do_byte(&id, offset, true);
    break;
  case T_SHORT:
    visitor->do_short(&id, offset, true);
    break;
  case T_INT:
    visitor->do_int(&id, offset, true);
    break;
  case T_LONG:
    visitor->do_long(&id, offset, true);
    break;
  case T_OBJECT:
    visitor->do_oop(&id, offset, true);
    break;
  }
#endif
}

bool BasicOop::is_boundary() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_boundary();
}

#endif

#ifndef PRODUCT

#if USE_OOP_VISITOR
class VisitDispatcher : public OopDispatcher {
 public:
  VisitDispatcher(BasicOop* obj, OopVisitor* visitor) : OopDispatcher(obj) {
    this->_visitor = visitor;
  }

 private:
  OopVisitor* _visitor;
  virtual void do_null() { /* do nothing */ }
  virtual void do_generic(Oop* obj, const char* type) {
    (void)obj; (void)type;
    /* do nothing */
  }
  virtual void do_far_class(FarClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_instance(Instance* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_type_array(TypeArray* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_obj_array(ObjArray* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_constant_pool(ConstantPool* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_method(Method* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_near(Near* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_obj_near(ObjNear* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_java_near(JavaNear* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_generic_near_class(NearClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_obj_near_class(NearClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_java_near_class(NearClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_symbol(Symbol* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_symbol_class(FarClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_method_class(FarClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_instance_class(InstanceClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_obj_array_class(ObjArrayClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_type_array_class(TypeArrayClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_constant_pool_class(FarClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_instance_class_class(FarClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_type_array_class_class(FarClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_obj_array_class_class(FarClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_meta_class(FarClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_string(String* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_java_lang_Class(JavaClassObj* obj) {
    obj->iterate(_visitor);
  }
#if USE_COMPILER_STRUCTURES
  virtual void do_compiled_method(CompiledMethod* obj) {
    obj->iterate(_visitor);
  }
#endif
#if 0 && ENABLE_COMPILER
  virtual void do_entry(Entry* obj) {
    obj->iterate(_visitor);
  }
#endif
  virtual void do_mixed_oop(MixedOop* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_compiled_method_class(FarClass* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_entry_activation(EntryActivation* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_execution_stack(ExecutionStack* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_class_info(ClassInfo* obj) {
    obj->iterate(_visitor);
  }
#if ENABLE_ISOLATES
  virtual void do_task_mirror(TaskMirror* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_task(Task* obj) {
    obj->iterate(_visitor);
  }
#endif
  virtual void do_stackmap_list(StackmapList* obj) {
    obj->iterate(_visitor);
  }
  virtual void do_thread(Thread* obj) {
    obj->iterate(_visitor);
  }
};
#endif

void BasicOop::visit(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop object = obj();
  visitor->set_obj(&object);
  visitor->prologue();

  VisitDispatcher vd(this, visitor);
  vd.dispatch();

  visitor->epilogue();
#endif
}

/*
 * This is used by ROMWriter.cpp to print the definition of an object. It's
 * the same as BasicOop::print_value_on, except an extra ":" character is
 * printed to aid searching in a text editor. E.g., if you see a reference
 * to an object, it's printed as
 *
 *     (0x4a00e8) The MetaClass
 *
 * and the definition of this object is printed as
 *
 *     (0x4a00e8:) The MetaClass
 */
void BasicOop::print_rom_definition_on(Stream* st) {
#if USE_DEBUG_PRINTING
  DebugHandleMarker debug_handle_marker;
  if (is_null()) {
    st->print("null");
    return;
  }
  if (!check_valid_for_print(st)) {
    return;
  }
  if (Verbose && VerbosePointers) {
    st->print("(0x%lx:) ", obj());
  }
  PrintDispatcher pd(this, st);
  pd.dispatch();

  if (VerboseROMComments) {
    if (is_method() || is_class_info() || is_java_class() || is_java_oop() ||
        is_task_mirror() || obj()->is_compiled_method()) {
      st->cr();
      OopPrinter printer(st);
      Oop::Raw object = obj();
      printer.set_obj(&object);
      VisitDispatcher vd(this, &printer);
      vd.dispatch();
    }
    if (is_method()) {
      ((Method*)this)->print_bytecodes(st);
    } 
#if ENABLE_COMPILER
    else if (obj()->is_compiled_method()) {
      Method m = ((CompiledMethod*)this)->method();
      st->print_cr("[++++++++ Java Method ++++++++]");
      m.print_on(st);
      st->print_cr("[++++++++ Java Bytecodes ++++++++]");
      m.print_bytecodes(st);
      st->print_cr("[++++++++ Generated Code ++++++++]");
      ((CompiledMethod*)this)->print_code_on(st);
      st->cr();
      ((CompiledMethod*)this)->print_relocation_on(st);
    }
#endif
  }
#endif
}


void BasicOop::print() {
  print_on(tty);
}

void BasicOop::p() {
  print_on(tty);
}

void BasicOop::print_on(Stream* st) {
#if USE_DEBUG_PRINTING
  DebugHandleMarker debug_handle_marker;
  OopPrinter printer(st);
  visit(&printer);
#endif
}

bool LinkedBasicOop::is_persistent() const {
  return Universe::is_persistent_handle((Oop*) this);
}

void LinkedBasicOop::verify_basic_properties() {
#if !ENABLE_OOP_TAG
  GUARANTEE(sizeof(OopDesc) == sizeof(OopDesc*),
    "OopDesc should only contain near class pointer (no C++ virtuals)");
#endif
  GUARANTEE(sizeof(LinkedBasicOop) == sizeof(OopDesc*) + sizeof(Oop*),
    "Oop should only contain object pointer and previous link (no C++ virtuals)");
  GUARANTEE(FIELD_OFFSET(LinkedBasicOop, _previous) == sizeof(OopDesc*),
    "Object pointer field must be first in handle (meaning previous must be last)");
}

#endif // !defined(PRODUCT);

bool BasicOop::is_instance_class() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_instance_class();
}

bool BasicOop::is_obj_array_class() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_obj_array_class();
}

bool BasicOop::is_type_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_type_array();
}

bool BasicOop::is_obj_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_obj_array();
}

bool BasicOop::is_type_array_class() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
   return obj()->is_type_array_class();
}

bool BasicOop::is_symbol() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_symbol();
}

bool BasicOop::is_string() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_string();
}

bool BasicOop::is_char_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_char_array();
}

bool BasicOop::is_method() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_method();
}

bool BasicOop::is_stackmap_list() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_stackmap_list();
}

bool BasicOop::is_short_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_short_array();
}

bool BasicOop::is_constant_pool() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_constant_pool();
}

bool BasicOop::is_byte_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_byte_array();
}

bool BasicOop::is_obj_near() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_obj_near();
}

bool BasicOop::is_int_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_int_array();
}

bool BasicOop::is_class_info() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_class_info();
}

bool BasicOop::is_task_mirror() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_task_mirror();
}

bool BasicOop::is_thread() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_thread();
}

bool BasicOop::is_jvm_thread() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_jvm_thread();
}

bool BasicOop::is_compiled_method() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_compiled_method();
}

bool BasicOop::is_instance() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_instance();
}

bool BasicOop::is_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_type_array() || obj()->is_obj_array();
}

bool BasicOop::is_inflater() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_inflater();
}

bool BasicOop::is_jar_file_parser() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_jar_file_parser();
}

bool BasicOop::is_java_near() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_java_near();
}

bool BasicOop::is_java_class() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return (obj()->is_instance_class() ||
          obj()->is_type_array_class()  ||
          obj()->is_obj_array_class());
}

#if !defined(PRODUCT) || ENABLE_TTY_TRACE

bool BasicOop::is_execution_stack() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_execution_stack();
}

#endif

#ifndef PRODUCT
bool BasicOop::is_bool_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_bool_array();
}

bool BasicOop::is_long_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_long_array();
}

bool BasicOop::is_float_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_float_array();
}
bool BasicOop::is_double_array() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_double_array();
}

bool BasicOop::is_throwable() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return (obj()->is_instance() && obj()->is_throwable());
}

bool BasicOop::is_mixed_oop() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_mixed_oop();
}

bool BasicOop::is_task() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_task();
}

bool BasicOop::is_java_oop() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_instance() || obj()->is_type_array()
      || obj()->is_obj_array();
}

bool BasicOop::is_entry_activation() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_entry_activation();
}

bool BasicOop::is_file_decoder() const {
  GUARANTEE(not_null(), "Cannot ask for type of NULL");
  return obj()->is_file_decoder();
}

ReturnOop BasicOop::klass() const {
  if (UseROM && ROM::system_text_contains(obj())) {
    return ROM::text_klass_of(_obj);
  }
  return obj_field(klass_offset());
}

bool BasicOop::check_valid_for_print_cr(Stream* st) {
  if (!check_valid_for_print(st)) {
    st->cr();
    return false;
  } else {
    return true;
  }
}
#endif // PRODUCT

#if ENABLE_ROM_GENERATOR

int BasicOop::generate_fieldmap(TypeArray* field_map) {
  if (is_null()) {
    GUARANTEE(false, "Invalid invocation of generate_fieldmap");
    return 0;
  }

  FarClass fc =  blueprint();
  switch(fc.instance_size_as_jint()) {
  case InstanceSize::size_obj_array:
    { ObjArray it = _obj; return it.generate_fieldmap(field_map);}
  case InstanceSize::size_type_array_1:
  case InstanceSize::size_type_array_2:
  case InstanceSize::size_type_array_4:
  case InstanceSize::size_type_array_8:
    { TypeArray it = _obj; return it.generate_fieldmap(field_map);}

  case InstanceSize::size_method:
    { Method it = _obj; return it.generate_fieldmap(field_map); }

    case InstanceSize::size_constant_pool:
    { ConstantPool it = _obj; return it.generate_fieldmap(field_map); }

  case InstanceSize::size_symbol:
    { Symbol it = _obj; return it.generate_fieldmap(field_map); }

  case InstanceSize::size_class_info:
    { ClassInfo it = _obj; return it.generate_fieldmap(field_map); }

  case InstanceSize::size_stackmap_list:
    {StackmapList it = _obj; return it.generate_fieldmap(field_map);}

  case InstanceSize::size_type_array_class:
  case InstanceSize::size_obj_array_class:
  case InstanceSize::size_instance_class:
    { JavaClass it = _obj; return it.generate_fieldmap(field_map); }

  case InstanceSize::size_generic_near:
    { Near it = _obj; return it.generate_fieldmap(field_map); }

  case InstanceSize::size_java_near:
    { JavaNear it = _obj; return it.generate_fieldmap(field_map); }

  case InstanceSize::size_obj_near:
    { ObjNear it = _obj; return it.generate_fieldmap(field_map); }

  case InstanceSize::size_far_class:
    { FarClass it = _obj; return it.generate_fieldmap(field_map); }

#if ENABLE_COMPILER
  case InstanceSize::size_compiled_method:
    { CompiledMethod it = _obj; return it.generate_fieldmap(field_map); }
#endif

#if ENABLE_ISOLATES
  case InstanceSize::size_task_mirror:
    { TaskMirror it = _obj; return it.generate_fieldmap(field_map); }
#endif

  //default:
  //  SHOULD_NOT_REACH_HERE();??
  }
  return 0;
}

#endif /* #if ENABLE_ROM_GENERATOR */
