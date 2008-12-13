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

#if ENABLE_ROM_GENERATOR

class ROMHashtableManager;

// This class rewrites ConstantPools of the romized classes to reduce
// the footprint of the ROM image.
//
// Many constant pool entries (as much as 70%) are duplicated.
// ConstantPoolRewriter removes the duplicated entries by merging all
// ConstantPools of the romized classes into a single shared pool.
class ConstantPoolRewriter {
  // The instance of the optimizer
  ROMOptimizer *_optimizer;
  Stream *_log_stream;
  // The constant pool created by merging the pools of all system classes.
  ConstantPool _merged_pool;
#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR
  BytecodeOptimizer _bytecode_optimizer;
#endif
  // Number of entries in the original constant pools
  int _original_pools_count;

  // Number of entries in the original constant pools  
  int _original_entries_count;

  // Number of entries in the merged constant pools
  int _merged_entries_count;

  // Changes in the size of methods after CP rewritting.
  int _method_size_delta;

  // maximum length of a method in the system
  int _max_method_length;

  // we calculate it form _original_entries_count and _hashtab_mgr
  int _predicted_merged_pool_size;

  // Mapping from original cp# to merged cp#.
  ObjArray _entry_lookup_table;

  // Mapping from old methods to new methods
  ObjArray method_map_array;

  // Constant pools that have been processed by update_methods_in_constants()
  ObjArray _updated_constant_pools;

  // Mapping from old bytecode address to new bytecode address, for the
  // method that's currently being rewritten.
  TypeArray _new_bytecode_address;

  // Marks all the branch targets in the method that's currently 
  // being rewritten.
  TypeArray _branch_targets;

  // Accounting information.
  int _count_of_utf8;
  int _count_of_string;
  int _count_of_static_method;
  int _count_of_long;
  int _count_of_double;
  int _count_of_field_ref;
  int _count_of_interface_method_ref;
  int _count_of_virtual_method_ref;
  int _count_of_invalid;
  int _count_of_integer;
  int _count_of_class;
  int _count_of_float;

  void initialize(JVM_SINGLE_ARG_TRAPS);
  void count_original_pools();
  void allocate_merged_pool(JVM_SINGLE_ARG_TRAPS);
  void flush_merged_pool(JVM_SINGLE_ARG_TRAPS);
  void free_unused_space(JVM_SINGLE_ARG_TRAPS);
  void init_embedded_hashtables(JVM_SINGLE_ARG_TRAPS);
  void stream_hashtable(ObjArray *rom_table, bool is_symbol JVM_TRAPS);
  jushort get_merged_pool_entry(ConstantPool *orig_cp, int i JVM_TRAPS);
  jint get_merged_method_ref(ConstantPool *orig_cp, int i JVM_TRAPS);
  jint get_merged_field_ref(ConstantPool *orig_cp, int i);
  juint  compute_hashcode(ConstantPool *orig_cp, int i);
  static juint  hashcode_for_method(Method *method);
  static juint  hashcode_for_symbol(Symbol *symbol);
  juint  hashcode_for_string(String *string);
  juint  hashcode_for_method_ref(ConstantPool *orig_cp, int i);
  juint  hashcode_for_field_ref(ConstantPool *orig_cp, int i);
  bool match_method_ref(jint new_ref, ConstantPool *orig_cp, int i JVM_TRAPS);
  bool match_field_ref(jint new_ref, ConstantPool *orig_cp, int i);
  int  find_stored_hash_entry(ConstantPool *orig_cp, int i, ConstantTag tag,
                              juint hash);
  void store_hash_entry(ConstantPool *orig_cp, int i, ConstantTag tag,
                        juint hash, int new_index JVM_TRAPS);
  void rewrite_class(InstanceClass *klass JVM_TRAPS);
  void rewrite_class_object(InstanceClass *klass JVM_TRAPS);
  void rewrite_method(Method *method JVM_TRAPS);
  void rewrite_method_header(Method *method JVM_TRAPS);
  void rewrite_exception_table(Method *method, ConstantPool *orig_cp JVM_TRAPS);

  bool shall_create_new_method(Method* pMethod JVM_TRAPS);

  ReturnOop create_method_replacement(Method *method JVM_TRAPS);
  void record_method_replacement(Method *old_method, Method *new_method);
  void replace_method_references(JVM_SINGLE_ARG_TRAPS);
  void update_methods_in_ivtables(JavaClass *klass);
  void update_methods_in_constants(InstanceClass *ic);
  void update_methods_in_original_info(int klass_index JVM_TRAPS);

  void init_branch_targets(Method *method JVM_TRAPS) {
    _branch_targets = method->compute_entry_counts( JVM_SINGLE_ARG_NO_CHECK );
  }

  bool is_branch_target(const int old_bci) const {
    return _branch_targets.ubyte_at(old_bci) > 1;
  }

  bool is_redundant_push_pop(Method *method, int bci);  
  bool shall_create_new_method(Method *method, int* p_new_size JVM_TRAPS);
  ReturnOop copy_method(Method *src, int new_size JVM_TRAPS);
  void correct_cp_indices(Method *method JVM_TRAPS);
  void stream_bytecodes(Method *old_method, Method *new_method JVM_TRAPS);
  void stream_raw_bytes(Method *old_method, Method *new_method, int old_bci,
                        int new_bci, int len);
  int  stream_ldc(Method *old_method, Method *new_method, int old_bci,
                  int new_bci JVM_TRAPS);
  void stream_java_cp_index(Method *old_method, Method *new_method,
                            int old_bci, int new_bci JVM_TRAPS);
  void stream_invokeinterface(Method *old_method, Method *new_method,
                              int old_bci, int new_bci JVM_TRAPS);
  void stream_multianewarray(Method *old_method, Method *new_method,
                             int old_bci, int new_bci JVM_TRAPS);
  void stream_branch2(Method *old_method, Method *new_method, int old_bci,
                      int new_bci);
  void stream_branch4(Method *old_method, Method *new_method, int old_bci,
                      int new_bci);
  int  stream_tableswitch(Method *old_method, Method *new_method, int old_bci,
                          int new_bci);
  int  stream_lookupswitch(Method *old_method, Method *new_method, int old_bci,
                           int new_bci);
  void update_branch4(Method *old_method, Method *new_method,
                      int old_bci, int new_bci,
                      int old_loc, int new_loc);

  void update_branch_switch(Method *old_method, Method *new_method,
                            int old_bci, int new_bci,
                            int old_loc, int new_loc);

  inline void rewrite_index(ConstantPool *orig_cp, TypeArray *t, int offset JVM_TRAPS);
  void account_for_new_entry(ConstantTag new_entry_tag);
  void init_method_map(JVM_SINGLE_ARG_TRAPS);

  ROMHashtableManager *_hashtab_mgr;

public:
  ConstantPoolRewriter(ROMOptimizer *opt, Stream *log_stream,
    ROMHashtableManager *hashtab_mgr)
  {
    _optimizer = opt;
    _log_stream = log_stream;
    _hashtab_mgr = hashtab_mgr;
#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR 
   _bytecode_optimizer.set_owner(this);
#endif
  }

  void rewrite(JVM_SINGLE_ARG_TRAPS);
  void print_statistics(Stream *stream);
  ReturnOop find_method_replacement(Method *old_method);

private:
  static ConstantPoolRewriter* _current_rewriter;
  static void replace_oop_if_method(OopDesc** addr);

#if ENABLE_ROM_JAVA_DEBUGGER
  void rewrite_line_number_tables(Method *old_method, 
                                  Method *new_method,
                                  bool compress_if_possible JVM_TRAPS);
#endif

friend class ErrorMethodCounter;
friend class ErrorMethodRewriter;
friend class BytecodeOptimizer;

};

#endif // ENABLE_ROM_GENERATOR
