/*
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

inline void ROMBundle::restore_vm_structures(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  OopDesc** handles = persistent_handles();

  // Dictionary
  OopDesc *object = handles[0];
  relocate_pointer_to_heap(&object);
  ObjArray::Fast bun_dictionary = object;

  // class_list
  object = handles[1];
  relocate_pointer_to_heap(&object);
  ObjArray::Fast bun_class_list  = object;

#if ENABLE_ISOLATES
  // mirror_list
  object = handles[3];
  relocate_pointer_to_heap(&object);
  ObjArray::Fast bun_mirror_list = object;
#endif

  // names_of_bad_classes
  object = handles[2];
  relocate_pointer_to_heap(&object);  
#if ENABLE_LIB_IMAGES
  ObjArray::Fast bad_classes = Task::current()->names_of_bad_classes();
  ObjArray::Fast new_bad_classes = object;
  ObjArray::Fast restored_classes_list;
  if (bad_classes.is_null()) {
    Task::current()->set_names_of_bad_classes(object);
  } else {    
     restored_classes_list = Universe::new_obj_array(bad_classes().length() + 
                       new_bad_classes().length() JVM_CHECK);
     int i;
    for (i = 0; i < bad_classes().length(); i++) {
      restored_classes_list().obj_at_put(i, bad_classes().obj_at(i));
    }
    for (i = 0; i < bad_classes().length(); i++) {
      int idx = bad_classes().length() + i;
      restored_classes_list().obj_at_put(idx, new_bad_classes().obj_at(i));
    }
    Task::current()->set_names_of_bad_classes(restored_classes_list.obj());
  }
#else
  Task::current()->set_names_of_bad_classes(object);
#endif
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
  ObjArray::array_copy(&bun_mirror_list, 0,
                       &restored_mirror_list, sys_class_count,
                       bun_class_count JVM_MUST_SUCCEED);
#endif

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
  update_system_array_class(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
}

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
  juint* pp = array;

#if USE_IMAGE_PRELOADING || USE_IMAGE_MAPPING
  int skip_bytes = 0;
  int bit_skip_bytes = 0;

  if (relocation_diff == 0 && USE_IMAGE_MAPPING) {
    // The TEXT part needs no relocation (and wants none, because it's mapped
    // read-only.
    skip_bytes = DISTANCE(this, method_variable_parts());
    skip_bytes = (int)align_size_down(skip_bytes, BitsPerWord*BytesPerWord);
    bit_skip_bytes = skip_bytes / BitsPerWord;
  }

  const int* bitp = (const int*)relocation_bit_map();
  const int* const bit_end = DERIVED( const int*, bitp, *bitp ); // inclusive
  bitp = DERIVED( const int*, bitp, bit_skip_bytes );
  pp   = DERIVED( juint*, pp, skip_bytes );
  for( ; ++bitp <= bit_end; pp += BitsPerWord )
#else
  const juint* bitp = LargeObject::bitvector( this );
  const juint* const bit_end = LargeObject::head( this )->next_bitvector();

  // Never make this one a pointer. See bug 6362860
  GUARANTEE(((juint)(*bitp) & (1 << RELOCATION_BITMAP_OFFSET)) == 0,
            "Relocation bitmap bit must not be set");

  for( ; bitp < bit_end; bitp++, pp += BitsPerWord )
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
        int value = *p;
        if( (value & 0x1) == 0 ) {
#if ENABLE_LIB_IMAGES
          value = Task::current()->decode_reference(value);
#else
          value += relocation_diff;
#endif
#if CLEANUP_EXTERNAL_BITS
          new_bitword |= mask;
#endif
        } else {
          // reference to romized system image
#ifdef AZZERT
          const unsigned int type = unsigned(*p) >> ROM::offset_width;
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
            } break;
            case -InstanceSize::size_obj_array: {
#endif          
              value = int( Universe::empty_obj_array()->obj() );
            } break;
#if ENABLE_ISOLATES
            case -InstanceSize::size_task_mirror: {
              GUARANTEE(class_id < ROM::number_of_system_classes(), 
                        "Bad classid");
              value = int( Universe::task_class_init_marker()->obj() );
            } break;
#endif
            case -InstanceSize::size_obj_array_class:
            case -InstanceSize::size_type_array_class:
            case -InstanceSize::size_instance_class: {
              JavaClass::Raw jc = Universe::class_from_id(class_id);
              value = int( jc.obj() );
            } break;
            case -InstanceSize::size_java_near: {
              JavaClass::Raw jc = Universe::class_from_id(class_id);
              JavaNear::Raw jn = jc().prototypical_near();
              value = int( jn.obj() );
            } break;
          }
        }
        *p++ = value;
#if CLEANUP_EXTERNAL_BITS
        mask <<= 1;
#endif
      } while( (bitword >>= 1) != 0 );
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

inline void ROMBundle::copy_heap_block(JVM_SINGLE_ARG_TRAPS) const {
  // (1) Allocate space in the heap for the heap_block
  const unsigned size = heap_block_size();
  OopDesc* dst = ObjectHeap::allocate( size JVM_CHECK);

  // (2) Copy heap_block to its final destination
  const void* src = heap_block();
  jvm_memcpy( dst, src, size );

  // (3) Relocate pointers in the heap_block that point into heap_block
  const int diff = DISTANCE( src, dst );
  heap_relocation_offset = diff;
  if( size > 0 ) { 
    OopDesc* q = dst;
    OopDesc* const end = (OopDesc*)_inline_allocation_top;
    while( q < end ) {
#ifdef AZZERT
      OopDesc* orig_near_obj = q->klass();
#endif
      relocate_pointer_to_heap((OopDesc**)q);
#ifdef AZZERT
      if( heap_src_block_contains( address(orig_near_obj) ) ) {
        // If the near object also needs to be relocated, either of
        // the following must be true:
        // [1] (q->klass() < q) => the near object has already been relocated,
        //                         so q->klass()->klass() is good.
        // [2] (q->klass()->klass()) does not need to be relocated.
        GUARANTEE((q->klass() < q) ||
                  (ROM::system_contains(q->klass()->klass())),
                  "sanity");
      }
#endif
      FarClassDesc* blueprint = q->blueprint();
      if( heap_src_block_contains( address(blueprint) ) ) {
        blueprint = DERIVED(FarClassDesc*, blueprint, diff);
        GUARANTEE((OopDesc*)blueprint < q, 
                  "blueprint must have been relocated");
      }
      q->oops_do_for( blueprint, relocate_pointer_to_heap );
      q = DERIVED( OopDesc*, q, q->object_size_for(blueprint) );
    }
    GUARANTEE(q == end, "foo");
  }
}

#if USE_IMAGE_PRELOADING
OsFile_Handle ROMBundle::_preloaded_handle;
int           ROMBundle::_preloaded_length;

void ROMBundle::preload( const JvmPathChar class_path[] ) {
  int length = 0;
  OsFile_Handle handle = NULL;
  if( class_path ) {
    int count = 0;
    DECLARE_STATIC_BUFFER(JvmPathChar, new_class_path, NAME_BUFFER_SIZE);
    for(; class_path[count] != 0 && 
          class_path[count] != OsFile_path_separator_char; count++) {
    }

    jvm_memcpy(new_class_path, class_path, 
                count * sizeof(JvmPathChar));
    new_class_path[count] = 0;
    handle = open(new_class_path, length);
  }
  _preloaded_handle = handle;
  _preloaded_length = length;
  ObjectHeap::set_preallocated_space_size( length );
}
#endif

#if USE_IMAGE_MAPPING
inline ROMBundle* ROMBundle::load( const int /*task_id*/,
                      const JvmPathChar path_name[], void** map_handle)
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

  address preferred_addr = (address)header[BASE];
  int ro_length = header[METHOD_VARIABLE_PARTS] - header[BASE];
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

#else

#define FOREACH_TASK(t) \
  { UsingFastOops fast_oops; Task::Fast _task; Task *t = &_task; \
    for (int _i=0; _i < MAX_TASKS; _i++) { \
      _task = Universe::task_from_id(_i); \
      if (_task.not_null()) {

#define ENDEACH_TASK }}}

#endif
        
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

#else
// !USE_IMAGE_MAPPING

inline OsFile_Handle ROMBundle::open(const JvmPathChar* file, int& length) {
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
    juint magics[3];
    OsFile_read(file_handle, &magics, 1, sizeof magics);
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

// USE_IMAGE_PRELOADING || USE_LARGE_OBJECT_AREA
inline ROMBundle* ROMBundle::load(const int task_id,
                          const JvmPathChar path_name[], void** map_handle)
{
  ROMBundle* bundle = NULL;
  (void)map_handle;
#if USE_IMAGE_PRELOADING
  const int length = _preloaded_length;
  OsFile_Handle const handle = _preloaded_handle;
  (void)path_name;
#else
  int length;
  OsFile_Handle const handle = open( path_name, length );
#endif

  if( handle ) {
#if USE_IMAGE_PRELOADING
    ROMBundle* const p = (ROMBundle*) ObjectHeap::get_preallocated_space();
#elif USE_LARGE_OBJECT_AREA
    LargeObject* object =
      LargeObject::allocate( LargeObject::allocation_size( length ), task_id );
    ROMBundle* p = (ROMBundle*) (object ? object->body() : object);
#else
    SHOULD_NOT_REACH_HERE();
    // Used to be this, but we don't support it anymore
    // ROMBundle* const p = (ROMBundle*) OsMemory_allocate( length );
#endif

    if( p ) {
      const int n = OsFile_read( handle, p, 1, size_t(length) );
      if( n == length ) {
#if USE_LARGE_OBJECT_AREA
        juint bit_length;
        if( OsFile_read( handle, &bit_length, 1, sizeof bit_length )
              == sizeof bit_length &&
            OsFile_read( handle, LargeObject::bitvector( p ), 1, bit_length )
              == bit_length ) {
#endif
          bundle = p;
#if USE_LARGE_OBJECT_AREA
        }
#endif
      } else {
        tty->print_cr("Error: Unable to read entire binary ROM image: "
                      "need %d, got %d bytes", length, n);
#if USE_LARGE_OBJECT_AREA
        LargeObject::head( p )->free();
#endif
      }
    } else {
      tty->print_cr("Error: Unable to allocate memory "
                    "for the binary ROM image");
    }
    OsFile_close( handle );
  }
  return bundle;
}
#endif // !USE_IMAGE_MAPPING

#if ENABLE_ISOLATES
void ROMBundle::add_to_global_binary_images(
#if ENABLE_LIB_IMAGES
                                            JVM_SINGLE_ARG_TRAPS) {
  ObjArray::Raw list = Universe::binary_images();
  for (int i=0; i<list().length(); i++) {
    if (list().obj_at(i) == NULL) {
      list().obj_at_put(i, (OopDesc*)this);
      return;
    }
  }
  //there are no free space inside Universe::binary_images()!
  ObjArray::Raw new_list = Universe::new_obj_array(list().length() + MAX_TASKS JVM_CHECK);
  list = Universe::binary_images();
  ObjArray::array_copy(&list, 0, &new_list, 0, list().length() JVM_CHECK);
  new_list().obj_at_put(list().length(), (OopDesc*)this);
#else
                                            void ) {
  ObjArray::Raw list = Universe::binary_images();
  for (int i=0; i<MAX_TASKS; i++) {
    if (list().obj_at(i) == NULL) {
      list().obj_at_put(i, (OopDesc*)this);
      return;
    }
  }
  SHOULD_NOT_REACH_HERE();
#endif
}

void ROMBundle::remove_from_global_binary_images() {
  ObjArray::Raw list = Universe::binary_images();
#if !ENABLE_LIB_IMAGES  
  int last;
  for (last=0; last<MAX_TASKS; last++) {
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
      return;
    }
  }

  SHOULD_NOT_REACH_HERE();
#else
  //IMPL_NOTE: This shall be modified!
  for (int i=0; i<list().length(); i++) {
    if (list().obj_at(i) == (OopDesc*)this) {
        list().obj_at_clear(i);
      return;
    }
  }  
#endif
}
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
  ROMBundle* bun = ROMBundle::load( task->task_id(), path_name, &image_handle );
  if( bun == NULL) {
    return false;
  }
#if ENABLE_LIB_IMAGES
  if (!check_bundle_references(bun)) {
    //IMPL_NOTE: free image_handle
    return false;
  }
#endif

#if ENABLE_LIB_IMAGES
  ObjArray::Fast old_binary_images = task->binary_images();
  if (old_binary_images.not_null()) {
    ObjArray::Raw binary_images = Universe::new_obj_array(old_binary_images().length() + 1 JVM_CHECK_0);
    int i;
    for ( i = 0; i < old_binary_images().length(); i++) {
      binary_images().obj_at_put(i, old_binary_images().obj_at(i));
    }
    binary_images().obj_at_put(i, (OopDesc*)bun);
    task->set_binary_images(&binary_images);
  } else {
    ObjArray::Raw binary_images = Universe::new_obj_array(1 JVM_CHECK_0);
    binary_images().obj_at_put(0, (OopDesc*)bun);
    task->set_binary_images(&binary_images);
  }
#endif

#if USE_IMAGE_MAPPING
  {
    TypeArray::Raw img_handles = Universe::new_int_array(1 JVM_CHECK_0);
    img_handles().int_at_put(0, (int)image_handle);
    task->set_mapped_image_handles(&img_handles);
  }
#endif

  ROMBundle::set_current( bun );

#if ENABLE_PERFORMANCE_COUNTERS
  const jlong link_start_time = Os::elapsed_counter();
#endif

  // (0c) Adjust all the pointers in the binary ROM image by 
  // the relocation difference
  bun->fixup();

  {
    // The effect of this block would be undone in Task::link_dynamic()
    // if we fail later on.

    // We need to do this here. At this point, there's no reference
    // from into the image yet, but that will happen below this
    // block. If a GC happens, VerifyGC requires all references to
    // binary objects to be those that live in Universe::binary_images().
#if ENABLE_LIB_IMAGES
#if ENABLE_ISOLATES
    bun->add_to_global_binary_images(JVM_SINGLE_ARG_CHECK_0);
#endif
#else
    bun->add_to_global_binary_images();
    // This is necessary for restoring ROMBundle:current() -- if a GC happens
    // during loading, we may switch task temporarily to run finalizers.
    ObjArray::Raw binary_images = Universe::new_obj_array(1 JVM_CHECK_0);
    binary_images().obj_at_put(0, (OopDesc*)bun);
    task->set_binary_images(&binary_images);
#endif
  }

  bun->copy_heap_block( JVM_SINGLE_ARG_CHECK_0 );

#if ENABLE_PERFORMANCE_COUNTERS
  const jlong link_elapsed = Os::elapsed_counter() - link_start_time;
  jvm_perf_count.binary_link_hrticks += link_elapsed;
#endif

  // (7) Misc initialization
  Universe::set_number_of_java_classes( bun->number_of_java_classes() );

  // (8) Method entry initialization (for handling -comp flag)
  bun->update_rom_default_entries();

  //
  // OBSOLETE ObjectHeap::recalc_slices_for_binary();
  //

  // restore VM structures expecting normal heap layout
  bun->restore_vm_structures(JVM_SINGLE_ARG_CHECK_0);

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
  ObjArray::Raw list = Universe::binary_images();
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
bool ROM::check_bundle_references(ROMBundle* bun) {
  int bundle_offset = bun->int_at(bun->ROM_LINKED_BUNDLES_OFFSET);
  int* bundles = (int*)bun + (bundle_offset / sizeof(int));  
  int bundle_count = bundles[0];
  Task::Raw current_task = Task::current();
  ObjArray::Raw images = current_task().binary_images();
  if (images.is_null()) {
    if (bundle_count == 0) {
      return true;
    } else {
#ifdef AZZERT
      tty->print_cr("Your bundle number 1 requres another number of shared libraries(%d instead of 0).",
           bundle_count);
#endif
      return false;
    }
  }
  int i;
  for (i = 0; i < bundle_count; i++) {
    if (i >= images().length()) {
#ifdef AZZERT
    tty->print_cr("Your bundle number %d requres another number of shared libraries(%d instead of %d).",
      i, bundle_count, i - 1);
#endif
    return false;
    }
    ROMBundle* bundle = (ROMBundle*)images().obj_at(i);
    int expected_bundle_id = bundles[1+i];
    if (bundle == NULL) {
      if (bundle->int_at(ROMBundle::ROM_BUNDLE_ID) != expected_bundle_id) {
#ifdef AZZERT
        tty->print_cr("Your bundle number %d requres another version of shared libraries."
                                                          , bundle_count + 1);
#endif //AZZERT
        return false;
      }
    }
  }
  if (i < images().length()) {
    if ((ROMBundle*)images().obj_at(i) != bun) {
#ifdef AZZERT
      tty->print_cr("Your bundle number %d requres another number of shared libraries(%d instead of %d).",
        bundle_count + 1, images().length(), bundle_count);
#endif
      return false;
    }
  }

  return true;
}
#endif //ENABLE_LIB_IMAGES

#endif // USE_BINARY_IMAGE_LOADER
