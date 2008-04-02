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

class StackmapGenerator: public BytecodeClosure { 

public:

  StackmapGenerator(Method* method);

  static void initialize(int max_stack JVM_TRAPS);

  // find verifier stackmap given search key
  static ReturnOop find_verifier_map(Method* m) {
    return m->stackmaps();
  }

  static int find_compressed_stackmap(StackmapList* stackmaps, 
                                      unsigned short gc_bci);

  // compress verifier stackmaps to short/long format
  static void compress_verifier_stackmaps(InstanceClass *ic JVM_TRAPS);
  static ReturnOop compress_verifier_stackmap(Method* method, 
                                              ObjArray* method_stackmaps JVM_TRAPS);
  static ReturnOop write_long_map(TypeArray* verifier_stackmap JVM_TRAPS);
  static unsigned int write_short_map(TypeArray* verifier_stackmap);

  // read compressed verifier stackmaps in short/long format
  static void read_short_map(StackmapList* list, int entry_index, 
                             int max_locals, TypeArray* gc_stackmap);
  static void read_long_map(StackmapList* list, int entry_index,
                            int max_locals, TypeArray* gc_stackmap);

  // print stackmaps in various formats
  static void print_verifier_stackmap(TypeArray* stackmap);
  static void print_gc_stackmap(TypeArray* gc_stackmap, int max_locals,
                                int stack);

  static Tag basictype_to_stackmaptag(BasicType type);
  static Tag convert_to_stackmaptag(StackMapKind kind);

#if ENABLE_ROM_GENERATOR
  // converts an existing short map entry to a long map
  static void convert_short_to_long_map(StackmapList* list, int index JVM_TRAPS);
  // rewrite stackmaps to compensate for bytecode shift while ROMizing
  // constantpool 
  static void rewrite_stackmap_bcis(Method* method, unsigned short shifted_bci,
                                    short bci_shift JVM_TRAPS);
#endif

 // generate gc stackmaps from compressed verifier stackmaps
  ReturnOop generate_stackmap(StackmapList* method_stackmaps, int gc_bci);

  // provides the stack height of the gc stackmap after abstract interpretation
  int abstract_stack_top() { return _abstract_tos;}

  // abstract interpretation 
  virtual void push_int   (jint    value JVM_TRAPS);
  virtual void push_long  (jlong   value JVM_TRAPS);
  virtual void push_float (jfloat  value JVM_TRAPS);
  virtual void push_double(jdouble value JVM_TRAPS);
  virtual void push_obj   (Oop*    value JVM_TRAPS);

  virtual void load_local(BasicType kind, int index JVM_TRAPS);
  virtual void store_local(BasicType kind, int index JVM_TRAPS);

  virtual void array_length(JVM_SINGLE_ARG_TRAPS);
  virtual void load_array(BasicType kind JVM_TRAPS);
  virtual void store_array(BasicType kind JVM_TRAPS);

  virtual void pop(JVM_SINGLE_ARG_TRAPS);
  virtual void pop2(JVM_SINGLE_ARG_TRAPS);
  virtual void dup(JVM_SINGLE_ARG_TRAPS);
  virtual void dup2(JVM_SINGLE_ARG_TRAPS);
  virtual void dup_x1(JVM_SINGLE_ARG_TRAPS);
  virtual void dup2_x1(JVM_SINGLE_ARG_TRAPS);
  virtual void dup_x2(JVM_SINGLE_ARG_TRAPS);
  virtual void dup2_x2(JVM_SINGLE_ARG_TRAPS);

  virtual void swap(JVM_SINGLE_ARG_TRAPS);

  virtual void convert(BasicType from, BasicType to JVM_TRAPS);
  virtual void branch_if     (cond_op cond, int dest JVM_TRAPS);
  virtual void branch_if_icmp(cond_op cond, int dest JVM_TRAPS);
  virtual void branch_if_acmp(cond_op cond, int dest JVM_TRAPS);

  virtual void binary(BasicType kind, binary_op op JVM_TRAPS);

  virtual void compare(BasicType kind, cond_op cond JVM_TRAPS);

  virtual void get_field(int index JVM_TRAPS) {
    interpret_field_ops(index, false, false JVM_NO_CHECK_AT_BOTTOM);
  }
  virtual void put_field(int index JVM_TRAPS) {
    interpret_field_ops(index, true, false JVM_NO_CHECK_AT_BOTTOM); 
  }
  virtual void get_static(int index JVM_TRAPS) {
    interpret_field_ops(index, false, true JVM_NO_CHECK_AT_BOTTOM); 
  }
  virtual void put_static(int index JVM_TRAPS) { 
    interpret_field_ops(index, true, true JVM_NO_CHECK_AT_BOTTOM); 
  }
  virtual void invoke_interface(int index, int num_of_args JVM_TRAPS);
  virtual void invoke_special(int index JVM_TRAPS);
  virtual void invoke_static(int index JVM_TRAPS);
  virtual void invoke_virtual(int index JVM_TRAPS);
  virtual void invoke_native(BasicType return_kind, address /*entry*/ JVM_TRAPS)
  {
    JVM_IGNORE_TRAPS;
    push(return_kind);
  }

  virtual void new_object(int index JVM_TRAPS);
  virtual void new_basic_array(int type JVM_TRAPS);
  virtual void new_object_array(int index JVM_TRAPS);
  virtual void new_multi_array(int index, int num_of_dims JVM_TRAPS);

  virtual void instance_of(int index JVM_TRAPS);
  virtual void monitor_enter(JVM_SINGLE_ARG_TRAPS) {
    pop(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
  virtual void monitor_exit(JVM_SINGLE_ARG_TRAPS){
    pop(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }

  virtual void fast_invoke_virtual(int index JVM_TRAPS);
  virtual void fast_invoke_virtual_final(int index JVM_TRAPS);
  virtual void fast_invoke_special(int index JVM_TRAPS);
  virtual void fast_get_field(BasicType field_type, int field_offset JVM_TRAPS);
  virtual void fast_put_field(BasicType field_type, int field_offset JVM_TRAPS);
 
#if ENABLE_ROM_GENERATOR
  virtual bool checking_duplication() { return false; }
  virtual bool aborted()              { return false; }
#endif

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR
  // These entries are needed only by the Romizer.
  // IMPL_NOTE: these are used by StackmapChecker to compress stack maps.
  // This optimization is too expensive in binary romizer, so we may
  // as well remove the code for Monet mode to save footprint.
  virtual void should_not_reach_here() { SHOULD_NOT_REACH_HERE(); }

  virtual void branch(int /*dest*/ JVM_TRAPS) { 
    JVM_IGNORE_TRAPS;
    should_not_reach_here(); 
  }
  virtual void return_op(BasicType /*kind*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    should_not_reach_here();
  }

  virtual void table_switch(jint /*table_index*/, jint /*default_dest*/,
                            jint /*low*/, jint /*high*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    should_not_reach_here();
  }
  virtual void lookup_switch(jint /*table_index*/, jint /*default_dest*/,
                             jint /*num_of_pairs*/ JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    should_not_reach_here();
  }
  virtual void throw_exception(JVM_SINGLE_ARG_TRAPS) {
    JVM_IGNORE_TRAPS;
    should_not_reach_here();
  }
  virtual void uncommon_trap(JVM_SINGLE_ARG_TRAPS){
    JVM_IGNORE_TRAPS;
    should_not_reach_here();
  }
  virtual void illegal_code(JVM_SINGLE_ARG_TRAPS) {
    JVM_IGNORE_TRAPS;
    should_not_reach_here();
  }
#endif

private:
  int       _abstract_tos;
  TypeArray _gc_stackmap;
  
private:

  void push_ref();
  
  void push(StackMapKind kind);
  void push(BasicType type);
  void push2(StackMapKind kind1, StackMapKind kind2);

  void pop(StackMapKind* kind);
  void pop(BasicType type);
  void pop2(StackMapKind* kind1, StackMapKind* kind2);

  void pop(int slots);

  void interpret_field_ops(int index, bool is_put, bool is_static JVM_TRAPS);
  void interpret_invoke_ops(int index, bool is_virtual, bool pop_this JVM_TRAPS);
  void interpret_unresolved_invoke_ops(int index, bool pop_this JVM_TRAPS);

private:
  static void print_map_internal(const char* name, int index,
                                 StackMapKind kind);
  static int read_bitmap_internal(unsigned short& bmp_bits,
                                  TypeArray* gc_stackmap,
                                  int array_index, int bits_count);
  jubyte tag_value_at(int index);
};

#if !defined(PRODUCT) || USE_PRODUCT_BINARY_IMAGE_GENERATOR || USE_DEBUG_PRINTING

class StackmapChecker : public StackmapGenerator {
public:
  static bool is_redundant(Method* method, int stackmap_index JVM_TRAPS);

  StackmapChecker(Method* method) : StackmapGenerator(method), _abort(false) {}
  virtual bool checking_duplication()  { return true; }
  virtual bool aborted()               { return _abort; }
  virtual void should_not_reach_here() { _abort = true; }

private:
  bool _abort;
};

#endif
