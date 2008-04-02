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

#include "incls/_precompiled.incl"
#include "incls/_BinaryROM.cpp.incl"

#if USE_BINARY_IMAGE_LOADER
ROMBundle* ROMBundle::_current;

inline bool ROMBundle::heap_src_block_contains( const address target ) const {
  const juint p = DISTANCE( heap_block(), target );
  return p < heap_block_size();
}

// If a binary image contains an array class AC of a system class SC,
// make sure that SC->array_class() points to AC.
inline void ROMBundle::update_system_array_class(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast class_list = Universe::class_list()->obj();
  ObjArrayClass::Fast oac;
  JavaClass::Fast element;

  const int limit = _rom_number_of_java_classes;
  for( int i = class_list().length(); --i >= limit; ) {
    JavaClass::Raw jc = class_list().obj_at(i);
    if (jc.is_null()) {
      // The tail end of the class_list may have empty slots.
      // IMPL_NOTE: Having empty slots is not necessary for Monet.
      continue;
    }
    if (jc.is_obj_array_class()) {
      oac = jc.obj();
      element = oac().element_class();

      // it's possible that TaskMirror for element already contains
      // correct pointer to array class, if it was set during 
      // convertion of previous bundles
      if (element().class_id() < _rom_number_of_java_classes &&
          element().array_class() == NULL) {
        element().get_or_allocate_java_mirror(JVM_SINGLE_ARG_CHECK); 
        element().set_array_class(&oac JVM_CHECK);
      }
    }
  }
}

#if ENABLE_COMPILER && ENABLE_INLINE
// Vtable bitmaps can be updated when the application classes are initialized
// during Monet conversion. For the classes being converted the updated
// bitmaps will be stored in the Monet image. For all other classes (system or
// previously loaded) bitmap changes aren't saved. So we restore them here
inline void ROMBundle::update_vtable_bitmaps(const int sys_class_count 
                                             JVM_TRAPS) const {
  UsingFastOops fast_oops;
  ObjArray::Fast class_list = Universe::class_list()->obj();

  const int total_class_count = class_list().length();
  for( int i = total_class_count; --i >= sys_class_count; ) {
    JavaClass::Raw jc = class_list().obj_at(i);
    if (jc.is_null() || jc().is_fake_class()) {
      // The tail end of the class_list may have empty slots.
      // IMPL_NOTE: Having empty slots is not necessary for Monet.
      // If some class was not found during Monet conversion it is fake.
      continue;
    }
    if (jc.is_instance_class()) {
      InstanceClass::Raw ic = jc.obj();
      if (ic().is_initialized()) {
        ic().update_vtable_bitmaps(JVM_SINGLE_ARG_CHECK);
      }
    }
  }
}
#endif

inline void ROMBundle::restore_vm_structures(  
#if ENABLE_LIB_IMAGES && ENABLE_ISOLATES
  int global_bundle_id, bool already_loaded, bool sharable JVM_TRAPS) {
#else //ENABLE_LIB_IMAGES
  JVM_SINGLE_ARG_TRAPS) {
#endif //ENABLE_LIB_IMAGES  
  UsingFastOops fast_oops;
  ObjArray::Fast bun_handles;
  ObjArray::Fast bun_dictionary; 
  ObjArray::Fast bun_class_list;
  ObjArray::Fast bun_mirror_list;
  ObjArray::Fast bun_bad_classes_list;
#if ENABLE_LIB_IMAGES && ENABLE_ISOLATES
  if (already_loaded) { 
    GUARANTEE(sharable, "coudn't reuse non-sharable bundles!");
    bun_handles = Universe::global_binary_persistante_handles()->obj_at(global_bundle_id);
    bun_dictionary = bun_handles().obj_at(0); 
    bun_class_list = bun_handles().obj_at(1); 
    bun_mirror_list = bun_handles().obj_at(3); 
    bun_bad_classes_list = bun_handles().obj_at(2); 
  } else 
#endif //ENABLE_LIB_IMAGES
  {
    OopDesc** handles = persistent_handles();            

    // Dictionary
    OopDesc *object = handles[0];  
    relocate_pointer_to_heap(&object);    
    bun_dictionary = object;

    // class_list
    object = handles[1];
    relocate_pointer_to_heap(&object);    
    bun_class_list  = object;

    // names_of_bad_classes
    object = handles[2];
    relocate_pointer_to_heap(&object);
    bun_bad_classes_list = object;

#if ENABLE_ISOLATES
    // mirror_list
    object = handles[3];  
    relocate_pointer_to_heap(&object);    
    bun_mirror_list = object;
#endif 
#if ENABLE_LIB_IMAGES && ENABLE_ISOLATES 
    if (sharable) {
      const int task_id = ObjectHeap::start_system_allocation();
      bun_handles = Universe::new_obj_array(4 JVM_CHECK); 
      ObjectHeap::finish_system_allocation(task_id);
      bun_handles().obj_at_put(0, &bun_dictionary);  
      bun_handles().obj_at_put(1, &bun_class_list);  
      bun_handles().obj_at_put(2, &bun_bad_classes_list);
      bun_handles().obj_at_put(3, bun_mirror_list);
    Universe::global_binary_persistante_handles()->obj_at_put(global_bundle_id, bun_handles.obj());
    }
#endif //ENABLE_LIB_IMAGES    
  }
#if ENABLE_LIB_IMAGES
  ObjArray::Fast bad_classes = Task::current()->names_of_bad_classes();
  ObjArray::Fast new_bad_classes = bun_bad_classes_list.obj();
  ObjArray::Fast restored_classes_list;
  if (bad_classes.is_null()) {
    //we nedd clone it here to reuse later
    restored_classes_list = Universe::new_obj_array(new_bad_classes().length() JVM_CHECK);
    ObjArray::array_copy(&new_bad_classes, 0,
                       &restored_classes_list, 0, 
                       new_bad_classes().length() JVM_MUST_SUCCEED);    
  } else {    
     restored_classes_list = Universe::new_obj_array(bad_classes().length() + 
                       new_bad_classes().length() JVM_CHECK);
     ObjArray::array_copy(&bad_classes, 0,
                       &restored_classes_list, 0, 
                       bad_classes().length() JVM_MUST_SUCCEED);
    for (int i = 0; i < new_bad_classes().length(); i++) {
      int idx = bad_classes().length() + i;
      restored_classes_list().obj_at_put(idx, new_bad_classes().obj_at(i));
    }    
  }
  Task::current()->set_names_of_bad_classes(restored_classes_list.obj());
#else //ENABLE_LIB_IMAGES
  Task::current()->set_names_of_bad_classes(bun_bad_classes_list.obj());
#endif //ENABLE_LIB_IMAGES
  {
    // restore gc stackmaps. During GC, we need to have a gc_block_stackmap()
    // that's big enough for all the methods in this bundle (as calculated
    // during binary romization).
    int bi_stackmap_size = stackmap_size();
#if ENABLE_ISOLATES
    TypeArray::Raw block_stackmap( Universe::gc_block_stackmap() );
    if( block_stackmap.not_null() ) {
      const int current_gc_stackmap_size = block_stackmap().length();
      if( bi_stackmap_size < current_gc_stackmap_size ) {
        bi_stackmap_size = current_gc_stackmap_size;
      }
    }
#endif
    StackmapGenerator::initialize(bi_stackmap_size JVM_CHECK);
  }

  // restore class list
#if ENABLE_LIB_IMAGES
  const int sys_class_count = Task::current()->classes_in_images();
#else
  const int sys_class_count = ROM::number_of_system_classes();
#endif
  const int bun_class_count = bun_class_list().length();
  const int restored_class_list_len = sys_class_count + bun_class_count;

  ObjArray::Fast restored_class_list = 
    Universe::new_obj_array(restored_class_list_len JVM_CHECK);
  ObjArray::array_copy(Universe::class_list(), 0,
                       &restored_class_list, 0, 
                       sys_class_count JVM_MUST_SUCCEED);
  ObjArray::array_copy(&bun_class_list, 0,
                       &restored_class_list, sys_class_count,
                       bun_class_count JVM_MUST_SUCCEED);
#if ENABLE_ISOLATES
  ObjArray::Fast restored_mirror_list = 
    Universe::new_obj_array(restored_class_list_len JVM_CHECK);
  ObjArray::array_copy(Universe::mirror_list(), 0,
                       &restored_mirror_list, 0, 
                       sys_class_count JVM_MUST_SUCCEED);
  //here we must clone all task mirrors in case on sharing bundles between Isolates
#if ENABLE_LIB_IMAGES
  if (sharable) {
    for (int j = 0; j < bun_class_count; j++) {
      Oop mir = bun_mirror_list().obj_at(j);
      OopDesc* cloned_mir = mir;
      if (!mir.equals(Universe::task_class_init_marker())) {
        cloned_mir = ObjectHeap::clone(cloned_mir JVM_MUST_SUCCEED);
      }    
      restored_mirror_list().obj_at_put(sys_class_count + j, cloned_mir);
    }
  } else 
#endif //ENABLE_LIB_IMAGES
  {
    ObjArray::array_copy(&bun_mirror_list, 0,
                     &restored_mirror_list, sys_class_count,
                     bun_class_count JVM_MUST_SUCCEED);
  }
#endif //ENABLE_ISOLATES
  {
    // Update All of these things at the same time to avoid partial failures
    *Universe::class_list() = restored_class_list.obj();
    *Universe::current_dictionary() = bun_dictionary.obj();
#if ENABLE_ISOLATES
    *Universe::mirror_list() = restored_mirror_list.obj();

    // update proper fields of the current task
    Task::Raw task = Task::current();
    task().set_class_list(&restored_class_list());
    task().set_class_count(restored_class_list_len);
    task().set_mirror_list(&restored_mirror_list());    
    task().set_dictionary(&bun_dictionary);
#endif
#if ENABLE_LIB_IMAGES    
    Task::current()->set_classes_in_images(restored_class_list_len);
#endif
    Universe::update_relative_pointers();
  }
  // update array classes
  update_system_array_class(JVM_SINGLE_ARG_CHECK);
#if ENABLE_COMPILER && ENABLE_INLINE
  // Update vtable bitmaps of super classes.
  update_vtable_bitmaps(sys_class_count JVM_NO_CHECK_AT_BOTTOM);
#endif
}

#if USE_AOT_COMPILATION
ReturnOop 
ROMBundle::compiled_method_from_address(const address addr) {
  FOREACH_BINARY_IMAGE_IN_CURRENT_TASK(bundle)
  {
    if (bundle->text_contains((const OopDesc*)addr)) {
      return (OopDesc*)CompiledMethodDesc::find(
        (const CompiledMethodDesc * const *)
        bundle->compiled_method_table_addr(),
        bundle->compiled_method_table_size() / sizeof (OopDesc*),
        addr);
    }
  }
  ENDEACH_BINARY_IMAGE_IN_CURRENT_TASK;

  return NULL;
}
#endif

// This function is an updates each pointer P in the bundle to point to:
// [1] If P points to an object to the system TEXT/DATA/HEAP, it's
//     updated to point to the final location
// [2] If P pointers to an object in the app's TEXT, it's
//     updated to point to the final location
// [3] If P pointers to an object in the app's HEAP, it's
//     updated to point to the location of this object within the ROMBundle.
// Not that in case [3], the app HEAP is later copied into the ObjectHeap, 
// and all pointers in case [3] will be updated again to point to the
// final location.
inline void ROMBundle::fixup( void ) {
#define CLEANUP_EXTERNAL_BITS USE_LARGE_OBJECT_AREA 
  const int relocation_diff = DISTANCE( base(), this );
  juint* heap_ptr = (juint*)heap_block();
  juint* heap_border = DERIVED(juint*, heap_ptr, relocation_diff);
  juint* pp = array;

#if USE_IMAGE_PRELOADING || USE_IMAGE_MAPPING
  int skip_bytes = 0;
  int bit_skip_bytes = 0;

  if (relocation_diff == 0 && USE_IMAGE_MAPPING ) {
    // The TEXT part needs no relocation (and wants none, because it's mapped
    // read-only.
#if ENABLE_LIB_IMAGES
    //but it needs in case of multiple images!!!
    skip_bytes = DISTANCE(this, text_block());
#else
    skip_bytes = DISTANCE(this, method_variable_parts());
#endif
    skip_bytes = (int)align_size_down(skip_bytes, BitsPerWord*BytesPerWord);
    bit_skip_bytes = skip_bytes / BitsPerWord;
  }

  const int* bitp = (const int*)relocation_bit_map();
  const int* const bit_end = DERIVED( const int*, bitp, *bitp ); // inclusive
  bitp = DERIVED( const int*, bitp, bit_skip_bytes );
  pp   = DERIVED( juint*, pp, skip_bytes );
  
  for( ; ++bitp <= bit_end && pp < heap_border; pp += BitsPerWord )
#else
  const juint* bitp = LargeObject::bitvector( this );
  const juint* const bit_end = LargeObject::head( this )->next_bitvector();

  // Never make this one a pointer. See CR 6362860
  GUARANTEE(((juint)(*bitp) & (1 << RELOCATION_BITMAP_OFFSET)) == 0,
            "Relocation bitmap bit must not be set");

  for( ; bitp < bit_end && pp < heap_border; bitp++, pp += BitsPerWord )
#endif
  {
    juint bitword = *bitp;
    if( bitword ) {
#if CLEANUP_EXTERNAL_BITS
      juint new_bitword = 0;
      juint mask = 1;
#endif
      juint* p = pp;
      do {
#if CLEANUP_EXTERNAL_BITS
  #define SHIFT_ZEROS(n)\
    if((bitword & ((1 << n)-1)) == 0) { bitword >>= n; mask <<= n; p += n; }
#else
  #define SHIFT_ZEROS(n)\
    if((bitword & ((1 << n)-1)) == 0) { bitword >>= n; p += n; }
#endif
        SHIFT_ZEROS(16)
        SHIFT_ZEROS( 8)
        SHIFT_ZEROS( 4)
        SHIFT_ZEROS( 2)
        SHIFT_ZEROS( 1)
#undef SHIFT_ZEROS        
        if (p < (juint*)heap_border) {          
          int value = *p;        
#if ENABLE_LIB_IMAGES
          if( (value & 0x3) == 3 ) {
            value = Task::current()->decode_reference(value);
#if CLEANUP_EXTERNAL_BITS
            new_bitword |= mask; 
#endif          
          } else 
#endif
          if( (value & 0x1) == 0 ) {
            value += relocation_diff;
#if CLEANUP_EXTERNAL_BITS            
            new_bitword |= mask;
#endif
          } else {                   
            value = (int)ROM::decode_heap_reference(value);                    
          }        
          *p = value;
#if CLEANUP_EXTERNAL_BITS
        } else { 
            //this link is inside heap block, so it will be resolved in copy_heap_block
            new_bitword |= mask; 
#endif          
        }
        p++;
#if CLEANUP_EXTERNAL_BITS
        mask <<= 1;
#endif
      } while( (bitword >>= 1) != 0 && pp < heap_border);
#if CLEANUP_EXTERNAL_BITS 
      *(juint*)bitp = new_bitword;   
#endif
    }
  }
#undef CLEANUP_EXTERNAL_BITS
}

int ROMBundle::heap_relocation_offset;
void ROMBundle::relocate_pointer_to_heap(OopDesc** p) {
  GUARANTEE( ROMBundle::current() != NULL, "sanity" );
  const address obj = *(address*)p;    
  if( ROMBundle::current()->heap_src_block_contains( obj ) ) {
    *p = DERIVED( OopDesc*, obj, heap_relocation_offset );
  }
}

inline ReturnOop  ROMBundle::copy_heap_block(JVM_SINGLE_ARG_TRAPS) {    
#define CLEANUP_EXTERNAL_BITS USE_LARGE_OBJECT_AREA
  // (1) Allocate space in the heap for the heap_block
  const unsigned size = heap_block_size();
#if ENABLE_LIB_IMAGES 
  const bool sharable = is_sharable();
  int task_id = 0;
  if( sharable ) {
    task_id = ObjectHeap::start_system_allocation();   
  }
#endif 
  OopDesc* dst = ObjectHeap::allocate(size JVM_NO_CHECK); 
#if ENABLE_LIB_IMAGES 
  if( sharable ) {
    ObjectHeap::finish_system_allocation(task_id); 
  }
#endif 
  if( dst == NULL ) { 
    return NULL; 
  } 
  // (2) Copy heap_block to its final destination
  const void* src = heap_block();
  jvm_memcpy( dst, src, size );
  const int relocation_diff = DISTANCE( base(), this ) ;
  const int diff_heap = DISTANCE( src, dst );
  heap_relocation_offset = diff_heap;
  juint* pp = array;
  pp = DERIVED( juint*, pp, diff_heap); //we are modifing copied block
  int skip_bytes = DISTANCE(this, heap_block());
  skip_bytes = (int)align_size_down(skip_bytes, BitsPerWord*BytesPerWord);
  const int bit_skip_bytes = skip_bytes / BitsPerWord;;

#if USE_IMAGE_PRELOADING || USE_IMAGE_MAPPING
  const int* bitp = (const int*)relocation_bit_map();
  const int* const bit_end = DERIVED( const int*, bitp, *bitp ); // inclusive
  bitp = DERIVED( const int*, bitp, bit_skip_bytes );
  pp   = DERIVED( juint*, pp, skip_bytes );
  for( ; ++bitp <= bit_end; pp += BitsPerWord )
#else
  const juint* bitp = LargeObject::bitvector( this );
  const juint* const bit_end = LargeObject::head( this )->next_bitvector();  
  pp   = DERIVED( juint*, pp, skip_bytes );

  // Never make this one a pointer. See CR 6362860
  GUARANTEE(((juint)(*bitp) & (1 << RELOCATION_BITMAP_OFFSET)) == 0,
            "Relocation bitmap bit must not be set");
  bitp = DERIVED( const juint*, bitp, bit_skip_bytes );

  for( ; bitp < bit_end; bitp++, pp += BitsPerWord )
#endif
  {
    juint bitword = *bitp;
    if( bitword ) {
      juint* p = pp;
#if CLEANUP_EXTERNAL_BITS
      juint new_bitword = 0;
      juint mask = 1;
#endif
      do {
#if CLEANUP_EXTERNAL_BITS
  #define SHIFT_ZEROS(n)\
    if((bitword & ((1 << n)-1)) == 0) { bitword >>= n; mask <<= n; p += n; }
#else
  #define SHIFT_ZEROS(n)\
    if((bitword & ((1 << n)-1)) == 0) { bitword >>= n; p += n; }
#endif
        SHIFT_ZEROS(16)
        SHIFT_ZEROS( 8)
        SHIFT_ZEROS( 4)
        SHIFT_ZEROS( 2)
        SHIFT_ZEROS( 1)
#undef SHIFT_ZEROS                
        if (p >= (juint*)dst) { //IMPROVE ME:: align heap block in image!          
          int value = *p;
#if ENABLE_LIB_IMAGES
          if( (value & 0x3) == 3 ) {          
            value = Task::current()->decode_reference(value);
          } else 
#endif
          if( (value & 0x1) == 0 ) {
            value += relocation_diff;
            if (value >= (int)heap_block()) {//this is a heap ref
              value += diff_heap;
            }
          } else {          
            value = (int)ROM::decode_heap_reference(value);                    
          }
          *p = value;
        }
#if CLEANUP_EXTERNAL_BITS
        else {
            new_bitword |= mask; 
        }
#endif          
#if CLEANUP_EXTERNAL_BITS
        mask <<= 1;
#endif
        p++;
      } while( (bitword >>= 1) != 0);
#if CLEANUP_EXTERNAL_BITS 
      *(juint*)bitp = new_bitword;   
#endif
    }
  }
#undef CLEANUP_EXTERNAL_BITS
  return dst;
}

#if USE_IMAGE_PRELOADING
#if ENABLE_LIB_IMAGES
OsFile_Handle ROMBundle::_preloaded_handles[MAX_LIB_IMAGES];
int           ROMBundle::_preloaded_lengths[MAX_LIB_IMAGES];
int           ROMBundle::_loaded_bundle_count = 0;
int           ROMBundle::_loaded_bundle_size = 0;
#else
OsFile_Handle ROMBundle::_preloaded_handle;
int           ROMBundle::_preloaded_length;
#endif

void ROMBundle::preload( const JvmPathChar class_path[] ) {
  int final_length = 0;
  int length = 0;
  OsFile_Handle handle = NULL;
  if( class_path ) {
    int start = 0;
    int bundle_num = 0;
    DECLARE_STATIC_BUFFER(JvmPathChar, new_class_path, NAME_BUFFER_SIZE);
    while (class_path[start]) {    
      int count = 0;
      for(; class_path[start+count] != 0 && 
          class_path[start+count] != OsFile_path_separator_char; count++) {
      }
      if (count >= NAME_BUFFER_SIZE) {
#if ENABLE_TTY_TRACE
        jvm_memcpy(new_class_path, class_path + start, 
            (NAME_BUFFER_SIZE-1) * sizeof(JvmPathChar));
        new_class_path[NAME_BUFFER_SIZE-1] = 0;
        tty->print_cr("bundle name:%s is too long to be loaded. Skipped.", new_class_path);
#endif
#if ENABLE_LIB_IMAGES
        _preloaded_handles[bundle_num] = NULL;
        _preloaded_lengths[bundle_num] = 0;                  
        start += count;      
        if (class_path[start] == OsFile_path_separator_char) start++;
        bundle_num++;
        continue;
#else
        _preloaded_handle = NULL;
        _preloaded_length = 0;
        final_length = 0;
        break;
#endif      
      }
      jvm_memcpy(new_class_path, class_path + start, 
                count * sizeof(JvmPathChar));
      new_class_path[count] = 0;
      
      int dummy;
      handle = open(new_class_path, length, &dummy);
#if ENABLE_LIB_IMAGES
      _preloaded_handles[bundle_num] = handle;
      _preloaded_lengths[bundle_num] = length;      
      if (!handle) break;
      final_length += align_size(length);
      start += count;      
      if (class_path[start] == OsFile_path_separator_char) start++;
      bundle_num++;
#else
      _preloaded_handle = handle;
      _preloaded_length = length;
      final_length = length;
      break;
#endif      
    } 
  } else {
#if ENABLE_LIB_IMAGES
    _preloaded_handles[0] = handle;
    _preloaded_lengths[0] = length;
#else
    _preloaded_handle = handle;
    _preloaded_length = length;    
#endif
    final_length = length;
  }  
  
  ObjectHeap::set_preallocated_space_size( final_length );
}
#endif
#if ENABLE_LIB_IMAGES && ENABLE_ISOLATES
ROMBundle* ROM::already_loaded(const int bundle_id, void** map_handle) {
  ObjArray::Raw list = Universe::global_binary_images();
  for( int i = list().length(); --i >= 0; ) {
    ROMBundle* bun = (ROMBundle*)list().obj_at(i);
    if( bun && bun->bundle_id() == bundle_id ) {
      return bun;
    }
  }
  return NULL;
}
#endif
#if USE_IMAGE_MAPPING
inline ROMBundle* ROMBundle::load( const int /*task_id*/,
                      const JvmPathChar path_name[], void** map_handle, bool* already_loaded)
{
  OsFile_Handle file_handle = OsFile_open(path_name, "rb");
  if (file_handle == NULL) {
    return NULL;
  }

  int header[HEADER_SIZE];
  header[0] = 0; // If header is not completely read, header_tag
  header[1] = 0; // header_word_count
  header[2] = 0; // and ROM_BINARY_MAGIC will not match

  OsFile_read(file_handle, &header, 1, sizeof(header));
  int file_length = OsFile_length(file_handle);
  OsFile_close(file_handle);

  OopDesc * header_tag = Universe::int_array_class()->prototypical_near();
  int header_word_count = HEADER_SIZE - 2;

  if (header[0] != (int)header_tag ||
      header[1] != header_word_count ||
      header[2] != ROM_BINARY_MAGIC) {
    // Not an image file
    return NULL;
  }

#if ENABLE_LIB_IMAGES && ENABLE_ISOLATES
  int bundle_id = header[ROM_BUNDLE_ID];
  ROMBundle* tbun = ROM::already_loaded(bundle_id, map_handle);  
  *already_loaded = (tbun != NULL && tbun->is_sharable());
  if (*already_loaded) {        
    return tbun;
  }
#endif  
  address preferred_addr = (address)header[BASE];
#if ENABLE_LIB_IMAGES
  int ro_length = 0; // text block may contains references to other bundles
#else
  int ro_length = header[METHOD_VARIABLE_PARTS] - header[BASE];
#endif
  int rw_offset = ro_length;
  int rw_length = file_length - ro_length;

  OsFile_MappedImageHandle map_image =
      OsFile_MapImage(path_name, preferred_addr, file_length, rw_offset,
                      rw_length);
  if (!map_image) {
    return NULL;
  }

  int* image = (int*)map_image->mapped_address;
  GUARANTEE(image != NULL, "sanity");
  GUARANTEE(image[0] == (int)header_tag &&
            image[1] == header_word_count &&
            image[2] == ROM_BINARY_MAGIC, "sanity");

  *map_handle = map_image;
  return (ROMBundle*)image;
}

#if !ENABLE_ISOLATES

#define FOREACH_TASK(t) \
  { Task *t = Task::current(); if (t->not_null()) {
#define ENDEACH_TASK }}

#else // ENABLE_ISOLATES

#define FOREACH_TASK(t) \
  if (Universe::task_list()->not_null()) { \
    UsingFastOops fast_oops; Task::Fast _task; Task *t = &_task; \
    for (int _i=0; _i < MAX_TASKS; _i++) { \
      _task = Universe::task_from_id(_i); \
      if (_task.not_null()) {

#define ENDEACH_TASK }}}

#endif // ENABLE_ISOLATES
        
void ROM::dispose_binary_images() {
  // This function is called when VM is shutting down. There's needed only
  // if USE_IMAGE_MAPPING, because in USE_IMAGE_PRELOADING or 
  // USE_LARGE_OBJECT_AREA cases there's no opened files left after the image
  // is loaded.
  FOREACH_TASK(t)
  {
    t->free_binary_images();
  }
  ENDEACH_TASK;
}

#else // !USE_IMAGE_MAPPING

inline OsFile_Handle ROMBundle::open(const JvmPathChar* file, int& length, int* bundle_id) {
  OsFile_Handle file_handle = OsFile_open(file, "rb");
  if (file_handle == NULL) {
    return NULL;
  }

  // Universe::int_array_class() may not be initialized yet if we're 
  // preloading, so let's read Universe::int_array_class()->prototypical_near()
  // directly from the ROM
  const int iac = Universe::int_array_class_index;
  FarClassDesc* int_array_class = (FarClassDesc*)_rom_persistent_handles[iac];
  OopDesc *header_tag = ((FarClass*)&int_array_class)->prototypical_near();
  const juint header_word_count = HEADER_SIZE - 2;

#if USE_IMAGE_PRELOADING
  {
    juint magics[6];
    OsFile_read(file_handle, &magics, 1, sizeof magics);
    *bundle_id = magics[5];
    if( magics[0] != juint( header_tag        ) ||
        magics[1] != juint( header_word_count ) ||
        magics[2] != juint( ROM_BINARY_MAGIC  ) ) {
      goto error;
    }
  }
  length = OsFile_length(file_handle);
#else
  {
    ROMBundle header;
    if( OsFile_read(file_handle, &header, 1, sizeof header ) != sizeof header) {
      goto error;
    }
    *bundle_id = header.array[ ROM_BUNDLE_ID];
    if ( header.array[ HEADER_TAG        ] != juint( header_tag         ) ||
         header.array[ HEADER_WORD_COUNT ] != juint( header_word_count  ) ||
         header.array[ MAGIC             ] != juint( ROM_BINARY_MAGIC   ) ) {
      goto error;
    }
    // We just need to copy everything up to the end of the heap block.
    // IMPL_NOTE: no need to copy heap block into the LargeObject. It can
    // be copied directly into _inline_allocation_top.
    length = DISTANCE(header.base(), header.heap_block()) + 
             header.heap_block_size();
  }
#endif

  // Must do this: we called OsFile_length() previously, which
  // on some platfroms may reset the file position.
  OsFile_seek(file_handle, 0, SEEK_SET);
  if (length == 0) {
    goto error;
  }
  return file_handle;

error:
  if (file_handle) {
    OsFile_close(file_handle);
  }
  return NULL;
}

#if USE_IMAGE_PRELOADING
inline ROMBundle* ROMBundle::load(const int task_id,
                          const JvmPathChar path_name[], void** map_handle, bool* already_loaded)
{
  ROMBundle* bundle = NULL;
  (void)map_handle;
#if ENABLE_LIB_IMAGES
  const int length = _preloaded_lengths[_loaded_bundle_count];
  OsFile_Handle const handle = _preloaded_handles[_loaded_bundle_count];
#else
  const int length = _preloaded_length;
  OsFile_Handle const handle = _preloaded_handle;
#endif
  (void)path_name;

  if( handle ) {
#if ENABLE_LIB_IMAGES
    address bun_address = (address) ObjectHeap::get_preallocated_space();
    bun_address = DERIVED(address, bun_address, _loaded_bundle_size);
    _loaded_bundle_size += align_size(length);
    _loaded_bundle_count++;
    ROMBundle* const p = (ROMBundle*) bun_address;
    bun_address = DERIVED(address, bun_address, length);
    Universe::fill_heap_gap(bun_address, align_size(length) - length);
#else
    ROMBundle* const p = (ROMBundle*) ObjectHeap::get_preallocated_space();
#endif

    if( p ) {
      const int n = OsFile_read( handle, p, 1, size_t(length) );
      if( n == length ) {
        bundle = p;
      } else {
        tty->print_cr("Error: Unable to read entire binary ROM image: "
                      "need %d, got %d bytes", length, n);
      }
    } else {
      tty->print_cr("Error: Unable to allocate memory "
                    "for the binary ROM image");
    }
    OsFile_close( handle );
  }
  return bundle;
}
#else //USE_LARGE_OBJECT_AREA
inline ROMBundle* ROMBundle::load(const int task_id,
                          const JvmPathChar path_name[], void** map_handle, bool* already_loaded)
{
  ROMBundle* bundle = NULL;
  (void)map_handle;
  int length;
  int bundle_id;
  OsFile_Handle const handle = open( path_name, length, &bundle_id );
#if ENABLE_LIB_IMAGES   
  ROMBundle* tbun = ROM::already_loaded(bundle_id, NULL);  
  *already_loaded = (tbun != NULL && tbun->is_sharable());
  if( *already_loaded ) {
    return tbun;
  }
#endif //ENABLE_LIB_IMAGES 

  if( handle ) {
    LargeObject* object =
      LargeObject::allocate( LargeObject::allocation_size( length ) );
    ROMBundle* p = (ROMBundle*) (object ? object->body() : object);
    if( p ) {
      const int n = OsFile_read( handle, p, 1, size_t(length) );
      if( n == length ) {
        juint bit_length;
        if( OsFile_read( handle, &bit_length, 1, sizeof bit_length )
              == sizeof bit_length &&
            OsFile_read( handle, LargeObject::bitvector( p ), 1, bit_length )
              == bit_length ) {
          bundle = p;
        }
      } else {
        tty->print_cr("Error: Unable to read entire binary ROM image: "
                      "need %d, got %d bytes", length, n);
        p->free();
      }
    } else {
      tty->print_cr("Error: Unable to allocate memory "
                    "for the binary ROM image");
    }
    OsFile_close( handle );
  }
  return bundle;
}
#endif // !USE_LARGE_OBJECT_AREA
#endif // !USE_IMAGE_MAPPING

#if ENABLE_ISOLATES
#if ENABLE_LIB_IMAGES
int ROMBundle::add_to_global_binary_images( void* image_handle JVM_TRAPS) {
  UsingFastOops fast;
  ObjArray::Fast image_list = Universe::global_binary_images();
  ObjArray::Fast binary_persistant_list = Universe::global_binary_persistante_handles();
#if USE_IMAGE_MAPPING
  TypeArray::Fast handle_list = Universe::global_image_handles();
#endif
  int i=0;
  for (; i<image_list().length(); i++) {
    if (image_list().obj_at(i) == (OopDesc*)this) {
      return i;
    }
  }
  for (i = 0; i<image_list().length(); i++) {
    if (image_list().obj_at(i) == NULL) {
      image_list().obj_at_put(i, (OopDesc*)this);
#if USE_IMAGE_MAPPING
      handle_list().int_at_put(i, (int)image_handle);
#endif
      return i;
    }
  }
  //there are no free space inside Universe::binary_images()!  
  int old_length = image_list().length();
  ObjArray::Fast new_image_list = Universe::new_obj_array(old_length + MAX_TASKS JVM_CHECK_(-1));
  ObjArray::array_copy(&image_list, 0, &new_image_list, 0, old_length JVM_CHECK_(-1));
  new_image_list().obj_at_put(old_length, (OopDesc*)this);
  *Universe::global_binary_images() = new_image_list;  
#if USE_IMAGE_MAPPING
  TypeArray::Fast new_handle_list = Universe::new_int_array(old_length + MAX_TASKS JVM_CHECK_(-1));  
  TypeArray::array_copy(&handle_list, 0, &new_handle_list , 0, old_length);
  new_handle_list().int_at_put(old_length, (int)image_handle);
  *Universe::global_image_handles() = new_handle_list;
#endif
  return old_length;
}
#else //ENABLE_LIB_IMAGES
void ROMBundle::add_to_global_binary_images( ) {
  ObjArray::Raw list = Universe::global_binary_images();
  for (int i=0; i<MAX_TASKS; i++) {
    if (list().obj_at(i) == NULL) {
      list().obj_at_put(i, (OopDesc*)this);
      return;
    }
  }
  SHOULD_NOT_REACH_HERE();
}
#endif //ENABLE_LIB_IMAGES


#if ENABLE_LIB_IMAGES
bool ROMBundle::remove_if_not_currently_shared( void  ) {
  ObjArray::Raw list = Universe::global_binary_images();    
  bool shared = is_shared( Task::current() ); 
  if (shared) return true;
  ObjArray::Raw persistance_handles = Universe::global_binary_persistante_handles();
  for (int i=0; i<list().length(); i++) {
    if (list().obj_at(i) == (OopDesc*)this) {      
        persistance_handles().obj_at_clear(i);
        return false;
    }
  }
  SHOULD_NOT_REACH_HERE();
  return false;
}

bool ROMBundle::is_shared(const Task* dead_task) {
  TaskList::Raw tlist = Universe::task_list()->obj();
  int task_id;
  const int len = tlist().length();
  Task::Raw task;
  ObjArray::Raw images;
  for (task_id = Task::FIRST_TASK; task_id < len; task_id++) {
    task =  tlist().obj_at(task_id);
    if (task.is_null()) continue;
    if (task.obj() == dead_task->obj()) continue;
    images = task().binary_images();
    if (images.is_null()) continue;
    const int images_len = images().length();
    for (int i = 0; i < images_len; i++) {
      if (images().obj_at(i) == (OopDesc*)this) return true;
    }
  }
  return false;
}
#ifdef AZZERT
bool ROMBundle::is_shared() {
  TaskList::Raw tlist = Universe::task_list()->obj();
  int task_id;
  const int len = tlist().length();
  Task::Raw task;
  ObjArray::Raw images;
  int count = 0;
  for (task_id = Task::FIRST_TASK; task_id < len; task_id++) {
    task =  tlist().obj_at(task_id);    
    if (task.is_null()) continue;
    images = task().binary_images();
    if (images.is_null()) continue;
    const int images_len = images().length();
    for (int i = 0; i < images_len; i++) {
      if (images().obj_at(i) == (OopDesc*)this) count++;
    }
  }
  return count > 1;
}
#endif //AZZERT
bool ROMBundle::remove_from_global_binary_images() {  
#ifdef AZZERT
  GUARANTEE(!is_shared(), "cannot remove shared image from global list!"); 
#endif //AZZERT
  ObjArray::Raw list = Universe::global_binary_images();  
  int last;
  for (last=0; last<list().length(); last++) {
    if (list().obj_at(last) == NULL) {
      break;
    }
  }
  GUARANTEE(last > 0, "must have at least one item in the list");  
  for (int i=0; i<list().length(); i++) {
    if (list().obj_at(i) == (OopDesc*)this) {           
#if USE_IMAGE_MAPPING
      OsFile_MappedImage* image_handle = (OsFile_MappedImage*)
                              Universe::global_image_handles()->int_at(i);
      OsFile_UnmapImage(image_handle);
      Universe::global_image_handles()->int_at_put(i, 0);        
#endif
      if (i == last - 1) {
        list().obj_at_clear(i);
      } else {
        last --;
        list().obj_at_put(i, list().obj_at(last));
        list().obj_at_clear(last);
#if USE_IMAGE_MAPPING
        Universe::global_image_handles()->int_at_put(i, 
                       Universe::global_image_handles()->int_at(last));
        Universe::global_image_handles()->int_at_put(last, 0);
#endif
      }
      return true;
    }
  }  
  SHOULD_NOT_REACH_HERE();
  return true;
}

#else //ENABLE_LIB_IMAGES
bool ROMBundle::remove_from_global_binary_images() {    
  ObjArray::Raw list = Universe::global_binary_images();  
  int last;
  for (last=0; last<list().length(); last++) {
    if (list().obj_at(last) == NULL) {
      break;
    }
  }
  GUARANTEE(last > 0, "must have at least one item in the list");
  for (int i=0; i<MAX_TASKS; i++) {
    if (list().obj_at(i) == (OopDesc*)this) {
      if (i == last - 1) {
        list().obj_at_clear(i);
      } else {
        last --;
        list().obj_at_put(i, list().obj_at(last));
        list().obj_at_clear(last);
      }
      return true;
    }
  }
  SHOULD_NOT_REACH_HERE();
  return true;
}

#endif //!ENABLE_LIB_IMAGES

#endif // ENABLE_ISOLATES

/* This function is used for loading in a "dynamic" ROM image. */
/* A dynamic ROM image is contained in a separate file such */
/* as "ROM_binary.bun" */

bool ROM::link_dynamic(Task* task, FilePath* unicode_file JVM_TRAPS) {  
  DECLARE_STATIC_BUFFER(PathChar, path_name, NAME_BUFFER_SIZE + 7);
  unicode_file->string_copy(path_name, NAME_BUFFER_SIZE);

  void* image_handle = NULL;
  UsingFastOops fast_oops;

#if ENABLE_PERFORMANCE_COUNTERS
  jlong start_time = Os::elapsed_counter();
#endif

  // (0a) Open and read the bundle
  bool already_loaded = false;
  ROMBundle* bun = ROMBundle::load( task->task_id(), path_name, &image_handle, &already_loaded );
  if( bun == NULL) {
    return false;
  }
#if ENABLE_LIB_IMAGES   
  if (!check_bundle_references(bun, already_loaded)) {    
    if (!already_loaded) {
#if USE_IMAGE_MAPPING    
      OsFile_UnmapImage((OsFile_MappedImage*)image_handle);
#elif USE_LARGE_OBJECT_AREA 
      bun->free();
      LargeObject::compact();
#endif //USE_IMAGE_MAPPING
    }
    return false;
  }
#endif //ENABLE_LIB_IMAGES 

#if ENABLE_LIB_IMAGES
  task->add_binary_image(bun JVM_CHECK_0);
#endif

#if USE_IMAGE_MAPPING && !ENABLE_LIB_IMAGES
  task->add_binary_image_handle(image_handle JVM_CHECK_0);
#endif

  ROMBundle::set_current( bun );

#if ENABLE_PERFORMANCE_COUNTERS
  const jlong link_start_time = Os::elapsed_counter();
#endif

  // (0c) Adjust all the pointers in the binary ROM image by 
  // the relocation difference
  if (!already_loaded) {
    bun->fixup();    
  }

#if ENABLE_LIB_IMAGES && ENABLE_ISOLATES
  int global_bundle_id;
#endif
  {
    // The effect of this block would be undone in Task::link_dynamic()
    // if we fail later on.

    // We need to do this here. At this point, there's no reference
    // from into the image yet, but that will happen below this
    // block. If a GC happens, VerifyGC requires all references to
    // binary objects to be those that live in Universe::binary_images().
#if ENABLE_LIB_IMAGES
#if ENABLE_ISOLATES    
    global_bundle_id = bun->add_to_global_binary_images(image_handle JVM_CHECK_0);    
#else
    bun->add_to_global_binary_images();    
#endif
#else //!ENABLE_LIB_IMAGES
#if ENABLE_ISOLATES
    bun->add_to_global_binary_images();
    // This is necessary for restoring ROMBundle:current() -- if a GC happens
    // during loading, we may switch task temporarily to run finalizers.
    ObjArray::Raw binary_images = Universe::new_obj_array(1 JVM_CHECK_0);
    binary_images().obj_at_put(0, (OopDesc*)bun);
    task->set_binary_images(&binary_images);
#endif
#endif //ENABLE_LIB_IMAGES
  }

  if (!already_loaded) {
    OopDesc* heap_ptr = bun->copy_heap_block( JVM_SINGLE_ARG_CHECK_0 );
  }

#if ENABLE_PERFORMANCE_COUNTERS
  const jlong link_elapsed = Os::elapsed_counter() - link_start_time;
  jvm_perf_count.binary_link_hrticks += link_elapsed;
#endif

  // (7) Misc initialization
  Universe::set_number_of_java_classes( bun->number_of_java_classes() );

  // (8) Method entry initialization (for handling -comp flag)
  if (!already_loaded) {
    bun->update_rom_default_entries();
  }
  //
  // OBSOLETE ObjectHeap::recalc_slices_for_binary();
  //

  // restore VM structures expecting normal heap layout
#if ENABLE_LIB_IMAGES && ENABLE_ISOLATES
  bun->restore_vm_structures(global_bundle_id, already_loaded, bun->is_sharable() JVM_CHECK_0);    
#else //ENABLE_LIB_IMAGES
  bun->restore_vm_structures(JVM_SINGLE_ARG_CHECK_0);
#endif //ENABLE_LIB_IMAGES

#if ENABLE_PERFORMANCE_COUNTERS
  jlong elapsed = Os::elapsed_counter() - start_time;
  jvm_perf_count.binary_load_hrticks += elapsed;
  jvm_perf_count.total_load_hrticks += elapsed;
#endif

  if( VerifyGC ) {
    // The heap and universe handles should be consistent now
    ObjectHeap::verify();
  }    
  return true;
}

#if ENABLE_COMPILER
void ROMBundle::update_rom_default_entries( void ) {
  if (!UseCompiler || MixedMode) {
    return;
  }

  MethodVariablePart* ptr = (MethodVariablePart*)method_variable_parts();
  MethodVariablePart* const end =
    DERIVED(MethodVariablePart*, ptr, method_variable_parts_size());

  for( ; ptr < end; ptr++ ) {
    const address entry = ptr->execution_entry();
    if ((entry == (address)interpreter_fast_method_entry_0) ||
        (entry == (address)interpreter_fast_method_entry_1) ||
        (entry == (address)interpreter_fast_method_entry_2) ||
        (entry == (address)interpreter_fast_method_entry_3) ||
        (entry == (address)interpreter_fast_method_entry_4) ||
        (entry == (address)interpreter_method_entry)) {
      ptr->set_execution_entry( (address)shared_invoke_compiler );
    }
  }
}
#endif

#ifndef PRODUCT
void ROMBundle::method_variable_parts_oops_do(void do_oop(OopDesc**)) {
#if 0
  // IMPL_NOTE: ptr->_execution_entry may point inside the CompiledMethodDesc, so
  // oop verification may fail. This is disabled right now.
  MethodVariablePart* ptr = (MethodVariablePart*)method_variable_parts();
  MethodVariablePart* const end =
    DERIVED(MethodVariablePart*, ptr, method_variable_parts_size());

  for( ; ptr < end; ptr++ ) {
    OopDesc **obj = (OopDesc**)(ptr->_execution_entry);
    if (_heap_start <= obj && obj < _heap_top) {
      do_oop((OopDesc**)&(ptr->_execution_entry));
    }
  }
#endif
  (void)do_oop;
}
#endif

#if ENABLE_ISOLATES
void ROM::on_task_switch(int tid) {
#if !ENABLE_LIB_IMAGES
  // ROMBundle::current is useful only if you have no more than one
  // bundle per task.
  ROMBundle::set_current(NULL);
  Task::Raw task = Universe::task_from_id(tid);
  if (task.not_null()) {
    ObjArray::Raw images = task().binary_images();
    if (images.not_null()) {
      ROMBundle* bundle = (ROMBundle*) images().obj_at(0);
      ROMBundle::set_current(bundle);

    }
  }
#endif
}
#endif

#ifndef PRODUCT
void ROMBundle::print_on(Stream *st) {
#define ROMBUNDLE_ENUM_DUMP(x) \
      st->print_cr("array[%2d] = 0x%08x // %s", x, array[x], STR(x));
  ROMBUNDLE_FIELDS_DO(ROMBUNDLE_ENUM_DUMP)
}

void ROMBundle::p() {
  print_on(tty);
}

#if ENABLE_ISOLATES
void ROMBundle::print_all() {
  int i;
  ObjArray::Raw list = Universe::global_binary_images();
  GUARANTEE(!ENABLE_LIB_IMAGES, "doesn't implemented");
  /*IMPL_NOTE: here shall be list().length() which is != MAX_TASKS in case of ENABLE_LIB_IMAGES*/
  for (i=0; i<MAX_TASKS; i++) {
    if (list().obj_at(i) != NULL) {
      tty->print_cr("Universe::binary_images[%d] = 0x%08x", i,
                    int(list().obj_at(i)));
    }
  }

  list = Universe::task_list();
  for (i=0; i<MAX_TASKS; i++) {
    Task::Raw t = list().obj_at(i);
    if (t.not_null()) {
      tty->print("task_%d.binary_images = ", i);
      const char* prefix = "";
      ObjArray::Raw images = t().binary_images();
      if (images.not_null()) {
        for (int n=0; n<images().length(); n++) {
          tty->print("%s0x%08x\n", prefix, int(images().obj_at(n)));
          prefix = ", ";
        }
      }
      tty->cr();
    }
  }
}
#endif // ENABLE_ISOLATES
#endif // PRODUCT

#if ENABLE_LIB_IMAGES
bool ROM::check_bundle_references(ROMBundle* bun, bool already_loaded) {
  int* bundles = (int*)bun->ptr_at(bun->ROM_LINKED_BUNDLES_OFFSET);  
  if (!already_loaded) { //we have already relocated this ptr
    const int relocation_diff = DISTANCE( bun->base(), bun );  
    bundles = (int*)((int)bundles + relocation_diff);
  }
  int bundle_count = bundles[0];
  Task::Raw current_task = Task::current();
  ObjArray::Raw images = current_task().binary_images();
  if (images.is_null()) {
    if (bundle_count == 0) {
      return true;
    } else {
#ifdef AZZERT
      tty->print_cr("Your bundle number 1 requires another number of shared libraries(%d instead of 0).",
           bundle_count);
#endif
      return false;
    }
  }
  int i;
  for (i = 0; i < bundle_count; i++) {
    if (i >= images().length()) {
#ifdef AZZERT
    tty->print_cr("Your bundle number %d requires another number of shared libraries(%d instead of %d).",
      i, bundle_count, i - 1);
#endif
    return false;
    }
    ROMBundle* bundle = (ROMBundle*)images().obj_at(i);
    int expected_bundle_id = bundles[1+i];
    if (bundle == NULL || expected_bundle_id != bundle->bundle_id()) {
#ifdef AZZERT
      tty->print_cr("Your bundle number %d requires another version of shared libraries."
                                                        , bundle_count + 1);
#endif //AZZERT
      return false;
    }
  }
  if (i < images().length()) {
    if ((ROMBundle*)images().obj_at(i) != bun) {
#ifdef AZZERT
      tty->print_cr("Your bundle number %d requires another number of shared libraries(%d instead of %d).",
        bundle_count + 1, images().length(), bundle_count);
#endif
      return false;
    }
  }
  return true;
}
#endif //ENABLE_LIB_IMAGES

#endif // USE_BINARY_IMAGE_LOADER
