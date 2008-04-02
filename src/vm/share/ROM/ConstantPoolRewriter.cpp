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
# include "incls/_ConstantPoolRewriter.cpp.incl"

// This class rewrites ConstantPools of the romized classes to reduce
// the footprint of the ROM image.

#if ENABLE_ROM_GENERATOR

#define NUM_BUCKETS 1024

void ConstantPoolRewriter::rewrite(JVM_SINGLE_ARG_TRAPS) {
#if USE_ROM_LOGGING
  _log_stream->print("Merging constant pools...");
#endif
  initialize(JVM_SINGLE_ARG_CHECK);

  // If possible, put all the rom hashtables into constant pool to save space
  init_embedded_hashtables(JVM_SINGLE_ARG_CHECK);

  // Rewrite each InstanceClass -- TypeArrayClasses don't have associated CP,
  // so no need to rewrite them.
  InstanceClass klass;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();    
#if ENABLE_LIB_IMAGES && ENABLE_MONET
    if (ROM::in_any_loaded_bundle(klass.obj()) || (klass.obj() <= ROM::romized_heap_marker())) {
      continue;
    }
#endif
    if (PostponeErrorsUntilRuntime) {
      // Rewrite CP only if a class is successfully verified and we haven't 
      // found any failures in previous optimizations.
      if (!klass.is_verified() || !klass.is_optimizable()) {
        continue;
      }
    }
    GUARANTEE(klass.is_verified() && klass.is_optimizable(), "Sanity");
    rewrite_class(&klass JVM_CHECK);
  }
  flush_merged_pool(JVM_SINGLE_ARG_CHECK);

  // Replace all references to old methods to new methods
  replace_method_references(JVM_SINGLE_ARG_CHECK);

#if USE_ROM_LOGGING
  _log_stream->print_cr("Done!");
#endif
}

void ConstantPoolRewriter::initialize(JVM_SINGLE_ARG_TRAPS) {    
  _merged_pool.set_null();
  _method_size_delta = 0;

  count_original_pools();
  _predicted_merged_pool_size = _original_entries_count + 
                      _hashtab_mgr->symbol_count() + _hashtab_mgr->string_count() + 1;
  init_method_map(JVM_SINGLE_ARG_CHECK);  
  if (_predicted_merged_pool_size > 0xfff0) {    
    _predicted_merged_pool_size = 0xfff0;
  }
  allocate_merged_pool(JVM_SINGLE_ARG_CHECK);
  
  int class_count = ROMWriter::number_of_romized_java_classes();
  _updated_constant_pools = Universe::new_obj_array(class_count JVM_CHECK);

  _new_bytecode_address = Universe::new_short_array(_max_method_length + 1 JVM_CHECK);

  _count_of_utf8 = 0;
  _count_of_string = 0;
  _count_of_static_method = 0;
  _count_of_long = 0;
  _count_of_double = 0;
  _count_of_field_ref = 0;
  _count_of_interface_method_ref = 0;
  _count_of_virtual_method_ref = 0;
  _count_of_invalid = 0;
  _count_of_integer = 0;
  _count_of_class = 0;
  _count_of_float = 0;
}

// Counting for statistics purposes
void ConstantPoolRewriter::count_original_pools() {
  _original_pools_count = 0;
  _original_entries_count = 0;
  UsingFastOops level1;
  InstanceClass::Fast klass;
  ConstantPool::Fast cp;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    cp = klass().constants();
    _original_pools_count ++;
    _original_entries_count += cp().length();
  }  
}

void ConstantPoolRewriter::allocate_merged_pool(JVM_SINGLE_ARG_TRAPS) {
  if (!_merged_pool.is_null()) {
    flush_merged_pool(JVM_SINGLE_ARG_CHECK);
  }
  // Allocate a new merged pool. Always set the first entry to Invalid. We may
  // have assumptions in the code that assumes CP #0 is JVM_CONSTANT_Invalid.
  _merged_pool = Universe::new_constant_pool(_predicted_merged_pool_size JVM_CHECK);
  _merged_pool.tag_at_put(0, JVM_CONSTANT_Invalid);
  _merged_entries_count = 1;

  _entry_lookup_table = Universe::new_obj_array(NUM_BUCKETS JVM_CHECK);
}

void ConstantPoolRewriter::flush_merged_pool(JVM_SINGLE_ARG_TRAPS) {
  free_unused_space(JVM_SINGLE_ARG_CHECK);
}

// Free unused space at the end of merged_pool (and the associated tag array)
void ConstantPoolRewriter::free_unused_space(JVM_SINGLE_ARG_TRAPS) {
  int unused_entries = _predicted_merged_pool_size - _merged_entries_count;
  if (unused_entries > 0) {
    GCDisabler no_gc_must_happen_in_this_block;
    TypeArray::Raw tags = _merged_pool.tags();

    size_t old_pool_size = _merged_pool.object_size();
    size_t old_tags_size = tags().object_size();

    
    ROMTools::shrink_object(&_merged_pool, old_pool_size, 
                            unused_entries * sizeof(jobject));
    //we must allign tags by 4 bytes!
    int unused_tags_entries = unused_entries & (~3);
    ROMTools:: shrink_object(&tags, old_tags_size, 
                            unused_tags_entries * sizeof(jbyte));

    // If a GC happens here things will go really bad
    _merged_pool.set_length(_merged_entries_count);
    tags().set_length(_predicted_merged_pool_size - unused_tags_entries);
  }

  if (VerifyGC) {
    ObjectHeap::verify(); // It pays to be paranoid
    ObjectHeap::full_collect(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
}

void ConstantPoolRewriter::init_embedded_hashtables(JVM_SINGLE_ARG_TRAPS) {
  if (EmbeddedROMHashTables) {
    int table_count = _hashtab_mgr->symbol_count() +
                      _hashtab_mgr->string_count();
    if (table_count == 0 || 
        table_count + _merged_entries_count > _predicted_merged_pool_size) {

#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR
      tty->print_cr("EmbeddedROMHashTable disabled");
#endif
      EmbeddedROMHashTables = false;
    } else {
      UsingFastOops level2;
      ObjArray::Fast string_table = _hashtab_mgr->string_table();
      ObjArray::Fast symbol_table = _hashtab_mgr->symbol_table();

      // Stream the Symbols first. This makes it possible to skip the
      // JVM_CONSTANT_Utf8 tags in the tags array.
      int embedded_symbols_offset = _merged_entries_count;
      stream_hashtable(&symbol_table, true  JVM_CHECK);

      int embedded_strings_offset = _merged_entries_count;
      stream_hashtable(&string_table, false JVM_CHECK);

      _hashtab_mgr->set_embedded_hashtables(&_merged_pool,
                                            embedded_strings_offset,
                                            embedded_symbols_offset);
    }
  }
}

// Copy the ROM hashtable into the current merged constant pool.
void ConstantPoolRewriter::stream_hashtable(ObjArray *rom_table,
                                            bool is_symbol JVM_TRAPS) {
  UsingFastOops level1;
  ConstantPool::Fast fake_cp = Universe::new_constant_pool(2 JVM_CHECK);
  ObjArray::Fast bucket;
  TypeArray::Fast idx_array;
  Symbol::Fast symbol;
  String::Fast string;
  int num_buckets = rom_table->length();
  for (int b=0; b<num_buckets; b++) {
    bucket = rom_table->obj_at(b);
    int bucket_size = bucket().length();
    idx_array = Universe::new_short_array(1 JVM_CHECK);

    for (int i=0; i<bucket_size; i++) {
      // We enter this string/symbol into the merged pool by creating
      // a faked constant pool and attempting to rewrite index 1.
      idx_array().short_at_put(0, 1);

#ifdef AZZERT
      int old_count = _merged_entries_count;
#endif
      if (is_symbol) {
        symbol = bucket().obj_at(i);
        fake_cp().symbol_at_put(1, &symbol);
      } else {
        string = bucket().obj_at(i);
        fake_cp().string_at_put(1, &string);
      }

      rewrite_index(&fake_cp, &idx_array, 0 JVM_CHECK);

      GUARANTEE(old_count + 1 == _merged_entries_count, "entry must be new");
    }
  }
}

class ErrorMethodSearcher : public OopROMVisitor {
 public:
  virtual void do_int(VisitorField* /*field*/, int /*offset*/,
                      bool /*visit_value*/)        {}
  virtual void do_comment(const char* /*comment*/) {}

  virtual void do_oop(VisitorField* /*field*/, int offset,
                      bool /*visit_value*/) {
    UsingFastOops level1;
    Oop::Fast value = _obj->obj_field(offset);
    Method::Fast method;
    if (value.not_null() && value().is_method()) {
      method = value.obj();
      if (method().is_native()) {
        address native_code = method().get_native_code();
        if (native_code == (address) Java_incompatible_method_execution &&
            !method.is_null()) {
          process_error_method(&method);
        }
      }
    }
  } 
private:
  virtual void process_error_method(Method* /*method*/) JVM_PURE_VIRTUAL;
};

class ErrorMethodCounter : public ErrorMethodSearcher {
 public:
  ErrorMethodCounter(TypeArray* sizes) {
    _sizes = sizes;
  }
private:
  virtual void process_error_method(Method* method) {
    juint code = 
      ConstantPoolRewriter::hashcode_for_method(method) % _sizes->length();
    _sizes->int_at_put(code, _sizes->int_at(code) + 1);
  }
  TypeArray* _sizes;
};

class ErrorMethodRewriter : public ErrorMethodSearcher {
 public:
  ErrorMethodRewriter(ConstantPoolRewriter *cp_rewriter) {
    _cp_rewriter = cp_rewriter;
  }
private:
  virtual void process_error_method(Method* method) {
    SETUP_ERROR_CHECKER_ARG;
    _cp_rewriter->rewrite_method(method JVM_NO_CHECK);
  }
  ConstantPoolRewriter *_cp_rewriter;
};

void ConstantPoolRewriter::rewrite_class(InstanceClass *klass JVM_TRAPS) {
  UsingFastOops level1;
  ConstantPool::Fast orig_cp = klass->constants();

  if (orig_cp().length() + _merged_entries_count > _predicted_merged_pool_size) {
    if (orig_cp().length() + 1 > _predicted_merged_pool_size) {
      // This class has a very big constant pool. We can't merge it
      return;
    } else {
      // The remaining space in the existing merged_pool may not be enough.
      // let's create a new pool
      allocate_merged_pool(JVM_SINGLE_ARG_CHECK);
    }
  }

  rewrite_class_object(klass JVM_CHECK);

  ObjArray::Fast methods = klass->methods();
  Method::Fast method;
  for (int i=0; i<methods().length(); i++) {
    method = methods().obj_at(i);
    if (method.is_null()) {
      // A nulled-out <clinit> method
      continue;
    } else {      
      rewrite_method(&method JVM_CHECK);
    }
  }

  // The above loop should catch all the methods. The only exceptions
  // are the lazy_error methods, which appear only in the itable but not in
  // klass->methods(). See InstanceClass::itable_copy_down().
  ErrorMethodRewriter error_method_rewriter(this);
  ClassInfo::Fast info = klass->class_info();
  error_method_rewriter.set_obj(&info);
  info().iterate_tables(&error_method_rewriter);
  JVM_DELAYED_CHECK; // ErrorMethodRewriter::do_oop() could throw!
}

void ConstantPoolRewriter::rewrite_class_object(InstanceClass *klass JVM_TRAPS) {
  UsingFastOops level1;
  TypeArray::Fast fields = klass->fields();
  ConstantPool::Fast orig_cp = klass->constants();
  int i;

  for (i = 0; i < fields().length(); i += 5) {
    //5-tuples of shorts [access, name index, sig index, initval index, offset]
    rewrite_index(&orig_cp, &fields, i + Field::NAME_OFFSET JVM_CHECK);
    rewrite_index(&orig_cp, &fields, i + Field::SIGNATURE_OFFSET JVM_CHECK);
#if ENABLE_ISOLATES
    // If the INITIAL_OFFSET location is 0 then that's a flag that means
    // that this field doesn't need initialization.  Since we re-init each
    // class for each new task we need to preserve the zeroness of that
    // field.  Otherwise field.has_initial_value() will not work right.
    // Should we ever re-write if the index is 0 ??
    if (*(jushort *)((jushort *)fields().base_address() + i + Field::INITVAL_OFFSET) != 0) {
      rewrite_index(&orig_cp, &fields, i + Field::INITVAL_OFFSET JVM_CHECK);
    }
#else
    rewrite_index(&orig_cp, &fields, i + Field::INITVAL_OFFSET JVM_CHECK);
#endif
  }

#if ENABLE_REFLECTION
  TypeArray::Fast inner_classes = klass->inner_classes();
  int length = inner_classes().length();
  for (i = 0; i < length; i++) {
    rewrite_index(&orig_cp, &inner_classes, i JVM_CHECK);
  }
#endif

  ClassInfo::Fast info = klass->class_info();
  info().set_constants(&_merged_pool);
}

#if ENABLE_ROM_JAVA_DEBUGGER
void ConstantPoolRewriter::rewrite_line_number_tables(Method *old_method, 
                                                      Method *new_method,
                                                      bool compress_if_possible JVM_TRAPS) {
  
  GUARANTEE(old_method != NULL && new_method != NULL, "Sanity");

  // compress and fix line number table based on new offsets possibly
  // created when we rewrote the method bytecodes
  if (MakeROMDebuggable && !new_method->is_null()) {
    UsingFastOops fast_oops;
    LineVarTable::Fast line_var_table = old_method->line_var_table();
    LineNumberTable::Fast line_num_table;
    LocalVariableTable::Fast local_var_table;
    bool can_compress = compress_if_possible;
    bool need_table = !compress_if_possible;
    if (!line_var_table.is_null()) {
      line_num_table = line_var_table().line_number_table();
    }
    if (!line_var_table.is_null() && !line_num_table.is_null()) {
      int i;
      int num_entries = line_num_table().count();
      jushort old_pc, new_pc;
      for (i = 0; i < num_entries; i++) {
        old_pc = line_num_table().pc(i);
        new_pc = _new_bytecode_address.ushort_at(old_pc);
        if (old_pc != new_pc) {
          need_table = true;
          line_num_table().set_pc(i, new_pc);
          if (new_pc > 255 || line_num_table().line_number(i) > 255) {
            can_compress = false;
          }
        }
      }
      if (!need_table) {
        line_var_table().clear_line_number_table();
      } else if (num_entries > 0 && can_compress) {
        // All entries in the line number table <= 255 so we can make it
        // a byte array to save some space
        LineNumberTable::Raw new_line_num_table =
          Universe::new_byte_array(num_entries * 2 JVM_CHECK);
        for (i = 0; i < num_entries; i++) {
          new_line_num_table().set_pc(i,
                                      (jubyte)line_num_table().pc(i));
          new_line_num_table().set_line_number(i,
                               (jubyte)line_num_table().line_number(i));
        }
        line_var_table().set_line_number_table(new_line_num_table().obj());
      }
    }
    if (!line_var_table.is_null()) {
      local_var_table = line_var_table().local_variable_table();
    }
    can_compress = compress_if_possible;
    need_table = !compress_if_possible;
    if (!line_var_table.is_null() && !local_var_table.is_null()) {
      int i;
      int num_entries = local_var_table().count();
      jushort new_code_size = new_method->code_size();
      jushort old_code_size = old_method->code_size();
      jushort start_pc, code_length, new_code_length, new_pc;
      for (i = 0; i < num_entries; i++) {
        start_pc = local_var_table().start_pc(i);
        code_length = local_var_table().code_length(i);
        new_pc = _new_bytecode_address.ushort_at(start_pc);
        if (start_pc != new_pc) {
          local_var_table().set_start_pc(i, new_pc);
          need_table = true;
        }
        if ((start_pc + code_length) == old_code_size) {
          if (old_code_size != new_code_size) {
            GUARANTEE(new_pc < new_code_size, "Sanity");
            local_var_table().set_code_length(i, 
                (jushort)(new_code_size - new_pc));
            need_table = true;
          }
        } else {
          new_code_length =
            _new_bytecode_address.ushort_at(start_pc + code_length) - new_pc;
          if (new_code_length != code_length) {
            local_var_table().set_code_length(i, new_code_length);
            need_table = true;
          }
        }
        if (start_pc > 255 || code_length > 255 ||
            local_var_table().slot(i) > 255) {
          can_compress = false;
        }
      }
      if (!need_table) {
        // don't need to keep local variable table since it didn't change
        // from the original that the debug agent is using
        line_var_table().clear_local_variable_table();
      } else if (num_entries > 0 && can_compress) {
        // All entries in the local var table <= 255 so we can make it
        // a byte array to save some space
        LocalVariableTable::Raw new_local_var_table =
          Universe::new_byte_array(num_entries * 3 JVM_CHECK);
        for (i = 0; i < num_entries; i++) {
          new_local_var_table().set_start_pc(i,
             (jubyte)local_var_table().start_pc(i));
          new_local_var_table().set_code_length(i,
             (jubyte)local_var_table().code_length(i));
          new_local_var_table().set_slot(i, (jubyte)local_var_table().slot(i));
        }
        line_var_table().set_local_variable_table(new_local_var_table().obj());
      }
    }
  } else {
    Oop::Raw null_oop;
    old_method->set_line_var_table(&null_oop);
  }
}
#endif

void ConstantPoolRewriter::rewrite_method(Method *method JVM_TRAPS) {
  Method new_method;
  if (method->is_abstract() || method->is_native() || 
      method->code_size() == 0) {
    rewrite_method_header(method JVM_CHECK);
    // The method is not really replaced, but we enter it into the hash
    // table anyway so that replace_method_references() has to do less work.
    record_method_replacement(method, method);
  } else {
    new_method = create_method_replacement(method JVM_CHECK);
    record_method_replacement(method, &new_method);
  }

#if ENABLE_ROM_JAVA_DEBUGGER
  rewrite_line_number_tables(method, &new_method, true JVM_CHECK);
#endif
}

void ConstantPoolRewriter::rewrite_exception_table(Method *method, 
                                                   ConstantPool *orig_cp 
                                                   JVM_TRAPS) {
  UsingFastOops level1;

  TypeArray::Fast exception_table = method->exception_table();
  TypeArray::Fast new_exception_table;
  if (!exception_table.is_null() && exception_table().length() > 0) {
    // 4-tuples of ints [start_pc, end_pc, handler_pc, catch_type index]
    GUARANTEE(!method->is_abstract() && !method->is_native(), "sanity");
    int len = exception_table().length();
    new_exception_table = Universe::new_short_array(len JVM_CHECK);

    for (int i=0; i<len; i+=4) {
      jushort start_pc   = exception_table().ushort_at(i + 0);
      jushort end_pc     = exception_table().ushort_at(i + 1);
      jushort handler_pc = exception_table().ushort_at(i + 2);
      jushort type_index = exception_table().ushort_at(i + 3);

      start_pc   = _new_bytecode_address.ushort_at(start_pc);
      end_pc     = _new_bytecode_address.ushort_at(end_pc);
      handler_pc = _new_bytecode_address.ushort_at(handler_pc);

      if (type_index != 0 && orig_cp != NULL) { // 0 means "any type"
        type_index = get_merged_pool_entry(orig_cp, type_index JVM_CHECK);
        GUARANTEE(_merged_pool.tag_at(type_index).is_resolved_klass(),
                  "sanity");
      }

      new_exception_table().ushort_at_put(i + 0, start_pc);
      new_exception_table().ushort_at_put(i + 1, end_pc);
      new_exception_table().ushort_at_put(i + 2, handler_pc);
      new_exception_table().ushort_at_put(i + 3, type_index);
    }
    method->set_exception_table(&new_exception_table);
  }
}

void ConstantPoolRewriter::rewrite_method_header(Method *method JVM_TRAPS) {
  UsingFastOops level1;
  ConstantPool::Fast orig_cp = method->constants();

#if USE_BINARY_IMAGE_GENERATOR
  if (orig_cp.obj() == (OopDesc*)_rom_constant_pool) {
    // This method is a lazy_error method. It's already using the romized
    // constant pool, so there's no need to rewrite it. Also, some tags
    // in the the romized constant pool are skipped, and would cause 
    // get_merged_pool_entry() below to fail.
    return;
  }
#endif

  jushort name_index = method->name_index();
  name_index = get_merged_pool_entry(&orig_cp, name_index JVM_CHECK);
  method->set_name_index(name_index);

  jushort signature_index = method->signature_index();
  signature_index = get_merged_pool_entry(&orig_cp, signature_index JVM_CHECK);
  method->set_signature_index(signature_index);

  method->set_constants(&_merged_pool);

  rewrite_exception_table(method, &orig_cp JVM_CHECK);

#if ENABLE_REFLECTION
  TypeArray::Fast thrown_exceptions = method->thrown_exceptions();
  if (thrown_exceptions.not_null()) {
    int length = thrown_exceptions().length();
    for (int i = 0; i < length; i++) {
      rewrite_index(&orig_cp, &thrown_exceptions, i JVM_CHECK);
    }
  }
#endif
}

ReturnOop
ConstantPoolRewriter::create_method_replacement(Method *method JVM_TRAPS) {
  // (0) Replace some bytecodes with equivalent but shorter bytecodes
  // Don't want to optimize if  you intend to debug ROMized methods.
  // Line number/offset tables will be messed up
  UsingFastOops fast;
  Method::Fast optimized_method = method->obj();  
#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR && !ENABLE_CPU_VARIANT
  if (OptimizeBytecodes) {        
    optimized_method = _bytecode_optimizer.optimize_bytecodes(method JVM_CHECK_0);
    // The first step of bytecode optimization is made, bytecode indices changed.
    // Exception table and line number table should be updated.
    // shall_create_new_method rewrites _new_bytecode_address so we 
    // should rewrite the tables right now.    
    rewrite_exception_table(method, NULL JVM_CHECK_0);
#if ENABLE_ROM_JAVA_DEBUGGER
    rewrite_line_number_tables(method, &optimized_method, false /*don't compress*/ 
                             JVM_CHECK_0);
#endif

    int new_method_length = optimized_method().code_size() + 1;
    if (new_method_length > _new_bytecode_address.length()) {
      _new_bytecode_address = Universe::new_short_array(new_method_length JVM_CHECK_0);
    }
  }
#endif
  

  // (1) determine the size of the new method
  int new_size;
  bool create_new_method = shall_create_new_method(&optimized_method, &new_size JVM_CHECK_0);
  _method_size_delta += new_size - method->code_size();

  Method::Fast new_method;  
  if (create_new_method) {
    // (2) Allocate the method and set up its header (to use the old CP for the
    //     time being)
    new_method = copy_method(&optimized_method, new_size JVM_CHECK_0);

    // (3) Stream bytecodes, replacing CP indices to use the new CP
    stream_bytecodes(&optimized_method, &new_method JVM_CHECK_0);
  } else { //just correct cp indexes!
    new_method = optimized_method.obj();
    correct_cp_indices(&new_method JVM_CHECK_0);
  }

  // (4) Rewrite the header to use the new CP
  rewrite_method_header(&new_method JVM_CHECK_0);

  if (!(method->is_native() || method->is_abstract())) {
    GUARANTEE(!(new_method().is_native() || new_method().is_abstract()), "sanity");
  }

  return new_method.obj();  
}

ReturnOop ConstantPoolRewriter::copy_method(Method *old_method, int new_size
                                            JVM_TRAPS) {
  UsingFastOops level1;
  const AccessFlags access_flags = old_method->access_flags();
  Method::Fast new_method = Universe::new_method(new_size, access_flags JVM_CHECK_0);
  TypeArray::Fast exception_table = old_method->exception_table();
  ConstantPool::Fast orig_cp = old_method->constants();
  StackmapList::Fast orig_maps = old_method->stackmaps();
#if ENABLE_REFLECTION
  TypeArray::Fast thrown_exceptions = old_method->thrown_exceptions();
  if (thrown_exceptions.not_null() && thrown_exceptions().length() > 0) {
    new_method().set_thrown_exceptions(&thrown_exceptions);
  }
#endif

  if (!exception_table.is_null() && exception_table().length() > 0) {
    new_method().set_exception_table(&exception_table);
  }

  new_method().set_constants(&orig_cp);
  new_method().set_stackmaps(&orig_maps);
  new_method().set_holder_id(old_method->holder_id());
  new_method().set_max_locals(old_method->max_locals());
  new_method().set_method_attributes(old_method->method_attributes());
  new_method().set_name_index(old_method->name_index());
  new_method().set_signature_index(old_method->signature_index());
  new_method().set_max_execution_stack_count(
      old_method->max_execution_stack_count());
  new_method().set_access_flags(access_flags);
#if ENABLE_ROM_JAVA_DEBUGGER
  ObjArray::Fast orig_line_var_table = old_method->line_var_table();
  new_method().set_line_var_table(&orig_line_var_table);
#endif

#if  ENABLE_JVMPI_PROFILE 
  // Copy the method ID
  new_method().set_method_id(old_method->method_id());
#endif  

  {
    GCDisabler disable_gc;

    MethodVariablePart *old_vpart = old_method->variable_part();
    MethodVariablePart *new_vpart = new_method().variable_part();

    new_vpart->set_execution_entry(old_vpart->execution_entry());
  }

  return new_method;
}

void ConstantPoolRewriter::record_method_replacement(Method *old_method, Method *new_method) {  
  UsingFastOops level1;
  unsigned int idx = hashcode_for_method(old_method) % method_map_array.length();
  ObjArray::Fast item = method_map_array.obj_at(idx);
  GUARANTEE(item.not_null(), "init_method_map works badly!");
  int i = 0;  
  Method::Fast current_old_method;
  while (i < item().length()) {
    current_old_method = item().obj_at(i);
    if (current_old_method.is_null()) {
      item().obj_at_put(i, old_method);
      item().obj_at_put(i + 1, new_method);
      return;
    } 
    GUARANTEE(!current_old_method.equals(old_method), "you try inserting duplicated method!");
    i += 2;
  }
  SHOULD_NOT_REACH_HERE();//problems with init_method_map() !
}

ReturnOop ConstantPoolRewriter::find_method_replacement(Method *p_old_method) {
  UsingFastOops level1;
  GUARANTEE(p_old_method->not_null(), "sanity");
  unsigned int idx = hashcode_for_method(p_old_method) % method_map_array.length();
  ObjArray::Fast item = method_map_array.obj_at(idx);
  if (item.is_null()) {//this could happen for method which wasn't record!
    return *p_old_method;
  }
  int i = 0;
  Method::Fast current_old_method;
  while (i < item().length()) {
    current_old_method = item().obj_at(i);
    if (current_old_method.is_null()) {
      break;
    } 
    if (current_old_method.equals(p_old_method)) {
      return item().obj_at(i + 1);
    }
    i += 2;
  }

  //in case we couldn't find method.
  return *p_old_method;
}

void ConstantPoolRewriter::replace_oop_if_method(OopDesc** addr) {
  OopDesc*  value = *addr;
  if (value->is_method()) {
    Method::Raw old_method = value;
    Method::Raw new_method =
        _current_rewriter->find_method_replacement(&old_method);
    oop_write_barrier(addr, new_method.obj());
  }
}


ConstantPoolRewriter* ConstantPoolRewriter::_current_rewriter;


void ConstantPoolRewriter::replace_method_references(JVM_SINGLE_ARG_TRAPS) {
  // Don't use SystemClassStream -- we have to iterate over ArrayClasses as
  // well.
  int num = Universe::number_of_java_classes();
  int start_id = UseROM ? ROM::number_of_system_classes() : 0;
  for (int klass_index=start_id; klass_index<num; klass_index++) {
    JavaClass::Raw klass = Universe::class_from_id(klass_index);

    // [1] Replace references in itables/vtables
    update_methods_in_ivtables(&klass);

    // [2] Replace references in method[] array
    if (klass.is_instance_class()) {
      InstanceClass::Raw ic = klass.obj();
      ObjArray::Raw methods = ic().methods();

      for (int i=0; i<methods().length(); i++) {
        Method::Raw old_method = methods().obj_at(i);
        if (old_method.not_null()) {
          Method::Raw new_method = find_method_replacement(&old_method);
          methods().obj_at_put(i, &new_method);
        }
      }

      // [3] Replace reference in constant pool
      update_methods_in_constants(&ic);

      // [4] Replace references in original info (that maps .unknown. methods
      //     to their original names.)
      //
      //     Note: renaming is done only for InstanceClass
#if !defined(PRODUCT) && !USE_BINARY_IMAGE_GENERATOR
      update_methods_in_original_info(klass_index JVM_CHECK);
#else
      JVM_IGNORE_TRAPS;
#endif
    }
  }
  
#ifdef AZZERT
  // No precompilation should have happened. The compiled code has
  // bci references to the old method, and will NOT match the new method.
  for (int j = 0; j < method_map_array.length(); j++) {
    ObjArray::Raw item = method_map_array.obj_at(j);
    if (item.is_null()) {
      continue;
    }
    int idx = 0; 
    while (idx < item().length()) {
      Method::Raw old_method = item().obj_at(idx);
      if (old_method().not_null()) {
        GUARANTEE(!old_method().has_compiled_code(), 
                "old compiled method would have bad bci");
      }
      idx += 2;
    }
  }
#endif

#if USE_AOT_COMPILATION
  // Replace methods in the ROMOptimizer's list of methods to compile
  ROMVector *vector = _optimizer->precompile_method_list();
  int precompile_size = vector->size();
  for (int i=0; i < precompile_size; i++) {
    Method::Raw old_method = vector->element_at(i);
    Method::Raw new_method = find_method_replacement(&old_method);
    vector->element_at_put(i, &new_method);
  }
#endif
}

class MethodReplacer : public OopROMVisitor {
 public:
  MethodReplacer(ConstantPoolRewriter *cp_rewriter) {
    _cp_rewriter = cp_rewriter;
  }
  virtual void do_comment(const char* /*comment*/) {}
  virtual void do_int(VisitorField* /*field*/, int /*offset*/,
                      bool /*visit_value*/)    {}

  virtual void do_oop(VisitorField* /*field*/, int offset,
                      bool /*visit_value*/) {
    Oop::Raw value = _obj->obj_field(offset);
    if (value.not_null() && value.is_method()) {
      Method::Raw oldm = value.obj();
      if (oldm.not_null()) {
        Method::Raw newm = _cp_rewriter->find_method_replacement(&oldm);
        _obj->obj_field_put(offset, &newm);
      }
    }
  }

private:
  ConstantPoolRewriter *_cp_rewriter;
};

void ConstantPoolRewriter::update_methods_in_ivtables(JavaClass *klass) {
  MethodReplacer replacer(this);
  ClassInfo info = klass->class_info();
  replacer.set_obj(&info);
  info.iterate_tables(&replacer);
}

void ConstantPoolRewriter::update_methods_in_constants(InstanceClass *ic) {
  ConstantPool::Raw pool = ic->constants();
  int i;

  for (i=0; i<_updated_constant_pools.length(); i++) {
    Oop::Raw obj = _updated_constant_pools.obj_at(i);
    if (obj.is_null()) {
      // This pool has not yet been updated.
      _updated_constant_pools.obj_at_put(i, &pool);
      break;
    } else if (pool.equals(&obj)) {
      // This pool has not yet been updated. No need to do anything.
      return;
    }
  }

  for (i=0; i<pool().length(); i++) {
    if (!pool().tag_at(i).is_oop()) {
      continue;
    }
    Oop::Raw oop = pool().oop_at(i);
    if (ROMWriter::write_by_reference(oop) || !oop.is_method()) {
      continue;
    }

    Method::Raw old_method = oop.obj();
    if (old_method.not_null()) {
      Method::Raw new_method = find_method_replacement(&old_method);
      pool().resolved_static_method_at_put(i, &new_method);
    }
  }
}

// Update the methods stored in the ROM::original_method_info_list, which maps
// renamed methods (with new name ".unknown.") to their original names.
void ConstantPoolRewriter::update_methods_in_original_info(int class_id
                                                           JVM_TRAPS) {
  UsingFastOops level1;
  ObjArray::Fast info_list = _optimizer->romizer_original_method_info()->obj();
  GUARANTEE(!info_list.is_null(), "ROMOptimizer didn't save original info?");
  ObjArray::Fast info = info_list().obj_at(class_id);

  Method::Fast old_method;
  Method::Fast new_method;
  Symbol::Fast name;
  ObjArray::Fast head;
  ObjArray::Fast new_info;
  while (!info.is_null()) {
    old_method = info().obj_at(ROM::INFO_OFFSET_METHOD);
    name = info().obj_at(ROM::INFO_OFFSET_NAME);

    if (old_method.not_null()) {
      new_method = find_method_replacement(&old_method);
      if (!old_method.equals(&new_method)) {
        // Note, we need to keep a record of the old method, because
        // we may still want to do old_method.p() somewhere down the
        // line (especially inside an interactive debugger)
        head = info_list().obj_at(class_id);
        new_info = Universe::new_obj_array(3 JVM_CHECK);
        new_info().obj_at_put(ROM::INFO_OFFSET_METHOD, &new_method);
        new_info().obj_at_put(ROM::INFO_OFFSET_NAME,   &name);
        new_info().obj_at_put(ROM::INFO_OFFSET_NEXT,   &head);

        info_list().obj_at_put(class_id, &new_info);
      }
    }
    info = info().obj_at(ROM::INFO_OFFSET_NEXT);
  }
}

// Compute a hash code of a CP entry, which we use to store the CP
// entry in _entry_lookup_table.
juint ConstantPoolRewriter::compute_hashcode(ConstantPool *orig_cp, int i) {
  ConstantTag tag = orig_cp->tag_at(i);

  switch (tag.value()) {
  case JVM_CONSTANT_Utf8:
    {
      Symbol::Raw symbol = orig_cp->symbol_at(i);
      return hashcode_for_symbol(&symbol);
    }

  case JVM_CONSTANT_String:
    {
      String::Raw string = orig_cp->oop_at(i);
      return hashcode_for_string(&string);
    }

  case JVM_CONSTANT_ResolvedStaticMethod:
    {
      Method::Raw method = orig_cp->resolved_static_method_at(i);
      Symbol::Raw name = method().name();
      return hashcode_for_symbol(&name);
    }

  case JVM_CONSTANT_ResolvedBooleanFieldref:
  case JVM_CONSTANT_ResolvedCharFieldref:
  case JVM_CONSTANT_ResolvedFloatFieldref:
  case JVM_CONSTANT_ResolvedDoubleFieldref:
  case JVM_CONSTANT_ResolvedByteFieldref:
  case JVM_CONSTANT_ResolvedShortFieldref:
  case JVM_CONSTANT_ResolvedIntFieldref:
  case JVM_CONSTANT_ResolvedLongFieldref:
  case JVM_CONSTANT_ResolvedObjectFieldref:
  case JVM_CONSTANT_ResolvedArrayFieldref:  
  case JVM_CONSTANT_ResolvedStaticBooleanFieldref:
  case JVM_CONSTANT_ResolvedStaticCharFieldref:
  case JVM_CONSTANT_ResolvedStaticFloatFieldref:
  case JVM_CONSTANT_ResolvedStaticDoubleFieldref:
  case JVM_CONSTANT_ResolvedStaticByteFieldref:
  case JVM_CONSTANT_ResolvedStaticShortFieldref:
  case JVM_CONSTANT_ResolvedStaticIntFieldref:
  case JVM_CONSTANT_ResolvedStaticLongFieldref:
  case JVM_CONSTANT_ResolvedStaticObjectFieldref:
  case JVM_CONSTANT_ResolvedStaticArrayFieldref:
    return hashcode_for_field_ref(orig_cp, i);

  case JVM_CONSTANT_ResolvedBooleanInterfaceMethod:
  case JVM_CONSTANT_ResolvedCharInterfaceMethod:
  case JVM_CONSTANT_ResolvedFloatInterfaceMethod:
  case JVM_CONSTANT_ResolvedDoubleInterfaceMethod:
  case JVM_CONSTANT_ResolvedByteInterfaceMethod:
  case JVM_CONSTANT_ResolvedShortInterfaceMethod:
  case JVM_CONSTANT_ResolvedIntInterfaceMethod:
  case JVM_CONSTANT_ResolvedLongInterfaceMethod:
  case JVM_CONSTANT_ResolvedObjectInterfaceMethod:
  case JVM_CONSTANT_ResolvedArrayInterfaceMethod:
  case JVM_CONSTANT_ResolvedVoidInterfaceMethod:
    /* fall through */
  case JVM_CONSTANT_ResolvedBooleanVirtualMethod:
  case JVM_CONSTANT_ResolvedCharVirtualMethod:
  case JVM_CONSTANT_ResolvedFloatVirtualMethod:
  case JVM_CONSTANT_ResolvedDoubleVirtualMethod:
  case JVM_CONSTANT_ResolvedByteVirtualMethod:
  case JVM_CONSTANT_ResolvedShortVirtualMethod:
  case JVM_CONSTANT_ResolvedIntVirtualMethod:
  case JVM_CONSTANT_ResolvedLongVirtualMethod:
  case JVM_CONSTANT_ResolvedObjectVirtualMethod:
  case JVM_CONSTANT_ResolvedArrayVirtualMethod:
  case JVM_CONSTANT_ResolvedVoidVirtualMethod:
  case JVM_CONSTANT_ResolvedUncommonInterfaceMethod:

    return hashcode_for_method_ref(orig_cp, i);

  case JVM_CONSTANT_ResolvedFinalUncommonInterfaceMethod: //constant pool contains Method*
    {
      Method::Raw method = orig_cp->resolved_uncommon_final_method_at(i);
      Symbol::Raw name = method().name();
      return hashcode_for_symbol(&name);
    }    

  case JVM_CONSTANT_Invalid:
    // Note that this block includes JVM_CONSTANT_Invalid - it is
    // used for valid reasons: for example We use it for
    // Field::INITVAL_OFFSET when there's no initial value assigned.
    // -- fall-through --
  case JVM_CONSTANT_Long:
  case JVM_CONSTANT_Double:
    // For Long and Double, let's just use the first 32-bits to compute
    // the hash.
    // -- fall-through --
  case JVM_CONSTANT_Integer:
  case JVM_CONSTANT_Class:
  case JVM_CONSTANT_Float:
    {
      GUARANTEE(!tag.is_oop(), "sanity");
      int type = tag.value();
      jint value32 = orig_cp->value32_at(i);
      juint hash = (juint)((type ^ value32) ^ 0x1a2b3c4d);
      return hash;
    }

  case JVM_CONSTANT_Fieldref:
  case JVM_CONSTANT_Methodref:
  case JVM_CONSTANT_InterfaceMethodref:
  case JVM_CONSTANT_UnresolvedString:
  case JVM_CONSTANT_UnresolvedClass:
  case JVM_CONSTANT_ClassIndex:
  default:
    // At this point, the ROMOptimizer is already executed so
    // all of these types of entries should have already be resolved.
    // The only exceptions are the next two cases.
    SHOULD_NOT_REACH_HERE();
    return 0;
  }
}

juint ConstantPoolRewriter::hashcode_for_method(Method *method) {
  Symbol::Raw name = method->name();
  Symbol::Raw sig = method->signature();

  return (hashcode_for_symbol(&name) ^ hashcode_for_symbol(&sig));
}

juint ConstantPoolRewriter::hashcode_for_symbol(Symbol *symbol) {
  return SymbolTable::hash(symbol);
}

juint ConstantPoolRewriter::hashcode_for_string(String *string) {
  // Note that we don't use Synchronizer::hash_code, because that requires
  // changing object headers
  return string->hash();
}

juint ConstantPoolRewriter::hashcode_for_method_ref(ConstantPool *orig_cp,
                                                    int i) {
  ConstantTag tag = orig_cp->tag_at(i);
  GUARANTEE(tag.is_resolved_interface_method() ||
            tag.is_resolved_virtual_method() ||
            tag.is_resolved_uncommon_interface_method(), "sanity");

  int type = tag.value();
  jint value32 = orig_cp->value32_at(i);  

  jint table_index = extract_low_jushort_from_jint(value32);
  jint class_id    = extract_high_jushort_from_jint(value32);

  InstanceClass::Raw klass = Universe::class_from_id(class_id);
  Symbol::Raw klass_name = klass().name();
  juint hash = (juint)((type ^ table_index) ^ 0x1a2b3c4d);

  hash ^= hashcode_for_symbol(&klass_name);

  return hash;
}

juint ConstantPoolRewriter::hashcode_for_field_ref(ConstantPool *orig_cp,
                                                 int i) {
  ConstantTag tag = orig_cp->tag_at(i);
  GUARANTEE(tag.is_resolved_field(), "sanity");

  int type = tag.value();
  jint value32 = orig_cp->value32_at(i);
  jint offset   = extract_high_jushort_from_jint(value32);
  jint class_id = extract_low_jushort_from_jint(value32);

  InstanceClass::Raw klass = Universe::class_from_id(class_id);
  Symbol::Raw klass_name = klass().name();
  juint hash = (juint)((type ^ offset) ^ 0x1a2b3c4d);

  hash ^= hashcode_for_symbol(&klass_name);

  return hash;
}

inline void
ConstantPoolRewriter::rewrite_index(ConstantPool *orig_cp, TypeArray *t,
                                    int offset JVM_TRAPS) {

  jushort index = t->ushort_at(offset);
  index = get_merged_pool_entry(orig_cp, index JVM_CHECK);
  t->ushort_at_put(offset, index);
}

// Structure of the _entry_lookup_table:
//
// _entry_lookup_table[i] = ObjArray item[3]
//
// item[0] = int[2] : (type of tag), (index in merged pool)
// item[1] = TypeArray[1] or ObjArray[1] - value
// item[2] = next item with the same hash

// Return the index of the entry in the merged constant pool
jushort ConstantPoolRewriter::get_merged_pool_entry(ConstantPool *orig_cp,
                                                    int i JVM_TRAPS) {
  ConstantTag tag = orig_cp->tag_at(i);
  GUARANTEE(ConstantTag::is_valid_tag(tag.value()), "sanity");

  juint hash = compute_hashcode(orig_cp, i);
  hash = hash % NUM_BUCKETS;

  /*
   * (1) Check if there's a duplicate of the entry.
   */
  int new_index = find_stored_hash_entry(orig_cp, i, tag, hash);
  if (new_index >= 0) {
    return (jushort)new_index;
  }

  new_index = _merged_entries_count;
  if (tag.is_long() || tag.is_double()) {
    _merged_entries_count += 2;
  } else {
    _merged_entries_count += 1;
  }
  GUARANTEE(_merged_entries_count <= 65535, "sanity");

  /*
   * (2) Save information about this entry in the hash table.
   */
  store_hash_entry(orig_cp, i, tag, hash, new_index JVM_CHECK_0);

  return (jushort)new_index;
}

void ConstantPoolRewriter::store_hash_entry(ConstantPool *orig_cp,
                                            int i, ConstantTag tag,
                                            juint hash, int new_index
                                            JVM_TRAPS) {
  int type = tag.value();
  UsingFastOops level1;
  ObjArray::Fast info = Universe::new_obj_array(3 JVM_CHECK);

  // info[0]: type and index
  TypeArray::Fast type_info = Universe::new_int_array(2 JVM_CHECK);
  type_info().int_at_put(0, type);
  type_info().int_at_put(1, new_index);
  info().obj_at_put(0, &type_info);

  // info[1]: a copy of the value
  ObjArray::Fast obj_info;
  Oop::Fast obj;
  TypeArray::Fast basictype_info;
  switch (type) {
  case JVM_CONSTANT_Utf8:
  case JVM_CONSTANT_String:
  case JVM_CONSTANT_ResolvedStaticMethod:
  case JVM_CONSTANT_ResolvedFinalUncommonInterfaceMethod:
    {
      // Store the hash entry
      GUARANTEE(tag.is_oop(), "sanity");
      obj_info = Universe::new_obj_array(1 JVM_CHECK);
      obj = orig_cp->oop_at(i);
      obj_info().obj_at_put(0, &obj);
      info().obj_at_put(1, &obj_info);

      // Store the new CP entry
      _merged_pool.obj_field_put(_merged_pool.offset_from_index(new_index),
                                 &obj);
    }
    break;

  case JVM_CONSTANT_Long:
  case JVM_CONSTANT_Double:
    {
      // Store the hash entry
      basictype_info = Universe::new_long_array(1 JVM_CHECK);
      basictype_info().long_at_put(0, orig_cp->value64_at(i));
      info().obj_at_put(1, &basictype_info);

      // Store the new CP entry
      _merged_pool.long_field_put(_merged_pool.offset_from_index(new_index),
                                  orig_cp->value64_at(i));
    }
    break;

  case JVM_CONSTANT_ResolvedBooleanFieldref:
  case JVM_CONSTANT_ResolvedCharFieldref:
  case JVM_CONSTANT_ResolvedFloatFieldref:
  case JVM_CONSTANT_ResolvedDoubleFieldref:
  case JVM_CONSTANT_ResolvedByteFieldref:
  case JVM_CONSTANT_ResolvedShortFieldref:
  case JVM_CONSTANT_ResolvedIntFieldref:
  case JVM_CONSTANT_ResolvedLongFieldref:
  case JVM_CONSTANT_ResolvedObjectFieldref:
  case JVM_CONSTANT_ResolvedArrayFieldref:
  case JVM_CONSTANT_ResolvedStaticBooleanFieldref:
  case JVM_CONSTANT_ResolvedStaticCharFieldref:
  case JVM_CONSTANT_ResolvedStaticFloatFieldref:
  case JVM_CONSTANT_ResolvedStaticDoubleFieldref:
  case JVM_CONSTANT_ResolvedStaticByteFieldref:
  case JVM_CONSTANT_ResolvedStaticShortFieldref:
  case JVM_CONSTANT_ResolvedStaticIntFieldref:
  case JVM_CONSTANT_ResolvedStaticLongFieldref:
  case JVM_CONSTANT_ResolvedStaticObjectFieldref:
  case JVM_CONSTANT_ResolvedStaticArrayFieldref:
    // Fall-through: resolved field refs can be copied from old pool
    // to new pool with the same numeric value

  case JVM_CONSTANT_ResolvedBooleanInterfaceMethod:
  case JVM_CONSTANT_ResolvedCharInterfaceMethod:
  case JVM_CONSTANT_ResolvedFloatInterfaceMethod:
  case JVM_CONSTANT_ResolvedDoubleInterfaceMethod:
  case JVM_CONSTANT_ResolvedByteInterfaceMethod:
  case JVM_CONSTANT_ResolvedShortInterfaceMethod:
  case JVM_CONSTANT_ResolvedIntInterfaceMethod:
  case JVM_CONSTANT_ResolvedLongInterfaceMethod:
  case JVM_CONSTANT_ResolvedObjectInterfaceMethod:
  case JVM_CONSTANT_ResolvedArrayInterfaceMethod:
  case JVM_CONSTANT_ResolvedVoidInterfaceMethod:
    /* fall through */
  case JVM_CONSTANT_ResolvedBooleanVirtualMethod:
  case JVM_CONSTANT_ResolvedCharVirtualMethod:
  case JVM_CONSTANT_ResolvedFloatVirtualMethod:
  case JVM_CONSTANT_ResolvedDoubleVirtualMethod:
  case JVM_CONSTANT_ResolvedByteVirtualMethod:
  case JVM_CONSTANT_ResolvedShortVirtualMethod:
  case JVM_CONSTANT_ResolvedIntVirtualMethod:
  case JVM_CONSTANT_ResolvedLongVirtualMethod:
  case JVM_CONSTANT_ResolvedObjectVirtualMethod:
  case JVM_CONSTANT_ResolvedArrayVirtualMethod:
  case JVM_CONSTANT_ResolvedVoidVirtualMethod:
  case JVM_CONSTANT_ResolvedUncommonInterfaceMethod:
    // Fall-through: virtual and interface method refs can be copied
    // from old pool to new pool with the same numeric value

  case JVM_CONSTANT_Invalid:
  case JVM_CONSTANT_Integer:
  case JVM_CONSTANT_Class:
  case JVM_CONSTANT_Float:
    {
      // Store the hash entry
      basictype_info = Universe::new_int_array(1 JVM_CHECK);
      basictype_info().int_at_put(0, orig_cp->value32_at(i));
      info().obj_at_put(1, &basictype_info);

      // Store the new CP entry
      _merged_pool.int_field_put(_merged_pool.offset_from_index(new_index),
                                 orig_cp->value32_at(i));
    }
    break;

  default:
    // See comments in compute_hashcode()
    SHOULD_NOT_REACH_HERE();
  }

  // info[2]: points to next item in the same bucket
  ObjArray::Fast next = _entry_lookup_table.obj_at(hash);
  info().obj_at_put(2, &next);
  _entry_lookup_table.obj_at_put(hash, &info);

  // Store new CP tag
  _merged_pool.tag_at_put(new_index, tag.value());

  // Accounting
  account_for_new_entry(tag);
}

// Check if a hash entry has already been stored for this constant pool item.
int ConstantPoolRewriter::find_stored_hash_entry(ConstantPool *orig_cp,
                                                 int i, ConstantTag tag,
                                                 juint hash) {
  UsingFastOops level1;
  ObjArray::Fast info = _entry_lookup_table.obj_at(hash);
  TypeArray::Fast type_info;
  while (!info.is_null()) {
    type_info = info().obj_at(0);
    bool found = false;

    if (type_info().int_at(0) == tag.value()) {
      switch (tag.value()) {
      case JVM_CONSTANT_Utf8:
      case JVM_CONSTANT_String:
      case JVM_CONSTANT_ResolvedStaticMethod:
      case JVM_CONSTANT_ResolvedFinalUncommonInterfaceMethod:
        {
          ObjArray::Raw obj_info = info().obj_at(1);
          Oop::Raw stored_obj = obj_info().obj_at(0);
          Oop::Raw my_obj = orig_cp->oop_at(i);

          if (my_obj.equals(&stored_obj)) {
            found = true;
          }
        }
        break;

      case JVM_CONSTANT_Long:
      case JVM_CONSTANT_Double:
        {
          TypeArray::Raw long_info = info().obj_at(1);
          jlong stored_value = long_info().long_at(0);
          jlong my_value = orig_cp->value64_at(i);
          if (my_value == stored_value) {
            found = true;
          }
        }

        break;

      case JVM_CONSTANT_ResolvedBooleanFieldref:
      case JVM_CONSTANT_ResolvedCharFieldref:
      case JVM_CONSTANT_ResolvedFloatFieldref:
      case JVM_CONSTANT_ResolvedDoubleFieldref:
      case JVM_CONSTANT_ResolvedByteFieldref:
      case JVM_CONSTANT_ResolvedShortFieldref:
      case JVM_CONSTANT_ResolvedIntFieldref:
      case JVM_CONSTANT_ResolvedLongFieldref:
      case JVM_CONSTANT_ResolvedObjectFieldref:
      case JVM_CONSTANT_ResolvedArrayFieldref:     
      case JVM_CONSTANT_ResolvedStaticBooleanFieldref:
      case JVM_CONSTANT_ResolvedStaticCharFieldref:
      case JVM_CONSTANT_ResolvedStaticFloatFieldref:
      case JVM_CONSTANT_ResolvedStaticDoubleFieldref:
      case JVM_CONSTANT_ResolvedStaticByteFieldref:
      case JVM_CONSTANT_ResolvedStaticShortFieldref:
      case JVM_CONSTANT_ResolvedStaticIntFieldref:
      case JVM_CONSTANT_ResolvedStaticLongFieldref:
      case JVM_CONSTANT_ResolvedStaticObjectFieldref:
      case JVM_CONSTANT_ResolvedStaticArrayFieldref:
      
        // Fall-through: resolved field refs can be copied from old pool
        // to new pool with the same numeric value

      case JVM_CONSTANT_ResolvedBooleanInterfaceMethod:
      case JVM_CONSTANT_ResolvedCharInterfaceMethod:
      case JVM_CONSTANT_ResolvedFloatInterfaceMethod:
      case JVM_CONSTANT_ResolvedDoubleInterfaceMethod:
      case JVM_CONSTANT_ResolvedByteInterfaceMethod:
      case JVM_CONSTANT_ResolvedShortInterfaceMethod:
      case JVM_CONSTANT_ResolvedIntInterfaceMethod:
      case JVM_CONSTANT_ResolvedLongInterfaceMethod:
      case JVM_CONSTANT_ResolvedObjectInterfaceMethod:
      case JVM_CONSTANT_ResolvedArrayInterfaceMethod:
      case JVM_CONSTANT_ResolvedVoidInterfaceMethod:
          /* fall through */
      case JVM_CONSTANT_ResolvedBooleanVirtualMethod:
      case JVM_CONSTANT_ResolvedCharVirtualMethod:
      case JVM_CONSTANT_ResolvedFloatVirtualMethod:
      case JVM_CONSTANT_ResolvedDoubleVirtualMethod:
      case JVM_CONSTANT_ResolvedByteVirtualMethod:
      case JVM_CONSTANT_ResolvedShortVirtualMethod:
      case JVM_CONSTANT_ResolvedIntVirtualMethod:
      case JVM_CONSTANT_ResolvedLongVirtualMethod:
      case JVM_CONSTANT_ResolvedObjectVirtualMethod:
      case JVM_CONSTANT_ResolvedArrayVirtualMethod:
      case JVM_CONSTANT_ResolvedVoidVirtualMethod:
      case JVM_CONSTANT_ResolvedUncommonInterfaceMethod:
        // Fall-through: virtual and interface method refs can be copied
        // from old pool to new pool with the same numeric value

      case JVM_CONSTANT_Invalid:
      case JVM_CONSTANT_Integer:
      case JVM_CONSTANT_Class:
      case JVM_CONSTANT_Float:
        {
          TypeArray::Raw int_info = info().obj_at(1);
          jint stored_value = int_info().int_at(0);
          jint value32 = orig_cp->value32_at(i);
          if (value32 == stored_value) {
            found = true;
          }
        }
        break;

      default:
        // See comments in compute_hashcode()
        SHOULD_NOT_REACH_HERE();
      }
    }

    if (found) {
      return type_info().int_at(1);
    } else {
      info = info().obj_at(2);
    }
  }

  return -1;
}

void ConstantPoolRewriter::account_for_new_entry(ConstantTag new_entry_tag) {
  int type = new_entry_tag.value();

  switch (type) {
  case JVM_CONSTANT_Invalid:
    _count_of_invalid ++;
    break;
  case JVM_CONSTANT_Integer:
    _count_of_integer ++;
    break;
  case JVM_CONSTANT_Class:
    _count_of_class ++;
    break;
  case JVM_CONSTANT_Float:
    _count_of_float ++;
    break;
  case JVM_CONSTANT_Long:
    _count_of_long ++;
    break;
  case JVM_CONSTANT_Double:
    _count_of_double ++;
    break;
  case JVM_CONSTANT_Utf8:
    _count_of_utf8 ++;
    break;
  case JVM_CONSTANT_String:
    _count_of_string ++;
    break;
  case JVM_CONSTANT_ResolvedStaticMethod:
  case JVM_CONSTANT_ResolvedFinalUncommonInterfaceMethod:
    _count_of_static_method ++;
    break;
  case JVM_CONSTANT_ResolvedBooleanFieldref:
  case JVM_CONSTANT_ResolvedCharFieldref:
  case JVM_CONSTANT_ResolvedFloatFieldref:
  case JVM_CONSTANT_ResolvedDoubleFieldref:
  case JVM_CONSTANT_ResolvedByteFieldref:
  case JVM_CONSTANT_ResolvedShortFieldref:
  case JVM_CONSTANT_ResolvedIntFieldref:
  case JVM_CONSTANT_ResolvedLongFieldref:
  case JVM_CONSTANT_ResolvedObjectFieldref:
  case JVM_CONSTANT_ResolvedArrayFieldref: 
  case JVM_CONSTANT_ResolvedStaticBooleanFieldref:
  case JVM_CONSTANT_ResolvedStaticCharFieldref:
  case JVM_CONSTANT_ResolvedStaticFloatFieldref:
  case JVM_CONSTANT_ResolvedStaticDoubleFieldref:
  case JVM_CONSTANT_ResolvedStaticByteFieldref:
  case JVM_CONSTANT_ResolvedStaticShortFieldref:
  case JVM_CONSTANT_ResolvedStaticIntFieldref:
  case JVM_CONSTANT_ResolvedStaticLongFieldref:
  case JVM_CONSTANT_ResolvedStaticObjectFieldref:
  case JVM_CONSTANT_ResolvedStaticArrayFieldref:
    _count_of_field_ref ++;
    break;
  case JVM_CONSTANT_ResolvedBooleanInterfaceMethod:
  case JVM_CONSTANT_ResolvedCharInterfaceMethod:
  case JVM_CONSTANT_ResolvedFloatInterfaceMethod:
  case JVM_CONSTANT_ResolvedDoubleInterfaceMethod:
  case JVM_CONSTANT_ResolvedByteInterfaceMethod:
  case JVM_CONSTANT_ResolvedShortInterfaceMethod:
  case JVM_CONSTANT_ResolvedIntInterfaceMethod:
  case JVM_CONSTANT_ResolvedLongInterfaceMethod:
  case JVM_CONSTANT_ResolvedObjectInterfaceMethod:
  case JVM_CONSTANT_ResolvedArrayInterfaceMethod:
  case JVM_CONSTANT_ResolvedVoidInterfaceMethod:
    _count_of_interface_method_ref ++;
    break;
  case JVM_CONSTANT_ResolvedBooleanVirtualMethod:
  case JVM_CONSTANT_ResolvedCharVirtualMethod:
  case JVM_CONSTANT_ResolvedFloatVirtualMethod:
  case JVM_CONSTANT_ResolvedDoubleVirtualMethod:
  case JVM_CONSTANT_ResolvedByteVirtualMethod:
  case JVM_CONSTANT_ResolvedShortVirtualMethod:
  case JVM_CONSTANT_ResolvedIntVirtualMethod:
  case JVM_CONSTANT_ResolvedLongVirtualMethod:
  case JVM_CONSTANT_ResolvedObjectVirtualMethod:
  case JVM_CONSTANT_ResolvedArrayVirtualMethod:
  case JVM_CONSTANT_ResolvedVoidVirtualMethod:
  case JVM_CONSTANT_ResolvedUncommonInterfaceMethod:
    _count_of_virtual_method_ref ++;
    break;

  default:
    // See comments in compute_hashcode()
    SHOULD_NOT_REACH_HERE();
  }
}

// Detect bytecodes of the sequence:
//    L0: aload_x
//    L1: pop
// These sequences are generated by the ROMInliner. We remove them to
// save footprint. To preserve the semantics of the bytecodes, we don't
// eliminate this sequence if L1 is a branch target.
bool ConstantPoolRewriter::is_redundant_push_pop(Method *method, int bci) {
  int next_bci = bci + 1;
  if (next_bci >= method->code_size()) {
    return false;
  }
  if (method->bytecode_at(next_bci) != Bytecodes::_pop) {
    return false;
  }
  if (is_branch_target(bci)) {
    // This may not be necessary, but I'm being paranoid.
    return false;
  }
  if (is_branch_target(next_bci)) {
    return false;
  }
  return true;
}

// Computes the size of the new method. Also generate a table of
// all the offsets of the bytes in the new method
//return value is true only if we MUST replace the method with new one and couldn't 
//just correct it's bytecodes
bool ConstantPoolRewriter::shall_create_new_method(Method *method, int* p_new_size JVM_TRAPS) {
#ifdef AZZERT
  void * base = _new_bytecode_address.base_address();
  jvm_memset(base, 0xff, sizeof(jushort) * _new_bytecode_address.length());
#endif  

  jushort old_bci, new_bci;
  bool result = false;
  jushort old_cp_index, new_cp_index;
  ConstantPool cp = method->constants();

  init_branch_targets(method JVM_CHECK_0);

  bool ignore_one_pop = false;
  for (old_bci = 0, new_bci=0; old_bci != method->code_size();) {
    GUARANTEE(old_bci < method->code_size(), "invalid bytecode");

    const Bytecodes::Code code = method->bytecode_at(old_bci);
    const int old_len = method->bytecode_length_for(old_bci);
    int new_len = old_len;

    switch (code) {
    case Bytecodes::_ldc:
#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_ildc:
    case Bytecodes::_fast_fldc:
    case Bytecodes::_fast_aldc:
#else
    case Bytecodes::_fast_1_ldc:
#endif
      old_cp_index = method->get_ubyte(old_bci + 1);
      new_cp_index = get_merged_pool_entry(&cp, old_cp_index JVM_CHECK_0);

      if (new_cp_index > 0xff) {
        new_len = old_len + 1;
        result = true;
      }
      break;

    case Bytecodes::_tableswitch:
    case Bytecodes::_lookupswitch:
      {
        int old_aligned = align_size_up(old_bci + 1, sizeof(jint));
        int new_aligned = align_size_up(new_bci + 1, sizeof(jint));
        int old_pad = old_aligned - old_bci;
        int new_pad = new_aligned - new_bci;

        new_len = old_len + (new_pad - old_pad);
        result = result || ((new_pad - old_pad) != 0); //it's unnecessary, cause old_bci already ne new_bci, but....
      }
      break;

    case Bytecodes::_aload_0:
    case Bytecodes::_aload_1:
    case Bytecodes::_aload_2:
    case Bytecodes::_aload_3:
    case Bytecodes::_dup:
      if (CompactROMBytecodes && is_redundant_push_pop(method, old_bci)) {
        new_len = 0;
        ignore_one_pop = true;
        result = true;
      }
      break;

    case Bytecodes::_pop:
      if (ignore_one_pop) {
        new_len = 0;
        ignore_one_pop = false;
        result = true;
      }
      break;

    case Bytecodes::_nop:
      if (CompactROMBytecodes && !is_branch_target(old_bci)) {
        new_len = 0;
        result = true;
      }
      break;
    }

    // Record the address of this bytecode in the new bytecode stream.
    _new_bytecode_address.ushort_at_put(old_bci, new_bci);
    old_bci += old_len;
    new_bci += new_len;
  }
  // Mark the address of code[code_length] so that we can rewrite the
  // exception header correctly.  The end_pc of an exception range is one
  // beyond the last pc that could cause an exception
  _new_bytecode_address.ushort_at_put(old_bci, new_bci);

  // IMPL_NOTE: new_bci cannot be more than 65535!

  *p_new_size = new_bci;
  return result;
}

void ConstantPoolRewriter::correct_cp_indices(Method *method JVM_TRAPS) {  
  ConstantPool orig_cp = method->constants();
  for (int bci = 0; bci != method->code_size();) {    
    // (1) determine the length of this bytecode
    const Bytecodes::Code code = method->bytecode_at(bci);
    const int len = method->bytecode_length_for(bci);
    jushort old_index;
    jushort new_index;
    
    // (2) Stream the bytecode, using new CP index and branch offsets
    //     if necessary
    switch (code) {
    case Bytecodes::_ldc_w:
    case Bytecodes::_ldc2_w:
    case Bytecodes::_anewarray:
    case Bytecodes::_putfield:
    case Bytecodes::_getfield:
    case Bytecodes::_putstatic:
    case Bytecodes::_getstatic:
    case Bytecodes::_invokevirtual:
    case Bytecodes::_invokespecial:
    case Bytecodes::_invokestatic:
    case Bytecodes::_fast_invokespecial:
    case Bytecodes::_new:
    case Bytecodes::_checkcast:
    case Bytecodes::_instanceof:
#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_ildc_w:
    case Bytecodes::_fast_fldc_w:
    case Bytecodes::_fast_aldc_w:
    case Bytecodes::_fast_lldc_w:
    case Bytecodes::_fast_dldc_w:
    case Bytecodes::_fast_iputstatic:
    case Bytecodes::_fast_lputstatic:
    case Bytecodes::_fast_fputstatic:
    case Bytecodes::_fast_dputstatic:
    case Bytecodes::_fast_aputstatic:
    case Bytecodes::_fast_igetstatic:
    case Bytecodes::_fast_lgetstatic:
    case Bytecodes::_fast_fgetstatic:
    case Bytecodes::_fast_dgetstatic:
    case Bytecodes::_fast_agetstatic:
#else
    case Bytecodes::_fast_1_ldc_w:
    case Bytecodes::_fast_2_ldc_w:
    case Bytecodes::_fast_1_putstatic:
    case Bytecodes::_fast_2_putstatic:
    case Bytecodes::_fast_a_putstatic:
    case Bytecodes::_fast_1_getstatic:
    case Bytecodes::_fast_2_getstatic:
    case Bytecodes::_fast_init_1_putstatic:
    case Bytecodes::_fast_init_2_putstatic:
    case Bytecodes::_fast_init_a_putstatic:
    case Bytecodes::_fast_init_1_getstatic:
    case Bytecodes::_fast_init_2_getstatic:
#endif
    case Bytecodes::_fast_invokevirtual:
    case Bytecodes::_fast_invokestatic:
    case Bytecodes::_fast_init_invokestatic:
    case Bytecodes::_fast_new:
    case Bytecodes::_fast_init_new:
    case Bytecodes::_fast_anewarray:
    case Bytecodes::_fast_checkcast:
    case Bytecodes::_fast_instanceof:
    case Bytecodes::_fast_invokevirtual_final:
      GUARANTEE(len == 3, "sanity");
      old_index = method->get_java_ushort(bci+1);      
      new_index = get_merged_pool_entry(&orig_cp, old_index JVM_CHECK);
      method->put_java_ushort(bci+1, new_index);
      break;

    case Bytecodes::_ldc:
#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_ildc:
    case Bytecodes::_fast_fldc:
    case Bytecodes::_fast_aldc:
#else
    case Bytecodes::_fast_1_ldc:
#endif
      {
        GUARANTEE(len == 2, "sanity");        
        old_index = method->get_ubyte(bci + 1);        
        new_index = get_merged_pool_entry(&orig_cp, old_index JVM_CHECK);
        GUARANTEE(new_index <= 0xff, "it must be so - we check it during get_new_method_code_size");        
        method->ubyte_at_put(bci+1, (jubyte)new_index);
      }
      break;

    case Bytecodes::_multianewarray:
      GUARANTEE(len == 4, "sanity");
      old_index = method->get_java_ushort(bci+1);
      new_index = get_merged_pool_entry(&orig_cp, old_index JVM_CHECK);
      method->put_java_ushort(bci+1, new_index);
      break;

    case Bytecodes::_invokeinterface:
    case Bytecodes::_fast_invokeinterface:
      GUARANTEE(len == 5, "sanity");
      old_index = method->get_java_ushort(bci+1);     
      new_index = get_merged_pool_entry(&orig_cp, old_index JVM_CHECK);
      method->put_java_ushort(bci+1, new_index);
      break;

    default:
      break;
    }

    // (3) Advance to next bytecode
    bci += len;    
  }
}

void ConstantPoolRewriter::stream_bytecodes(Method *old_method,
                                            Method *new_method JVM_TRAPS) {
  bool ignore_one_pop = false;
  for (int old_bci = 0, new_bci = 0; old_bci != old_method->code_size();) {
    // (1) determine the length of this bytecode
    const Bytecodes::Code code = old_method->bytecode_at(old_bci);
    GUARANTEE(old_bci < old_method->code_size(), "sanity");
    GUARANTEE(new_bci < new_method->code_size() || code == Bytecodes::_nop, "sanity");

    const int old_len = old_method->bytecode_length_for(old_bci);
    int new_len = old_len;

    // (2) Stream the bytecode, using new CP index and branch offsets
    //     if necessary
    switch (code) {
    case Bytecodes::_ldc_w:
    case Bytecodes::_ldc2_w:
    case Bytecodes::_anewarray:
    case Bytecodes::_putfield:
    case Bytecodes::_getfield:
    case Bytecodes::_putstatic:
    case Bytecodes::_getstatic:
    case Bytecodes::_invokevirtual:
    case Bytecodes::_invokespecial:
    case Bytecodes::_invokestatic:
    case Bytecodes::_fast_invokespecial:
    case Bytecodes::_new:
    case Bytecodes::_checkcast:
    case Bytecodes::_instanceof:
#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_ildc_w:
    case Bytecodes::_fast_fldc_w:
    case Bytecodes::_fast_aldc_w:
    case Bytecodes::_fast_lldc_w:
    case Bytecodes::_fast_dldc_w:
    case Bytecodes::_fast_iputstatic:
    case Bytecodes::_fast_lputstatic:
    case Bytecodes::_fast_fputstatic:
    case Bytecodes::_fast_dputstatic:
    case Bytecodes::_fast_aputstatic:
    case Bytecodes::_fast_igetstatic:
    case Bytecodes::_fast_lgetstatic:
    case Bytecodes::_fast_fgetstatic:
    case Bytecodes::_fast_dgetstatic:
    case Bytecodes::_fast_agetstatic:
#else
    case Bytecodes::_fast_1_ldc_w:
    case Bytecodes::_fast_2_ldc_w:
    case Bytecodes::_fast_1_putstatic:
    case Bytecodes::_fast_2_putstatic:
    case Bytecodes::_fast_a_putstatic:
    case Bytecodes::_fast_1_getstatic:
    case Bytecodes::_fast_2_getstatic:
    case Bytecodes::_fast_init_1_putstatic:
    case Bytecodes::_fast_init_2_putstatic:
    case Bytecodes::_fast_init_a_putstatic:
    case Bytecodes::_fast_init_1_getstatic:
    case Bytecodes::_fast_init_2_getstatic:
#endif
    case Bytecodes::_fast_invokevirtual:
    case Bytecodes::_fast_invokestatic:
    case Bytecodes::_fast_init_invokestatic:
    case Bytecodes::_fast_new:
    case Bytecodes::_fast_init_new:
    case Bytecodes::_fast_anewarray:
    case Bytecodes::_fast_checkcast:
    case Bytecodes::_fast_instanceof:
    case Bytecodes::_fast_invokevirtual_final:
      GUARANTEE(old_len == 3, "sanity");
      stream_java_cp_index(old_method, new_method, old_bci, new_bci JVM_CHECK);
      break;

    case Bytecodes::_ldc:
#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_ildc:
    case Bytecodes::_fast_fldc:
    case Bytecodes::_fast_aldc:
#else
    case Bytecodes::_fast_1_ldc:
#endif
      {
        GUARANTEE(old_len == 2, "sanity");
        int delta = stream_ldc(old_method, new_method, old_bci, new_bci JVM_CHECK);
        new_len += delta;
      }
      break;

    case Bytecodes::_multianewarray:
      GUARANTEE(old_len == 4, "sanity");
      stream_multianewarray(old_method, new_method, old_bci, new_bci JVM_CHECK);
      break;

    case Bytecodes::_invokeinterface:
    case Bytecodes::_fast_invokeinterface:
      GUARANTEE(old_len == 5, "sanity");
      stream_invokeinterface(old_method, new_method, old_bci, new_bci JVM_CHECK);
      break;

    case Bytecodes::_ifeq:
    case Bytecodes::_ifne:
    case Bytecodes::_iflt:
    case Bytecodes::_ifge:
    case Bytecodes::_ifgt:
    case Bytecodes::_ifle:
    case Bytecodes::_if_icmpeq:
    case Bytecodes::_if_icmpne:
    case Bytecodes::_if_icmplt:
    case Bytecodes::_if_icmpge:
    case Bytecodes::_if_icmpgt:
    case Bytecodes::_if_icmple:
    case Bytecodes::_if_acmpeq:
    case Bytecodes::_if_acmpne:
    case Bytecodes::_goto:
    case Bytecodes::_jsr:
    case Bytecodes::_ifnull:
    case Bytecodes::_ifnonnull:
      GUARANTEE(old_len == 3, "sanity");
      stream_branch2(old_method, new_method, old_bci, new_bci);
      break;

    case Bytecodes::_goto_w:
    case Bytecodes::_jsr_w:
      GUARANTEE(old_len == 5, "sanity");
      stream_branch4(old_method, new_method, old_bci, new_bci);
      break;

    case Bytecodes::_tableswitch:
      new_len = stream_tableswitch(old_method, new_method, old_bci, new_bci);
      break;

    case Bytecodes::_lookupswitch:
      new_len = stream_lookupswitch(old_method, new_method, old_bci, new_bci);
      break;

    case Bytecodes::_aload_0:
    case Bytecodes::_aload_1:
    case Bytecodes::_aload_2:
    case Bytecodes::_aload_3:
    case Bytecodes::_dup:
      if (CompactROMBytecodes && is_redundant_push_pop(old_method, old_bci)) {
        new_len = 0;
        ignore_one_pop = true;
      } else {
        stream_raw_bytes(old_method, new_method, old_bci, new_bci, old_len);
      }
      break;

    case Bytecodes::_pop:
      if (ignore_one_pop) {
        new_len = 0;
        ignore_one_pop = false;
      } else {
        stream_raw_bytes(old_method, new_method, old_bci, new_bci, old_len);
      }
      break;

    case Bytecodes::_nop:
      if (CompactROMBytecodes && !is_branch_target(old_bci)) {
        new_len = 0;
      }
      break;

    default:
      // none of the remaining bytecodes need rewriting.
      stream_raw_bytes(old_method, new_method, old_bci, new_bci, old_len);
      break;
    }

    // (3) Advance to next bytecode
    old_bci += old_len;
    new_bci += new_len;

    // (4) update the stackmaps for this shift in bci
    if (new_len != old_len) {
      short bci_shift = new_len - old_len;
      StackmapGenerator::rewrite_stackmap_bcis(old_method, new_bci - bci_shift,
                                               bci_shift JVM_CHECK);
    }
  }
}

// Copy raw bytes from the old method to the new method
void ConstantPoolRewriter::stream_raw_bytes(Method *old_method,
                                            Method *new_method,
                                            int old_bci, int new_bci,
                                            int len) {
  for (int i=0; i<len; i++) {
    new_method->ubyte_at_put(new_bci+i, old_method->ubyte_at(old_bci+i));
  }
}

// Stream the ldc (fast_ildc, etc) bytecode, rewriting it to ldc_w if
// necessary.
int ConstantPoolRewriter::stream_ldc(Method *old_method, Method *new_method,
                                     int old_bci, int new_bci JVM_TRAPS) {
  ConstantPool orig_cp = old_method->constants();
  Bytecodes::Code code = (Bytecodes::Code)old_method->get_ubyte(old_bci);
  juint old_index = old_method->get_ubyte(old_bci + 1);
  juint new_index = get_merged_pool_entry(&orig_cp, old_index JVM_CHECK_0);

  if (new_index > 0xff) {
    switch (code) {
    case Bytecodes::_ldc      : code =  Bytecodes::_ldc_w;       break;
#if ENABLE_JAVA_STACK_TAGS
    case Bytecodes::_fast_ildc: code =  Bytecodes::_fast_ildc_w; break;
    case Bytecodes::_fast_fldc: code =  Bytecodes::_fast_fldc_w; break;
    case Bytecodes::_fast_aldc: code =  Bytecodes::_fast_aldc_w; break;
#else
    case Bytecodes::_fast_1_ldc:code =  Bytecodes::_fast_1_ldc_w; break;
#endif
    default                   : SHOULD_NOT_REACH_HERE();
    }
    new_method->put_java_ushort(new_bci+1, (jushort)new_index);
  } else {
    new_method->ubyte_at_put(new_bci+1, (jubyte)new_index);
  }

  new_method->bytecode_at_put_raw(new_bci, code);

  if (new_index > 0xff) {
    return 1;
  } else {
    return 0;
  }
}

void ConstantPoolRewriter::stream_invokeinterface(Method *old_method,
                                                  Method *new_method,
                                                  int old_bci, int new_bci
                                                  JVM_TRAPS) {
  // the bytecode
  const Bytecodes::Code bc = old_method->bytecode_at(old_bci);
  new_method->bytecode_at_put_raw(new_bci, bc);

  // the CP index
  jushort old_index = old_method->get_java_ushort(old_bci+1);
  ConstantPool orig_cp = old_method->constants();
  jushort new_index = get_merged_pool_entry(&orig_cp, old_index JVM_CHECK);

  new_method->put_java_ushort(new_bci+1, new_index);

  // the nargs parameter
  jubyte n = old_method->ubyte_at(old_bci+3);
  new_method->ubyte_at_put(new_bci+3, n);

  // the zero parameter
  jubyte z = old_method->ubyte_at(old_bci+4);
  new_method->ubyte_at_put(new_bci+4, z);
}

void ConstantPoolRewriter::stream_multianewarray(Method *old_method,
                                                 Method *new_method,
                                                 int old_bci, int new_bci
                                                 JVM_TRAPS)
{
  // the bytecode
  const Bytecodes::Code bc = old_method->bytecode_at(old_bci);
  new_method->bytecode_at_put_raw(new_bci, bc);

  // the CP index
  jushort old_index = old_method->get_java_ushort(old_bci+1);
  ConstantPool orig_cp = old_method->constants();
  jushort new_index = get_merged_pool_entry(&orig_cp, old_index JVM_CHECK);

  new_method->put_java_ushort(new_bci+1, new_index);

  // the ndims parameter
  jubyte n = old_method->ubyte_at(old_bci+3);
  new_method->ubyte_at_put(new_bci+3, n);
}

// Copy a bytecode that takes a single 2-byte CP index as its operand.
// The index is in "Java" byte ordering.
void ConstantPoolRewriter::stream_java_cp_index(Method *old_method,
                                                Method *new_method,
                                                int old_bci, int new_bci
                                                JVM_TRAPS) {
  const Bytecodes::Code bc = old_method->bytecode_at(old_bci);
  new_method->bytecode_at_put_raw(new_bci, bc);

  jushort old_index = old_method->get_java_ushort(old_bci+1);
  ConstantPool orig_cp = old_method->constants();
  jushort new_index = get_merged_pool_entry(&orig_cp, old_index JVM_CHECK);

  new_method->put_java_ushort(new_bci+1, new_index);
}

// Stream the a branch byte code with a 2-byte operand
void ConstantPoolRewriter::stream_branch2(Method *old_method,
                                          Method *new_method,
                                          int old_bci, int new_bci) {
  const Bytecodes::Code bc = old_method->bytecode_at(old_bci);
  new_method->bytecode_at_put_raw(new_bci, bc);

  int old_target = old_bci + old_method->get_java_short(old_bci + 1);
  int new_target = _new_bytecode_address.ushort_at(old_target);
  GUARANTEE(0 <= new_target && new_target <= 0xffff, "sanity");

  int new_offset = new_target - new_bci;
  GUARANTEE(-0x8000 <= new_offset && new_offset <= 0x7fff, "sanity");
  new_method->put_java_short(new_bci + 1, new_offset);
}

// Stream the a branch byte code with a 4-byte operand
void ConstantPoolRewriter::stream_branch4(Method *old_method,
                                          Method *new_method,
                                          int old_bci, int new_bci) {
  const Bytecodes::Code bc = old_method->bytecode_at(old_bci);
  new_method->bytecode_at_put_raw(new_bci, bc);

  update_branch4(old_method, new_method, old_bci, new_bci,
                 old_bci + 1, new_bci+1);
}

// Stream the tableswitch bytecode. Returns the length of the bytecode
// in the new method.
int ConstantPoolRewriter::stream_tableswitch(Method *old_method,
                                             Method *new_method,
                                             int old_bci, int new_bci)
{
  const Bytecodes::Code bc = old_method->bytecode_at(old_bci);
  new_method->bytecode_at_put_raw(new_bci, bc);

  int old_aligned = align_size_up(old_bci + 1, sizeof(jint));
  int new_aligned = align_size_up(new_bci + 1, sizeof(jint));

  jint low       = old_method->get_java_switch_int(old_aligned + 4);
  jint high      = old_method->get_java_switch_int(old_aligned + 8);

  new_method->put_java_switch_int(new_aligned + 4, low);
  new_method->put_java_switch_int(new_aligned + 8, high);

  update_branch_switch(old_method, new_method, old_bci, new_bci,
                       old_aligned + 0, new_aligned + 0);

  for (int i = 0; i <= high - low; i++) {
    int loc = i * 4 + 12;
    update_branch_switch(old_method, new_method, old_bci, new_bci,
                         old_aligned + loc, new_aligned + loc);
  }

  int size = new_aligned + (3 + high - low + 1) * 4 - new_bci;
  return size;
}

// Stream the lookupswitch bytecode. Returns the length of the bytecode
// in the new method.
int ConstantPoolRewriter::stream_lookupswitch(Method *old_method,
                                              Method *new_method,
                                              int old_bci, int new_bci)
{
  const Bytecodes::Code bc = old_method->bytecode_at(old_bci);
  new_method->bytecode_at_put_raw(new_bci, bc);

  int old_aligned = align_size_up(old_bci + 1, sizeof(jint));
  int new_aligned = align_size_up(new_bci + 1, sizeof(jint));

  jint npairs = old_method->get_java_switch_int(old_aligned + 4);
  GUARANTEE(npairs >=0, "sanity");
  new_method->put_java_switch_int(new_aligned + 4, npairs);

  update_branch_switch(old_method, new_method, old_bci, new_bci,
                       old_aligned + 0, new_aligned + 0);

  for (int i=0; i<npairs; i++) {
    int loc = i * 8 + 8;

    jint match = old_method->get_java_switch_int(old_aligned + loc);
    new_method->put_java_switch_int(new_aligned + loc, match);

    update_branch_switch(old_method, new_method, old_bci, new_bci,
                   old_aligned + loc + 4, new_aligned + loc + 4);
  }

  int size = new_aligned + (2 + 2*npairs) * 4 - new_bci;
  return size;
}

void ConstantPoolRewriter::update_branch4(Method *old_method,
                                          Method *new_method,
                                          int old_bci, int new_bci,
                                          int old_loc, int new_loc)
{
  int old_target = old_bci + old_method->get_java_int(old_loc);
  int new_target = _new_bytecode_address.ushort_at(old_target);

  GUARANTEE(0 <= new_target && new_target <= 0xffff, "sanity");

  int new_offset = new_target - new_bci;
  GUARANTEE(-0x8000 <= new_offset && new_offset <= 0x7fff, "sanity");
  new_method->put_java_int(new_loc, new_offset);
}

void ConstantPoolRewriter::update_branch_switch(Method *old_method,
                                                Method *new_method,
                                                int old_bci, int new_bci,
                                                int old_loc, int new_loc)
{
  int old_target = old_bci + old_method->get_java_switch_int(old_loc);
  int new_target = _new_bytecode_address.ushort_at(old_target);
  GUARANTEE(0 <= new_target && new_target <= 0xffff, "sanity");

  int new_offset = new_target - new_bci;
  GUARANTEE(-0x8000 <= new_offset && new_offset <= 0x7fff, "sanity");
  new_method->put_java_switch_int(new_loc, new_offset);
}

void ConstantPoolRewriter::print_statistics(Stream *stream) {
#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR
  size_t cp_header   = align_allocation_size(sizeof(ConstantPoolDesc));
  size_t tags_header = align_allocation_size(sizeof(ConstantPoolDesc));

  stream->print_cr("[ConstantPool statistics]");
  stream->cr();

  int removed_pools = _original_pools_count - 1;
  int removed_pool_headers = removed_pools * cp_header;
  int removed_tags_headers = removed_pools * tags_header;
  int removed_entries =
      (_original_entries_count - _merged_entries_count) * (sizeof(int) + 1);
  int total = removed_pool_headers + removed_tags_headers
      + removed_entries - _method_size_delta;

  stream->print_cr("\t original pools   = %d", _original_pools_count);
  stream->print_cr("\t original entries = %d", _original_entries_count);
  stream->print_cr("\t   merged entries = %d", _merged_entries_count);
  stream->cr();

  stream->print_cr("\t removed pool headers             %6d bytes",
                       removed_pool_headers);
  stream->print_cr("\t removed tags headers           + %6d bytes",
                       removed_tags_headers);
  stream->print_cr("\t removed entries (tag + pool)   + %6d bytes",
                       removed_entries);
  stream->print_cr("\t method size reduction          + %6d bytes",
                     -_method_size_delta);
  stream->print_cr("\t ----------------------------------------------");
  stream->print_cr("\t total CP merge savings         = %6d bytes",
                       total);

  stream->cr();

  stream->print_cr("\t invalid          =%5d", _count_of_invalid);
  stream->print_cr("\t integer          =%5d", _count_of_integer);
  stream->print_cr("\t long             =%5d", _count_of_long);
  stream->print_cr("\t float            =%5d", _count_of_float);
  stream->print_cr("\t double           =%5d", _count_of_double);
  stream->print_cr("\t utf8             =%5d", _count_of_utf8);
  stream->print_cr("\t string           =%5d", _count_of_string);
  stream->print_cr("\t static_method    =%5d", _count_of_static_method);
  stream->print_cr("\t field            =%5d", _count_of_field_ref);
  stream->print_cr("\t interface_method =%5d", _count_of_interface_method_ref);
  stream->print_cr("\t virtual_method   =%5d", _count_of_virtual_method_ref);
  stream->print_cr("\t class            =%5d", _count_of_class);

  stream->cr();
  stream->print_cr("[Optimized bytecodes]");
  stream->cr();
#if !ENABLE_CPU_VARIANT
  _bytecode_optimizer.print_bytecode_statistics(stream);
#endif
#else
  (void)stream;
#endif
}

void ConstantPoolRewriter::init_method_map(JVM_SINGLE_ARG_TRAPS) {
  _max_method_length = 0;
  int method_count = 0;
  UsingFastOops level1;
  InstanceClass::Fast klass;
  ObjArray::Fast methods;  
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();    
    methods = klass().methods();
    method_count += methods().length();
  }

  method_map_array = Universe::new_obj_array(method_count JVM_CHECK); //hash set size shall be about number of elements
  TypeArray::Fast sizes = Universe::new_int_array(method_count JVM_CHECK); 

  //we shall count number of all methods with given hashcode in order to avoid dynamic allocation in future!
  //we iterating through all methods, fir which rewrite_method will be executed!!!
  Method::Fast method;
  ClassInfo::Fast info;
  for (SystemClassStream st1; st1.has_next();) {
    klass = st1.next();    
    methods = klass().methods();
    for (int i = 0 ; i < methods().length(); i++) {
      method = methods().obj_at(i);
      if (method.not_null()) {
        unsigned int code = (unsigned int)hashcode_for_method(&method) % method_count;
        sizes().int_at_put(code, sizes().int_at(code) + 1);        
        if (method().code_size() > _max_method_length) {
          _max_method_length = method().code_size();
        }
      }      
    }
    
    //looking for error methods
    ErrorMethodCounter error_method_counter(&sizes);
    info = klass().class_info();
    error_method_counter.set_obj(&info);
    info().iterate_tables(&error_method_counter);
  }

  ObjArray::Fast bucket;
  for (int i = 0; i < method_count; i++) {
    if (sizes().int_at(i) > 0) {
      //structure of bucket is pairs bucket[2*i] is old_method, bucket[2*i + 1] is new_method
      bucket = Universe::new_obj_array(2 * sizes().int_at(i) JVM_CHECK);
      method_map_array.obj_at_put(i, bucket);
    }
  }    
}

#endif // ENABLE_ROM_GENERATOR
