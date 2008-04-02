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
# include "incls/_StackmapGenerator.cpp.incl"

const int bits_per_longmap_entry = BitsPerShort;

StackmapGenerator::StackmapGenerator(Method* method) {
  BytecodeClosure::initialize(method);
  _abstract_tos = 0;

  // always use the global gc stackmap
  _gc_stackmap = Universe::gc_block_stackmap();
}

void StackmapGenerator::initialize(int max_stack JVM_TRAPS) {
  const int task = ObjectHeap::start_system_allocation();
  *Universe::gc_block_stackmap() = Universe::new_byte_array(max_stack 
                                                            JVM_NO_CHECK);
  ObjectHeap::finish_system_allocation(task);
}

jubyte StackmapGenerator::tag_value_at(int index) {
  return cp()->tag_value_at(index);
}

// writes compressed verifier stackmap in the short format
unsigned int StackmapGenerator::write_short_map(TypeArray* verifier_stackmap) {
  AllocationDisabler raw_pointers_used_in_this_function;
  jint *verifier_stackmap_base = verifier_stackmap->int_base_address();
  unsigned short bitmap = 0;

  // Write the locals
  int locals = verifier_stackmap_base[1];
  int stack = verifier_stackmap_base[locals+2];
  unsigned short bit = 1;
  int i;
  
  for (i = 2; i < locals + 2; i++) {
    StackMapKind type =(StackMapKind) verifier_stackmap_base[i];
    if (convert_to_stackmaptag(type) == obj_tag) {
      bitmap = (unsigned short)(bitmap | bit);
    }
    bit <<= 1;
  } 

  // Write the expression stack 

  for (i = locals + 3; i < locals + stack + 3; i++) {
    StackMapKind type =(StackMapKind) verifier_stackmap_base[i];
    if (convert_to_stackmaptag(type) == obj_tag) {
      bitmap = (unsigned short)(bitmap | bit);
    }
    bit <<= 1;
  }

  return bitmap;
}

// writes compressed verifier stackmap in the long format
ReturnOop StackmapGenerator::write_long_map(TypeArray* verifier_stackmap
                                            JVM_TRAPS) {
  int bitmap_index = 3; // entries 0->2 are reserved
  unsigned short bmpBits = 0;
  unsigned short bit = 1;

  const int locals = verifier_stackmap->int_at(1); 
  const int stack = verifier_stackmap->int_at(locals+2);

  int array_max = 
    ((locals + stack + bits_per_longmap_entry - 1)/bits_per_longmap_entry) + 3;
  TypeArray::Raw bitmap = Universe::new_short_array(array_max JVM_CHECK_0);

  {
    AllocationDisabler raw_pointers_used_in_this_block;
    jint *verifier_stackmap_base = verifier_stackmap->int_base_address();
    jushort *bitmap_base = bitmap().ushort_base_address();

    int i;  
    // Write the locals  
    for (i = 2; i < locals + 2; i++) {
      StackMapKind type =(StackMapKind) verifier_stackmap_base[i];
      if (convert_to_stackmaptag(type) == obj_tag) {
        bmpBits = (unsigned short) (bmpBits | bit);
      }

      bit <<= 1;
    
      if (bit == 0) {
        bitmap_base[bitmap_index++] = bmpBits;
        bit = 1;
        bmpBits = 0;
      }
    }

    // Write the expression stack 
    for (i = locals + 3; i < locals + stack + 3; i++) {
      StackMapKind type =(StackMapKind) verifier_stackmap_base[i];
      if (convert_to_stackmaptag(type) == obj_tag) {
        bmpBits = (unsigned short) (bmpBits | bit);
      }

      bit <<= 1;

      if (bit == 0) {
        bitmap_base[bitmap_index++] = bmpBits;
        bit = 1;
        bmpBits = 0;
      }
    } 

    GUARANTEE((bitmap_index == array_max) || (bitmap_index == array_max - 1), 
              "Invalid long map");
  
    if (bitmap_index < array_max) {
      bitmap_base[bitmap_index] = bmpBits;
    }

    return bitmap;
  }
}

// reads the given compressed stackmap entry  and pushes stack
// elements to the abstract stack 
int StackmapGenerator::read_bitmap_internal(unsigned short& bmp_bits, 
                                            TypeArray* gc_stackmap,
                                            int array_index, 
                                            int bits_count) {
  AllocationDisabler raw_pointers_used_in_this_function;
  jubyte *raw_ptr = gc_stackmap->ubyte_base_address() + array_index;

  for (int i = 0; i < bits_count; i++) {
    if (bmp_bits & 0x1) {
      *raw_ptr++ = ITEM_Object;
    } else {
      *raw_ptr++ =ITEM_Integer;
    }
    bmp_bits >>= 1;
  }
  return bits_count;
}

// reads compressed stackmap in the short format
void StackmapGenerator::read_short_map(StackmapList* entry, 
                                       int index,
                                       int max_locals, 
                                       TypeArray* gc_stackmap) {
  int locals = entry->get_locals(index);
  int stack_height = entry->get_stack_height(index);

  unsigned short bitmap = entry->get_short_map(index);
  
  // read the locals
  read_bitmap_internal(bitmap, gc_stackmap, 0, locals);

  // complete the locals list
  for (int i = locals; i < max_locals; i++) {
    gc_stackmap->ubyte_at_put(i, ITEM_Bogus);
  }
  
  // read the stack entries
  read_bitmap_internal(bitmap, gc_stackmap, max_locals, stack_height);

  return;
}

// reads compressed stackmap in the long format
void StackmapGenerator::read_long_map(StackmapList* entry, int index,
                                      int max_locals, TypeArray* gc_stackmap) {

  AllocationDisabler raw_pointers_used_in_this_function;
  TypeArray::Raw bitmap = entry->get_long_map(index);

  int locals = entry->get_locals(index);
  int stack = entry->get_stack_height(index);

#ifdef AZZERT
  int expected_length = 
    ((locals + stack + bits_per_longmap_entry - 1)/bits_per_longmap_entry)+3;
  GUARANTEE(expected_length == bitmap().length(), 
            "Invalid compressed long map");
#endif

  int bitmap_index = 3; // entries 0-2 are reserved
  int stackmap_index = 0;
  unsigned short bmp_bits;
  int bits_left_after_locals = 0;  
  jushort *bitmap_base = bitmap().ushort_base_address();

  // read the locals  
  if (locals > 0) {
    int full_entries = locals/bits_per_longmap_entry;

    for (int i = 0; i < full_entries; i++) {
      bmp_bits = bitmap_base[bitmap_index++];
      stackmap_index += read_bitmap_internal(bmp_bits, gc_stackmap, 
                              stackmap_index, bits_per_longmap_entry);
    }

    int spillovers = locals % bits_per_longmap_entry;

    if (spillovers > 0) {
      bmp_bits = bitmap_base[bitmap_index++];
      stackmap_index += read_bitmap_internal(bmp_bits, gc_stackmap, 
                                             stackmap_index, spillovers);
      bits_left_after_locals = bits_per_longmap_entry - spillovers;
    }
  }
  
  // complete the locals list
  for (int i = locals; i < max_locals; i++) {
    gc_stackmap->ubyte_at_put(stackmap_index++, ITEM_Bogus);
  }

  // read the stack
  if (stack > 0) {  
    if (bits_left_after_locals > 0) {
      int read_count = (bits_left_after_locals > stack) ? stack
                          : bits_left_after_locals;
      stackmap_index += read_bitmap_internal(bmp_bits, gc_stackmap, 
                                             stackmap_index, read_count);
      stack -= read_count;
    }

    int full_entries = stack/bits_per_longmap_entry;

    for (int i = 0; i < full_entries; i++) {    
      bmp_bits = bitmap_base[bitmap_index++];
      stackmap_index += read_bitmap_internal(bmp_bits, gc_stackmap, 
                              stackmap_index, bits_per_longmap_entry);
    } 

    int spillovers = stack % bits_per_longmap_entry;

    if (spillovers > 0) {
      bmp_bits = bitmap_base[bitmap_index++];
      read_bitmap_internal(bmp_bits, gc_stackmap, 
                           stackmap_index, spillovers);
    }
  }
  
  return;
}

// convert the verifier stackmaps to a compressed java-ref/int stackmap format
// for the given method
ReturnOop StackmapGenerator::compress_verifier_stackmap(Method* method, 
                                                      ObjArray* method_stackmaps
                                                      JVM_TRAPS) {
  int entries = method_stackmaps->length();
  UsingFastOops fast_oops;
  StackmapList::Fast map_entry = Universe::new_stackmap_list(entries/2
                                                             JVM_CHECK_0);
  TypeArray::Fast verifier_stackmap;

  for (int i = 0, j = 0; i < entries; i+=2, j++) {
    verifier_stackmap = method_stackmaps->obj_at(i);
    juint locals = verifier_stackmap().int_at(1);
    juint stack_height = locals + verifier_stackmap().int_at(locals + 2); // get max_stack
    juint bci = verifier_stackmap().int_at(0);
    
#ifndef PRODUCT
    if (TraceStackmapsVerbose) {
      tty->print("StackmapGenerator: Compressing stackmaps of method:");
      method->print_value_on(tty);
      tty->cr();
      print_verifier_stackmap(&verifier_stackmap);
    }
#else
    (void)method;
#endif

    if (StackmapListDesc::is_shortmap_type(locals, stack_height-locals, bci)) {
      unsigned int bitmap = write_short_map(&verifier_stackmap);  
      map_entry().set_short_map(j, (jushort) bitmap);
    } else {
      TypeArray::Raw bitmap = write_long_map(&verifier_stackmap JVM_CHECK_0);
      map_entry().set_long_map(j, &bitmap);
    }

    map_entry().update(j, (jushort)locals, (jushort)(stack_height-locals),
                       (jushort)bci);

#ifndef PRODUCT
    if (TraceStackmapsVerbose) {
      tty->print("StackmapGenerator: Compressed entry:");
      map_entry.print_value_on(tty);
      tty->cr();
    }
#endif
  }
  
  return map_entry.obj();
}

// compress the stackmaps of all methods in the given class
void 
StackmapGenerator::compress_verifier_stackmaps(InstanceClass *ic JVM_TRAPS) {
  UsingFastOops fast_oops;

  Method::Fast method;
  ObjArray::Fast method_stackmap;
  StackmapList::Fast stackmap_list;
  TypeArray::Fast gc_stackmap;

  ObjArray::Fast methods = ic->methods();
  int num_methods = methods().length();
  ObjArray::Fast compressed_class_stackmaps = 
      Universe::new_obj_array(num_methods JVM_CHECK);
  int i;

  for (i = 0; i < num_methods; i++) {
    method = methods().obj_at(i);

    // Skip abstract and native methods. 
    if (method().access_flags().is_native() || 
        method().access_flags().is_abstract()) {
      compressed_class_stackmaps().obj_at_clear(i);
      continue;
    }

    // Compress verifier stackmaps into
    // space conserving reference-only stackmap
    method_stackmap = method().stackmaps();

    gc_stackmap = Universe::gc_block_stackmap(); 
    // max_execution_stack_count() returns maximum stack +
    // number of words of stack locks for this method.  We
    // could calculate the real execution stack count but at this
    // point it's not critical
    int max_stack_height = method().max_execution_stack_count();
    if (gc_stackmap.is_null() || 
        max_stack_height > gc_stackmap().length()) {
      // need to reallocate the global gc stackmap byte array
      StackmapGenerator::initialize(max_stack_height JVM_CHECK);
    }

    if (method_stackmap.not_null()) {
       stackmap_list = compress_verifier_stackmap(&method, 
                                                  &method_stackmap JVM_CHECK);
       if (stackmap_list().entry_count() == 0) {
         // if there are no entries for this method then
         // clear the entry from the class stackmap table
         // so that we can save some space during ROMization
         compressed_class_stackmaps().obj_at_clear(i);
       } else {
         compressed_class_stackmaps().obj_at_put(i, &stackmap_list);
       }
    }
  }

  // Replace the stackmaps held by the InstanceClass now
  // that we have compressed all of them to a new format
  for (i = 0; i < num_methods; i++) {
    Method::Raw m = methods().obj_at(i);
    StackmapList::Raw map_entry = compressed_class_stackmaps().obj_at(i);
    m().set_stackmaps(&map_entry);
  }
  return;
}

#if ENABLE_ROM_GENERATOR

void StackmapGenerator::convert_short_to_long_map(StackmapList* map_entry,
                                                  int index JVM_TRAPS) {
  int locals = map_entry->get_locals(index); 
  int stack = map_entry->get_stack_height(index);
  int bci = map_entry->get_bci(index);

  int array_max = 
    ((locals + stack + bits_per_longmap_entry - 1)/bits_per_longmap_entry) + 3;
  TypeArray bitmap = Universe::new_short_array(array_max JVM_CHECK);
  // transfer the stackmap to the long map
  // entries 0-2 are reserved for stackmap attributes
  if (array_max > 3) {
    bitmap.ushort_at_put(3, map_entry->get_short_map(index));
  }
  // setup the long map attributes
  map_entry->set_long_map(index, &bitmap);
  map_entry->update(index, (jushort)locals, (jushort)stack, (jushort)bci);  

  return;
}

// rewrites the stackmap bci's to account for shift in bytecodes
// used specifically by the ROMizer
void StackmapGenerator::rewrite_stackmap_bcis(Method* method, 
                                              unsigned short shifted_bci,
                                              short bci_shift JVM_TRAPS) {
  UsingFastOops fastoops;  
  StackmapList::Fast method_stackmaps = find_verifier_map(method);

  int num_stackmaps = 0;

  if (method_stackmaps().is_null()) {
    return;
  }

  num_stackmaps = method_stackmaps().entry_count();

#ifndef PRODUCT
  if (TraceStackmapsVerbose) {
    tty->print("StackmapGenerator: Rewriting stackmaps shifted_bci =%d"
               "bci_shift=%d of method:", shifted_bci, bci_shift);
    method->print_value_on(tty);
    tty->cr();
  }
#endif

  for (int i = 0 ; i < num_stackmaps; i++) {
    unsigned short map_bci = method_stackmaps().get_bci(i);
    if (map_bci >= shifted_bci) {

#ifndef PRODUCT
      if (TraceStackmapsVerbose) {
	tty->print_cr("StackmapGenerator: Shifting stackmap old_bci=%d new_bci=%d",
		      map_bci, map_bci + bci_shift);
      }
#endif

      // Check if the change in bci caused a change
      // in map entry type. Short map entries are sensitive
      // to the range of bci and stack height values
      int locals = method_stackmaps().get_locals(i); 
      int stack = method_stackmaps().get_stack_height(i);
      bool is_short = StackmapListDesc::is_shortmap_type(locals, stack,
                                                          map_bci + bci_shift);

      if (!is_short && method_stackmaps().is_short_map(i)) {
	convert_short_to_long_map(&method_stackmaps, i JVM_CHECK);
      }
      // IMPL_NOTE: try to convert long map to short map if possible
      method_stackmaps().set_bci(i, map_bci + bci_shift);
    }    
  } /* for loop*/

  return;
}

#endif

// finds the compressed verifier stackmaps entry for the given bytecode index
int StackmapGenerator::find_compressed_stackmap(StackmapList* stackmaps, 
                                                unsigned short gc_bci) {
  int target_index = -1; 
  int num_stackmaps = stackmaps->entry_count();

  if (num_stackmaps <= 0) {
    return target_index;
  }

  // Could it be the last block in the method ?
  if (gc_bci >= (unsigned short) stackmaps->get_bci(num_stackmaps-1)) {
    return num_stackmaps-1;
  } 

  // iterate through the list to find the block
  for (int i = 0; i < num_stackmaps; i++) {
    if (gc_bci < (unsigned short) stackmaps->get_bci(i)) {
      target_index = i-1;
      break;
    }
  }

  return target_index;
}

// generates the GC stackmap for the given bci - handles 
// compressed verifier stackmaps only
ReturnOop StackmapGenerator::generate_stackmap(StackmapList* method_stackmaps,
                                               int gc_bci) {
  // The CHECK macros used by this function is purely to conform to the APIs
  // in BytecodeClosure that could potentially throw exceptions (in other
  // subclasses of BytecodeClosure, but never in StackmapGenerator).
  AllocationDisabler lock_heap;
  SETUP_ERROR_CHECKER_ARG;

  Method *method = this->method();
  int gc_block_bci = -1;
  int max_locals = method->max_locals();
  int stack_items = 0;
  int stackmap_index = -1;

  if (method_stackmaps->not_null()) {
    stackmap_index = find_compressed_stackmap(method_stackmaps, 
                                              (jushort)gc_bci);
  }

#if ENABLE_ROM_GENERATOR
  if (checking_duplication()) { 
    GUARANTEE(method_stackmaps->not_null(), "Must have live stackmaps");
    GUARANTEE(stackmap_index >= 0, "Stack map must exist at index");
    GUARANTEE(method_stackmaps->get_bci(stackmap_index) == gc_bci, "Sanity");
    stackmap_index -= 1;
  }
#endif

  if (stackmap_index < 0) {
#ifndef PRODUCT
    if (TraceStackmapsVerbose) {
      tty->print_cr("StackmapGenerator: unable to find stackmap for bci=%d",
                    gc_bci);      
      if (method_stackmaps->not_null() && gc_bci > 0) {
        for (int i = 0 ; i < method_stackmaps->entry_count(); i++) {
          tty->print_cr("StackmapGenerator:stackmap for bci = %d available", 
                        method_stackmaps->get_bci(i));
        }
      }
    }
#endif

    Signature signature = method->signature();
    SignatureStream ss(&signature, method->is_static(), true);

    for (; !ss.eos(); ss.next()) {
      push(ss.type());
    }
    while (_abstract_tos < max_locals) { 
      push(ITEM_Bogus);
    } 
    // since stackmap is missing, begin abstract interpretation from
    // start of method till gc point
    gc_block_bci = 0;
  } else {
#ifndef PRODUCT
  if (TraceStackmapsVerbose) {
    tty->print_cr("StackmapGenerator:found stackmap for bci=%d", gc_bci);
    method_stackmaps->print_value_on(tty);
  }
#endif

    gc_block_bci = method_stackmaps->get_bci(stackmap_index);
    stack_items = method_stackmaps->get_stack_height(stackmap_index);

    if (method_stackmaps->is_short_map(stackmap_index)) {
      // read the short map into the gc_stackmap
      read_short_map(method_stackmaps, stackmap_index, 
                     max_locals, &_gc_stackmap);
    } else {
      // read the long map
      read_long_map(method_stackmaps, stackmap_index, 
                    max_locals, &_gc_stackmap);
    }

    _abstract_tos = max_locals + stack_items;
  }

  GUARANTEE(gc_bci >= gc_block_bci, "Invalid compressed stackmap");

#if ENABLE_ROM_GENERATOR
  if (checking_duplication()) { 
    GUARANTEE(gc_bci >= gc_block_bci, "Invalid compressed stackmap");
  }
#endif

#ifndef PRODUCT
  if (TraceStackmaps || TraceStackmapsVerbose) {
    tty->print_cr("StackmapGenerator:begin stackmap generation:");
    print_gc_stackmap(&_gc_stackmap, max_locals, _abstract_tos);
  }
#endif

  // abstract interpret till GC point
#if ENABLE_ROM_GENERATOR
  while(gc_bci > gc_block_bci && !aborted()) { 
    const Bytecodes::Code code = method->bytecode_at(gc_block_bci);
    const int next_bci = method->next_bci(gc_block_bci);
    if (next_bci <= gc_bci) {
      method->iterate_bytecode(gc_block_bci, this, code 
                                JVM_MUST_SUCCEED);
    } else {
      SHOULD_NOT_REACH_HERE();
      illegal_code(JVM_SINGLE_ARG_MUST_SUCCEED);
    }
    gc_block_bci = next_bci;
  }
#else
  if (gc_bci > gc_block_bci) { 
    method->iterate(gc_block_bci, gc_bci, this JVM_MUST_SUCCEED);
  }
#endif

#ifndef PRODUCT
  if (TraceStackmaps || TraceStackmapsVerbose) {
    tty->print_cr("StackmapGenerator:end stackmap generation:");
    print_gc_stackmap(&_gc_stackmap, max_locals, _abstract_tos);
  }
#endif

#if ENABLE_ROM_GENERATOR
  if (aborted()) {
    return NULL;
  }
#endif

#if USE_SOURCE_IMAGE_GENERATOR
  if (_gc_stackmap.is_null()) {
    // We can get here after fixup_image()
    // where Universe::gc_block_stackmap() is reset to null
    GUARANTEE(_abstract_tos == 0, "should be native void method");
    return NULL;
  }
#else
  GUARANTEE(_gc_stackmap.not_null(), "sanity");
#endif

  {
    AllocationDisabler raw_pointers_used_in_this_block;
    jubyte *raw_ptr = _gc_stackmap.ubyte_base_address();
    jubyte *raw_end = raw_ptr + _abstract_tos;

    while (raw_ptr < raw_end) {
      StackMapKind kind = (StackMapKind)*raw_ptr;
      *raw_ptr ++ = (jubyte)convert_to_stackmaptag(kind);
    }
  }
    
  return _gc_stackmap;
}

Tag StackmapGenerator::basictype_to_stackmaptag(BasicType type) {
#ifdef AZZERT
  switch (type) {
  case T_BOOLEAN:
  case T_CHAR:
  case T_BYTE:
  case T_SHORT:
  case T_INT:
  case T_FLOAT:
  case T_DOUBLE:
  case T_LONG:
  case T_OBJECT:
  case T_ARRAY:
  case T_VOID:
  case T_ILLEGAL:
    // OK
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }
#endif

  if (stack_type_for_table[type] == T_OBJECT) {
    return obj_tag;
  } else {
    return int_tag;
  }
}

void StackmapGenerator::push_int(jint /*val*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  push(T_INT);
}
 
void StackmapGenerator::push_long(jlong /*val*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  push(T_LONG);
}
void StackmapGenerator::push_float(jfloat /*val*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  push(T_FLOAT);
}
 
void StackmapGenerator::push_double(jdouble /*val*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  push(T_DOUBLE);
}

void StackmapGenerator::push_obj(Oop* /*val*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  push(T_OBJECT);
}

void StackmapGenerator::load_local(BasicType kind, int /*index*/ JVM_TRAPS)
{
  JVM_IGNORE_TRAPS;
  push(kind);
}

void StackmapGenerator::store_local(BasicType kind, int index JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  AllocationDisabler raw_pointers_used_in_this_function;
  pop(kind);

  jubyte *raw_ptr = _gc_stackmap.ubyte_base_address();
  int next = index + 1;
  
  switch(kind) { 
    case T_INT:   case T_BOOLEAN:
    case T_BYTE:  case T_CHAR: case T_SHORT:
      raw_ptr[index] = ITEM_Integer;
      break;
    case T_LONG:    
      raw_ptr[index]   = ITEM_Long;
      raw_ptr[index+1] = ITEM_Long_2;
      next = index + 2;
      break;
    case T_FLOAT:   
      raw_ptr[index] = ITEM_Float;
      break;
    case T_DOUBLE:  
      raw_ptr[index]   = ITEM_Double;
      raw_ptr[index+1] = ITEM_Double_2;
      next = index + 2;
      break;
    case T_OBJECT:
      raw_ptr[index] = ITEM_Object;
      break;
    case T_VOID:    
      break;
    default:        
      SHOULD_NOT_REACH_HERE();
  }  

  if (index > 0) {
    StackMapKind kind = (StackMapKind)raw_ptr[index - 1];
    if (kind == ITEM_Long || kind == ITEM_Double) {
      raw_ptr[index] = ITEM_Bogus;
    }
  }

  if (next < method()->max_locals()) {
    StackMapKind kind = (StackMapKind)raw_ptr[next];
    if (kind == ITEM_Long_2 || kind == ITEM_Double_2) {
      raw_ptr[next] =ITEM_Bogus;
    }
  }
}

void StackmapGenerator::array_length(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(1);
  push(T_INT);
}

void StackmapGenerator::load_array(BasicType kind JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(2);
  push(kind);
}

void StackmapGenerator::store_array(BasicType kind JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(kind);
  pop(2);
}

void StackmapGenerator::pop(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(1);
}

void StackmapGenerator::pop2(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(2);
}

void StackmapGenerator::dup(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  StackMapKind kind = (StackMapKind)_gc_stackmap.ubyte_at(_abstract_tos - 1);
  push(kind);
}

void StackmapGenerator::dup2(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  StackMapKind kind1 = (StackMapKind)_gc_stackmap.ubyte_at(_abstract_tos - 1);
  StackMapKind kind2 = (StackMapKind)_gc_stackmap.ubyte_at(_abstract_tos - 2);
  push2(kind1, kind2);
}

void StackmapGenerator::dup_x1(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  StackMapKind kind1, kind2;

  pop(&kind1); 
  pop(&kind2); 
  
  push(kind1);
  push(kind2);
  push(kind1);
}

void StackmapGenerator::dup2_x1(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;

  StackMapKind kind1, kind2, kind3;
 
  pop2(&kind1, &kind2);
  pop(&kind3);
 
  push2(kind1, kind2);
  push(kind3);
  push2(kind1, kind2);
}

void StackmapGenerator::dup_x2(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  StackMapKind kind1, kind2, kind3;
 
  pop(&kind1);
  pop2(&kind2, &kind3);
 
  push(kind1);
  push2(kind2, kind3);
  push(kind1);

}

void StackmapGenerator::dup2_x2(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  StackMapKind kind1, kind2, kind3, kind4;

  pop2(&kind1, &kind2);
  pop2(&kind3, &kind4);
  push2(kind1,kind2);
  push2(kind3, kind4);
  push2(kind1, kind2);
}

void StackmapGenerator::binary(BasicType type, binary_op op JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  switch(op) {
    case BytecodeClosure::bin_shl:
    case BytecodeClosure::bin_shr:
    case BytecodeClosure::bin_ushr:
      pop(1);  break;
    default:
      pop(type); break;
  } 
}
 
void StackmapGenerator::convert(BasicType from, BasicType to JVM_TRAPS)  {
  JVM_IGNORE_TRAPS;
  pop(from);
  push(to);
}

void StackmapGenerator::branch_if(cond_op /*cond*/, int /*dest*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(1);
}
 
void StackmapGenerator::branch_if_icmp(cond_op /*cond*/, int /*dest*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(2);
}
 
void StackmapGenerator::branch_if_acmp(cond_op /*cond*/, int /*dest*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(2);
}
 
void StackmapGenerator::compare(BasicType kind, cond_op /*cond*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(kind);
  pop(kind);
  push(T_INT);
}

void StackmapGenerator::swap(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  StackMapKind kind1, kind2;

  pop(&kind1);
  pop(&kind2);
  push(kind1);
  push(kind2);
}

void StackmapGenerator::instance_of(int /*index*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  pop(1);
  push(T_INT);
}

void StackmapGenerator::new_multi_array(int /*index*/, int num_of_dims 
                                        JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  // pop dimensions
  pop(num_of_dims);
  push_ref();
}

void StackmapGenerator::invoke_interface(int index, int /*num_of_args*/
                                         JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  // Get the class index from the resolved constant pool entry.
  int class_id, itable_index;
  cp()->resolved_interface_method_at(index, itable_index, class_id);

  // Get the class from the constant pool.
  InstanceClass::Raw interface_klass = Universe::class_from_id(class_id);
  Method::Raw callee = interface_klass().interface_method_at(itable_index);
  int params = callee().size_of_parameters();
  jubyte tag = cp()->tag_value_at(index);
  BasicType return_type = ConstantTag::resolved_interface_method_type(tag);

  pop(params);
  push(return_type);
}

void StackmapGenerator::invoke_special(int index JVM_TRAPS) {
  jubyte tag = tag_value_at(index);
  if (ConstantTag::is_resolved_static_method(tag)) {
    interpret_invoke_ops(index, false, true JVM_NO_CHECK_AT_BOTTOM);
  } else if (ConstantTag::is_resolved_virtual_method(tag)) {
    interpret_invoke_ops(index, true, true JVM_NO_CHECK_AT_BOTTOM);
  } else {
    interpret_unresolved_invoke_ops(index, true JVM_NO_CHECK_AT_BOTTOM);
  }
}

void StackmapGenerator::invoke_static(int index JVM_TRAPS) {
  jubyte tag = tag_value_at(index);
  if (ConstantTag::is_resolved_static_method(tag)) {
    interpret_invoke_ops(index, false, false JVM_NO_CHECK_AT_BOTTOM);
  } else {
    interpret_unresolved_invoke_ops(index, false JVM_NO_CHECK_AT_BOTTOM);
  }
}

void StackmapGenerator::invoke_virtual(int index JVM_TRAPS) {
  jubyte tag = tag_value_at(index);
  if (ConstantTag::is_resolved_static_method(tag)) {
    interpret_invoke_ops(index, false, true JVM_NO_CHECK_AT_BOTTOM);
  } else if (ConstantTag::is_resolved_virtual_method(tag)) {
    interpret_invoke_ops(index, true, true JVM_NO_CHECK_AT_BOTTOM);
  } else {
    interpret_unresolved_invoke_ops(index, true JVM_NO_CHECK_AT_BOTTOM);
  }
}

void StackmapGenerator::new_object(int /*index*/ JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  push_ref();
}

void StackmapGenerator::new_basic_array(int /*type*/ JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  pop(1);
  push_ref(); 
}

void StackmapGenerator::new_object_array(int /*index*/ JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  pop(1);
  push_ref(); 
}

void StackmapGenerator::fast_invoke_virtual(int index JVM_TRAPS) {
  interpret_invoke_ops(index, true, true JVM_NO_CHECK_AT_BOTTOM);
}

void StackmapGenerator::fast_invoke_virtual_final(int index JVM_TRAPS) {
  interpret_invoke_ops(index, false, true JVM_NO_CHECK_AT_BOTTOM);
}

void StackmapGenerator::fast_invoke_special(int index JVM_TRAPS) {
  interpret_invoke_ops(index, true, true JVM_NO_CHECK_AT_BOTTOM);
}

void StackmapGenerator::fast_get_field(BasicType field_type,
                                       int /*field_offset*/ JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  pop(1);
  push(field_type);
}

void StackmapGenerator::fast_put_field(BasicType field_type,
                                       int /*field_offset*/ JVM_TRAPS) { 
  JVM_IGNORE_TRAPS;
  pop(field_type); 
  pop(1);  
}

void StackmapGenerator::interpret_field_ops(int index, bool is_put, 
                                            bool is_static JVM_TRAPS) {
  BasicType field_kind;
  int offset;

  jubyte tag = cp()->tag_value_at(index);

  if (ConstantTag::is_resolved_field(tag)) {
    field_kind = cp()->resolved_field_type_at(index, offset);
  } else {
    UsingFastOops fast_oops;
    FieldType::Fast field_type;
    Symbol::Fast field_name, field_klass_name, field_type_name;

    cp()->resolve_helper(index, &field_name, &field_type,
                         &field_klass_name JVM_CHECK);
    field_kind = field_type().basic_type();
  }

  if (is_put) {
    pop(field_kind);
    if (!is_static) {
      pop(1);
    }
  } else {
    if (!is_static) {
      pop(1);
    }
    push(field_kind);
  }
}

void StackmapGenerator::interpret_unresolved_invoke_ops(int index, bool 
                                                        pop_this JVM_TRAPS) {
  UsingFastOops fast_oops;
  Signature::Fast callee_signature;

  Symbol::Fast callee_name, callee_klass_name;
  cp()->resolve_helper(index, &callee_name, &callee_signature,
                    &callee_klass_name JVM_CHECK);
  
  // pop arguments
  int params = callee_signature().parameter_word_size(!pop_this);
  pop(params);

  BasicType return_type = callee_signature().return_type(true);
  push(return_type);
}

void StackmapGenerator::interpret_invoke_ops(int index, bool is_virtual, 
                                             bool /*pop_this*/ JVM_TRAPS) {
  JVM_IGNORE_TRAPS;
  Method::Raw callee;
  Signature::Raw callee_signature;
  BasicType return_type;

  if (is_virtual) {
    int vtable_index, class_id;
    return_type = cp()->resolved_virtual_method_at(index, vtable_index, 
                                                  class_id);
    JavaClass::Raw klass = Universe::class_from_id(class_id);
    ClassInfo::Raw info = klass().class_info();
    callee = info().vtable_method_at(vtable_index);
    callee_signature = callee().signature();
  } else {
    callee = cp()->resolved_static_method_at(index);
    callee_signature = callee().signature();
    return_type = callee_signature().return_type(true);
  }

  // pop arguments
  int params = callee().size_of_parameters();
  pop(params);

  // push the return
  push(return_type);
}

void StackmapGenerator::push_ref() {
  push(ITEM_Object);
}

void StackmapGenerator::push(StackMapKind kind) {
  _gc_stackmap.ubyte_at_put(_abstract_tos++, (jubyte)kind);
}

void StackmapGenerator::push(BasicType type) {
  switch(type) { 
    case T_INT:   case T_BOOLEAN:
    case T_BYTE:  case T_CHAR: case T_SHORT:   
      push(ITEM_Integer);  
      break;
    case T_LONG:    
      push(ITEM_Long); 
      push(ITEM_Long_2); 
      break;
    case T_FLOAT:   
      push(ITEM_Float); 
      break;
    case T_DOUBLE:  
      push(ITEM_Double); 
      push(ITEM_Double_2); 
      break;
    case T_OBJECT:
    case T_ARRAY:
      push_ref();
      break;
    case T_VOID:    
      break;
    default:        
      SHOULD_NOT_REACH_HERE();
  }
}

void StackmapGenerator::push2(StackMapKind kind1, StackMapKind kind2) {
  _gc_stackmap.ubyte_at_put(_abstract_tos++, (jubyte)kind2);
  _gc_stackmap.ubyte_at_put(_abstract_tos++, (jubyte)kind1);
}

void StackmapGenerator::pop(StackMapKind* kind) {
  *kind = (StackMapKind)_gc_stackmap.ubyte_at(_abstract_tos - 1);
  pop(1);
}

void StackmapGenerator::pop(BasicType type) {
  if (is_two_word(type)) {
    pop(2);
  } else {
    pop(1);
  }
}

void StackmapGenerator::pop(int slots) {
  _abstract_tos -= slots;
  GUARANTEE(_abstract_tos >= 0, "Invalid abstract stack state");
}

void StackmapGenerator::pop2(StackMapKind* kind1, StackMapKind* kind2) {
  *kind1 = (StackMapKind)_gc_stackmap.ubyte_at(_abstract_tos - 1);
  *kind2 = (StackMapKind)_gc_stackmap.ubyte_at(_abstract_tos - 2);
  pop(2);
}

Tag StackmapGenerator::convert_to_stackmaptag(StackMapKind kind) {
#ifdef AZZERT
  switch(kind) {
  case ITEM_Bogus: 
  case ITEM_Float:   
  case ITEM_Long:  
  case ITEM_Long_2:   
  case ITEM_Double:  
  case ITEM_Double_2: 
  case ITEM_Integer:    /* return int_tag; */

  case ITEM_Null: 
  case ITEM_InitObject:
  case ITEM_Object: 
  case ITEM_NewObject:  /* return obj_tag; */
    break;
  default:
    GUARANTEE(kind & ITEM_NewObject_Flag, "Must be new object");
    /* return obj_tag; */
  }
#endif
  if (kind <= ITEM_Long || kind == ITEM_Long_2 || kind == ITEM_Double_2) {
    return int_tag;
  } else {
    return obj_tag;
  }
}

#ifndef PRODUCT
void StackmapGenerator::print_verifier_stackmap(TypeArray* stackmap) {
#if USE_DEBUG_PRINTING
  int i = 0;

  int locals = stackmap->int_at(1);
  int stack  = stackmap->int_at(locals + 2);

  tty->print_cr("Verifier Stackmap:");
  for (i = 2; i < locals + 2 ; i++) {
    StackMapKind kind = (StackMapKind)stackmap->int_at(i);
    print_map_internal("LOCAL", i-2, kind);
  }
  if (locals > 0 && stack > 0) {
    tty->print_cr("   ---");
  }
  for (i = locals + 3; i < locals + stack + 3; i++) {
    StackMapKind kind = (StackMapKind)stackmap->int_at(i);
    print_map_internal("STACK", i-locals-3, kind);
  }
  tty->print_cr("---------");
#endif
}

void StackmapGenerator::print_gc_stackmap(TypeArray* stackmap, int max_locals,
                                          int stack) {
#if USE_DEBUG_PRINTING
  tty->print_cr("GC Stackmap:");

  for (int i = 0; i < max_locals; i++) {
    StackMapKind kind = (StackMapKind)stackmap->ubyte_at(i);
    print_map_internal("LOCAL", i, kind);
  }
  tty->print_cr("---------");

  for (int j = max_locals; j < stack; j++) {
    StackMapKind kind = (StackMapKind)stackmap->ubyte_at(j);
    print_map_internal("STACK", j, kind);
  }
  tty->print_cr("---------");
#endif
}

void StackmapGenerator::print_map_internal(const char *name, int index,
                                         StackMapKind kind) {
#if USE_DEBUG_PRINTING
  tty->print("  %s[%d] %d: ", name, index, kind);
  switch(kind) {
    case ITEM_Object:     tty->print_cr("Ref"); break;
    case ITEM_Integer:    tty->print_cr("Integer"); break;
    case ITEM_Float:      tty->print_cr("Float"); break;
    case ITEM_Double:     tty->print_cr("Double"); break;
    case ITEM_Double_2:   tty->print_cr("Double_2"); break;
    case ITEM_Long:       tty->print_cr("Long"); break;
    case ITEM_Long_2:     tty->print_cr("Long_2"); break;
    case ITEM_InitObject: tty->print_cr("Uninitialized This"); break;
    case ITEM_Null:       tty->print_cr("Null"); break;
    case ITEM_Bogus:      tty->print_cr("Bogus"); break;
    default:              tty->print_cr("???"); break;
  }
#endif
}

#endif // PRODUCT

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR || USE_DEBUG_PRINTING

bool StackmapChecker::is_redundant(Method* method,
				   int stackmap_index JVM_TRAPS) {
  TypeArray gc_stackmap = Universe::gc_block_stackmap();   
  bool was_null = gc_stackmap.is_null();
  jubyte temp[1000];
  {
    int max_stack_height = method->max_execution_stack_count();
    if (max_stack_height > 1000) {
      return false;
    }
    if (was_null) {
      // need to reallocate the global gc stackmap byte array
      StackmapGenerator::initialize(max_stack_height JVM_CHECK_0);
    }
  }

  StackmapList method_stackmaps = method->stackmaps();
  int bci = method_stackmaps.get_bci(stackmap_index);
  
  StackmapChecker gen1(method);
  StackmapGenerator gen2(method);
  TypeArray map = gen1.generate_stackmap(&method_stackmaps, bci);
  if (map.is_null()) {
    if (was_null) { 
      *Universe::gc_block_stackmap() = (OopDesc*)0;
    }
    return false;
  }
  int map_length = gen1.abstract_stack_top();
  for (int i = 0; i < map_length; i++) { 
    temp[i] = map.ubyte_at(i);
  }

  map = gen2.generate_stackmap(&method_stackmaps, bci);
  GUARANTEE(gen2.abstract_stack_top() == map_length, "Same length??");
  
  if (was_null) { 
    *Universe::gc_block_stackmap() = (OopDesc*)0;
  }

  for (int j = 0; j < map_length; j++) {
    if (temp[j] != map.ubyte_at(j)) { 
      return false;
    }
  }
  return true;
}

#endif
