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
# include "incls/_Universe.cpp.incl"

OopDesc* persistent_handles[Universe::__number_of_persistent_handles];

bool Universe::_is_compilation_allowed = true;
int  Universe::_compilation_abstinence_ticks = 0;
bool Universe::_is_bootstrapping  = true;
bool Universe::_before_main       = true;
bool Universe::_is_stopping       = false;

#if ENABLE_JVMPI_PROFILE
jint Universe::_number_of_java_methods = 0;
#endif

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
int  Universe::_profile_id = DEFAULT_PROFILE_ID; 


int Universe::current_profile_id() {
#if ENABLE_ISOLATES
  Task::Raw task = Task::current();
  GUARANTEE(task.not_null(), "Sanity");
  return task().profile_id();   
#else
  return profile_id();
#endif
} 

void Universe::set_profile_id(const int id) { 
  GUARANTEE((id >= 0) && (id < ROM::profiles_count()), "Sanity");  
  _profile_id = id; 
} 

int Universe::profile_id_by_name(const char * profile) {
  const char ** profiles_names = ROM::profiles_names();
  for (int i = 0; i < ROM::profiles_count(); i++) {
    if (jvm_strcmp(profile, profiles_names[i]) == 0) {            
      return i;
    }
  }  
  return DEFAULT_PROFILE_ID;
}

#if USE_SOURCE_IMAGE_GENERATOR
ReturnOop Universe::new_profile(JVM_SINGLE_ARG_TRAPS) {
  return new_mixed_oop(MixedOopDesc::Type_ROMProfile,
                       ROMProfileDesc::allocation_size(),
                       ROMProfileDesc::pointer_count()
                       JVM_NO_CHECK_AT_BOTTOM);
}

ReturnOop Universe::new_vector(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fastOops;
  ROMVector::Fast rom_vector = new_mixed_oop(MixedOopDesc::Type_ROMVector,
                                             ROMVectorDesc::allocation_size(),
                                             ROMVectorDesc::pointer_count()
                                             JVM_CHECK_0);
  rom_vector().initialize(JVM_SINGLE_ARG_CHECK_0);
  return rom_vector;
}
#endif // USE_SOURCE_IMAGE_GENERATOR
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
bool Universe::name_matches_pattern(const char* name, int name_len, 
                                    const char* pattern, int pattern_len) {

  GUARANTEE(name_len > 0 && pattern_len > 0, "Sanity");

  int p_len = pattern_len - 2;

  if (pattern_len > 1 && 
      jvm_strncmp(&pattern[p_len], "/*", 2) == 0) {
    if (jvm_strncmp(pattern, name, p_len) == 0) {
      if(name_len == p_len) return true;
      else if(name_len < p_len) return false;
      else if (jvm_strncmp(name+(p_len), "/", 1) == 0) {
        return true;
      }
    }
  }

  if (name_len < pattern_len) {
    return false;
  }

  if ((jvm_strncmp(pattern, name, pattern_len) == 0) && 
       pattern_len == name_len) {
    return true;
  }

  return false;
}


#if ENABLE_DYNAMIC_NATIVE_METHODS
int Universe::dynamic_lib_count = 0;
#endif

#if ENABLE_ISOLATES

OopDesc* Universe::_raw_task_list;
#endif

ReturnOop Universe::setup_classpath(TypeArray* classpath JVM_TRAPS) {
  const jchar path_separator = (jchar)OsFile_path_separator_char;
  int len = classpath->length();

  UsingFastOops fast_oops;
  ObjArray::Fast result;
  TypeArray::Fast path;
  int nof_paths = 0;

  for (int pass=0; pass<2; pass++) {
    int end = 0;
    int start;

    if (pass == 1) {
      int length = nof_paths;
      result = new_obj_array(length JVM_CHECK_0);
    }

    // Compute nof_paths
    for (nof_paths = 0, start = 0; start < len; start = end) {
      while (end < len && classpath->char_at(end) != path_separator) {
        end++;
      }

      if (pass == 1) {
        path = Universe::new_char_array(end - start JVM_CHECK_0);
        TypeArray::array_copy(classpath, start, &path, 0, end - start);
        result().obj_at_put(nof_paths, &path);
      }

      nof_paths++;

      if (end == len) { 
        break; 
      }
      while (end < len && classpath->char_at(end) == path_separator) {
        end++;
      }
    }
  }

  return result;
}

#if !ROMIZED_PRODUCT
void Universe::load_root_class(InstanceClass* ic, Symbol* class_name) {
  GUARANTEE(!UseROM, "root class should already be loaded in ROM");
  SETUP_ERROR_CHECKER_ARG;
  *ic = SystemDictionary::resolve(class_name, ErrorOnFailure JVM_NO_CHECK);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    JVM_FATAL(root_class_loading_failure);
  }
}
#endif

#if !ROMIZED_PRODUCT

// Structure for allowing iteration over array classes.
// Needed to avoid code replication.

struct ArrayInfo {
  jubyte           _type;
  jubyte           _size;
  jbyte            _instance_size;  // a small negative number
  char             _name_char;

  jint size           (void) const { return _size; }
  jint instance_size  (void) const { return _instance_size; }
  char name_char      (void) const { return _name_char; }
  BasicType basic_type(void) const { return (BasicType)_type; }

  TypeArrayClass* type_array_class(void) const{
    return Universe::as_TypeArrayClass((BasicType)_type);
  }
  bool at_end(void) const { return _size == 0; }
};

static ArrayInfo array_table[] = {
  { T_BOOLEAN, sizeof(jboolean), InstanceSize::size_type_array_1, 'Z' },
  { T_CHAR,    sizeof(jchar),    InstanceSize::size_type_array_2, 'C' },
  { T_FLOAT,   sizeof(jfloat),   InstanceSize::size_type_array_4, 'F' },
  { T_DOUBLE,  sizeof(jdouble),  InstanceSize::size_type_array_8, 'D' },
  { T_BYTE,    sizeof(jbyte),    InstanceSize::size_type_array_1, 'B' },
  { T_SHORT,   sizeof(jshort),   InstanceSize::size_type_array_2, 'S' },
  { T_INT,     sizeof(jint),     InstanceSize::size_type_array_4, 'I' },
  { T_LONG,    sizeof(jlong),    InstanceSize::size_type_array_8, 'J' },
  { T_ILLEGAL, 0,                0,                                0  }
};

#endif // !ROMIZED_PRODUCT

#if !ROMIZED_PRODUCT

struct MetaClassStruct {
  jushort  index;
  jubyte   object_size;     // Is it always sizeof(FarClassDesc)??
  jbyte    instance_size;   // Small negative number
  const jubyte* extern_oop_map;
};

inline void Universe::create_meta(JVM_SINGLE_ARG_TRAPS) {
  static const MetaClassStruct MetaClasses [] = {
    { constant_pool_class_index, sizeof(FarClassDesc),
      InstanceSize::size_constant_pool, oopmap_ConstantPool },
    { entry_activation_class_index, sizeof(FarClassDesc),
      InstanceSize::size_entry_activation, oopmap_EntryActivation },
    { mixed_oop_class_index, sizeof(FarClassDesc),
      InstanceSize::size_mixed_oop, oopmap_Empty },
    { class_info_class_index, sizeof(FarClassDesc),
      InstanceSize::size_class_info, oopmap_ClassInfo },
    { execution_stack_class_index, sizeof(FarClassDesc),
      InstanceSize::size_execution_stack, oopmap_Empty },
    { stackmap_list_class_index, sizeof(FarClassDesc),
      InstanceSize::size_stackmap_list, oopmap_StackmapList },
    { compiled_method_class_index, sizeof(FarClassDesc),
      InstanceSize::size_compiled_method, oopmap_CompiledMethod },
    { symbol_class_index, sizeof(FarClassDesc),
      InstanceSize::size_symbol, oopmap_Empty },
#if ENABLE_JAVA_DEBUGGER
    { refnode_class_index, sizeof(FarClassDesc),
      InstanceSize::size_refnode, oopmap_Empty },
#endif
#if ENABLE_ISOLATES
    { boundary_class_index, sizeof(FarClassDesc),
      InstanceSize::size_boundary, oopmap_Boundary },
    { task_mirror_class_index, sizeof(FarClassDesc),
      InstanceSize::size_task_mirror, oopmap_TaskMirror }
#endif
  };

  static const MetaClassStruct JavaMetaClasses [] = {
    { type_array_class_class_index, sizeof(FarClassDesc),
      InstanceSize::size_type_array_class, oopmap_ArrayClass},
    { obj_array_class_class_index, sizeof(FarClassDesc),
      InstanceSize::size_obj_array_class, oopmap_ArrayClass},
    { instance_class_class_index, sizeof(FarClassDesc),
      InstanceSize::size_instance_class, oopmap_InstanceClass}
  };

  static const MetaClassStruct ObjMetaClasses [] = {
    { method_class_index, sizeof(FarClassDesc),
      InstanceSize::size_method, oopmap_Method }
  };

  GCDisabler cant_gc_while_creating_hierarchy;

  // Create the meta class hierarchy
  {
    FarClass::Raw result =
        ObjectHeap::allocate(sizeof(FarClassDesc) JVM_NO_CHECK);
    ((FarClassDesc*)result.obj())->initialize(result,
                                              sizeof(FarClassDesc),
                                              InstanceSize::size_far_class,
                                              (jubyte*)oopmap_FarClass);
    *meta_class() = result;
  }

  /* There are three different classes of objects whose blueprint is the
   * meta_class(),
   *
   * 1) Those whose prototypical near is a  Near Object
   * 2) Those whose prototypical near is a  JavaNearClass
   * 3) Those whose prototypical near is an ObjectNear
   */

  // Create the metaclasses with simple near objects
  {
     NearClass::Raw near_class =
         generic_allocate_near_class(meta_class(),
                                     NearClassDesc::allocation_size(),
                                     InstanceSize::size_generic_near,
                                     (jubyte*)oopmap_Empty JVM_NO_CHECK);
     for (int i = 0; i < ARRAY_SIZE(MetaClasses); i++) {
       const MetaClassStruct *p = &MetaClasses[i];

       FarClass::Raw result = ObjectHeap::allocate(p->object_size  JVM_NO_CHECK);
       ((FarClassDesc*)result.obj())->initialize(near_class,
                                                 p->object_size,
                                                 p->instance_size,
                                                 (jubyte*)p->extern_oop_map);
       persistent_handles[p->index] = result;

       // Allocate a prototypical near
       JavaNear::Raw this_near = allocate_near(&result JVM_NO_CHECK);
       result().set_prototypical_near(&this_near);
     }
  }

  // Create the metaclasses whose prototypical near is a JavaNearClass
  {
    NearClass::Raw near_class = meta_class();
    for (int i = 0; i < ARRAY_SIZE(JavaMetaClasses); i++) {
      const MetaClassStruct *p = &JavaMetaClasses[i];

      FarClass::Raw result = ObjectHeap::allocate(p->object_size JVM_NO_CHECK);
      ((FarClassDesc*)result.obj())->initialize(near_class,
                                                p->object_size,
                                                p->instance_size,
                                                (jubyte*)p->extern_oop_map);
      persistent_handles[p->index] = result;

      // Allocate a prototypical near
      JavaNear::Raw this_near =
          generic_allocate_near_class(&result, 
                                      NearClassDesc::allocation_size(),
                                      InstanceSize::size_java_near,
                                      // IMPL_NOTE:  Create its own oopmap
                                      (jubyte*)oopmap_ObjNear
                                      JVM_NO_CHECK);
      result().set_prototypical_near(&this_near);
    }
  }

  {
    NearClass::Raw near_class =
        generic_allocate_near_class(meta_class(),
                                    NearClassDesc::allocation_size(),
                                    InstanceSize::size_obj_near,
                                    (jubyte*)oopmap_ObjNear JVM_NO_CHECK);
    for (int i = 0; i < ARRAY_SIZE(ObjMetaClasses); i++) {
      const MetaClassStruct *p = &ObjMetaClasses[i];

      FarClass::Raw result = ObjectHeap::allocate(p->object_size JVM_NO_CHECK);
      ((FarClassDesc*)(result.obj()))->initialize(near_class,
                                                  p->object_size,
                                                  p->instance_size,
                                                  (jubyte*)p->extern_oop_map);
      persistent_handles[p->index] = result;

      // Allocate a prototypical near
      JavaNear::Raw this_near = allocate_obj_near(&result JVM_NO_CHECK);
      result().set_prototypical_near(&this_near);
    }
  }

#if ENABLE_ISOLATES
  // Class initialization marker(s) must be allocated before any InstanceClass
  // object in order for the invariant 
  //   klass < klass->mirror => not initialized to hold.
  *task_class_init_marker() = Universe::new_task_mirror(0, 0 JVM_NO_CHECK);
#endif
  
  create_type_array_classes(JVM_SINGLE_ARG_CHECK);

  {
    JavaClass::Raw element;
    // The element class will be set as soon as java.lang.Object is loaded
    *object_array_class() = new_obj_array_class(&element JVM_NO_CHECK);
  }

  // Allocate empty singleton arrays.
  *empty_obj_array()   = allocate_array(object_array_class(), 0,
                                        sizeof(OopDesc*) JVM_NO_CHECK);
  *empty_short_array() = allocate_array(short_array_class(), 0,
                                        sizeof(jshort) JVM_NO_CHECK);

  // Allocate Symbol nears that mark if a Symbol is a FieldType or Signature
  *field_type_near()  = allocate_near(symbol_class() JVM_NO_CHECK);
  *method_signature_near() = allocate_near(symbol_class() JVM_NO_CHECK);
#if ENABLE_HEAP_NEARS_IN_HEAP && USE_SOURCE_IMAGE_GENERATOR
  *rom_meta_class() = meta_class();
  *rom_type_array_class_class() = type_array_class_class();
  *rom_obj_array_class_class() = obj_array_class_class();
  *rom_instance_class_class() = instance_class_class();
  
  *rom_bool_array_class() = bool_array_class(); 
  *rom_char_array_class() = char_array_class(); 
  *rom_float_array_class() = float_array_class();
  *rom_double_array_class() = double_array_class();
  *rom_byte_array_class() = byte_array_class();
  *rom_short_array_class() = short_array_class();
  *rom_int_array_class() = int_array_class();
  *rom_long_array_class() = long_array_class();
#endif
}

void Universe::create_type_array_classes(JVM_SINGLE_ARG_TRAPS) {
  const jint object_size = JavaClassDesc::allocation_size(0, 0);
  for(const ArrayInfo* array_info=array_table; !array_info->at_end(); array_info++){
#ifdef AZZERT
    const int scale = array_info->size();
    const jint size_type = array_info->instance_size();
    GUARANTEE(   (scale == 1 && size_type == InstanceSize::size_type_array_1)
              || (scale == 2 && size_type == InstanceSize::size_type_array_2)
              || (scale == 4 && size_type == InstanceSize::size_type_array_4)
              || (scale == 8 && size_type == InstanceSize::size_type_array_8),
        "Scale and size_type match");
#endif
    UsingFastOops fast_oops;

    // Allocate the ClassInfo
    ClassInfo::Fast info =
      allocate_class_info(JavaVTable::base_vtable_size(), 0, 0,
                          ISOLATES_PARAM(0)
                          true JVM_NO_CHECK);
    info().set_scale(array_info->size());
    info().set_type(array_info->basic_type());

    // Allocate the TypeArrayClass
    TypeArrayClass::Fast result = ObjectHeap::allocate(object_size JVM_NO_CHECK);
    *array_info->type_array_class() = result;
    ((TypeArrayClassDesc*)result.obj())->initialize(
                    type_array_class_class()->prototypical_near(),
                    object_size, array_info->instance_size(),
                    (jubyte*)oopmap_Empty);
    result().set_class_info(&info);
    JavaNear::Fast this_near = allocate_java_near(&result JVM_NO_CHECK);
    result().set_prototypical_near(&this_near);
    // With fixes to ClassBySig in VMImpl.cpp we don't need to do this
    // VMEvent::class_prepare_event(&result);
  }
}

void Universe::setup_super_and_vtables(JVM_SINGLE_ARG_TRAPS) {
  object_array_class()->set_element_class(object_class());

  // Now we have loaded the vtable from java.lang.Object
  // and must fixup all previously loaded array classes.
  for (int i=0; i<number_of_java_classes(); i++) {
    JavaClass::Raw klass = class_from_id(i);
    if (!klass.is_obj_array_class()) {
      continue;
    }
    UsingFastOops fast_oops;
    ObjArrayClass::Fast oac = klass().obj();
    oac().set_super(object_class());
    oac().initialize_vtable();
    oac().compute_name(JVM_SINGLE_ARG_CHECK);
  }

  for (ArrayInfo* array_info=array_table; !array_info->at_end();array_info++) {
    TypeArrayClass* t = array_info->type_array_class();
    t->set_super(object_class());
    t->initialize_vtable();
  }
}

#endif // !ROMIZED_PRODUCT


#if !ENABLE_ISOLATES && !ROMIZED_PRODUCT
inline void Universe::setup_mirrors(JVM_SINGLE_ARG_TRAPS) {
  object_class()->setup_java_mirror(JVM_SINGLE_ARG_CHECK);

  for (int i=0; i<number_of_java_classes(); i++) {
    JavaClass::Raw klass = class_from_id(i);
    if (!klass().is_array_class()) {
      continue;
    }
    UsingFastOops fast_oops;
    ArrayClass::Fast ac = klass().obj();
    ac().setup_java_mirror(JVM_SINGLE_ARG_CHECK);
  }

  java_lang_Class_class()->setup_java_mirror(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}
#endif // !ROMIZED_PRODUCT && !ENABLE_ISOLATES


void Universe::setup_thread(Thread *thread) {
  GUARANTEE(before_main(), "call this only to initialize the first thread");
  OopDesc * proto_near = Universe::mixed_oop_class()->prototypical_near();
  (*(ThreadDesc**)thread)->reinitialize(proto_near,
                                        MixedOopDesc::Type_Thread,
                                        ThreadDesc::allocation_size(),
                                        ThreadDesc::pointer_count());
}

void Universe::setup_thread_priority_list(JVM_SINGLE_ARG_TRAPS) {
  // create scheduler priority queue array
  // We create NUM + 1 (11) so that we don't have to fuss with
  // subtracting 1 from the priority to index into the array.
  // Priorities are 1 to 10, slot 0 is therefore unused
#if ENABLE_ISOLATES
  Task task = Task::get_task(Task::FIRST_TASK);
  ObjArray::Raw oa = new_obj_array(ThreadObj::NUM_PRIORITY_SLOTS + 1 JVM_CHECK);
  task.set_priority_queue(oa);
#else
  *scheduler_priority_queues() = 
      new_obj_array(ThreadObj::NUM_PRIORITY_SLOTS + 1 JVM_NO_CHECK_AT_BOTTOM);
#endif
}

#if ENABLE_INTERPRETATION_LOG

void Universe::set_compilation_allowed( const bool enable ) {
  if( enable && !_is_compilation_allowed ) reset_interpretation_log();
  _is_compilation_allowed = enable;
}
#endif

bool Universe::bootstrap(const JvmPathChar* classpath) {
#ifdef AZZERT
  check_basic_types();
  check_rom_layout();
#endif

#if ENABLE_INTERPRETATION_LOG
  reset_interpretation_log();
#endif

#if ENABLE_COMPILER
  CompiledMethodCache::init();
  Compiler::initialize();
#endif

#if ENABLE_ISOLATES
  Task::initialize();
#endif


  ObjectHeap::initialize();
  JarFileParser::initialize();

  ROM::initialize(classpath);
  if (!UseROM && HeapMin < 200 * 1024) {
    // We need a pretty big heap to load the bootstrap classes.
    HeapMin = 200 * 1024;
  }
  if (!ObjectHeap::create()) {
    return false;
  }
#if ENABLE_TRAMPOLINE && !CROSS_GENERATOR
  BranchTable::init();
#endif
  if (!Scheduler::initialize()) {
    return false;
  }

  if (UseROM) {
    if (!bootstrap_with_rom(classpath)) {
      return false;
    }
  } else {
    if (!bootstrap_without_rom(classpath)) {
      return false;
    }
  }

  SETUP_ERROR_CHECKER_ARG;

#if USE_BINARY_IMAGE_LOADER
#if !ENABLE_LIB_IMAGES
  if (!GenerateROMImage) 
#endif
  {
    bootstrap_dynamic_rom(JVM_SINGLE_ARG_CHECK_0);
  }
#endif

#if ENABLE_KVM_COMPAT
  kvmcompat_initialize();
#endif

  if (!GenerateROMImage) {
    // System.<clinit> calls System.getProperty(). We cannot do that
    // during Romization (which happens on the development host)
    // because System.getProperty() may return a different value in
    // the target platform.
    system_class()->initialize(JVM_SINGLE_ARG_CHECK_0);
  }

  jvm_class()->initialize(JVM_SINGLE_ARG_CHECK_0);
  file_descriptor_class()->initialize(JVM_SINGLE_ARG_CHECK_0);

  if (gc_block_stackmap()->is_null()) {
    StackmapGenerator::initialize(10 JVM_NO_CHECK);
  }

#if ENABLE_ISOLATES
  IsolateObj::verify_fields();
#endif

#if ENABLE_REMOTE_TRACER
  if (RemoteTracePort > 0) {
    RemoteTracer::initialize();
  }
#endif

  return true;
}

void Universe::allocate_bootstrap_thread() {
  SETUP_ERROR_CHECKER_ARG;
  GCDisabler cant_gc_while_initializing_vm;

  // We need to cheat a bit here: thread.klass() will remain NULL for
  // while until the Universe::mixed_oop_class() is initialized.
  OopDesc* thread = ObjectHeap::allocate(ThreadDesc::allocation_size()
                                         JVM_MUST_SUCCEED);  
  GUARANTEE(thread != NULL, "can't fail here");

  // don't want to do any potential task setup so we do this directly
  // rather than calling Thread::set_current()
  _current_thread = thread;
}

bool Universe::bootstrap_with_rom(const JvmPathChar* classpath) {
  SETUP_ERROR_CHECKER_ARG;

  // Load ROM::heap_block[] into the heap and relocate all heap pointers
  // NOTE: last <NUM_HANDLES_SKIP> handles not ROMIZED.
  {
    int num_rom_handles = __number_of_persistent_handles - NUM_HANDLES_SKIP;
#if ENABLE_HEAP_NEARS_IN_HEAP
    //num_rom_handles is number of handles in PERS_HANDLES_BLOCK of ROM Image
    //last NUM_DUPLICATE_ROM_HANDLES are in other block and must be treated differently
    num_rom_handles -= NUM_DUPLICATE_ROM_HANDLES;
#endif
    bool status = ROM::link_static(persistent_handles, num_rom_handles);
    *Universe::system_mirror_list() = *Universe::mirror_list();
    if (!status) {
      // out of memory
      JVM_FATAL(bootstrap_heap_too_small);
    }
  }

  // Allocate sleep queue in system area
  *(OopDesc**)scheduler_waiting() =
    ObjectHeap::allocate(ThreadDesc::allocation_size() JVM_MUST_SUCCEED);
  GUARANTEE(*scheduler_waiting() != NULL, "can't fail here");
  Thread *waiting = scheduler_waiting();
  setup_thread(waiting);
  // Set sleep queue to point to head of list
  waiting->clear_next();
  waiting->clear_previous();
  waiting->clear_next_waiting();
  waiting->set_global_next(waiting);

  _interned_string_near_addr = (OopDesc *)interned_string_near()->obj();
  _persistent_handles_addr = persistent_handles;
  StackmapGenerator::initialize(_gc_stackmap_size JVM_MUST_SUCCEED);
  
  // Allocate task list and the system Task object
  init_task_list(JVM_SINGLE_ARG_MUST_SUCCEED);

  // The rest stuff is allocated in the context of new task
  ObjectHeap::finish_system_allocation(Task::FIRST_TASK);
  allocate_bootstrap_thread();
  create_first_task(classpath JVM_CHECK_0);
#if ENABLE_ISOLATES
  Task::init_first_task(JVM_SINGLE_ARG_CHECK_0);
#endif

#if !defined(PRODUCT) || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
  {
    const int task = ObjectHeap::start_system_allocation();
    ROM::init_debug_symbols(JVM_SINGLE_ARG_NO_CHECK);
    ObjectHeap::finish_system_allocation(task);
    JVM_DELAYED_CHECK_0;
  }
#endif

  update_relative_pointers();

  setup_thread_priority_list(JVM_SINGLE_ARG_MUST_SUCCEED);
  setup_thread(Thread::current()); // Now set up Thread::current()->klass()

  // Allocate and set java.lang.Thread mirror for current thread
  create_main_thread_mirror(JVM_SINGLE_ARG_MUST_SUCCEED);

  Thread::set_current(Thread::current());
  Scheduler::start(Thread::current() JVM_MUST_SUCCEED);
  Thread::current()->initialize_main(JVM_SINGLE_ARG_MUST_SUCCEED);

  if (VerifyGC) {
    // The heap should be consistent now.
    ObjectHeap::verify();
  }

  if (GenerateROMImage) {
    *current_dictionary() = Universe::new_obj_array(64 JVM_CHECK_0);
  }

#if ENABLE_ISOLATES
#ifndef PRODUCT
  check_romized_obj_array_classes();
#endif
#endif

  _is_bootstrapping = false;
  _before_main = false;

  if (VerboseGC || TraceGC || TraceHeapSize) {
    TTY_TRACE_CR(("young gen min (actual)   = %dK",
                  DISTANCE(_heap_start, _inline_allocation_top)/1024));
  }

#if ENABLE_ISOLATES
  Task::current()->init_classes_inited_at_build(JVM_SINGLE_ARG_CHECK_0);
#endif

#if ENABLE_JVMPI_PROFILE
  {
  UsingFastOops fast_oops;
  // Send all the romized method info to the profiler.
  jint class_length = Universe::class_list()->length();

  JavaClass::Fast klass;
  for(int i = 0; i < _rom_number_of_java_classes; i++)
  {
    JavaClassDesc* jcdes = (JavaClassDesc*) Universe::class_from_id(i);
    klass = jcdes;

    if(klass().is_instance_class())
    {
      // Send the class load event
      if(UseJvmpiProfiler && JVMPIProfile::VMjvmpiEventClassLoadIsEnabled()) {
        JVMPIProfile::VMjvmpiPostClassLoadEvent(
                      (InstanceClassDesc*)jcdes JVM_CHECK_0);
      }
    }
  }
  }
#endif

  if (VerifyGC) {
    ObjectHeap::verify();
  }

#ifndef PRODUCT
  // All system JavaClasses must be either in DATA block, or permanent
  // section of HEAP. See comments in BlockTypeFinder::finish() in
  // ROMWriter.cpp.
  {
    for (int i=0; i<number_of_java_classes(); i++) {
      JavaClass::Raw klass = class_from_id(i);
      OopDesc *klassobj = klass;
      GUARANTEE_R(ROM::system_data_contains(klassobj) || 
                  ObjectHeap::permanent_contains((OopDesc**)klassobj),
                  "system JavaClasses must be non-moveable");
    }
  }
#endif

  if (!jvm_class()->access_flags().is_hidden()) {
    // The com.sun.cldchi.jvm.JVM class contains sensitive information
    // that must not be accessible from user applications. If you see this
    // error, please make sure you have this line in your cldc_rom.cfg
    // file:
    //     HiddenPackage = com.sun.cldchi.jvm

    JVM_FATAL(jvm_class_must_be_hidden);
    return false;
  }
  return true;
}

#if !ROMIZED_PRODUCT
bool Universe::bootstrap_without_rom(const JvmPathChar* classpath) {
  SETUP_ERROR_CHECKER_ARG;

  allocate_bootstrap_thread();
  create_meta(JVM_SINGLE_ARG_CHECK_0);

#if ENABLE_ISOLATES
  allocate_boundary_near_list(JVM_SINGLE_ARG_CHECK_0);
#endif

  // Meta hierarchy is now in place, initialize Thread::current()->klass().
  Thread::current()->initialize_main(JVM_SINGLE_ARG_NO_CHECK);

#if ENABLE_JAVA_DEBUGGER
  // create object mapping array for debugger code
  {
    ObjArray::Raw p =
      Universe::new_obj_array(JavaDebugger::HASH_SLOT_SIZE JVM_CHECK_0);
    *Universe::objects_by_id_map() = p;
  }
  {
    ObjArray::Raw p =
      Universe::new_obj_array(JavaDebugger::HASH_SLOT_SIZE JVM_CHECK_0);
    *Universe::objects_by_ref_map() = p;
  }
#endif

#if ENABLE_MEMORY_PROFILER
  *mp_stack_list() = Universe::new_obj_array(16 JVM_CHECK_0);
#endif

  // lock table for interned Strings
  *lock_obj_table() = Universe::new_obj_array(4 JVM_CHECK_0);

  // Allocate tables used for class loading
  *class_list() = new_obj_array(50 JVM_CHECK_0);

  // Global reference support
  *global_refs_array() = Universe::new_int_array(5 JVM_CHECK_0);

#if ENABLE_ISOLATES
  *mirror_list() = setup_mirror_list(50 JVM_CHECK_0);
#endif
  init_task_list(JVM_SINGLE_ARG_CHECK_0);
#if ENABLE_ISOLATES
  ObjectHeap::on_task_switch(Task::FIRST_TASK);
  _current_task = Task::get_task(Task::FIRST_TASK);
#endif
  create_first_task(classpath JVM_CHECK_0);

  setup_thread_priority_list(JVM_SINGLE_ARG_CHECK_0);

  {
    OopDesc* dictionary = Universe::new_obj_array(64 JVM_CHECK_0);
    *current_dictionary() = dictionary;

    // When we're generating the system library image, only want one dictionary
    if( !GenerateROMImage ) {
      *system_dictionary() = dictionary;
    }
  }

  update_relative_pointers();

#if ENABLE_COMPILER_TYPE_INFO
  // Need java/lang/Object to be the first on class list
  GUARANTEE(number_of_java_classes() == 0, "Sanity");
  Symbol::Raw symbol = TypeSymbol::parse("Ljava/lang/Object;" JVM_CHECK_0);
  GUARANTEE(number_of_java_classes() == 1, "Sanity");
#endif

  // We need to register the class for [java/lang/Object; if we want
  // signature including it in the initialized symbol table.
  register_java_class(object_array_class());

  {
    DECLARE_STATIC_BUFFER(char, buffer, 3);
    buffer[0] = '[';
    buffer[2] = '\0';
    for(ArrayInfo* array_info=array_table; !array_info->at_end(); array_info++){
      TypeArrayClass* t = array_info->type_array_class();
      register_java_class(t);

      // This must be done after the class has a valid class_id.
      buffer[1] = array_info->name_char();
      Symbol::Raw symbol = TypeSymbol::parse(buffer JVM_CHECK_0);
      t->set_name(&symbol);
    }
  }

  // Initialize global VM symbols (must be done after classes such as "[C"
  // have been registered.
  Symbols::initialize(JVM_SINGLE_ARG_CHECK_0);

  // Verify bytecodes.
  Bytecodes::verify();
  BytecodeClosure::verify();

#if ENABLE_JAVA_DEBUGGER
  vmevent_request_head()->set_null();
#endif

  // Load java.lang.Object class.
  *object_class() = SystemDictionary::resolve(Symbols::java_lang_Object(),
                                              ExceptionOnFailure JVM_NO_CHECK);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    JVM_FATAL(java_lang_object_not_found);
  }

  object_class()->set_array_class(object_array_class() JVM_CHECK_0);
  // Now we have loaded the vtable from java.lang.Object
  // and must fixup all previously loaded array classes.
  setup_super_and_vtables(JVM_SINGLE_ARG_CHECK_0);

  // Enable vtable construction from now on.
  _is_bootstrapping = false;

  allocate_gc_dummies(JVM_SINGLE_ARG_CHECK_0);

  load_root_class(java_lang_Class_class(),
                  Symbols::java_lang_Class());
  JavaClassObj::verify_fields();

  setup_thread(Thread::current());

  // Load remaining root classes.
  load_root_class(string_class(),
                  Symbols::java_lang_String());
  load_root_class(system_class(),
                  Symbols::java_lang_System());
  load_root_class(math_class(),
                  Symbols::java_lang_Math());
  load_root_class(jvm_class(),
                  Symbols::com_sun_cldchi_jvm_JVM());
  load_root_class(file_descriptor_class(),
                  Symbols::com_sun_cldchi_jvm_FileDescriptor());
#if ENABLE_JAVA_DEBUGGER
  load_root_class(dbg_class(),       Symbols::com_sun_cldchi_jvm_DBG());
#endif
  // Special interned string near that marks interned strings for 
  // synchronization
  {
    OopDesc* string_near = allocate_java_near(string_class() JVM_NO_CHECK);
    *interned_string_near() = string_near;
    _interned_string_near_addr = string_near;
  }
  _persistent_handles_addr = persistent_handles;
#if ENABLE_CLDC_11
  WeakReference::verify_fields();
#endif
  *(OopDesc**)scheduler_waiting() = ObjectHeap::allocate(ThreadDesc::allocation_size()
                                         JVM_MUST_SUCCEED);  
  GUARANTEE(*scheduler_waiting() != NULL, "can't fail here");
  Thread *waiting = scheduler_waiting();
  setup_thread(waiting);
  // Set sleep queue to point to head of list
  waiting->clear_next();
  waiting->clear_previous();
  waiting->clear_next_waiting();
  waiting->set_global_next(waiting);

  // load java.lang.throwable
  load_root_class(throwable_class(), Symbols::java_lang_Throwable());
  Throwable::verify_fields();

  // Initialize exceptions
  Throw::initialize(JVM_SINGLE_ARG_CHECK_0);

  load_root_class(error_class(), Symbols::java_lang_Error());

  // Load java.lang.Thread
  load_root_class(thread_class(), Symbols::java_lang_Thread());
  ThreadObj::verify_fields();

  // Classes needed by the compiler
  load_root_class(null_pointer_exception_class(),
                  Symbols::java_lang_NullPointerException());
  load_root_class(array_index_out_of_bounds_exception_class(),
                  Symbols::java_lang_ArrayIndexOutOfBoundsException());
  load_root_class(illegal_monitor_state_exception_class(),
                  Symbols::java_lang_IllegalMonitorStateException());
  load_root_class(arithmetic_exception_class(),
                  Symbols::java_lang_ArithmeticException());
  load_root_class(incompatible_class_change_error_class(),
                  Symbols::java_lang_IncompatibleClassChangeError());

#if ENABLE_REFLECTION
  load_root_class(boolean_class(),   Symbols::java_lang_Boolean());
  load_root_class(character_class(), Symbols::java_lang_Character());
  load_root_class(float_class(),     Symbols::java_lang_Float());
  load_root_class(double_class(),    Symbols::java_lang_Double());
  load_root_class(byte_class(),      Symbols::java_lang_Byte());
  load_root_class(short_class(),     Symbols::java_lang_Short());
  load_root_class(integer_class(),   Symbols::java_lang_Integer());
  load_root_class(long_class(),      Symbols::java_lang_Long());
  load_root_class(void_class(),      Symbols::java_lang_Void());
#endif

  // Now we have loaded java.lang.Class update the existing classes with
  // java_mirror
#if ENABLE_ISOLATES
  // Class char[][] is used during Monet conversion to copy classpath.
  char_array_class()->get_array_class(2 JVM_CHECK_0);
  load_root_class(isolate_class(), Symbols::com_sun_cldc_isolate_Isolate());  
  Task::init_first_task(JVM_SINGLE_ARG_CHECK_0);
  Task::setup_mirrors(JVM_SINGLE_ARG_CHECK_0);
#else
  setup_mirrors(JVM_SINGLE_ARG_CHECK_0);
  // Fixed initialization order for Object, Thread & Class
  object_class          ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK_0);
  thread_class          ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK_0);
  java_lang_Class_class ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK_0);
  // bootstrap java/lang/String and java/lang/Throwable
  string_class          ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK_0);
  throwable_class       ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK_0);
  error_class           ()->bootstrap_initialize(JVM_SINGLE_ARG_CHECK_0);
#endif

  Symbol thrower = SymbolTable::symbol_for("throwNullPointerException"
                                           JVM_MUST_SUCCEED);
  *throw_null_pointer_exception_method() = 
    system_class()->find_local_method(&thrower, Symbols::void_signature());
  thrower = SymbolTable::symbol_for("throwArrayIndexOutOfBoundsException"
                                           JVM_MUST_SUCCEED);
  *throw_array_index_exception_method() = 
    system_class()->find_local_method(&thrower, Symbols::void_signature());
  thrower = SymbolTable::symbol_for("quickNativeThrow" JVM_MUST_SUCCEED);
  
  *quick_native_throw_method() =
    system_class()->find_local_method(&thrower, Symbols::void_signature());

  // Allocate and set java.lang.Thread mirror for current thread
  create_main_thread_mirror(JVM_SINGLE_ARG_CHECK_0);
  Scheduler::start(Thread::current() JVM_NO_CHECK);

  // Fix dummy FileDescriptor allocated before the file_descriptor_class()
  // was loaded.
  JarFileParser::fix_bootstrap();

  // At this point all array classes should have vtables
  // and current thread has been initialized.
  if (VerifyGC) {
    ObjectHeap::verify();
  }
  _before_main = false;

  *inlined_stackmaps() = new_stackmap_list(1 JVM_CHECK_0);
  inlined_stackmaps()->set_short_map(0, 0);

  // We don't have any romized resources in a non-romized VM.
  *resource_names() = new_obj_array(0 JVM_CHECK_0);

#if ENABLE_SEMAPHORE
  // Verify classes that aren't stored in persistent handles.
  Semaphore::verify_fields();
  SemaphoreLock::verify_fields();
#endif
  return true;
}
#endif

#if USE_BINARY_IMAGE_LOADER
void Universe::bootstrap_dynamic_rom(JVM_SINGLE_ARG_TRAPS) {
  if (UseROM) {
    Task::current()->link_dynamic(JVM_SINGLE_ARG_NO_CHECK);
    if (CURRENT_HAS_PENDING_EXCEPTION) {
      JVM_FATAL(bootstrap_heap_too_small);
    }

    update_relative_pointers();

    if (VerifyGC) {
      // The heap should be consistent now.
      ObjectHeap::verify();
    }
  }
}
#endif

void Universe::create_main_thread_mirror(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  InstanceClass::Fast instance_class = thread_class();
  ThreadObj::Fast thread_obj =
      instance_class().new_instance(ExceptionOnFailure JVM_CHECK);

#if ENABLE_CLDC_11
  Method::Fast init =
      instance_class().find_local_method(Symbols::object_initializer_name(),
                                         Symbols::void_signature());
  GUARANTEE(!init.is_null(), "Unable to initialize");

  // Delay invocation of instance initialization:
  EntryActivation::Fast entry = Universe::new_entry_activation(&init, 1 
                                                               JVM_CHECK);
  entry().obj_at_put(0, &thread_obj);
  Thread::current()->append_pending_entry(&entry);
#else
  // Thread doesn't have name in CLDC 1.0. No initialization is necessary.
#endif

  Thread::current()->set_thread_obj(&thread_obj);
  thread_obj().set_priority(ThreadObj::PRIORITY_NORMAL);
}

#if USE_JAR_ENTRY_ENUMERATOR
void Universe::load_jar_entry(char* name, int length, JarFileParser* jf_parser
                              JVM_TRAPS) {
  const int post_length = STATIC_STRLEN(".class");
  FileDecoder fd = jf_parser->open_entry(0 JVM_CHECK);
  if (fd.not_null()) {
    Buffer b = fd.read_completely(JVM_SINGLE_ARG_CHECK);
    if (b.not_null()) {
      Symbol class_name = SymbolTable::symbol_for(name, length - post_length
                                                  JVM_CHECK);
      InstanceClass instance_class =
        SystemDictionary::resolve(&class_name, ErrorOnFailure, &b JVM_NO_CHECK);
      if (CURRENT_HAS_PENDING_EXCEPTION) {
        if (VerifyOnly) {
          return;
        }

        if (TestCompiler) {
          TTY_TRACE(("TestCompiler: failed to load class "));
          for (int i=0; i<length; i++) {
            TTY_TRACE(("%c", name[i]));
          }
          TTY_TRACE_CR((""));
        }
#if ENABLE_ROM_GENERATOR
        if (GenerateROMImage) {
          Throwable::Raw exception = Thread::current_pending_exception();

          Thread::clear_current_pending_exception();

          InstanceClass::Raw exception_klass = exception.blueprint();
          Symbol::Raw exception_klass_name = exception_klass().name();
          ROMWriter::record_class_loading_failure(
            &class_name, &exception_klass_name JVM_CHECK);
        }
#endif
        Thread::clear_current_pending_exception();
        return;
      }

      if (UseVerifier || VerifyOnly) {
        instance_class.verify(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
      }
      return;
    }
  }

  TTY_TRACE_CR(("Error loading JAR entry: %s", name));
}


int Universe::load_next_in_classpath_segment(FilePath* path, 
                                              int entry_id, 
                                              int chunk_size JVM_TRAPS) {
  GUARANTEE(entry_id >= 0 && chunk_size > 0, "Sanity");
  const int buffer_size = 512;
  DECLARE_STATIC_BUFFER(PathChar, file_name, buffer_size);
  const char suffix[] = {'.','c','l','a','s','s','\0'};

  path->string_copy(file_name, buffer_size);
  // check to see if this path is a jar file
  if (OsFile_exists(file_name)) {
    return JarFileParser::do_next_entries(file_name, suffix, true, 
               (JarFileParser::do_entry_proc)load_jar_entry, entry_id, 
               chunk_size JVM_NO_CHECK_AT_BOTTOM_0);
  } else {
    // Either the jarfile does not exist or the openJarFile() failed
    // due to corruption or other problem.  Loading non-JarFile is
    // UNIMPLEMENTED, but is treated as an error here.
    Throw::error(jarfile_error JVM_THROW_(-1));
  }
}

// Used by the romizer
void Universe::load_all_in_classpath(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast classpath = Task::current()->app_classpath();

  GUARANTEE(classpath().not_null(), "Sanity");
  
  int index = 0;
  for (; index < classpath().length(); index++) {
    FilePath path = classpath().obj_at(index);
    load_all_in_classpath_segment(&path JVM_CHECK);
  }
  
  GUARANTEE(Task::current()->sys_classpath() == NULL ||
            ENABLE_ISOLATES, "sys_classpath should be null in SVM");

#if ENABLE_ISOLATES
  classpath = Task::current()->sys_classpath();
  GUARANTEE(classpath().not_null(), "Sanity");

  for (index = 0; index < classpath().length(); index++) {
    FilePath path = classpath().obj_at(index);
    load_all_in_classpath_segment(&path JVM_CHECK);
  }
#endif
}

// Used by the romizer and TestCompiler
void Universe::invoke_pending_entries(JVM_SINGLE_ARG_TRAPS) {
  while (Thread::current()->has_pending_entries()) {
    if (Scheduler::active_count() <= 0) {
      Thread::current()->initialize_main(JVM_SINGLE_ARG_CHECK);
      Scheduler::start(Thread::current() JVM_CHECK);
    }
    primordial_to_current_thread();
    if (CURRENT_HAS_PENDING_EXCEPTION) {
      tty->print("Unexpected exception in ROMOptimizer: ");
      Thread::print_current_pending_exception_stack_trace();
    }
  }
}
#endif // USE_JAR_ENTRY_ENUMERATOR

#if ENABLE_VERIFY_ONLY
int Universe::load_next_and_verify(FilePath* path, int entry_id,
                                   int chunk_size JVM_TRAPS) {
  GlobalSaver verify_saver(&UseVerifier);

  UseVerifier = 1;
  AZZERT_ONLY(ObjArray::Raw classpath = Task::current()->app_classpath());
  GUARANTEE(classpath().length() == 1, 
    "We should have the only item in classpath");

#ifdef AZZERT
  // Some classes may be loaded by the verifier during verification --
  // we'd better make sure they are loaded from the same classpath.
  TypeArray::Raw path2 = classpath().obj_at(0);
  if (!path->equals(&path2)) {
    GUARANTEE(path->length() == path2().length(), 
              "Path should be the first item in classpath");
    for (int i=0; i<path->length(); i++) {
      GUARANTEE(path->char_at(i) == path2().char_at(i),
              "Path should be the first item in classpath");
    }
  }
#endif

  // Load and verify the next chunk of class files from the JAR.
  return load_next_in_classpath_segment(path, entry_id, 
                                         chunk_size JVM_NO_CHECK_AT_BOTTOM_0);
}
#endif

#if !defined(PRODUCT) || ENABLE_ROM_GENERATOR

void Universe::record_inited_at_build(InstanceClass* klass JVM_TRAPS) {
#if ENABLE_ISOLATES
  UsingFastOops fast_until_die;
  ObjArray::Fast cur = Universe::inited_at_build();

  if (cur.is_null()) {
    cur = Universe::new_obj_array(10 JVM_CHECK);
    *Universe::inited_at_build() = cur;
  }
  
  int len = cur().length();
  for (int i=0; i<len; i++) {
    if ( cur().obj_at(i) == NULL) {
      cur().obj_at_put(i, klass->obj());
      return;
    }
  }

  int new_len = len + 10;
  ObjArray::Fast new_cur = Universe::new_obj_array(new_len JVM_CHECK);
  ObjArray::array_copy(&cur, 0, &new_cur, 0, len JVM_CHECK);

  new_cur().obj_at_put(len, klass->obj());
  *Universe::inited_at_build() = new_cur;
#else
  JVM_IGNORE_TRAPS;
  (void)klass;
#endif

}

void Universe::check_rom_layout() {
  GUARANTEE((address)(&persistent_handles[__number_of_persistent_handles-1]) ==
            (address)global_threadlist(),
            "global_threadlist must be last persistent handle");
}

#endif

void Universe::apocalypse() {
  Scheduler::clear_next_runnable_thread();
  Scheduler::dispose();  
  Thread::dispose();   
  JarFileParser::flush_caches();
  ROM::dispose_binary_images();

  jvm_memset(persistent_handles, 0, sizeof(persistent_handles));

  _is_compilation_allowed = true;
  _compilation_abstinence_ticks = 0;
  _before_main      = true;
  _is_bootstrapping = true;
  _is_stopping      = false;
  set_number_of_java_classes(0);
  ExecutionStackDesc::_stack_list = NULL;

#if ENABLE_ISOLATES
  TaskContext::set_current_task_id(0);
#endif

#if ENABLE_METHOD_TRAPS
  MethodTrap::dispose();
#endif

  ROM::dispose();

  ObjectHeap::dispose();
  Os::dispose();
}

ReturnOop Universe::new_type_array(TypeArrayClass* klass, jint length JVM_TRAPS) {
  return allocate_array(klass, length, klass->scale() JVM_NO_CHECK_AT_BOTTOM);
}

ReturnOop Universe::new_method(const int code_length,
                               const AccessFlags access_flags JVM_TRAPS)
{
  Method::Raw m = allocate_method(code_length JVM_NO_CHECK);
  if (m.not_null()) {
    m().set_access_flags(access_flags);
  }
  return m;
}

ReturnOop Universe::new_constant_pool(int length JVM_TRAPS) {
  UsingFastOops fast_oops;
  ConstantPool::Fast cp = allocate_constant_pool(length JVM_OZCHECK(cp));
  TypeArray::Raw tags = new_byte_array(length JVM_OZCHECK(tags));
  cp().set_tags(&tags);
  return cp;
}

ReturnOop Universe::new_entry_activation(Method* method, jint length JVM_TRAPS)
{
  EntryActivation::Raw entry = allocate_entry_activation(length JVM_NO_CHECK);

  if (entry.not_null()) {
    entry().set_method(method);
#if ENABLE_REFLECTION || ENABLE_JAVA_DEBUGGER
    entry().set_return_point((address) entry_return_void);
#endif
  }
  return entry;
}

ReturnOop Universe::new_instance(InstanceClass* klass JVM_TRAPS) {
  InstanceSize instance_size = klass->instance_size();
  OopDesc* result = ObjectHeap::allocate(instance_size.fixed_value() 
                                         JVM_ZCHECK_0(result));
  result->initialize(klass->prototypical_near());
  AccessFlags flags = klass->access_flags();
  if (flags.has_finalizer()) {
    UsingFastOops fast_oops;
    Oop::Fast obj(result);  // create handle, call below can gc
    if (flags.has_unresolved_finalizer()) {
      UsingFastOops fast_oops2;
      ObjArray::Fast methods = klass->methods();
      Method::Fast finalize_method = 
        InstanceClass::find_method(&methods, Symbols::finalize_name(),
                                   Symbols::void_signature());
#if ENABLE_DYNAMIC_NATIVE_METHODS
      address finalizer = 
        Natives::load_dynamic_native_code(&finalize_method JVM_CHECK_0);
      GUARANTEE(finalizer != NULL, "Error must be thrown");

      flags.clear_has_unresolved_finalizer();
      klass->set_access_flags(flags);
#else 
      Throw::unsatisfied_link_error(&finalize_method JVM_THROW_0)
#endif
    }
    ObjectHeap::register_finalizer_reachable_object(&obj JVM_CHECK_0);
    return obj;
  }
  return result;
}

ReturnOop Universe::new_instance_class(int vtable_length,
                                       size_t itable_size, int itable_length,
                                       size_t static_field_size,
                                       size_t oop_map_size,
                                       size_t instance_size
                                       JVM_TRAPS) {
  check_class_list_size(JVM_SINGLE_ARG_CHECK_0);
  if ((((juint)vtable_length)     & 0xffff0000) != 0 ||
      (((juint)itable_size)       & 0xffff0000) != 0 ||
      (((juint)itable_length)     & 0xffff0000) != 0 ||
      (((juint)static_field_size) & 0xffff0000) != 0 ||
      (((juint)oop_map_size)      & 0xffff0000) != 0) {
    // All these must be <= 16 bit to fit in jushort
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
  }

  if ((((juint)instance_size)     & 0xffff8000) != 0) {
    // This must be <= 15 bits, to fit in the positive range of a jshort.
    // See FarClassDesc::_instance_size.
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
  }

  UsingFastOops fast_oops;

  // (1) Allocate the ClassInfo
  ClassInfo::Fast info = allocate_class_info(vtable_length, itable_length,
                                             itable_size,
                                             ISOLATES_PARAM(static_field_size)
                                             false JVM_CHECK_0);

  // (2) Allocate the InstanceClass
  size_t object_size =
      InstanceClassDesc::allocation_size(static_field_size, oop_map_size,
                                         vtable_length);
  InstanceClassDesc* raw = (InstanceClassDesc*)
        ObjectHeap::allocate(object_size JVM_ZCHECK_0(raw));
  raw->initialize(instance_class_class()->prototypical_near(), object_size,
      instance_size, NULL);
  InstanceClass::Fast result = ReturnOop(raw);
  result().set_embedded_oop_map_start(
    InstanceClassDesc::allocation_size(static_field_size, 0, 0));
  result().set_class_info(&info);

  {
    Near::Raw prototype = allocate_java_near(&result JVM_OZCHECK(prototype));
    result().set_prototypical_near(&prototype);
  }

  register_java_class(&result);

  return result;
}

#if ENABLE_ISOLATES
#ifndef PRODUCT
// Make sure that all ObjArrayClass instances are pointed at by the
// appropriate class
void Universe::check_romized_obj_array_classes(void) {
  UsingFastOops fast_oops;
  JavaClass::Fast jc, elem_class;
  ObjArrayClass::Fast oac;
  ObjArray::Fast list = Universe::task_list();
  TaskMirror::Fast sys_tm;
  list = Universe::class_list();
  GUARANTEE(!list.is_null(), "No class list");

  for(int i = 0; i < list().length(); i++) {
    UsingFastOops fast_oops2;
    jc = list().obj_at(i);
    if (!jc.is_null() && jc().is_obj_array_class()) {
      oac = jc;
      elem_class = oac().element_class();
      GUARANTEE(!elem_class.is_null(), "Element class is null");
      oac = elem_class().array_class();
      GUARANTEE(!oac.is_null(), "ROMized array class is null");
    }
  }
}
#endif
ReturnOop Universe::setup_mirror_list(int size JVM_TRAPS) {
  ObjArray::Raw new_list = new_obj_array(size JVM_CHECK_0);
  OopDesc* marker = task_class_init_marker()->obj();
  for (int i = 0; i < size; i++) {
    new_list().obj_at_put(i, marker);
  }
  return new_list;
}

inline ReturnOop Universe::new_task_list(JVM_SINGLE_ARG_TRAPS) {
  return new_obj_array(MAX_TASKS JVM_NO_CHECK_AT_BOTTOM);
}

#endif

void Universe::check_class_list_size(JVM_SINGLE_ARG_TRAPS) {
  if (number_of_java_classes() >= 0x3fff) {
    // Our signature scheme allows no more than 16384 classes (represented
    // by 14 bits
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW);
  }
  if (!is_bootstrapping()) {
    if (class_list()->length() == number_of_java_classes()) {
      resize_class_list(20 JVM_NO_CHECK_AT_BOTTOM);
    }
  }
}

void Universe::resize_class_list(int delta JVM_TRAPS) {
  UsingFastOops fast_oops;
  int new_size = class_list()->length() + delta; // delta may be negative
  int size_to_copy = min(new_size, class_list()->length());
  ObjArray::Fast src = class_list();
  ObjArray::Fast dst = new_obj_array(new_size JVM_CHECK);
  ObjArray::array_copy(&src, 0, &dst, 0, size_to_copy JVM_MUST_SUCCEED);

#if ENABLE_ISOLATES
  src = mirror_list();
  size_to_copy = min(new_size, src().length());
  ObjArray::Fast dst2 = new_obj_array(new_size JVM_CHECK);
  ObjArray::array_copy(&src, 0, &dst2, 0, size_to_copy JVM_MUST_SUCCEED);    
  TaskMirror::Raw t = task_class_init_marker();
  // set all empty mirror list slots to the task_class_init_marker
  for (int i = src().length(); i < dst2().length(); i++) {
    dst2().obj_at_put(i, &t);
  }

  // NOTE: changes have to happen after allocations, otherwise failed 
  // allocation can lead to corruption of structures

  // In MVM-Monet mode, this code has the side effect of copying 
  // Universe::mirror_list above ROM::_romized_heap_marker, which is required
  // by the binary image conversion process.
  {
    Task::Raw task = Task::current();
    task().set_class_list(dst);
    task().set_mirror_list(dst2);
  }
  *mirror_list() = dst2;
#endif
  
  // have to be here, otherwise class list and mirror list will mismatch
  // if mirror list allocation will fail
  *class_list() = dst;
  update_relative_pointers();
}

void Universe::register_java_class(JavaClass *klass) {
  GUARANTEE(class_list()->length() > number_of_java_classes(),
            "class_list should have been expanded");
  int class_id = number_of_java_classes();
  class_list()->obj_at_put(class_id, klass);
  ClassInfo::Raw info = klass->class_info();
  info().set_class_id(class_id);
  set_number_of_java_classes(class_id + 1);
#if ENABLE_ISOLATES
  Task::current()->set_class_count(class_id + 1);
#endif
}

void Universe::unregister_last_java_class() {
  int class_id = number_of_java_classes() - 1;
  class_list()->obj_at_clear(class_id);
#if ENABLE_ISOLATES
  Task::current()->set_class_count(class_id);
#endif
  set_number_of_java_classes(class_id);
}

// This is called after we load a real class to replace a fake class of
// the same name. We need to use the same class_id for the encoded
// field/method signatures to work.
void Universe::pop_class_id(InstanceClass *new_cls, InstanceClass *old_cls)
{
  int class_id = number_of_java_classes() - 1;
  GUARANTEE(new_cls->class_id() == class_id, "sanity");
  GUARANTEE(old_cls->is_fake_class(), "only fake classes can be replaced");
  class_list()->obj_at_clear(class_id);

  int old_index = old_cls->class_id();
  class_list()->obj_at_put(old_index, new_cls);
  ClassInfo::Raw info = new_cls->class_info();
  info().set_class_id(old_index);
  set_number_of_java_classes(class_id);
}

ReturnOop Universe::new_obj_array(int length JVM_TRAPS) {
  if (length == 0) {
    return *Universe::empty_obj_array();
  }
  return allocate_array(object_array_class(), length, sizeof(OopDesc*)
                        JVM_NO_CHECK_AT_BOTTOM);
}

ReturnOop Universe::new_obj_array(JavaClass* klass, int length JVM_TRAPS) {
  UsingFastOops fast_oops;
  FarClass::Fast f = klass->get_array_class(1 JVM_OZCHECK(f));
  return allocate_array(&f, length, sizeof(OopDesc*) JVM_NO_CHECK_AT_BOTTOM_0);
}

ReturnOop Universe::interned_string_from_utf8(Oop *oop JVM_TRAPS) {
  // Here's a fast path for interning a string that contains only
  // ASCII characters.

  UsingFastOops fast_oops;
  String::Fast s = new_instance(string_class() JVM_CHECK_0);
  bool is_symbol = oop->is_symbol();
  jint length;

  if (is_symbol) {
    length = ((Symbol*)oop)->length();
  } else {
    GUARANTEE(oop->is_byte_array(), "sanity");
    length = ((TypeArray*)oop)->length();
  }

  s().set_offset(0);
  s().set_count(length);
  TypeArray::Fast t = new_char_array(length JVM_CHECK_0);
  s().set_value(&t);

  {
    AllocationDisabler raw_pointers_used_in_this_block;
    jubyte *src;
    jubyte *src_end;
    jchar  *dst = (jchar*) t().base_address();
    jint mask = 0x80;

    if (is_symbol) {
      src = (jubyte*) ((Symbol*)oop)->base_address();
    } else {
      src = (jubyte*) ((TypeArray*)oop)->base_address();
    }
    src_end = src + length;

    while (src < src_end) {
      juint b = (jint)(*src++);
      if ((b == 0) || (b & mask) != 0) {
        // This string contains non-ascii chars. Let's go to slow case
        goto slow;
      }
      *dst++ = (jchar)(b);
    }
  }

  return Universe::interned_string_for(&s JVM_NO_CHECK_AT_BOTTOM);

slow:
  if (is_symbol) {
    SymbolStream stream((Symbol*)oop);
    return Universe::interned_string_for(&stream JVM_NO_CHECK_AT_BOTTOM_0);
  } else {
    ByteStream stream((TypeArray*)oop);
    return Universe::interned_string_for(&stream JVM_NO_CHECK_AT_BOTTOM_0);
  }
}

ReturnOop Universe::interned_string_for(CharacterStream *stream JVM_TRAPS) {

  // This function is called to intern Strings defined in classfiles.
  // Need revisit : We rarely have duplications --  the same String is
  // defined in multiple classfiles in the same Midlet. So we always
  // create the String at the beginning to make things go faster.
  UsingFastOops fast_oops;
  String::Fast string = Universe::new_string(stream JVM_OZCHECK(string));

  return Universe::interned_string_for(&string JVM_NO_CHECK_AT_BOTTOM_0);
}

// Unfortunately we duplicated some code here and in SymbolTable.cpp,
// StringTable.cpp.
// Our payoff for separating strings from symbols is more efficient romization
// We may later refactor this

ReturnOop Universe::interned_string_for(String *string JVM_TRAPS) {
  return string_table()->interned_string_for(string JVM_NO_CHECK_AT_BOTTOM);
}

ReturnOop Universe::new_string(const char* name, int length JVM_TRAPS) {
  LiteralStream ls((char*) name, 0, length);
  return new_string(&ls JVM_NO_CHECK_AT_BOTTOM);
}

ReturnOop Universe::new_string(Symbol* symbol JVM_TRAPS) {
  SymbolStream ss(symbol);
  return new_string(&ss JVM_NO_CHECK_AT_BOTTOM);
}

ReturnOop Universe::new_string(CharacterStream* stream, int lead_spaces,
                               int trail_spaces JVM_TRAPS) {
  UsingFastOops fast_oops;
  String::Fast s = new_instance(string_class() JVM_CHECK_0);
  const jint stream_length = stream->length();
  GUARANTEE(stream_length >= 0 || stream_length == UTF8Stream::UTF8_ERROR,
            "Unexpected length value");
  const jint length = (stream_length != UTF8Stream::UTF8_ERROR) ? 
    stream_length + lead_spaces + trail_spaces : 0;
  s().set_offset(0);
  s().set_count(length);
  TypeArray::Fast t = new_char_array(length JVM_CHECK_0);
  s().set_value(&t);
  stream->reset();
  for (int index = lead_spaces; index < length; index++) {
    t().char_at_put(index, stream->read());
  }
  return s;
}

ReturnOop Universe::new_string(TypeArray *char_array, int offset, int length
                               JVM_TRAPS) {
  String::Raw s = new_instance(string_class() JVM_NO_CHECK);
  if (s.not_null()) {
    s().set_value(char_array);
    s().set_offset(offset);
    s().set_count(length);
  }

  return s;
}

ReturnOop Universe::new_symbol(TypeArray *byte_array, utf8 name, int length
                               JVM_TRAPS) {
  int byte_array_offset = 0;
  if (byte_array != NULL) {
    // <name> must point to inside of byte_array
    utf8 base = (utf8)byte_array->base_address();
    GUARANTEE((name - base >= 0),                             "bounds check");
    GUARANTEE((name - base <= byte_array->length()),          "bounds check");
    GUARANTEE((name - base + length >= 0),                    "bounds check");
    GUARANTEE((name - base + length <= byte_array->length()), "bounds check");

    byte_array_offset = name - base;
  }

  SymbolDesc* result = (SymbolDesc*)
    ObjectHeap::allocate(SymbolDesc::allocation_size(length) JVM_NO_CHECK);
  if( result ) {
    result->initialize(symbol_class()->prototypical_near(), length);
    if (byte_array) {
      // This is necessary because the source of the symbol may be in
      // a byte_array that lives on the heap. The above allocation may have
      // moved the byte_array already.
      name = ((utf8)byte_array->base_address()) + byte_array_offset;
    }
    jvm_memcpy(result->utf8_data(), name, length);
  }
  return result;
}

ReturnOop Universe::new_mixed_oop(int type, size_t size, int pointer_count
                                  JVM_TRAPS) {
  MixedOopDesc *result = 
    (MixedOopDesc*) generic_allocate_oop(mixed_oop_class(), size JVM_NO_CHECK);
  if (result) {
    result->initialize(type, size, pointer_count);
  }
  return (ReturnOop)result;
}

#if ENABLE_JAVA_DEBUGGER
ReturnOop Universe::new_refnode(JVM_SINGLE_ARG_TRAPS) {
  RefNodeDesc* result = (RefNodeDesc*)
    ObjectHeap::allocate(RefNodeDesc::allocation_size() JVM_NO_CHECK);
  if (result) {
    result->initialize(refnode_class()->prototypical_near());
  }
  return result;
}
#endif

#if ENABLE_ISOLATES
void Universe::allocate_boundary_near_list(JVM_SINGLE_ARG_TRAPS) {
  // Because of the current GC design,
  // we can guarantee that the MAX_ISOLATES nears in the list are
  // always in the same order.
  //
  // This behavior is assumed by ObjectHeap.cpp, so for a BoundaryDesc* B,
  // its near B->_klass would point to N, where
  //     NearDesc* N == Universe::boundary_near_list()->obj_at(i)
  // Then we can tell that B belongs to task_id == i

  *boundary_near_list() = new_obj_array(MAX_TASKS JVM_CHECK);
  for (int i=0; i<MAX_TASKS; i++) {
    OopDesc* p = allocate_near(boundary_class() JVM_ZCHECK(p));
    boundary_near_list()->obj_at_put(i, p);
  }
}

ReturnOop Universe::new_task(int id JVM_TRAPS) {
  UsingFastOops fast_oops;
  Task::Fast task = allocate_task(JVM_SINGLE_ARG_CHECK_0);

  enum { pad = 20 }; // pad it a little to avoid immediate expansion
  const int num = UseROM ?
    _rom_number_of_java_classes : number_of_java_classes();
  
  if (id == 0) {
    // special System task, not much here
    // class_list is set to support debug printing of objects
    // belonging to this System task
    task().set_class_count(num);
    task().set_class_list(system_class_list());
    return task;
  }

  {
    ObjArray::Raw cl = Universe::new_obj_array(num + pad JVM_CHECK_0);
    ObjArray::Raw cl_src  = Universe::system_class_list();
    ObjArray::array_copy(&cl_src, 0, &cl, 0, num JVM_MUST_SUCCEED);
    task().set_class_count(num);
    task().set_class_list(cl);
  }
  { 
    ObjArray::Raw ml = (OopDesc*)NULL; 
    ObjArray old_class_list = class_list()->obj();
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES
    //we MUST HAVE SYSTEM CLASSES ALREADY LOADED to resolve references to them!!
    class_list()->set_obj(system_class_list()->obj()); 
    update_relative_pointers(); 
    //we do not check OOM here. If we get it, we will try to create task 
    //normally! 
    ml = ROM::link_static_mirror_list(JVM_SINGLE_ARG_NO_CHECK);     
    class_list()->set_obj(old_class_list.obj()); 
    update_relative_pointers(); 
    if (ml.is_null()) 
      Thread::clear_current_pending_exception();
    if (ml.is_null()) 
#endif
    {
      ml = Universe::new_obj_array(num + pad JVM_CHECK_0);          
      const int len = ml().length();
      TaskMirror::Raw t = task_class_init_marker();
      for (int i = 0; i < len; i++) {
        ml().obj_at_put(i, t);
      }
    }
    task().set_mirror_list(ml);
  } 
  {
    ObjArray::Raw task_dictionary = Universe::new_obj_array(64 JVM_CHECK_0);
    task().set_dictionary(task_dictionary);
  }
  {
    StringTable::Raw table = StringTable::initialize(64 JVM_CHECK_0);
    task().set_string_table(table);
  }
  {
    SymbolTable::Raw table = SymbolTable::initialize(64 JVM_CHECK_0);
    task().set_symbol_table(table);
  }
  {
    RefArray::Raw array = RefArray::initialize(JVM_SINGLE_ARG_CHECK_0);
    task().set_global_references(array);
  }

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  // Initialize new task with default profile id.
  task().set_profile_id(Universe::DEFAULT_PROFILE_ID);
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
  update_relative_pointers();
  return task;
}
#endif

ReturnOop Universe::allocate_entry_activation(jint length JVM_TRAPS) {
  size_t size = EntryActivationDesc::allocation_size(length);
  EntryActivationDesc* result = (EntryActivationDesc*)
      ObjectHeap::allocate(size JVM_NO_CHECK);
  if (result) {
    Oop::Raw pn = entry_activation_class()->prototypical_near();
    result->initialize(pn, length);
  }
  return result;
}

ReturnOop Universe::generic_allocate_oop(FarClass *klass, 
                                         size_t allocation_size
                                         JVM_TRAPS) {
  OopDesc* result = 
    (OopDesc*) ObjectHeap::allocate(allocation_size JVM_NO_CHECK);
  if (result) {
    result->initialize(klass->prototypical_near());
  }
  return result;
}


ReturnOop Universe::new_stackmap_list(jint length JVM_TRAPS) {
  size_t size = StackmapListDesc::allocation_size(length);
  StackmapListDesc* result = 
      (StackmapListDesc*) ObjectHeap::allocate(size JVM_NO_CHECK);
  if (result) {
    Oop::Raw pn = stackmap_list_class()->prototypical_near();
    result->initialize(pn, length);
  }
  return result;
}

ReturnOop Universe::allocate_class_info(int vtable_length, int itable_length,
                                        int itable_size,
                                        ISOLATES_PARAM(int static_field_size)
                                        bool is_array JVM_TRAPS)
{
  jint size = ClassInfoDesc::allocation_size(vtable_length, itable_size);
  ClassInfoDesc* result = 
      (ClassInfoDesc*) ObjectHeap::allocate(size JVM_NO_CHECK);
  if (result) {
    Oop::Raw pn = class_info_class()->prototypical_near();
    result->initialize(pn, (jushort) size, (jushort) vtable_length, 
                       (jushort) itable_length,
                       ISOLATES_PARAM(static_field_size)
                       is_array);
  }
  return result;
}

ReturnOop Universe::new_execution_stack(jint length JVM_TRAPS) {
  size_t size = ExecutionStackDesc::allocation_size(length);
  ExecutionStackDesc* result = 
      (ExecutionStackDesc*) ObjectHeap::allocate(size JVM_NO_CHECK);
  if (result) {
    Oop::Raw pn = execution_stack_class()->prototypical_near();
    result->initialize(pn, length);
  }
  return result;
}

#if ENABLE_COMPILER
ReturnOop Universe::new_compiled_method(int code_size JVM_TRAPS) {
  size_t size = CompiledMethodDesc::allocation_size(code_size);
  CompiledMethodDesc* result = (CompiledMethodDesc*)
      ObjectHeap::allocate_code(size JVM_NO_CHECK);
  if (result) {
    result->initialize(compiled_method_class()->prototypical_near(), code_size);
  }
  return result;
}

#endif

ReturnOop Universe::allocate_array_raw(FarClass* klass, int length,
                                   int scale JVM_TRAPS) {
  return generic_allocate_array(&ObjectHeap::allocate_raw, klass, length, scale
                                JVM_NO_CHECK);
}

ReturnOop Universe::allocate_array(FarClass* klass, int length,
                                   int scale JVM_TRAPS) { 
  return generic_allocate_array(&ObjectHeap::allocate, klass, length, scale 
                                JVM_NO_CHECK);
}


ReturnOop Universe::generic_allocate_array(Allocator* allocate, 
                                          FarClass* klass, 
                                          int length,
                                          int scale JVM_TRAPS)
{
  if( unsigned(length) <= 0x08000000) {
    GUARANTEE(!klass->is_null(), 
              "Cannot allocate array with element type NULL");
    const size_t size = ArrayDesc::allocation_size(length, scale);
    ArrayDesc* result = (ArrayDesc*) (*allocate)(size JVM_NO_CHECK);
    if (result) {
      result->initialize(klass->prototypical_near(), length);
    }
    return result;
  }

  if (length < 0) {
    Throw::throw_exception(Symbols::java_lang_NegativeArraySizeException()
                           JVM_THROW_0);
  } else {
    // Maximum allocation: 128MB for bytes ... 1024MB for longs
    //
    // Make sure the (length * scale) operation won't overflow 32-bit values.
    // In reality we're never going to allocate such big arrays
    // anyway because our heap is small.
    Throw::out_of_memory_error(JVM_SINGLE_ARG_THROW_0);
  }
}

ReturnOop Universe::allocate_method(int length JVM_TRAPS) {
  size_t size = MethodDesc::allocation_size(length);
  MethodDesc* result = (MethodDesc*) ObjectHeap::allocate(size 
                                                          JVM_NO_CHECK);
  if (result) {
    result->initialize(method_class()->prototypical_near(), length);
  }
  return result;
}

ReturnOop Universe::allocate_constant_pool(int length JVM_TRAPS) {
  size_t size = ConstantPoolDesc::allocation_size(length);
  ConstantPoolDesc* result = 
      (ConstantPoolDesc*) ObjectHeap::allocate(size JVM_NO_CHECK);
  if (result) {
    result->initialize(constant_pool_class()->prototypical_near(), length);
  }
  return result;
}

ReturnOop Universe::allocate_task(JVM_SINGLE_ARG_TRAPS) {
  Task::Raw task = new_mixed_oop(MixedOopDesc::Type_Task,
                       TaskDesc::allocation_size(),
                       TaskDesc::pointer_count()
                       JVM_NO_CHECK_AT_BOTTOM);

#if USE_BINARY_IMAGE_LOADER && ENABLE_LIB_IMAGES    
  if( task.not_null() ) {
    task().set_classes_in_images(ROM::number_of_system_classes());
  }
#endif

  return task;
}

#if ENABLE_ISOLATES
ReturnOop Universe::allocate_task_mirror(int statics_size,
                                         int vtable_length JVM_TRAPS) {
  size_t size = TaskMirrorDesc::allocation_size(statics_size, vtable_length);
  TaskMirrorDesc *result = 
      (TaskMirrorDesc*)ObjectHeap::allocate(size JVM_NO_CHECK);
  if (result) {
    // IMPL_NOTE: result should be placed in a Fast oop!
    result->initialize(task_mirror_class()->prototypical_near(), 
                       statics_size, vtable_length);
  }
  return result;
}
#endif

//
//  Allocation of near objects
//
ReturnOop Universe::generic_allocate_near(FarClass* klass, size_t size JVM_TRAPS)
{
  NearDesc* raw = (NearDesc*) ObjectHeap::allocate(size JVM_NO_CHECK);
  if (raw) {
    raw->initialize(klass->obj());
  }
  return raw;
}

#if !ROMIZED_PRODUCT || ENABLE_ISOLATES
ReturnOop Universe::allocate_near(FarClass* klass JVM_TRAPS) {
  return generic_allocate_near(klass, NearDesc::allocation_size() 
                               JVM_NO_CHECK_AT_BOTTOM);
}
#endif

ReturnOop Universe::allocate_java_near(JavaClass* klass JVM_TRAPS) {
  size_t size = JavaNearDesc::allocation_size();
  JavaNear::Raw result = generic_allocate_near(klass, size JVM_NO_CHECK);
  if (result) {
    ClassInfo::Raw class_info = klass->class_info();
    GUARANTEE(class_info.not_null(), "class info must not be null");
    result().set_class_info(&class_info);
  }
  return result;
}

#if !ROMIZED_PRODUCT
ReturnOop Universe::allocate_obj_near(FarClass* klass JVM_TRAPS) {
  return generic_allocate_near(klass, ObjNearDesc::allocation_size() 
                               JVM_NO_CHECK_AT_BOTTOM);
}
#endif

//
// Allocation of near classes
//
#if !ROMIZED_PRODUCT
ReturnOop Universe::generic_allocate_near_class(FarClass* klass,
                                                size_t class_size,
                                                jint instance_size,
                                                jubyte* extern_oop_map JVM_TRAPS)
{
  NearClassDesc* raw = (NearClassDesc*)
      ObjectHeap::allocate(class_size JVM_NO_CHECK);
  if (raw) {
    raw->initialize(klass->obj(), class_size, instance_size, extern_oop_map);
  }
  return raw;
}
#endif

//
// Allocation of Java class classes
//

ReturnOop Universe::new_obj_array_class(JavaClass* element_class JVM_TRAPS) {
  UsingFastOops fast_oops;

  check_class_list_size(JVM_SINGLE_ARG_CHECK_0);

  // (1) Allocate the ClassInfo
  ClassInfo::Fast info = allocate_class_info(JavaVTable::base_vtable_size(),
                                             0, 0, ISOLATES_PARAM(0)
                                             true JVM_CHECK_0);

  // (2) Allocate the InstanceClass
  jint object_size = JavaClassDesc::allocation_size(0, 0);

  ObjArrayClassDesc* raw =
      (ObjArrayClassDesc*) ObjectHeap::allocate(object_size JVM_ZCHECK_0(raw));
  raw->initialize(obj_array_class_class()->prototypical_near(),
                  object_size,
                  InstanceSize::size_obj_array,
                  (jubyte*)oopmap_Empty);
  ObjArrayClass::Fast result = ReturnOop(raw);
  result().set_class_info(&info);

  Near::Fast prototype = allocate_java_near(&result JVM_CHECK_0);
  JavaClass::Fast jc = element_class;
  result().set_prototypical_near(&prototype);
  result().set_element_class(&jc);

  if (class_list()->not_null()) {
    register_java_class(&result);
  }

  if (!is_bootstrapping()) {
    result().set_super(object_class());
    result().initialize_vtable();
#if ENABLE_ISOLATES
    result().setup_task_mirror(0, 0, false JVM_CHECK_0);
#else
    result().setup_java_mirror(JVM_SINGLE_ARG_CHECK_0);
#endif
    result().compute_name(JVM_SINGLE_ARG_CHECK_0);
  }

  // Make sure ACC_ARRAY_CLASS is set.
  GUARANTEE(result().access_flags().is_array_class(), "bad access flags");

  return result;
}

void Universe::fill_heap_gap(address ptr, size_t size_to_fill) {
  GUARANTEE(size_to_fill >= 0, "sanity");
  GUARANTEE(!(size_to_fill & 0x3), "alignment");
  OopDesc* filler = (OopDesc*)ptr;
  if (size_to_fill == sizeof(OopDesc*)) {
    // Shrink by one word only, allocate dummy java.lang.Object
    filler->reinitialize(object_class()->prototypical_near());
  } else {
    // Shrink by multiple words, allocate dummy byte array
    DIRTY_HEAP(filler, size_to_fill);
    ((ArrayDesc*) filler)->reinitialize(
                           byte_array_class()->prototypical_near(),
                           size_to_fill - ArrayDesc::header_size());
  }
}
ReturnOop Universe::shrink_object(Oop* m, size_t new_size, bool down) {
  size_t diff = m->object_size() - new_size;
  if (diff == 0) {
    return m->obj();
  }

  GUARANTEE((new_size & 3) == 0, "should be aligned");
  GUARANTEE(diff > 0, "sanity check");

  OopDesc *result;
  OopDesc *filler;
  if (down) {
    // The default.  We keep the object at its current location
    result = m->obj();
    filler = (OopDesc*) (((address)result) + new_size);
  } else {
    // We want to save the end of the object and return the beginning of the
    // object.
    filler = m->obj();
    result = (OopDesc*) (((address) filler) + diff);
    result->reinitialize(m->klass());
  }
  fill_heap_gap((address)filler, diff);
  return result;
}

void Universe::oops_do( void do_oop(OopDesc**), const bool young_only) {
  (void)young_only;
  {
    for( int index = 0; index < __number_of_persistent_handles; index++) {
      do_oop(&persistent_handles[index]);
    }
  }
#if !ROMIZED_PRODUCT
  Symbols::oops_do(do_oop);
#endif
#if ENABLE_KVM_COMPAT
  if (!_before_main) {
    kvmcompat_oops_do(do_oop);
  }
#endif
}

void Universe::update_relative_pointers() {
  if (TraceTaskContext) {
    tty->print("Upd 0x%x, ", _class_list_base);
  }
  _class_list_base = (address)class_list()->obj();
  _class_list_base += ObjArray::base_offset();
  if (TraceTaskContext) {
    tty->print_cr("0x%x", _class_list_base);
  }

#if ENABLE_ISOLATES
  _raw_task_list = (OopDesc*)task_list()->obj();
  _task_class_init_marker = task_class_init_marker()->obj();
  _mirror_list_base = (address)mirror_list()->obj();
  _mirror_list_base += ObjArray::base_offset();
#endif
  _interned_string_near_addr = interned_string_near()->obj();
}

#if ENABLE_ISOLATES
ReturnOop Universe::task_from_id(int task_id) {
  // We don't use Universe::task_list() directly because it may be temporarily
  // clobbered during GC.
  ObjArray::Raw list = _raw_task_list;
  return list().obj_at(task_id);
}

inline void Universe::set_task_list(ObjArray *tl) {
  *task_list() = tl->obj();
  _raw_task_list = tl->obj();
}
#endif

void Universe::init_task_list(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_ISOLATES
  {
    ObjArray::Raw list = Universe::new_task_list(JVM_SINGLE_ARG_MUST_SUCCEED);
    set_task_list(&list);
  }

#if USE_BINARY_IMAGE_LOADER
  *global_binary_images() = new_obj_array(MAX_TASKS JVM_MUST_SUCCEED);
#if ENABLE_LIB_IMAGES
  *global_binary_persistante_handles() = new_obj_array(MAX_TASKS JVM_MUST_SUCCEED);
#if USE_IMAGE_MAPPING
  *global_image_handles() = new_int_array(MAX_TASKS JVM_MUST_SUCCEED);
#endif  
#endif
#endif

  // Task 0 reserved for system use
  Task::allocate_task(0 JVM_NO_CHECK_AT_BOTTOM);

#else
  *current_task_obj()   = allocate_task(JVM_SINGLE_ARG_CHECK);
  *symbol_table()       = SymbolTable::initialize(64 JVM_CHECK);
  *string_table()       = StringTable::initialize(64 JVM_CHECK);
  *global_refs_array()  = RefArray::initialize(JVM_SINGLE_ARG_CHECK);
#endif
}

void Universe::create_first_task(const JvmPathChar* classpath JVM_TRAPS) {
  UsingFastOops fast_oops;

#if ENABLE_ISOLATES  
  Task::Fast task = Task::allocate_task(Task::FIRST_TASK JVM_CHECK);
  task().add_thread();
  Thread::current()->set_task_id(Task::FIRST_TASK);
  // The system will not properly bootstrap if
  // the first isolate reserves almost all available memory.
  ObjectHeap::set_task_memory_reserve_limit(Task::FIRST_TASK,
                                            ReservedMemory, TotalMemory);
#endif
  
  set_current_task(Task::FIRST_TASK);
  TypeArray::Fast path = FilePath::convert_to_unicode(
    classpath, fn_strlen(classpath) JVM_CHECK);
  ObjArray::Raw cp = setup_classpath(&path JVM_CHECK);
  Task::current()->set_app_classpath(cp());
  Task::current()->set_sys_classpath(Universe::empty_obj_array()->obj());
}

#if ENABLE_ISOLATES
ReturnOop Universe::copy_strings_to_byte_arrays(OopDesc* strings JVM_TRAPS) {
  UsingFastOops fastoops;
  ObjArray::Fast string_array = strings;
  const int new_len = string_array.is_null() ? 0 : string_array().length();  
  ObjArray::Fast result = new_obj_array(new_len JVM_OZCHECK(result));
  for( int i = 0; i < new_len; i++ ) {
    String::Raw str = string_array().obj_at(i);
    TypeArray::Raw sym = Symbol::copy_string_to_byte_array(str.obj(), false
                                                           JVM_OZCHECK(sym));
    result().obj_at_put(i, sym);
  }
  return result.obj();
}

ReturnOop Universe::copy_strings_to_char_arrays(OopDesc* strings JVM_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast string_array = strings;
  GUARANTEE(string_array.not_null(), "no nulls allowed today");
  const int length = string_array().length();
  ObjArray::Fast result = new_obj_array(char_array_class(), length
                                        JVM_OZCHECK(result));
  String::Fast s;
  TypeArray::Fast chars, chars_copy;

  for (int i = 0; i < length; i++) {
    s = string_array().obj_at(i);
    chars = s().value();
    int char_count = s().count();
    chars_copy = Universe::new_char_array(char_count JVM_OZCHECK(chars_copy));
    TypeArray::array_copy(&chars, s().offset(), &chars_copy, 0, char_count);
    result().obj_at_put(i, &chars_copy);
  }
  return result;
}

ReturnOop Universe::make_strings_from_char_arrays(OopDesc* chars JVM_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast char_arrays = chars;
  GUARANTEE(char_arrays.not_null(), "no nulls allowed today");
  const int length = char_arrays().length();
  ObjArray::Fast result = new_obj_array(string_class(), length
                                        JVM_OZCHECK(result));
  TypeArray::Fast ca;
  String::Fast s;

  for (int i = 0; i < length; i++) {
    ca = char_arrays().obj_at(i);
    s = new_string(&ca, 0, ca().length() JVM_OZCHECK(s));
    result().obj_at_put(i, &s);
  }
  return result;
}

ReturnOop Universe::deep_copy(OopDesc* obj JVM_TRAPS) {
  if (obj == NULL) {
    return NULL;
  } else if (obj->is_obj_array()) {
    UsingFastOops fast_oops;
    ObjArray::Fast orig = obj;
    const int length = orig().length();
    ObjArray::Fast copy = new_obj_array(length JVM_OZCHECK(copy));
    for (int i = 0; i < length; i++) {
      OopDesc* p = deep_copy(orig().obj_at(i) JVM_CHECK_0); // can be NULL
      copy().obj_at_put(i, p);
    }
    return copy.obj();
  } else if (obj->is_string()) {
    UsingFastOops fast_oops;
    String::Fast orig = obj;
    const int offset = orig().offset();
    const int length = orig().count();
    TypeArray::Fast orig_value = orig().value();
    TypeArray::Fast copy_value = new_char_array(length JVM_OZCHECK(copy_value));
    TypeArray::array_copy(&orig_value, offset, &copy_value, 0, length);
    return new_string(&copy_value, 0, length JVM_NO_CHECK_AT_BOTTOM);
  } else {
    SHOULD_NOT_REACH_HERE();
    return NULL;
  }
}
#endif // ENABLE_ISOLATES

#ifndef PRODUCT
// Checks whether handle is inside block of persistent handles
bool Universe::is_persistent_handle(Oop* obj) {
  BasicOop* start = (BasicOop*) &(persistent_handles[0]);
  BasicOop* top   = (BasicOop*) &(persistent_handles[__number_of_persistent_handles]);
  return obj >= start && obj < top;
}

void Universe::allocate_gc_dummies(JVM_SINGLE_ARG_TRAPS) {
  for (int i = 0; i < GCDummies; i++) {
    ObjArray temp = allocate_array(object_array_class(), 1, sizeof(OopDesc*)
                                   JVM_CHECK);
    temp.obj_at_put(0, gc_dummies());
    *gc_dummies() = temp;
  }
}

void Universe::release_gc_dummy() {
  if (gc_dummies()->not_null()) {
    *gc_dummies() = gc_dummies()->obj_at(0);
  }
}
#endif // PRODUCT

#if USE_DEBUG_PRINTING 
void Universe::print_values_on(Stream* st) {
  st->print_cr("Universe");
  for (int index = 0; index < __number_of_persistent_handles; index++) {
    Oop value = persistent_handles[index];
    st->print("  [%d] ", index);
    value.print_value_on(st);
    st->print_cr("");
  }
}

/*
 * The following macro makes it easy to print out the values of the
 * persistent handles inside a C++ debugger. Some debuggers, such as
 * VC++, have problems showing values like Universe::jvm_class(). To
 * work around this problem, use Universe_jvm_class instead.
 */

#define UNIVERSE_HANDLES_PEEK(name, type) \
 type& Universe_##name = *((type*)&persistent_handles[Universe::name##_index]);

UNIVERSE_HANDLES_DO(UNIVERSE_HANDLES_PEEK)


/*
 * The following code is called by Globals.cpp to print the names
 * of all persistent handles.
 */

#define UNIVERSE_HANDLES_DUMP(name, type) \
 tty->print_cr("%-40s = %d", #name, i++);

void Universe::print_persistent_handle_definitions() {
  int i=0;

  UNIVERSE_HANDLES_DO(UNIVERSE_HANDLES_DUMP);
}
#endif // USE_DEBUG_PRINTING

