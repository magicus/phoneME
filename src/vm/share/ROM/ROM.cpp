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

#include "incls/_precompiled.incl"
#include "incls/_ROM.cpp.incl"

/*====================================================================
 * Comment:
 *
 * There are two ROM image loaders in this file.
 *
 * The first one, known as the "static ROM image" loader,
 * is used for loading in statically linked ROM images.
 * Static ROM images are C++ source/object files generated
 * by the ROMWriter.  The most common static ROM image is
 * the ROMImage.cpp/ROMImage.o file, which usually contains
 * the CLDC/MIDP system classes.  The static ROM images are
 * linked in with the rest of the VM source code when the
 * virtual machine is being built.  If you don't use the
 * static ROM image loader, the VM will have to load all
 * the system classes dynamically from the classpath (JAR
 * files).  This is very slow, so the static ROM image
 * loader should be enabled at all times.
 *
 * The second ROM image loader loads in "dynamic ROM images".
 * Dynamic ROM images are binary files that are contained
 * separately from the VM.  They are loaded in dynamically
 * when the VM starts.  A typical example of a dynamic ROM
 * image is a "ROM_binary.bun" file that contains the code
 * of a Java application.  Dynamic ROM images can be thought
 * of as more optimized versions of JAR files.  Dynamic ROM
 * images can be loaded far more efficiently than regular 
 * JAR files, and they also save RAM because the dynamic ROM
 * images can be executed directly from Flash/storage memory.
 *===================================================================*/

#if ENABLE_PERFORMANCE_COUNTERS
ROM_PerformanceCounters rom_perf_counts;
#endif

void ROM::initialize(const JvmPathChar* classpath) {
  (void)classpath;

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  init_rom_text_constants();
#endif

  if (!_rom_has_linked_image) {
    GUARANTEE(UseROM == false, "sanity");
  }

  // At this point the ObjectHeap is not yet initialized, so we can
  // only initialize the part that doesn't require the ObjectHeap.
  // We have to do this here to facilitate ROMBundle::preload()
  if (UseROM) {
    if (_rom_is_relaunchable) {
      if (_rom_data_block_size > 0) {
        jvm_memcpy(_rom_data_block, _rom_data_block_src, _rom_data_block_size);
      }
      jvm_memcpy(_rom_method_variable_parts, _rom_method_variable_parts_src,
                 _rom_method_variable_parts_size);
    }
  }

#if USE_IMAGE_PRELOADING
  // In SVM mode, we can load the app image into the low address of
  // the ObjectHeap. In MVM we don't do this because we need to handle
  // multiple app images.
#if !ENABLE_LIB_IMAGES
  if (!GenerateROMImage) 
#endif
  {
    ROMBundle::preload(classpath);
  }
#endif

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  ROM::arrange_text_block();
#endif
}

int ROM::_heap_relocation_offset;

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  juint ROM::_min_text_seg_addr; 
  juint ROM::_text_total_size;
  juint ROM::_rom_text_block_segment_sizes[ROM::TEXT_BLOCK_SEGMENTS_COUNT];
  juint ROM::_rom_text_block_segments[ROM::TEXT_BLOCK_SEGMENTS_COUNT];

void ROM::init_rom_text_constants() {
  ROM::_rom_text_block_segment_sizes[0] = _rom_text_block0_size;
  ROM::_rom_text_block_segment_sizes[1] = _rom_text_block1_size;
  ROM::_rom_text_block_segment_sizes[2] = _rom_text_block2_size;
  ROM::_rom_text_block_segment_sizes[3] = _rom_text_block3_size;
  ROM::_rom_text_block_segment_sizes[4] = _rom_text_block4_size;
  ROM::_rom_text_block_segment_sizes[5] = _rom_text_block5_size;
  ROM::_rom_text_block_segment_sizes[6] = _rom_text_block6_size;
  ROM::_rom_text_block_segment_sizes[7] = _rom_text_block7_size;
  ROM::_rom_text_block_segment_sizes[8] = _rom_text_block8_size;
  ROM::_rom_text_block_segment_sizes[9] = _rom_text_block9_size;

  ROM::_rom_text_block_segments[0] = (juint)_rom_text_block0;
  ROM::_rom_text_block_segments[1] = (juint)_rom_text_block1;
  ROM::_rom_text_block_segments[2] = (juint)_rom_text_block2;
  ROM::_rom_text_block_segments[3] = (juint)_rom_text_block3;
  ROM::_rom_text_block_segments[4] = (juint)_rom_text_block4;
  ROM::_rom_text_block_segments[5] = (juint)_rom_text_block5;
  ROM::_rom_text_block_segments[6] = (juint)_rom_text_block6;
  ROM::_rom_text_block_segments[7] = (juint)_rom_text_block7;
  ROM::_rom_text_block_segments[8] = (juint)_rom_text_block8;
  ROM::_rom_text_block_segments[9] = (juint)_rom_text_block9;
}

void ROM::arrange_text_block() {
  // First sort the text segments
  ROM::sort_text_segments();

  // Then set the text segment's minimum and maximum addresses.
  _min_text_seg_addr = _rom_text_block_segments[0];
  _text_total_size = 
    _rom_text_block_segments[ROM::TEXT_BLOCK_SEGMENTS_COUNT - 1] + 
    _rom_text_block_segment_sizes[ROM::TEXT_BLOCK_SEGMENTS_COUNT - 1] - 
    _min_text_seg_addr;
#ifdef AZZERT
  int delta = _text_total_size;
  for (int i = 0; i < ROM::TEXT_BLOCK_SEGMENTS_COUNT; i++) {
    delta -= _rom_text_block_segment_sizes[i];
  }  
  GUARANTEE(delta < MAX_SECTIONS_GAP, 
    "TEXT sections should be separated only by alignment gaps");
#endif
}

void ROM::sort_text_segments() {
  for(int i = 0; i < ROM::TEXT_BLOCK_SEGMENTS_COUNT - 1; i++) {
    int min = i;
    for(int j = i + 1; j < ROM::TEXT_BLOCK_SEGMENTS_COUNT; j++) {
      if (_rom_text_block_segments[j] < _rom_text_block_segments[min]) {
        min = j;
      }
    }
    // swap segments first addresses
    juint temp = _rom_text_block_segments[min];
    _rom_text_block_segments[min] = _rom_text_block_segments[i];
    _rom_text_block_segments[i] = temp;

    // swap segments sizes
    temp = _rom_text_block_segment_sizes[min];
    _rom_text_block_segment_sizes[min] = _rom_text_block_segment_sizes[i];
    _rom_text_block_segment_sizes[i] = temp;
  }
}

int ROM::text_segment_of(const OopDesc* obj) {
  const int seg_count = ROM::TEXT_BLOCK_SEGMENTS_COUNT;
  int offset;
  for (int pass = 0; pass < seg_count; pass++) {
    offset = ((int)obj) - ((int)_rom_text_block_segments[pass]);
    if (offset >= 0 && (juint)offset < _rom_text_block_segment_sizes[pass]) {
      return pass;
    }
  }
  return ROM::MAIN_SEGMENT_INDEX;
}
#endif // ENABLE_SEGMENTED_ROM_TEXT_BLOCK

/*====================================================================
 * The code for the static ROM image loader starts here
 *===================================================================*/

/**
 * These global variables makes sure ROMImage.o is build with
 * the same flags as the VM.
 */
#if !USE_BINARY_IMAGE_LOADER
const int* rom_linkcheck_hle  = &_ROM_LINKCHECK_HLE;
const int* rom_linkcheck_mffl = &_ROM_LINKCHECK_MFFL;
const int* rom_linkcheck_mffd = &_ROM_LINKCHECK_MFFD;
#endif

#if USE_BINARY_IMAGE_GENERATOR
OopDesc* ROM::_romized_heap_marker;
#endif
OopDesc** _romized_heap_top;

#if !defined(PRODUCT) || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
OopDesc* ROM::_original_class_name_list;
OopDesc* ROM::_original_method_info_list;
OopDesc* ROM::_original_fields_list;
OopDesc* ROM::_alternate_constant_pool;
#endif

ROMWriter* _rom_writer;

// here we optimize in the subtle way - we save one comparision
// by using unsigned math
inline bool ROM::heap_src_block_contains(address target) {
  juint p = ((juint)target) - ((juint)&_rom_heap_block[0]);
  bool ret = p < (juint)_rom_heap_block_size;
  return ret;
}

void ROM::relocate_pointer_to_heap(OopDesc** p) {
  OopDesc* const obj = *p;
  if( heap_src_block_contains( address(obj) ) ) {
    *p = DERIVED(OopDesc*, obj, _heap_relocation_offset);
  }
}

void ROM::relocate_heap_block() {
  OopDesc* q = (OopDesc*)_romized_heap_top;
  int offset = _heap_relocation_offset;

  while (q < (OopDesc*)_inline_allocation_top) { 
    relocate_pointer_to_heap((OopDesc**)q);
    FarClassDesc* blueprint = q->blueprint();
    if (heap_src_block_contains((address)blueprint)) { 
      blueprint = DERIVED(FarClassDesc*, blueprint, offset);
    }
    q->oops_do_for(blueprint, relocate_pointer_to_heap);
    q = DERIVED(OopDesc*, q, q->object_size_for(blueprint));
  }
  GUARANTEE(q == (OopDesc*)_inline_allocation_top, "sanity");
}

/* This function is used for loading in a statically linked ROM image */
/* A statically linked ROM image is contained in a C++ source/object file */
/* (usually 'ROMImage.cpp'/'ROMImage.o') */

bool ROM::link_static(OopDesc** ram_persistent_handles, int num_handles) {
  check_consistency();

  // (1) Copy heap, data, and persistent handles to their final locations
  if (ObjectHeap::free_memory() <= (size_t)_rom_heap_block_size) {
    return false;
  }
#if !ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  _rom_text_block_size_fast = _rom_text_block_size;
#endif
  _rom_data_block_size_fast = _rom_data_block_size;

  // IMPL_NOTE: avoid doing a full GC if necessary.
  ObjectHeap::expand_young_generation();
#ifdef AZZERT
  const int num_rom_handles = _rom_persistent_handles_size/sizeof(int);
  GUARANTEE(num_rom_handles == num_handles, "wrong ROM image");
#endif
  (void)num_handles;

  jvm_memcpy(_inline_allocation_top, &_rom_heap_block[0], _rom_heap_block_size);
  jvm_memcpy(ram_persistent_handles, _rom_persistent_handles,
             _rom_persistent_handles_size);
#if ENABLE_HEAP_NEARS_IN_HEAP
  jvm_memcpy(ram_persistent_handles + _rom_persistent_handles_size/sizeof(int), _rom_rom_duplicated_handles,
             _rom_rom_duplicated_handles_size);
#endif
#if !ROMIZED_PRODUCT
  int num_bytes = Symbols::number_of_system_symbols() * sizeof(OopDesc*);
  jvm_memcpy(system_symbols, _rom_system_symbols_src, num_bytes);
#endif

#if ENABLE_ISOLATES
  *Universe::system_mirror_list() = *Universe::mirror_list();
#endif

  // (2) Allocate the heap space and calculate the offset
  _heap_relocation_offset= DISTANCE(&_rom_heap_block[0],_inline_allocation_top);
  _romized_heap_top = _inline_allocation_top;
  _inline_allocation_top = DERIVED(OopDesc**,
        _inline_allocation_top, _rom_heap_block_size);

  OopDesc **permanent_top = 
      DERIVED(OopDesc**, _romized_heap_top, _rom_heap_block_permanent_size);
  ObjectHeap::rom_init_heap_bounds(_inline_allocation_top, permanent_top);

#if USE_BINARY_IMAGE_GENERATOR
  SETUP_ERROR_CHECKER_ARG;
  ROM::set_romized_heap_marker(JVM_SINGLE_ARG_MUST_SUCCEED);
#endif

  // (3) Relocate pointers inside the objects newly loaded into the heap.
  relocate_heap_block();

  // (4) Relocate persistent handles into heap_block
  Universe::oops_do(relocate_pointer_to_heap);

  // (5) Relocate data block pointers into heap_block
  relocate_data_block();

  // (7) Misc initialization
  Universe::set_number_of_java_classes(_rom_number_of_java_classes);
  *Universe::system_class_list() = Universe::class_list()->obj();

  // (8) Method entry initialization (for handling -comp flag)
  Method::update_rom_default_entries();

#if ARM || HITACHI_SH
  // (9) Copy this so that ARM has fast access to its value
  _rom_constant_pool_fast = (int*)_rom_constant_pool;
#endif

  return true;
}

#ifndef PRODUCT
#define COMPARE_FRAME_OFFSETS_CHECKER(x) \
  GUARANTEE_R(_rom_check_##x==x(), "AOT compiler must have same frame layout");

void ROM::check_consistency() {
  // The beginning of the heap must be filled with objects that will never be 
  // GC'ed -- i.e., the InstanceClassDesc and JavaClassObj of system classes.
  // This fact is used to make young GC faster during ROM::oops_do().
  GUARANTEE_R(_inline_allocation_top == _heap_start, 
              "Required by permanent heap block");

  if (_rom_compilation_enabled) {
    GUARANTEE_R(_rom_check_JavaFrame__arg_offset_from_sp_0 ==
                JavaFrame__arg_offset_from_sp(0), "sanity");
    FRAME_OFFSETS_DO(COMPARE_FRAME_OFFSETS_CHECKER);
    GUARANTEE(_rom_generator_soft_float_enabled == 
                  (ENABLE_SOFT_FLOAT && ENABLE_FLOAT),
              "AOT compiler must have same GP table layout");
    GUARANTEE(_rom_generator_target_msw_first_for_double == 
                  MSW_FIRST_FOR_DOUBLE,
              "AOT compiler must have same double layout\n"
              "Check TARGET_MSW_FIRST_FOR_DOUBLE value in romgen.");
  }
}
#endif

#if !defined(PRODUCT) || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
// Init the symbols for debug or JVMPI interface.
void ROM::init_debug_symbols(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_ROM_DEBUG_SYMBOLS || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
  // Initialize mapping from renamed ".unknown." methods/field
  // to their original names (non-product build only)
    if (LoadROMDebugSymbols
#if ENABLE_JVMPI_PROFILE  
        || UseJvmpiProfiler
#endif 
    ) {
    initialize_original_class_name_list(JVM_SINGLE_ARG_CHECK);
    initialize_original_method_info_list(JVM_SINGLE_ARG_CHECK);
    initialize_original_fields_list(JVM_SINGLE_ARG_CHECK);
    initialize_alternate_constant_pool(JVM_SINGLE_ARG_CHECK);
  }
#endif
}
#endif // !PRODUCT

/*====================================================================
 * The code for the dynamic ROM image loader starts here
 *===================================================================*/


#if (!defined(PRODUCT) && ENABLE_ROM_DEBUG_SYMBOLS) \
       || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE

void ROM::initialize_original_class_name_list(JVM_SINGLE_ARG_TRAPS) {
  int count = _rom_original_class_info_count;

  ObjArray name_list;
  if (count == 0) {
    _original_class_name_list = NULL;
    return;
  } else {
    name_list = Universe::new_obj_array(count JVM_CHECK);
    _original_class_name_list = name_list.obj();
  }

  for (int i=0; i<count; i++) {
    const OriginalClassInfo *clsinfo = &_rom_original_class_info[i];
    if (clsinfo->name != NULL) {
      // +1 to skip the first char, which is the type of this symbol 
      // (always == 3 in this case).
      Symbol name = SymbolTable::symbol_for(clsinfo->name+1 JVM_CHECK);
      name_list.obj_at_put(i, &name);
    }
  }
}

void ROM::initialize_original_method_info_list(JVM_SINGLE_ARG_TRAPS) {
  int count = _rom_original_class_info_count;

  ObjArray info_list;
  if (count == 0) {
    _original_method_info_list = NULL;
    return;
  } else {
    info_list = Universe::new_obj_array(count JVM_CHECK);
    _original_method_info_list = info_list.obj();
  }

  for (int i=0; i<count; i++) {
    const OriginalClassInfo *clsinfo = &_rom_original_class_info[i];
    const OriginalMethodInfo *minfo = clsinfo->methods;

    for (int n=0; n<clsinfo->num_methods; n++, minfo++) {
      Method method = (OopDesc*)minfo->method;
      if (method.not_null()) {
        ObjArray old  = info_list.obj_at(i);
        ObjArray info = Universe::new_obj_array(3 JVM_CHECK);
        // +1 to skip the first char, which is the type of this symbol 
        // (always == 3 in this case).
        Symbol name = SymbolTable::symbol_for(minfo->name+1 JVM_CHECK);

        info.obj_at_put(INFO_OFFSET_METHOD, &method);
        info.obj_at_put(INFO_OFFSET_NAME,   &name);
        info.obj_at_put(INFO_OFFSET_NEXT,   &old);

        info_list.obj_at_put(i, &info);
      }
    }
  }
}

void ROM::initialize_original_fields_list(JVM_SINGLE_ARG_TRAPS) {
  int count = _rom_original_class_info_count;

  ObjArray info_list;

  if (count == 0) {
    _original_fields_list = NULL;
    return;
  } else {
    info_list = Universe::new_obj_array(count JVM_CHECK);
    _original_fields_list = info_list.obj();
  }

  for (int i=0; i<count; i++) {
    const OriginalClassInfo *clsinfo = &_rom_original_class_info[i];
    const OriginalFieldInfo *finfo = clsinfo->fields;
    int num = clsinfo->num_fields;

    if (num == 0) {
      continue;
    }

    GUARANTEE((num % 5) == 0, "sanity");

    TypeArray fields = Universe::new_short_array(num JVM_CHECK);
    for (int idx=0; idx<num; idx+=5, finfo++) {
      fields.ushort_at_put(idx + Field::ACCESS_FLAGS_OFFSET,finfo->flags);
      fields.ushort_at_put(idx + Field::NAME_OFFSET,        finfo->name_index);
      fields.ushort_at_put(idx + Field::SIGNATURE_OFFSET,   finfo->sig_index);
      fields.ushort_at_put(idx + Field::OFFSET_OFFSET,      finfo->offset);
      fields.ushort_at_put(idx + Field::INITVAL_OFFSET,     0xffff);
    }
    info_list.obj_at_put(i, &fields);
  }
}
void ROM::initialize_alternate_constant_pool(JVM_SINGLE_ARG_TRAPS) {
  int count = _rom_alternate_constant_pool_count;

  ConstantPool cp = Universe::new_constant_pool(count JVM_CHECK);
  _alternate_constant_pool = cp.obj();
  for (int i=0; i<count; i++) {
    const char* src = _rom_alternate_constant_pool_src[i];
    char type = *src++;
    Symbol name = SymbolTable::symbol_for(src JVM_CHECK);

    if (ObjectHeap::contains(name.obj())) {
      switch ((int)type) {
      case 0x01:
        // name is a field type. Do the following to satisfy 
        // FieldType::type_check().
        name.set_klass(Universe::method_signature_near());
        break;
      case 0x02:
        // name is a signature. Do the following to satisfy 
        // Signature::type_check().
        name.set_klass(Universe::field_type_near());
        break;
      }
    }
    cp.symbol_at_put(i, &name);
  }
}

#endif //!PRODUCT || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE

bool ROM::is_restricted_package(const char *name, int pkg_length) {

  char *rp = (char*)&_rom_restricted_packages[0];
  char *rp2;

  while (*rp) {
    int len = *((unsigned char*)rp); // may be up to 255;
    rp ++;
   
    //Checking for the asterisk case...
    rp2 = rp;
    rp2 += (len - 2);
    if(jvm_strncmp(rp2, "/" "*", 2) == 0) {
      //If foo.bar.* has been hidden, we don't want classes in 
      // foo.* to be hidden.
      if(pkg_length < (len-2)) return false;
      if(jvm_strncmp(rp, name, pkg_length) == 0) {
        return true;
      }
    }

    if (len == pkg_length) {
      if (jvm_memcmp(rp, name, pkg_length) == 0) {
        return true; // we have a match. The package is restricted.
      }
    }
    rp += len;
  }
  return false;
}

size_t ROM::get_max_offset() {
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  size_t text_size = _text_total_size;
#else
  size_t text_size = _rom_text_block_size;
#endif
  size_t data_size = _rom_data_block_size;
  size_t max_offset;

  if (text_size > data_size) {
    max_offset = text_size;
  } else {
    max_offset = data_size;
  }

  max_offset <<= 1;
  return max_offset;
}

void ROM::relocate_data_block() {
  OopDesc* obj = (OopDesc*)_rom_data_block;
  OopDesc* end = DERIVED(OopDesc*, _rom_data_block, _rom_data_block_size);

  while (obj < end) {
    // The data block contains only objects whose near object and blueprint
    // won't be moved during GC. This means at anytime obj->blueprint()
    // is always available and won't be encoded by the GC during compaction
    // phase.
    relocate_pointer_to_heap((OopDesc**)obj);

    OopDesc *near_obj = obj->klass();
    GUARANTEE(ROM::system_contains(near_obj) ||
              ObjectHeap::permanent_contains((OopDesc**)near_obj),
              "near object must never move");

    FarClassDesc* blueprint = (FarClassDesc*)near_obj->klass();
    GUARANTEE(ROM::system_text_contains(blueprint) ||
              ObjectHeap::permanent_contains((OopDesc**)near_obj),
              "blueprint object must never move");

    obj->oops_do_for(blueprint, relocate_pointer_to_heap);
    obj = DERIVED(OopDesc*, obj, obj->object_size_for(blueprint));
  }
  GUARANTEE(obj == end, "sanity");
}

#ifndef PRODUCT
void ROM::system_method_variable_parts_oops_do(void do_oop(OopDesc**)) {
  (void)do_oop;
#if 0
  int count = _rom_method_variable_parts_size / sizeof(MethodVariablePart);
  MethodVariablePart *ptr = (MethodVariablePart*)&_rom_method_variable_parts[0];
  MethodVariablePart *end = ptr + count;
  OopDesc ** heap_start = ::_heap_start; // cache in register
  OopDesc ** heap_top   = ::_heap_top;   // cache in register

  // IMPL_NOTE: ptr->_execution_entry may point inside the CompiledMethodDesc, so
  // oop verification may fail. This is disabled right now.
  for (; ptr<end; ptr++) {
    OopDesc **obj = (OopDesc**)(ptr->_execution_entry);
    if (heap_start <= obj && obj < heap_top) {
      do_oop((OopDesc**)&(ptr->_execution_entry));
    } else {
#if ENABLE_METHOD_TRAPS
      InvokeTrap* trap = ptr->get_trap();
      if (trap != NULL) {
        obj = (OopDesc**)(trap->old_entry);
        if (heap_start <= obj && obj < heap_top) {
          do_oop((OopDesc**) &(trap->old_entry));
        }
      }
#endif
    }
  }
#endif
}
#endif 

void ROM::oops_do(void do_oop(OopDesc**), bool do_all_data_objects,
                  bool do_method_variable_parts) {
#if ENABLE_PERFORMANCE_COUNTERS
  jlong start_time = Os::elapsed_counter();
#endif

  OopDesc* obj = (OopDesc*)_rom_data_block;
  OopDesc* end;
  if (do_all_data_objects) {
    end = DERIVED(OopDesc*, _rom_data_block, _rom_data_block_size);
  } else {
    end = DERIVED(OopDesc*, _rom_data_block, _rom_data_block_scanned_size);
  }

  while (obj < end) {
    // The data block contains only objects whose near object and blueprint
    // won't be moved during GC. This means at anytime obj->blueprint()
    // is always available and won't be encoded by the GC during compaction
    // phase.
    OopDesc *near_obj = obj->klass();
    GUARANTEE(ROM::system_contains(near_obj) ||
              ObjectHeap::permanent_contains((OopDesc**)near_obj),
              "near object must never move");

    FarClassDesc* blueprint = (FarClassDesc*)near_obj->klass();
    GUARANTEE(ROM::system_contains(blueprint) ||
              ObjectHeap::permanent_contains((OopDesc**)near_obj),
              "blueprint object must never move");

    obj->oops_do_for(blueprint, do_oop);
    obj = DERIVED(OopDesc*, obj, obj->object_size_for(blueprint));
  }
  GUARANTEE(obj == end, "sanity");

#ifdef PRODUCT
  (void)do_method_variable_parts;

#if ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
  if (UseROM || GenerateROMImage) {
    do_oop((OopDesc**)&_original_class_name_list);
    do_oop((OopDesc**)&_original_method_info_list);
    do_oop((OopDesc**)&_original_fields_list);
    do_oop((OopDesc**)&_alternate_constant_pool);
  }
#endif

#else
  // If we use the compiler area, no method execution entry will
  // ever move during regular GC -- it's either a C function pointer,
  // or a CompiledMethodDesc*, which is moved only during collection
  // of the compiler area.
  //
  // This code is executed only by VerifyGC code to verify that all
  // variable parts point to valid CompiledMethodDesc* objects.

  if (do_method_variable_parts) {
    system_method_variable_parts_oops_do(do_oop);

#if USE_BINARY_IMAGE_LOADER
    GUARANTEE(UseROM, "Monet requires the system classes to be romized");
#if ENABLE_ISOLATES
    ObjArray::Raw list = Universe::global_binary_images();
#else
    ObjArray::Raw list = Task::current()->binary_images();
#endif

    if (list.not_null()) {
      for (int i=0; i<list().length(); i++) {
        ROMBundle *bundle = (ROMBundle*)list().obj_at(i);
        if (bundle == NULL) {
          continue;
        }
        bundle->method_variable_parts_oops_do(do_oop);
      }
    }
#endif

  }

  if (UseROM || GenerateROMImage) {
    do_oop((OopDesc**)&_original_class_name_list);
    do_oop((OopDesc**)&_original_method_info_list);
    do_oop((OopDesc**)&_original_fields_list);
    do_oop((OopDesc**)&_alternate_constant_pool);
  }
#endif

#if USE_BINARY_IMAGE_GENERATOR
  GUARANTEE(_romized_heap_marker != NULL, "Sanity");
  do_oop(&_romized_heap_marker);
#endif

#if ENABLE_PERFORMANCE_COUNTERS
  jlong elapsed = Os::elapsed_counter() - start_time;
  rom_perf_counts.oops_do_hrticks += elapsed;
#endif
}

/*
 * For now we have only one image per task, so this is not really a loop.
 */
bool ROM::in_any_loaded_bundle_of_current_task(const OopDesc* target) {
  if (system_contains(target)) {
    return true;
  }

#if USE_BINARY_IMAGE_LOADER
  FOREACH_BINARY_IMAGE_IN_CURRENT_TASK(bundle) {
    if (bundle->text_contains(target)) {
      return true;
    }
  }
  ENDEACH_BINARY_IMAGE_IN_CURRENT_TASK;
#endif

  return false;
}

#if ENABLE_ISOLATES
bool ROM::in_any_loaded_bundle(const OopDesc* target) {
  if (system_contains(target)) {
    return true;
  }

#if USE_BINARY_IMAGE_LOADER
  if (Universe::global_binary_images()->is_null()) {return false;}
  const ROMBundle **p = 
      (const ROMBundle**)Universe::global_binary_images()->base_address();  
  while (*p) {
    if ((*p)->text_contains(target)) {
      return true;
    }
    p++;
  }
#endif

  return false;
}
#endif

bool ROM::in_any_loaded_readonly_bundle(const OopDesc* target) {
#if USE_IMAGE_MAPPING
  // When we use mapped files, all bundles are (ideally) mapped read-only.
  // In the rare cases when the RO mapping fails, a RW mapping may be
  // used, but let's not worry about that now until this becomes a
  // performance issue.
  return in_any_loaded_bundle(target);
#else
  return system_contains(target);
#endif
}

#if ENABLE_COMPILER && ENABLE_APPENDED_CALLINFO
ReturnOop ROM::compiled_method_from_address(const address addr) {
  if (ROM::system_text_contains((const OopDesc*)addr)) {
    return 
      (OopDesc*)CompiledMethodDesc::find((const CompiledMethodDesc * const *)
                                         _rom_compiled_methods,
                                         _rom_compiled_methods_count,
                                         addr);
  } else {
#if USE_AOT_COMPILATION && USE_BINARY_IMAGE_LOADER
    return ROMBundle::compiled_method_from_address(addr);
#else
    return NULL;
#endif
  }
}
#endif // ENABLE_COMPILER && ENABLE_APPENDED_CALLINFO

#if !defined(PRODUCT) || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
ReturnOop ROM::get_original_class_name(ClassInfo *clsinfo) {
  if (!GenerateROMImage && _original_class_name_list == NULL) {
    return Symbols::unknown()->obj();
  }

  GUARANTEE(GenerateROMImage || UseROM, "sanity");
  //  GUARANTEE(clsinfo->access_flags().is_romized(), "must be romized!");
  int class_id = clsinfo->class_id();

#if ENABLE_ROM_GENERATOR
  if (ROMWriter::is_active()) {
    // try romizer first
    ReturnOop name = ROMOptimizer::original_name(class_id);
    if (name != NULL) {
      return name;
    }
  }
#endif
  if (_original_class_name_list == NULL) {
    return Symbols::unknown()->obj();
  }

  ObjArray::Raw name_list = _original_class_name_list;  
  return name_list().obj_at(class_id);
}

ReturnOop ROM::get_original_method_info(const Method *method) {
  int class_id = method->holder_id();
  ObjArray::Raw info;

#if ENABLE_ROM_GENERATOR
  if (ROMWriter::is_active()) {
    // try romizer first
    info = ROMOptimizer::original_method_info(class_id);
  }
#endif

  if (info.is_null()) {
    ObjArray::Raw info_list = _original_method_info_list;

    if (info_list.is_null()) {
     GUARANTEE(!GenerateROMImage, "ROMOptimizer didn't record original info?");
    }

#ifdef AZZERT
    InstanceClass::Raw holder = method->holder();
    //  GUARANTEE(holder().access_flags().is_romized(), "must be romized!");
#endif

    info = info_list().obj_at(class_id);
  }
  while (!info.is_null()) {
    if (method->equals(info().obj_at(INFO_OFFSET_METHOD))) {
      return info;
    } else {
      info = info().obj_at(INFO_OFFSET_NEXT);
    }
  }

  SHOULD_NOT_REACH_HERE();
  return NULL;
}

ReturnOop ROM::get_original_method_name(const Method *method) {
  if (!GenerateROMImage && _original_method_info_list == NULL) {
    return Symbols::unknown()->obj();
  }
  GUARANTEE(GenerateROMImage || UseROM, "sanity");
  ObjArray::Raw info = get_original_method_info(method);
  GUARANTEE(!info.is_null(), "sanity");
  return info().obj_at(INFO_OFFSET_NAME);
}


// The result is NULL if this class doesn't have renamed fields.
ReturnOop ROM::get_original_fields(InstanceClass *ic) {
  ClassInfo::Raw info = ic->class_info();
  int class_id = info().class_id();

#if ENABLE_ROM_GENERATOR
  if (ROMWriter::is_active()) {
    ReturnOop fields = ROMOptimizer::original_fields(class_id);
    if (fields != NULL) {
      return fields;
    }
  }
#endif

  if (_original_fields_list == NULL) {
    return NULL;
  }

  if (!ic->access_flags().is_romized()) {
    return NULL;
  }

  ObjArray::Raw orig_list = _original_fields_list;
  if (class_id >= orig_list().length()) {
    return NULL;
  }
  return orig_list().obj_at(class_id);
}

ReturnOop ROM::alternate_constant_pool(InstanceClass *klass) {
  Oop::Raw fields;

#if ENABLE_ROM_GENERATOR
  if (ROMWriter::is_active()) {
    ClassInfo::Raw info = klass->class_info();
    int class_id = info().class_id();
      fields = ROMOptimizer::original_fields(class_id);
      if (fields != NULL) {
        ReturnOop cp = ROMOptimizer::alternate_constant_pool();
        if (cp != NULL) {
          return cp;
        }
      }
  }
#endif

  fields = get_original_fields(klass);
  if (!fields.is_null()) {
    return _alternate_constant_pool;
  } else {
    return klass->constants();
  }
}
#endif

#if !defined(PRODUCT)
OopDesc* ROM::raw_text_klass_of(const OopDesc* obj) {
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  const int pass = text_segment_of(obj);
  juint byte_offset = (juint)obj - _rom_text_block_segments[pass];  
#else
  juint byte_offset = ((juint)obj) - ((juint)&_rom_text_block[0]);
  GUARANTEE((byte_offset < (juint)_rom_text_block_size), "must be in TEXT");
#endif
  juint code = byte_offset / 4;
  int index = ((int)code) % _rom_text_klass_table_size;

  OopDesc**p = (OopDesc**)(_rom_text_klass_table[index]);
  while (p[0] != NULL) {
    if (p[0] == obj) {
      return p[1];
    }
    p += 2;
  }
  return NULL;
}
#endif /* !PRODUCT */

void ROM::dispose() {
#if USE_BINARY_IMAGE_LOADER && !ENABLE_LIB_IMAGES
  ROMBundle::set_current( NULL );
#endif

#if !defined( PRODUCT) || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
  _original_class_name_list = NULL;
  _original_method_info_list = NULL;
  _original_fields_list = NULL;
  _alternate_constant_pool = NULL;
#endif
}

ReturnOop ROM::string_from_table(String *string, juint hash_value) {
  // The ROM string table is laid out in the same way as the ROM
  // symbol table. See SymbolTable.cpp for a description of
  // the ROM symbol table.
  ROM_DETAILED_PERFORMANCE_COUNTER_START();
  juint i = hash_value % _rom_string_table_num_buckets;
  OopDesc ***rom_table = (OopDesc ***)_rom_string_table;
  OopDesc **p   = rom_table[i];   // start of the bucket
  OopDesc **end = rom_table[i+1]; // end of the bucket (exclusive)
  String::Raw old_string;
  while (p != end) {
    old_string = (ReturnOop)(*p);
    if (old_string().matches(string)) {
      ROM_DETAILED_PERFORMANCE_COUNTER_END(string_from_table_hrticks);
      return old_string;
    }
    p ++;
  }
#if USE_BINARY_IMAGE_LOADER
#if ENABLE_LIB_IMAGES
  ObjArray::Raw images = Task::current()->binary_images();
  if (images.is_null()) { 
    ROM_DETAILED_PERFORMANCE_COUNTER_END(string_from_table_hrticks);
    return NULL;
  }

  for (int bun_i = 0; bun_i < images().length(); bun_i++) {
    ROMBundle* bun = (ROMBundle*)images().obj_at(bun_i);
    if (bun == NULL) continue;
#else //ENABLE_LIB_IMAGES
  if( ROMBundle::current() != NULL &&
      ROMBundle::current()->string_table_num_buckets() > 0) {
    ROMBundle* bun = ROMBundle::current();
#endif //ENABLE_LIB_IMAGES    
    juint i = hash_value % bun->string_table_num_buckets();
    rom_table = (OopDesc ***) bun->ptr_at(bun->STRING_TABLE);
    p   = rom_table[i];   // start of the bucket
    end = rom_table[i+1]; // end of the bucket (exclusive)
    while (p != end) {
      old_string = (ReturnOop)(*p);
      if (old_string().matches(string)) {
       ROM_DETAILED_PERFORMANCE_COUNTER_END(string_from_table_hrticks);
       return old_string;
      }
      p ++;
    }
  }
#endif //USE_BINARY_IMAGE_LOADER
  ROM_DETAILED_PERFORMANCE_COUNTER_END(string_from_table_hrticks);
  return NULL;
}

ReturnOop ROM::symbol_for(utf8 s, juint hash_value, int len) {
  ROM_DETAILED_PERFORMANCE_COUNTER_START();
  if (_rom_symbol_table_num_buckets > 0) {
    juint i = hash_value % _rom_symbol_table_num_buckets;
    SymbolDesc*** rom_table = (SymbolDesc ***)_rom_symbol_table;
    SymbolDesc** p = rom_table[i];     // start of the bucket
    SymbolDesc** end = rom_table[i+1]; // end of the bucket (exclusive)
    while (p != end) {
      if ((*p)->matches(s, len)) {
        ROM_DETAILED_PERFORMANCE_COUNTER_END(symbol_for_hrticks);
        return (ReturnOop) *p;
      }
      p++; // advance to next element in the bucket
    }
  }
#if USE_BINARY_IMAGE_LOADER
#if ENABLE_LIB_IMAGES
  //IMPL_NOTE: maybe all images?
  ObjArray::Raw bundles = Task::current()->binary_images();
  if (bundles.not_null()) {
    for (int i = 0; i < bundles().length(); i++) {
      ROMBundle* bundle = (ROMBundle*)bundles().obj_at(i);
      if (bundle->symbol_table_num_buckets() != 0) {
        juint i = hash_value % bundle->symbol_table_num_buckets();
        SymbolDesc*** rom_table = (SymbolDesc ***)
          bundle->ptr_at( ROMBundle::SYMBOL_TABLE );
        SymbolDesc** p   = rom_table[i];   // start of the bucket
        SymbolDesc** end = rom_table[i+1]; // end of the bucket (exclusive)
        while (p != end) {
          if ((*p)->matches(s, len)) {
            ROM_DETAILED_PERFORMANCE_COUNTER_END(symbol_for_hrticks);
            return (ReturnOop) *p;
          }
          p ++;
        }
      }
    }
  }
#else
  if( ROMBundle::current() != NULL &&
      ROMBundle::current()->symbol_table_num_buckets() != 0) {
    juint i = hash_value % ROMBundle::current()->symbol_table_num_buckets();
    SymbolDesc*** rom_table = (SymbolDesc ***)
      ROMBundle::current()->ptr_at( ROMBundle::current()->SYMBOL_TABLE );
    SymbolDesc** p   = rom_table[i];   // start of the bucket
    SymbolDesc** end = rom_table[i+1]; // end of the bucket (exclusive)
    while (p != end) {
      if ((*p)->matches(s, len)) {
        ROM_DETAILED_PERFORMANCE_COUNTER_END(symbol_for_hrticks);
        return (ReturnOop) *p;
      }
      p ++;
    }
  }
#endif
#endif
  ROM_DETAILED_PERFORMANCE_COUNTER_END(symbol_for_hrticks);
  return NULL;
}

#if USE_BINARY_IMAGE_LOADER || USE_BINARY_IMAGE_GENERATOR

#if ENABLE_DYNAMIC_NATIVE_METHODS

#define TEMPLATE(jtype, ttype, arg) arg(Java_ ## jtype ## _unimplemented)

#define UNIMPLEMENTED_METHOD_ENTRIES_DO(template) \
  FOR_ALL_TYPES_ARG(TEMPLATE, template)

#else

#define UNIMPLEMENTED_METHOD_ENTRIES_DO(template) \
  template(Java_void_unimplemented)

#endif

#define METHOD_ENTRIES_DO(template) \
  template(interpreter_method_entry) \
  template(interpreter_fast_method_entry_0) \
  template(interpreter_fast_method_entry_1) \
  template(interpreter_fast_method_entry_2) \
  template(interpreter_fast_method_entry_3) \
  template(interpreter_fast_method_entry_4) \
  \
  template(fixed_interpreter_fast_method_entry_0) \
  template(fixed_interpreter_fast_method_entry_1) \
  template(fixed_interpreter_fast_method_entry_2) \
  template(fixed_interpreter_fast_method_entry_3) \
  template(fixed_interpreter_fast_method_entry_4) \
  \
  template(shared_fast_getbyte_accessor)         \
  template(shared_fast_getshort_accessor)        \
  template(shared_fast_getchar_accessor)         \
  template(shared_fast_getint_accessor)          \
  template(shared_fast_getlong_accessor)         \
  template(shared_fast_getbyte_static_accessor)  \
  template(shared_fast_getshort_static_accessor) \
  template(shared_fast_getchar_static_accessor)  \
  template(shared_fast_getint_static_accessor)   \
  template(shared_fast_getlong_static_accessor)  \
  \
  UNIMPLEMENTED_METHOD_ENTRIES_DO(template)    \
  template(Java_abstract_method_execution)     \
  template(Java_incompatible_method_execution) \
  \
  template(shared_fast_getfloat_accessor)        \
  template(shared_fast_getdouble_accessor)       \
  template(shared_fast_getfloat_static_accessor) \
  template(shared_fast_getdouble_static_accessor)

#if ENABLE_COMPILER
#define COMPILER_METHOD_ENTRIES_DO(template) \
  template(fixed_interpreter_method_entry)
#else
#define COMPILER_METHOD_ENTRIES_DO(template)
#endif

#define BINARY_IMAGE_LINKABLE_SYMBOLS_DO(template) \
  METHOD_ENTRIES_DO(template)                      \
  COMPILER_METHOD_ENTRIES_DO(template)

#if ENABLE_MONET_DEBUG_DUMP
#define DEFINE_NAME_FOR_ADDRESS_ENTRY(x)   {(address)x, (char*)#x},

static struct {
  address addr;
  char *name;
} ROM_NameForAddress[] = {
  BINARY_IMAGE_LINKABLE_SYMBOLS_DO(DEFINE_NAME_FOR_ADDRESS_ENTRY)
  {NULL, NULL}
};

char * ROM::getNameForAddress(address addr) {
  for (int i=0; ROM_NameForAddress[i].addr != NULL; i++) {
    if (ROM_NameForAddress[i].addr == addr) {
      return ROM_NameForAddress[i].name;
    }
  }

  SHOULD_NOT_REACH_HERE();
  return NULL;
}
#endif

#if USE_UNRESOLVED_NAMES_IN_BINARY_IMAGE
#define DEFINE_LINKABLE_SYMBOLS(x)   (address)x,

address ROM_StaticallyLinkedSymbols[] = {
  BINARY_IMAGE_LINKABLE_SYMBOLS_DO(DEFINE_LINKABLE_SYMBOLS)
};

#define STATICALLY_LINKED_TABLE_SIZE (sizeof(ROM_StaticallyLinkedSymbols)/8)

address ROM::getROMSymbolTarget(int index) {
  GUARANTEE(index >= 0 && index < STATICALLY_LINKED_TABLE_SIZE, "sanity");
  return ROM_StaticallyLinkedSymbols[index];
}

/*
 * This function is used to relocate symbols such as 
 * interpreter_method_entry if these symbols can be relocated
 * between conversion- and execution-time. This happens if you do
 * the conversion off-line witn a win32 VM and try to run the image on a
 * target device. However, this function is not necessary for on-device-only
 * conversions.
 */
int ROM::getROMSymbolIndex(address addr) {
  int index;

  addr += CompiledMethod::base_offset();
  for (index = 0; index < STATICALLY_LINKED_TABLE_SIZE; index++) {
    if (ROM_StaticallyLinkedSymbols[index] == addr) {
      return index;
    }
  }
  
  SHOULD_NOT_REACH_HERE();
  return -1;
}
  
#endif // USE_UNRESOLVED_NAMES_IN_BINARY_IMAGE
#endif // USE_BINARY_IMAGE_LOADER || USE_BINARY_IMAGE_GENERATOR

/*====================================================================
 * The code below is related to the ROM image writer
 *===================================================================*/

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR

bool ROM::is_synchronized_method_allowed(Method *method) {
  GUARANTEE(method->access_flags().is_synchronized(), "sanity");

  if (method->access_flags().is_static()) {
    return true;
  }

  InstanceClass::Raw klass = method->holder();
  if (klass.equals(Universe::string_class()) ||
      klass.equals(Universe::object_class())) {
    // Currently the only romized Java objects are Strings. To support
    // efficient operation in CodeGenerator::unlock_activation(), we
    // require that romized objects do not have synchronized instance
    // methods.
    //
    // This check ensures that the method is not a synchronized
    // instance method belonging to a String object,
    return false;
  } else {
    return true;
  }
}

#endif
#ifndef PRODUCT

OopDesc* ROM::text_klass_of(const OopDesc* obj) {
  OopDesc* p = raw_text_klass_of(obj);
  GUARANTEE(p != NULL, "Must be valid object in TEXT");
  return p;
}

bool ROM::is_valid_text_object(const OopDesc* obj) {
  return (raw_text_klass_of(obj) != NULL);
}

#endif // !defined(PRODUCT)

#if ENABLE_PERFORMANCE_COUNTERS
#define ROM_PRINT_TICKS(x) \
    print_hrticks(STR(rom_ ## x), rom_perf_counts.x##_hrticks);
void ROM::ROM_print_hrticks(void print_hrticks(const char *name,
                                               julong hrticks))
{
  ROM_PRINT_TICKS(oops_do);
#if ENABLE_DETAILED_PERFORMANCE_COUNTERS
  ROM_PRINT_TICKS(valid_method);
  ROM_PRINT_TICKS(valid_field);
  ROM_PRINT_TICKS(is_rom_symbol);
  ROM_PRINT_TICKS(is_rom_method);
  ROM_PRINT_TICKS(has_compact_method);
  ROM_PRINT_TICKS(heap_contains);
  ROM_PRINT_TICKS(oop_from_offset);
  ROM_PRINT_TICKS(offset_of);
  ROM_PRINT_TICKS(get_max_offset);
  ROM_PRINT_TICKS(string_from_table);
  ROM_PRINT_TICKS(symbol_for);
#endif
}

#endif

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
bool ROM::is_restricted_package_in_profile(const char *name, int name_len) {
  const int current_profile_id = Universe::current_profile_id();  
  if (current_profile_id == Universe::DEFAULT_PROFILE_ID) {
    return false;
  }

  GUARANTEE(current_profile_id >= 0 && 
            current_profile_id < _rom_profiles_count, "Sanity");

  const char** profile_wildcards = 
    _rom_profiles_restricted_packages[current_profile_id];  
  GUARANTEE(profile_wildcards != NULL, "Sanity");

  int ind = 0;
  const char* wildcard = profile_wildcards[ind++];
  while (wildcard != NULL) {
    int wildcard_len = jvm_strlen(wildcard);
    const bool name_matches_pattern = 
      Universe::name_matches_pattern(name, name_len,
        wildcard, wildcard_len);
    if (name_matches_pattern) {
      return true;
    }
    wildcard = profile_wildcards[ind++];
  }
  return false;  
}

bool ROM::class_is_hidden_in_profile(const JavaClass* const jc) {  
  GUARANTEE((jc != NULL) && jc->not_null(), "Sanity");
  const int profile_id = Universe::current_profile_id();
  
  if (profile_id == Universe::DEFAULT_PROFILE_ID) {
    return false;
  }
  GUARANTEE(profile_id >= 0 && 
            profile_id < _rom_profiles_count, "Sanity");
  
  const jushort class_id = jc->class_id();
  if (class_id >= _rom_profile_bitmap_row_size * BitsPerByte) {
    return false;
  }
  
  const int ind = 
    profile_id * _rom_profile_bitmap_row_size + class_id / BitsPerByte;
  
  const int shift = (class_id % BitsPerByte);
  if (((_rom_hidden_classes_bitmaps[ind] >> shift) & 1) == 1) {    
    return true;        
  }
  return false;
}
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

#if ENABLE_LIB_IMAGES && ENABLE_MONET && ENABLE_ISOLATES
void ROM::accumulate_task_memory_usage() {
  for( int task = 0; task < MAX_TASKS; task++ ) {      
    Task::Raw t = Task::get_task(task);
    if (t.is_null()) continue;
    ObjArray::Raw binary_images = t().binary_images();
    if (binary_images.is_null()) continue;
    int i = 0; 
    for (; i < binary_images().length(); i++ ) {
      ROMBundle* bun = (ROMBundle*)binary_images().obj_at(i);
      if (bun == NULL) continue; //break?
      ObjectHeap::get_task_info( task ).estimate += bun->heap_block_size();
    }
  }
}
#endif

#if ENABLE_MONET || (ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES )
int ROM::encode_heap_reference(Oop *object) {
  FarClass blueprint = object->blueprint();
  InstanceSize instance_size = blueprint.instance_size();
  const juint is_heap = juint(ROM::HEAP_BLOCK) << ROM::block_type_start;
  int encoded_value = int(is_heap) |
        ((-instance_size.value()) << ROM::instance_size_start) |
        (0x1 << ROM::flag_start);

  switch (instance_size.value()) {
  default:
  case InstanceSize::size_compiled_method:
  case InstanceSize::size_type_array_1:
  case InstanceSize::size_type_array_2:
  case InstanceSize::size_type_array_4:
  case InstanceSize::size_type_array_8:
  case InstanceSize::size_method:
  case InstanceSize::size_far_class:
  case InstanceSize::size_constant_pool:
  case InstanceSize::size_stackmap_list:
  case InstanceSize::size_class_info:
  case InstanceSize::size_symbol:
  case InstanceSize::size_mixed_oop:
  case InstanceSize::size_obj_near:
    tty->print_cr("BinaryObjectWriter::encode_heap_ref: instance_size 0x%x", 
                  instance_size.value());
    SHOULD_NOT_REACH_HERE();
    return 0;
  case InstanceSize::size_generic_near:
#if ENABLE_PREINITED_TASK_MIRRORS && USE_SOURCE_IMAGE_GENERATOR && ENABLE_ISOLATES
    GUARANTEE(object->equals(Universe::task_mirror_class()->prototypical_near()), "only such nears should be present here!");
    encoded_value |= (1 << ROM::type_start); //this is a near for TaskMirror!
    break;
#endif
    tty->print_cr("BinaryObjectWriter::encode_heap_ref: instance_size 0x%x", 
                  instance_size.value());
    SHOULD_NOT_REACH_HERE();
    return 0;
#if ENABLE_ISOLATES
  case InstanceSize::size_task_mirror:
    {
      TaskMirror *tm = (TaskMirror *)object;
      GUARANTEE(tm->equals(Universe::task_class_init_marker()), "sanity");
      JavaClass::Raw jc = tm->containing_class();
      encoded_value |= (jc().class_id() << ROM::type_start);
    }
    break;
#endif
  case InstanceSize::size_obj_array_class:
  case InstanceSize::size_type_array_class:
  case InstanceSize::size_instance_class:
    {
      JavaClass *jc = (JavaClass *)object;
      encoded_value |= (jc->class_id() << ROM::type_start);
    }
    break;

  case InstanceSize::size_java_near:
    {
      JavaNear *jn = (JavaNear *)object;
      ClassInfo cl = jn->class_info();
      encoded_value |= (cl.class_id() << ROM::type_start);
    }
    break;
  }  

  return encoded_value;
}

ReturnOop ROM::decode_heap_reference(int value) {  
  // reference to romized system image
#ifdef AZZERT
  const unsigned int type = unsigned(value) >> ROM::offset_width;
#if ENABLE_LIB_IMAGES
  //IMPL_NOTE:check this
  GUARANTEE(type == ROM::HEAP_BLOCK || type == 0/*UNKNOWN_BLOCK*/, 
    "only system HEAP references needs encoding");
#else
  GUARANTEE(type == ROM::HEAP_BLOCK, 
    "only system HEAP references needs encoding");
#endif
#endif          
  const int class_id = (value >> ROM::type_start) & ROM::type_mask;
  const int instance_size = 
        (value >> ROM::instance_size_start) & ROM::instance_size_mask;
  switch( instance_size ) {
    default: {
#ifdef AZZERT
        SHOULD_NOT_REACH_HERE();
        return NULL;
      }
    case -InstanceSize::size_obj_array: {
#endif          
      return Universe::empty_obj_array()->obj();
    }
#if ENABLE_ISOLATES
    case -InstanceSize::size_task_mirror: {
        //Need revisit:
#if !ENABLE_LIB_IMAGES
      GUARANTEE(class_id < ROM::number_of_system_classes(), 
                        "Bad classid");
#endif
      return Universe::task_class_init_marker()->obj();
    }
#endif
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES
    case -InstanceSize::size_generic_near:
    GUARANTEE(class_id == 1, "check");
    return Universe::task_mirror_class()->prototypical_near();
#endif

    case -InstanceSize::size_obj_array_class:
    case -InstanceSize::size_type_array_class:
    case -InstanceSize::size_instance_class: {
      JavaClass::Raw jc = Universe::class_from_id(class_id);
      return jc.obj();
    }
    case -InstanceSize::size_java_near: {
      JavaClass::Raw jc = Universe::class_from_id(class_id);
      JavaNear::Raw jn = jc().prototypical_near();
      return jn.obj() ;
    }
  }
}
#endif // ENABLE_MONET || (ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES )

#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES
ReturnOop ROM::link_static_mirror_list(JVM_SINGLE_ARG_TRAPS) {
  if (_rom_task_mirrors_size == 0) return NULL;
  int size = _rom_task_mirrors_size;
  OopDesc* result = ObjectHeap::allocate(_rom_task_mirrors_size JVM_CHECK_0);
  jvm_memcpy( result, _rom_task_mirrors, size );
  int offset = 0;
  juint* p_obj_start = (juint*)result;
  const juint* bitmap = (const juint*)_rom_task_mirrors_bitmap;
  const int bitmap_size =  size / BitsPerWord + 1; 
  const juint* bitmap_end = DERIVED( const juint*, bitmap, bitmap_size);
  const juint is_heap = juint(ROM::HEAP_BLOCK) << ROM::block_type_start;

  for( ; bitmap < bitmap_end; bitmap++, p_obj_start += BitsPerWord )
  {
    juint bitword = *bitmap;
    if( !bitword ) {
      continue;
    }
    juint* p = p_obj_start;
    do {
#define SHIFT_ZEROS(n) if((bitword & ((1 << n)-1)) == 0) { bitword >>= n; p += n; }
      SHIFT_ZEROS(16)
      SHIFT_ZEROS( 8)
      SHIFT_ZEROS( 4)
      SHIFT_ZEROS( 2)
      SHIFT_ZEROS( 1)
#undef SHIFT_ZEROS                
      int value = *p;
      if( (value & is_heap) == 0 ) {
        value = ((int)result)+value;
      } else {
        value = (int)ROM::decode_heap_reference(value);
      }
      *p = value;
      p++;
    } while( (bitword >>= 1) != 0);
  }
  return result;
}
#endif

