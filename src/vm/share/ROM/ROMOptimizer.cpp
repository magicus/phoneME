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
#include "incls/_ROMOptimizer.cpp.incl"

#if ENABLE_ROM_GENERATOR

// Define all static fields of ROMOptimizer
ROMOPTIMIZER_INT_FIELDS_DO(ROMOPTIMIZER_DEFINE_INT)
OopDesc* ROMOptimizer::_romoptimizer_oops[ROMOptimizer::_number_of_oop_fields];
int ROMOptimizer::_time_counters[ROMOptimizer::STATE_COUNT];

void ROMOptimizer::optimize(Stream *log_stream JVM_TRAPS) {
  _log_stream = log_stream;

  do {
    int last_state = state();
    jlong started_ms = Os::java_time_millis();

    switch (state()) {
    case STATE_MAKE_RESTRICTED_PACKAGES_FINAL:
      if (MakeRestrictedPackagesFinal) {
        // Optimize classes in restricted packages (no new classes
        // will be defined in the restricted packages at run-time).
        make_restricted_packages_final(JVM_SINGLE_ARG_CHECK);
        make_restricted_methods_final(JVM_SINGLE_ARG_CHECK);
      }

#if USE_ROM_LOGGING
      log_non_restricted_packages();
#endif
      set_next_state();
      break;

    case STATE_INITIALIZE_CLASSES:
      // Optimize the layout of all classes, constant pools and methods
      if (EnableBaseOptimizations) {
        initialize_classes(JVM_SINGLE_ARG_CHECK);
      }
      //we do not need set_next_state() call here - it is inside initialize_classes
      break;

    case STATE_QUICKEN_METHODS:
      if (EnableBaseOptimizations) {
        quicken_methods(JVM_SINGLE_ARG_CHECK);
      }
      set_next_state();
      break;

    case STATE_RESOLVE_CONSTANT_POOL:
      if (EnableBaseOptimizations && ENABLE_REFLECTION) {
        resolve_constant_pool(JVM_SINGLE_ARG_CHECK);
      }
      set_next_state();
      break;

    case STATE_REMOVE_REDUNDATE_STACKMAPS:
#if USE_SOURCE_IMAGE_GENERATOR
      if (EnableBaseOptimizations && RemoveDuplicatedROMStackmaps) {
        // remove redundant entries in StackmapList objects
        remove_redundant_stackmaps(JVM_SINGLE_ARG_CHECK);
      }
#endif
      set_next_state();
      break;

    case STATE_MERGE_STRING_BODIES:
#if USE_SOURCE_IMAGE_GENERATOR
      // Merge all string bodies into a single char array.
      merge_string_bodies(JVM_SINGLE_ARG_CHECK);
#endif
      set_next_state();
      break;

    case STATE_RESIZE_CLASS_LIST:
      ROMWriter::set_number_of_romized_java_classes();
      // Make sure we can do a fair number of class loading without
      // expanding Universe::class_list().
      resize_class_list(JVM_SINGLE_ARG_CHECK);
      set_next_state();
      break;

    case STATE_REPLACE_EMPTY_ARRAYS:
      // Even if base optimizations are turned off, cycle through and replace
      // Universe::empty_xxx_array with our local copy so it can go into
      // text section.
      // IMPL_NOTE: yet another loop through all the classes.
      // Need to combine loops.  Would require change to logging output
      replace_empty_arrays();
      set_next_state();
      break;

    case STATE_INLINE_METHODS:
      // inline all short methods
      if (EnableBaseOptimizations && SimpleROMInliner) {
        inline_exception_constructors();
        inline_short_methods(JVM_SINGLE_ARG_CHECK);
      }
      set_next_state();
      break;

    case STATE_OPTIMIZE_FAST_ACCESSORS:
      // Remove bytecodes from fast accessor methods. Do this after ROM inliner
      // so that fast get bytecodes may be inlined into callers.
      if (EnableBaseOptimizations) {
        optimize_fast_accessors(JVM_SINGLE_ARG_CHECK);
      }
      set_next_state();
      break;

    case STATE_REMOVE_DEAD_METHODS:
#if USE_SOURCE_IMAGE_GENERATOR
      // Too expensive for Monet.
      // remove dead methods by computing closure of all externally reachable 
      // methods and nullifying all others
      if (EnableBaseOptimizations && RemoveDeadMethods) {
        remove_dead_methods(JVM_SINGLE_ARG_CHECK);
      }
#endif
      set_next_state();
      break;

    case STATE_RENAME_NON_PUBLIC_SYMBOLS:
      // Change the name of non-public field/methods to .unknown.
      if (RenameNonPublicROMSymbols) {
        // Note: the RenameNonPublicROMSymbols option should be used with
        // care, and is disabled by default. If you want to use it, make
        // sure you use DontRenameNonPublicFields in your romconfig file to
        // exclude all classes accessed by KNI_GetFieldID.
#if USE_SOURCE_IMAGE_GENERATOR
        rename_non_public_symbols(JVM_SINGLE_ARG_CHECK);
#endif
      }
      set_next_state();
      break;

    case STATE_REMOVE_UNUSED_STATIC_FIELDS:
#if USE_SOURCE_IMAGE_GENERATOR
      // Remove unused static fields (such as non-public static final ints)
      if (RemoveROMUnusedStaticFields) {
        remove_unused_static_fields(JVM_SINGLE_ARG_CHECK);
      }
#endif
      set_next_state();
      break;

    case STATE_COMPACT_FIELD_TABLES:
#if USE_SOURCE_IMAGE_GENERATOR
      // Remove useless fields from the Java class field tables.
      // All used fields must be resolved before this optimization, 
      // so we require EnableBaseOptimizations option for it.
      if (EnableBaseOptimizations && CompactROMFieldTables) {
        // IMPL_NOTE: This optimization is disabled for Monet.
        compact_field_tables(JVM_SINGLE_ARG_CHECK);
      }
#endif
      set_next_state();
      break;

    case STATE_REMOVE_UNUSED_SYMBOLS:
#if USE_SOURCE_IMAGE_GENERATOR
      // Initialize the layout of the ROM hashtables
      // While we write all symbols and string to the system ROM image,
      // an application ROM image should only contain the symbols and strings
      // referenced from that application.

      // Remove all Symbols that are not used by any of the romized
      // classes.  For application conversion, only symbols reachable
      // from the application are written to the ROM image (see CR id
      // 6190703), so this optimization makes sense only for
      // GenerateSystemROMImage.
      if (RemoveUnusedSymbols) {
        remove_unused_symbols(JVM_SINGLE_ARG_CHECK);
      }
#endif
      set_next_state();
      break;

    case STATE_REWRITE_CONSTANT_POOLS:
      // Constant-pool rewriting must be done after method quickening and after
      // constant pools have been fully resolved.
      if (EnableBaseOptimizations && RewriteROMConstantPool) {
        ROMHashtableManager hashtab_mgr;
        hashtab_mgr.initialize(SymbolTable::current(), StringTable::current()
                                JVM_CHECK);
        ConstantPoolRewriter cp_rewriter(this, _log_stream, &hashtab_mgr);
        cp_rewriter.rewrite(JVM_SINGLE_ARG_CHECK);
        cp_rewriter.print_statistics(log_stream);

        *string_table()          = hashtab_mgr.string_table();
        *symbol_table()          = hashtab_mgr.symbol_table();
        *embedded_table_holder() = hashtab_mgr.embedded_table_holder();
        _embedded_symbols_offset = hashtab_mgr.embedded_symbols_offset();
        _embedded_strings_offset = hashtab_mgr.embedded_strings_offset();
      }
      set_next_state();
      break;

    case STATE_COMPACT_TABLES:
      // Remove useless fields from the Java class field tables. This
      // should be done after RewriteROMConstantPool --
      // ConstantPoolRewriter requires that all methods be included in
      // at least one InstanceClass::methods array.
      if (CompactROMMethodTables) {
        compact_method_tables(JVM_SINGLE_ARG_CHECK);
        compact_interface_classes(JVM_SINGLE_ARG_CHECK);
      }
      set_next_state();
      break;

    case STATE_PRECOMPILE_METHODS:
      if (EnableROMCompilation) { 
        // Must do it here, after constant pool rewriter and method inliner
        // have done modifying the romized bytecodes.
        precompile_methods(JVM_SINGLE_ARG_CHECK);
      }
      set_next_state();
      break;

    case STATE_REMOVE_DUPLICATED_OBJECTS:
      // Remove duplicated arrays and StackmapList's that have the exact 
      // same contents.
      if (USE_SOURCE_IMAGE_GENERATOR) {
        // Can't iterate through heap since objects may be owned by 
        // different tasks
        remove_duplicated_objects(JVM_SINGLE_ARG_CHECK);
      }
      set_next_state();
      break;

    case STATE_MARK_HIDDEN_CLASSES:
      // mark hidden classes as such
      mark_hidden_classes(JVM_SINGLE_ARG_CHECK);
      set_next_state();
      break;

    default:
      SHOULD_NOT_REACH_HERE();
    }

    jlong end_ms = Os::java_time_millis();
    _time_counters[last_state] += (int)(end_ms - started_ms);
  } while (!is_done() && !ROMWriter::work_timer_has_expired());

#if USE_ROM_LOGGING
  if (is_done()) {
    log_time_counters();
  }
#endif
}

#if USE_ROM_LOGGING
void ROMOptimizer::log_time_counters() {
#define PRINT_TIME_COUNTER(x) \
        _log_stream->print_cr("    ROMOptimizer[%-36s] =%6d ms", \
                              STR(x), _time_counters[x]); \
        total += _time_counters[x]

  int total = 0;
  _log_stream->print_cr("[ROMOptimizer timings]");
  PRINT_TIME_COUNTER(STATE_MAKE_RESTRICTED_PACKAGES_FINAL);
  PRINT_TIME_COUNTER(STATE_INITIALIZE_CLASSES);
  PRINT_TIME_COUNTER(STATE_QUICKEN_METHODS);
  PRINT_TIME_COUNTER(STATE_RESOLVE_CONSTANT_POOL);
  PRINT_TIME_COUNTER(STATE_REMOVE_REDUNDATE_STACKMAPS);
  PRINT_TIME_COUNTER(STATE_MERGE_STRING_BODIES);
  PRINT_TIME_COUNTER(STATE_RESIZE_CLASS_LIST);
  PRINT_TIME_COUNTER(STATE_REPLACE_EMPTY_ARRAYS);
  PRINT_TIME_COUNTER(STATE_INLINE_METHODS);
  PRINT_TIME_COUNTER(STATE_OPTIMIZE_FAST_ACCESSORS);
  PRINT_TIME_COUNTER(STATE_REMOVE_DEAD_METHODS);
  PRINT_TIME_COUNTER(STATE_RENAME_NON_PUBLIC_SYMBOLS);
  PRINT_TIME_COUNTER(STATE_REMOVE_UNUSED_STATIC_FIELDS);
  PRINT_TIME_COUNTER(STATE_COMPACT_FIELD_TABLES);
  PRINT_TIME_COUNTER(STATE_REMOVE_UNUSED_SYMBOLS);
  PRINT_TIME_COUNTER(STATE_REWRITE_CONSTANT_POOLS);
  PRINT_TIME_COUNTER(STATE_COMPACT_TABLES);
  PRINT_TIME_COUNTER(STATE_PRECOMPILE_METHODS);
  PRINT_TIME_COUNTER(STATE_REMOVE_DUPLICATED_OBJECTS);
  PRINT_TIME_COUNTER(STATE_MARK_HIDDEN_CLASSES);

  _log_stream->print_cr("    ROMOptimizer[%-36s] =%6d ms", "*total*", total);
#undef PRINT_TIME_COUNTER
}
#endif

void ROMOptimizer::mark_hidden_classes(JVM_SINGLE_ARG_TRAPS) {
#if !USE_SOURCE_IMAGE_GENERATOR
  JVM_IGNORE_TRAPS;
#else
   ROMVector log_vector;
#if USE_ROM_LOGGING
   log_vector.initialize(JVM_SINGLE_ARG_CHECK);
   
   _log_stream->cr();
   _log_stream->print_cr("[Classes marked as 'hidden']");
   _log_stream->cr();
#endif

  for (SystemClassStream st; st.has_next();) {
    UsingFastOops fast_oops;
    InstanceClass::Fast klass = st.next();
    AccessFlags flags = klass().access_flags();
    bool hidden = is_in_hidden_package(&klass JVM_CHECK);
    if (hidden) {
      flags.set_is_hidden();
      klass().set_access_flags(flags);
#if USE_ROM_LOGGING
      log_vector.add_element(&klass JVM_CHECK);
#endif
    }
  }
  
#if USE_ROM_LOGGING
  // Print the results
  log_vector.sort();
  for (int i=0; i<log_vector.size(); i++) {
    InstanceClass::Raw klass = log_vector.element_at(i);
    klass().print_name_on(_log_stream);
    _log_stream->cr();
  }
#endif

#endif
}

void ROMOptimizer::oops_do(void do_oop(OopDesc**)) {
  for (int i=_number_of_oop_fields-1; i>=0; i--) {
    do_oop(&_romoptimizer_oops[i]);
  }
}

void ROMOptimizer::init_handles() {
  jvm_memset(_romoptimizer_oops, 0, sizeof(_romoptimizer_oops));
}

void ROMOptimizer::initialize(Stream *log_stream JVM_TRAPS) {
  _log_stream = log_stream;
  set_state(STATE_FIRST_STATE);

  jvm_memset(_time_counters, 0, sizeof(int)*STATE_COUNT);

#if USE_AOT_COMPILATION
  precompile_method_list()->initialize(JVM_SINGLE_ARG_CHECK);
#endif

#if USE_SOURCE_IMAGE_GENERATOR
  // These operations are not supported by Monet:
  // - ROM configuration files
  // - original names (symbols are not renamed in Monet non-product modes)

  int cnt = Universe::class_list()->length() + 20;
  int alt_const_count = get_max_alternate_constant_pool_count();

  *init_at_build_classes()      = Universe::new_obj_array(cnt JVM_CHECK);
  *init_at_load_classes()       = Universe::new_obj_array(cnt JVM_CHECK);
  *dont_rename_fields_classes() = Universe::new_obj_array(cnt JVM_CHECK);
  *dont_rename_methods_classes()= Universe::new_obj_array(cnt JVM_CHECK);
  *dont_rename_classes()        = Universe::new_obj_array(cnt JVM_CHECK);
                                                               
  hidden_packages()->initialize(JVM_SINGLE_ARG_CHECK);
  restricted_packages()->initialize(JVM_SINGLE_ARG_CHECK);
  reserved_words()->initialize(JVM_SINGLE_ARG_CHECK);

  *romizer_original_class_name_list() = Universe::new_obj_array(cnt JVM_CHECK);
  *romizer_original_method_info()     = Universe::new_obj_array(cnt JVM_CHECK);
  *romizer_original_fields_list()     = Universe::new_obj_array(cnt JVM_CHECK);
  *romizer_alternate_constant_pool()  = Universe::new_constant_pool(
                                               alt_const_count JVM_CHECK);

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  profiles_vector()->initialize(JVM_SINGLE_ARG_CHECK);  
#endif
  read_config_file(JVM_SINGLE_ARG_CHECK);

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  create_profiles_hidden_bitmap(JVM_SINGLE_ARG_CHECK);
#endif

#endif // USE_SOURCE_IMAGE_GENERATOR

  allocate_empty_arrays(JVM_SINGLE_ARG_CHECK);
  initialize_subclasses_cache(JVM_SINGLE_ARG_CHECK);
}

// IMPL_NOTE: do we need this for Monet?
void ROMOptimizer::allocate_empty_arrays(JVM_SINGLE_ARG_TRAPS) {
  // Many objects contains a constant array which can be safely placed
  // in the TEXT block. E.g., InstanceClass::fields().
  //
  // In some cases, this array is empty and it's also stored in
  // Universe::empty_<type>_array(). Since Universe::empty_<type>_array()
  // may potentially be used for other purposes, it's better to leave
  // it in the HEAP block. Thus, we create a copy of the empty array
  // and place it in the TEXT block.

  *empty_short_array()= Universe::allocate_array(Universe::short_array_class(),
                                                0, sizeof(jshort) JVM_CHECK);
  *empty_obj_array()  = Universe::new_obj_array(Universe::object_class(), 0 
                                               JVM_CHECK);

#if USE_SOURCE_IMAGE_GENERATOR
  // This Universe handle is used in Monet images. YES, this should be done
  // only when we're generating the system ROM image.
  *Universe::rom_text_empty_obj_array() = empty_obj_array();
#endif

  GUARANTEE(!empty_obj_array()->equals(Universe::empty_obj_array()),
            "Universe::empty_obj_array may not be placed in TEXT");
}

// If a non-public class is in a restricted package, and it has no
// subclasses, make this class 'final'. This makes it possible to
// switch some invokevirtual bytecodes to the faster
// invokevirtual_final, which will be done later in quicken_methods().
void ROMOptimizer::make_restricted_packages_final(JVM_SINGLE_ARG_TRAPS) {
#if USE_ROM_LOGGING
  ROMVector log_vector;
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);

  _log_stream->cr();
  _log_stream->print_cr("[Classes made 'final']");
  _log_stream->cr();
#endif

  for (SystemClassStream st; st.has_next();) {
    UsingFastOops fast_oops;
    InstanceClass::Fast klass = st.next();
    AccessFlags flags = klass().access_flags();
    if (!flags.is_final() && !has_subclasses(&klass)) {
      bool make_final = false;
      bool hidden = is_in_hidden_package(&klass JVM_CHECK);
      if (hidden) {        
        make_final = true;
      } else if (is_in_restricted_package(&klass) && !flags.is_public()) {
        make_final = true;
      }

      if (make_final) {
        flags.set_is_final();
        klass().set_access_flags(flags);
#if USE_ROM_LOGGING
        log_vector.add_element(&klass JVM_CHECK);
#endif
      }
    }
  }

#if USE_ROM_LOGGING
  // Print the results
  log_vector.sort();
  for (int i=0; i<log_vector.size(); i++) {
    InstanceClass::Raw klass = log_vector.element_at(i);
    _log_stream->print("made class final: ");
    klass().print_name_on(_log_stream);
    _log_stream->cr();
  }
#endif
}

void ROMOptimizer::make_restricted_methods_final(JVM_SINGLE_ARG_TRAPS) {
  ROMVector log_vector;
#if USE_ROM_LOGGING
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);

  _log_stream->cr();
  _log_stream->print_cr("[Methods made 'final']");
  _log_stream->cr();
#endif

  for (SystemClassStream st; st.has_next();) {
    UsingFastOops fast_oops;
    InstanceClass::Fast klass = st.next();
    AccessFlags flags = klass().access_flags();

    if (!flags.is_final()) {
          bool hidden = is_in_hidden_package(&klass JVM_CHECK);
      if (hidden || is_in_restricted_package(&klass)) {
        make_virtual_methods_final(&klass, &log_vector JVM_CHECK);
      }
    }
  }

#if USE_ROM_LOGGING
  // Print the results
  log_vector.sort();
  for (int i=0; i<log_vector.size(); i++) {
    Method::Raw method = log_vector.element_at(i);
    _log_stream->print("made method final: ");
    method().print_name_on(_log_stream);
    _log_stream->cr();
  }
#endif
}


// Is the given method overridden in any subclass of ic?
bool ROMOptimizer::is_overridden(InstanceClass *ic, Method *method) {
  int vtable_index = method->vtable_index();
  if (vtable_index < 0) {
    // This is a final method, so it's not overridden (and is not
    // stored in the vtable).
    //
    // We shouldn't come to here, but just in case ...
    return true;
  }

  TypeArray::Raw subclass_id_array = get_subclass_list(ic->class_id());
  InstanceClass::Raw klass;
  ClassInfo::Raw info;
  Method::Raw m;
  for (int idx = 0; idx < subclass_id_array().length(); idx++) {
    klass = Universe::class_from_id(subclass_id_array().short_at(idx));
    info = klass().class_info();    
    m = info().vtable_method_at(vtable_index);
    if (!method->equals(&m)) {
      return true; // This method has been overridden in a subclass
    }    
  }
  
  return false;
}

void ROMOptimizer::make_virtual_methods_final(InstanceClass *ic, 
                                              ROMVector *log_vector
                                              JVM_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast methods = ic->methods();
  jint package_flags = get_package_flags(ic JVM_CHECK);
  AccessFlags class_flags = ic->access_flags();

  Method::Fast method;
  for (int i=0; i<methods().length(); i++) {
    
    method = methods().obj_at(i);
    if (method.is_null()) {
      continue;
    }
    if (method().is_object_initializer()) {
      // Not really a virtual method
      continue;
    }
    if (method().is_static()) {
      continue;
    }
    if (method().is_final()) {
      // Nothing to do
      continue;
    }

    AccessFlags method_flags = method().access_flags();
    if (is_member_reachable_by_apps(package_flags, class_flags, method_flags)){
      // This method may be overridden by application classes. Don't mark it
      // final
      continue;
    }

    if (!is_overridden(ic, &method)) {
      AccessFlags flags = method().access_flags();
      flags.set_is_final();
      method().set_access_flags(flags);

#if USE_ROM_LOGGING
      log_vector->add_element(&method JVM_CHECK);
#else
      (void)log_vector;
#endif
    }
  }
}

bool ROMOptimizer::is_in_restricted_package(InstanceClass *klass) {
#if USE_SOURCE_IMAGE_GENERATOR
  SETUP_ERROR_CHECKER_ARG; // IMPL_NOTE: consider whether this should be fixed 
  return class_matches_packages_list(klass, restricted_packages()
                                     JVM_NO_CHECK_AT_BOTTOM);
#else
  // IMPL_NOTE: Monet: all classes can be considered as restricted
  (void)klass;
  return false;
#endif
}

bool ROMOptimizer::is_in_hidden_package(InstanceClass *klass JVM_TRAPS) {
#if USE_SOURCE_IMAGE_GENERATOR
  return class_matches_packages_list(klass, hidden_packages()
                                     JVM_NO_CHECK_AT_BOTTOM);
#else
  (void)klass;
  return false;
#endif
}

bool ROMOptimizer::has_subclasses(InstanceClass *klass) {
  for (SystemClassStream st; st.has_next();) {
    InstanceClass::Raw ic = st.next();
    InstanceClass::Raw super = ic().super();
    if (super.not_null() && super.equals(klass)) {
      // klass has at least one subclass
      return true;
    }
  }
  return false;
}

#if USE_ROM_LOGGING
void ROMOptimizer::log_non_restricted_packages() {
  _log_stream->cr();
  _log_stream->print_cr("[classes in non-restricted packages]");
  _log_stream->cr();

  UsingFastOops fast_oops;
  InstanceClass::Fast klass;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    if (!is_in_restricted_package(&klass)) {
      klass().print_name_on(_log_stream);
      _log_stream->cr();
    }
  }
}
#endif

void ROMOptimizer::initialize_classes(JVM_SINGLE_ARG_TRAPS) {
  // Initialize all classes that may be initialized (e.g., if
  // a class has no <clinit> method, or if it's specified by
  // the ROM configuration file.)
  //
  // The order of initialization is important -- a class may be
  // initialized only if its superclass has been initialized.

  bool made_progress = false;

  InstanceClass klass;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    if (may_be_initialized(&klass)) {
      made_progress = true;
      klass.initialize(JVM_SINGLE_ARG_CHECK);
      if (TraceRomizer) {
        TTY_TRACE(("Initializing class: "));
        klass.print_name_on(tty);
        TTY_TRACE_CR((""));        
      }
#if USE_SOURCE_IMAGE_GENERATOR
      if (is_init_at_build(&klass)) {
              Universe::record_inited_at_build(&klass JVM_CHECK);
      }
#endif
    }
  }

  if (made_progress) {
    // We have added some pending entries. Now we will return to
    // Java-land to execute these entries to verify and initialize the
    // classes. ROMOptimizer::initialize_classes() will be called again
    // by the loop in the Java method com.sun.cldc.jvm.JVM.generateRomImage().
    ROMWriter::suspend();
  } else {
    // We have initialized all classes that may be initialized
#if USE_ROM_LOGGING && USE_SOURCE_IMAGE_GENERATOR
    // Print out diagnostic log message to say what classes are not
    // initialized, and why.
    print_class_initialization_log(JVM_SINGLE_ARG_CHECK);
#endif
    set_next_state();
  }
}

#if USE_ROM_LOGGING
void ROMOptimizer::print_class_initialization_log(JVM_SINGLE_ARG_TRAPS) {
  ROMVector log_vector;
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);
  int i;

  _log_stream->cr();
  _log_stream->print_cr("[Classes initialized at build time]");
  _log_stream->cr();

  UsingFastOops level1;  
  InstanceClass::Fast klass;
  for (i=0; i<init_at_build_classes()->length(); i++) {
    klass = init_at_build_classes()->obj_at(i);    
    if (klass.is_null()) {
      break;
    } else if (klass().is_initialized()) {
      log_vector.add_element(&klass JVM_CHECK);
    }
  }

  log_vector.sort();
  for (i=0; i<log_vector.size(); i++) {
    InstanceClass::Raw klass = log_vector.element_at(i);
    _log_stream->print("init at build: ");
#if USE_PRODUCT_BINARY_IMAGE_GENERATOR
    // IMPL_NOTE: debug info
    // Don't have klass->print_name_on so do it "by hand"
    Symbol::Raw name = klass().name();
    ClassInfo::Raw ci = klass().class_info();
    if (name.equals(Symbols::unknown())) {
      name = ROM::get_original_class_name(&ci);
    }
    if (name.not_null()) {
      name().print_symbol_on(_log_stream);
      _log_stream->cr();
    } else {
      _log_stream->print_cr("NULL");
    }
#else
    klass().print_name_on(_log_stream);
    _log_stream->cr();
#endif
  }

  _log_stream->cr();
  _log_stream->print_cr("[Uninitialized Classes]");
  _log_stream->cr();
  
  Method::Fast init;
  InstanceClass::Fast ic;
  TypeArray::Fast interfaces;
  InstanceClass::Fast intf;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    if (!klass().is_initialized()) {
      klass().print_name_on(_log_stream);
      _log_stream->cr();

      init = klass().find_local_method(Symbols::class_initializer_name(),
                                            Symbols::void_signature());
      if (!init.is_null()) {
        _log_stream->print_cr("\t-> <clinit> not executed (%d bytes)",
                             init().code_size());
      }
      
      for (ic = klass.obj(); !ic.is_null(); ic = ic().super()) {
        if (!ic.equals(&klass) && !ic().is_initialized()) {
          _log_stream->print("\t-> uninitialized super class ");
          ic().print_name_on(_log_stream);
          _log_stream->cr();
        }

        interfaces = ic().local_interfaces();
        int n_interfaces = interfaces().length();

        for (int i = 0; i < n_interfaces; i++) {
          int intf_id = interfaces().ushort_at(i);
          intf = Universe::class_from_id(intf_id);
          if (!intf().is_initialized()) {
            _log_stream->print("\t-> uninitialized super interface ");
            intf().print_name_on(_log_stream);
            _log_stream->cr();
          }
        }
      }
      _log_stream->cr();
    }
  }
}
#endif

bool ROMOptimizer::may_be_initialized(InstanceClass *klass) {
  if (klass->is_initialized()) {
    return false;
  }

  if (PostponeErrorsUntilRuntime) {
    // During romization we forcibly verify all classes as they are loaded, 
    // so the class can be non-verified at this point only if it failed
    // verification.
    if (!klass->is_verified()) {
      return false;
    }
  }
  GUARANTEE(klass->is_verified(), "Sanity");

  Method::Raw init = klass->find_local_method(Symbols::class_initializer_name(),
                                         Symbols::void_signature());
  if (!init.is_null()) {
#if USE_SOURCE_IMAGE_GENERATOR
    if (!is_init_at_build(klass)) {
      return false;
    }
#else
    return false;
#endif
  }

  // All super classes and super interfaces must be initialized
  UsingFastOops level1;
  InstanceClass::Fast ic;
  TypeArray::Fast interfaces;
  InstanceClass::Fast intf;
  for (ic = klass->obj(); !ic.is_null(); ic = ic().super()) {
    if (!ic.equals(klass) && !ic().is_initialized()) {
      return false;
    }

    interfaces = ic().local_interfaces();
    int n_interfaces = interfaces().length();

    for (int i = 0; i < n_interfaces; i++) {
      int intf_id = interfaces().ushort_at(i);
      intf = Universe::class_from_id(intf_id);
      if (!intf().is_initialized()) {
        return false;
      }
    }
  }

  return true;
}

// Turn on the JVM_ACC_PRELOADED flags of all romized system classes 
// and the JVM_ACC_CONVERTED flags of all romized application classes 
// (for some optimizations in the compiler)
void ROMOptimizer::set_classes_as_romized() {
  for (int i = 0; i < Universe::number_of_java_classes(); i++) {
    JavaClass::Raw klass = Universe::class_from_id(i);
    AccessFlags flags = klass().access_flags();
    if (!flags.is_romized()) {
      // need to check first, sinc we may be running the binary romizer,
      // in which case some classes may already live in ROM and can't be
      // (and don't need to be) modified.
#if ENABLE_MONET
      if (!EnableBaseOptimizations) {
      // At run-time we use JVM_ACC_NON_OPTIMIZABLE flag to indicate that
      // romizer didn't apply some optimimizations to this class 
      // (see ConstantPool::check_quickened_field_access()).
      // Since we don't optimize any classes when EnableBaseOptimizations 
      // is not set, we mark all classes as non-optimizable.
        flags.set_is_non_optimizable();
      }
      flags.set_is_converted();
#else
      flags.set_is_preloaded();
#endif
      klass().set_access_flags(flags);
    }
  }
}

// Try to quicken bytecodes in all methods in all classes.
void ROMOptimizer::quicken_methods(JVM_SINGLE_ARG_TRAPS) {
  int qcount = 0;
  int mcount = 0;
  // The class is marked non-optimizable if we found some failures while
  // quickening its methods. In this case we disable some ROM
  // optimizations on this class, which can hidden the failure, so that 
  // we can detect it and report properly at run-time.
  bool optimizableClassFound = false;

#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print_cr("[Unquickened Methods]");
  _log_stream->cr();
#endif

#if USE_SOURCE_IMAGE_GENERATOR || (ENABLE_MONET && !ENABLE_LIB_IMAGES) 
  *interface_implementation_cache() = 
        Universe::new_int_array(Universe::number_of_java_classes() JVM_CHECK);
  *direct_interface_implementation_cache() = 
        Universe::new_int_array(Universe::number_of_java_classes() JVM_CHECK);
  fill_interface_implementation_cache(JVM_SINGLE_ARG_CHECK);
#endif
  UsingFastOops level1;
  ObjArray::Fast methods;
  Method::Fast method;
  InstanceClass::Fast klass;

#if USE_BINARY_IMAGE_GENERATOR && USE_AOT_COMPILATION
  TypeArray::Fast buffer = Universe::new_byte_array(1024 JVM_CHECK);
#endif

  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    bool class_is_verified = klass().is_verified();

    // We must have been made an attempt to verify the class already, so at
    // at this point the class is either verified successfully or it failed 
    // verification and we postpone the verification error until runtime.
    GUARANTEE(PostponeErrorsUntilRuntime || class_is_verified, "Sanity");

#ifndef PRODUCT
    if (TraceRomizer) {
      TTY_TRACE(("Quickening class: "));
      klass().print_name_on(tty);
      TTY_TRACE_CR((""));
    }
#endif

    methods = klass().methods();

    for (int i=0; i<methods().length(); i++) {
#if ENABLE_LIB_IMAGES && ENABLE_MONET
      OopDesc* method_ptr = methods().obj_at(i);
      if (ROM::in_any_loaded_bundle( method_ptr ) || 
          (ObjectHeap::contains(method_ptr) && method_ptr <= ROM::romized_heap_marker())) {
        continue;
      }
#endif

      method = methods().obj_at(i);
      
      if (method.is_null()) {
        // A nulled-out <clinit> method
        continue;
      }
    
#if USE_BINARY_IMAGE_GENERATOR && USE_AOT_COMPILATION
      {
        UsingFastOops level2;
        Symbol::Fast method_name = method().name();
        Signature::Fast signature = method().signature();
        InstanceClass::Fast holder = method().holder();
        Symbol::Fast holder_name = holder().name();

        jboolean precompile = KNI_FALSE;

        {
          AllocationDisabler raw_pointers_used_in_this_block;

          char * buffer_data = (char*)buffer().data();

          FixedArrayOutputStream stream(buffer_data,
                                        buffer().length());  

          signature().print_decoded_on(&stream);

          precompile = 
            JVMSPI_IsPrecompilationTarget(holder_name().utf8_data(), 
                                          holder_name().length(),
                                          method_name().utf8_data(), 
                                          method_name().length(),
                                          stream.array(), 
                                          stream.current_size(),
                                          method().code_size());
        }
        
        if (precompile) {
          precompile_method_list()->add_element(&method JVM_CHECK);
        }
      }
#endif

      if (method().is_abstract()) {
        // By setting this flag, we won't need to include a variable
        // part of this method in the DATA section.
        method().set_impossible_to_compile();
      }

      if (method().access_flags().is_synchronized()) {
        if (!ROM::is_synchronized_method_allowed(&method)) {
          tty->print_cr("Fatal error: romizer does not support synchronized "
                        "instance methods in String or Object class:");
#ifndef PRODUCT
          method.print_value_on(tty);
#endif
          tty->cr();
          JVM::exit(0);
        }
      }

      // Do not quicken methods in a class that failed verification.
      // We are going to reverify this class at run-time to get the
      // verification failure reported, but quickening may change CP
      // contents and prevent verifier from working properly on this class.
      if (class_is_verified) {
        bool q = quicken_one_method(&method JVM_CHECK);
        if (q) {
          qcount ++;
        }
      }
      mcount ++;
    }

    if (klass().is_optimizable()) {
      optimizableClassFound = true;
    }
  }

  // If no optimizable class found, noone will reference the merged CP and 
  // we disable embedding ROM hashtables into the merged CP to keep it empty.
  if (!optimizableClassFound) {
    // No optimizable class found means we have found some errors in all classes,
    // so we cannot reach this point unless we postpone errors until runtime.
    GUARANTEE(PostponeErrorsUntilRuntime, "sanity");
#ifndef PRODUCT
    tty->print_cr("Warning: EmbeddedROMHashTables is disabled.");
#endif    
    EmbeddedROMHashTables = false;
  }
#if USE_SOURCE_IMAGE_GENERATOR || (ENABLE_MONET && !ENABLE_LIB_IMAGES) 
  interface_implementation_cache()->set_null();
  direct_interface_implementation_cache()->set_null();
#endif
#if USE_ROM_LOGGING
  if (mcount) {
    int percent = (qcount * 100) / mcount;
    _log_stream->print_cr("Fully quickened bytecodes in %d of %d methods "
                          "(%d %%)",
                          qcount, mcount, percent);
    if (percent != 100) {
      _log_stream->print_cr("... see ROMLog.txt for details");
    }
  }
#endif
  // IMPL_NOTE: on a final pass, quicken all 'static' bytecodes in <clinit>
  // methods that operate on the current class.
}
#if USE_SOURCE_IMAGE_GENERATOR || (ENABLE_MONET && !ENABLE_LIB_IMAGES)
void ROMOptimizer::fill_interface_implementation_cache(JVM_SINGLE_ARG_TRAPS) {
  //initialization
  int i = 0;
  for (; i < Universe::number_of_java_classes(); i++) {
    interface_implementation_cache()->int_at_put(i, NOT_IMPLEMENTED); 
    direct_interface_implementation_cache()->int_at_put(i, NOT_IMPLEMENTED); 
  }

  for (i = 0; i < Universe::number_of_java_classes(); i++) {
    UsingFastOops fast;
    JavaClass::Fast java_cls = Universe::class_from_id(i);

    if (java_cls().is_fake_class() || !java_cls().is_instance_class()) {
      continue;
    }

    InstanceClass::Fast cls = java_cls.obj();
    int class_id = cls().class_id();

#if USE_SOURCE_IMAGE_GENERATOR      
    bool reachable_by_applications = true;
    bool hidden = is_in_hidden_package(&cls JVM_CHECK);
    if (hidden) {
      reachable_by_applications = false;
    } else if (is_in_restricted_package(&cls) && !cls().is_public()) {
      reachable_by_applications = false;
    }
#elif (ENABLE_MONET && !ENABLE_LIB_IMAGES)
    bool reachable_by_applications = false;
#endif
    if (cls().is_interface()) {
      if (reachable_by_applications) {
        forbid_invoke_interface_optimization(&cls, false);
        forbid_invoke_interface_optimization(&cls, true);
      }
    } else {
      set_implementing_class(class_id, class_id, true, true);
      if (!reachable_by_applications || cls().is_final()) {
        set_implementing_class(class_id, class_id, true, false);        
      } else {
        forbid_invoke_interface_optimization(&cls, false);
      }
    }
  }
}
#endif

#if USE_SOURCE_IMAGE_GENERATOR || !ENABLE_LIB_IMAGES
void ROMOptimizer::forbid_invoke_interface_optimization(InstanceClass* cls, bool direct) {
  TypeArray::Raw local_interfaces = cls->local_interfaces();
  TypeArray::Raw cache = direct ? direct_interface_implementation_cache()->obj() : 
                                  interface_implementation_cache()->obj() ;
  InstanceClass::Raw interf;
  int len = local_interfaces().length();
  cache().int_at_put(cls->class_id(), FORBID_TO_IMPLEMENT);
  for (int i = 0; i < len; i++) {
    int interf_id = local_interfaces().ushort_at(i);
    cache().int_at_put(interf_id, FORBID_TO_IMPLEMENT);
  }
  InstanceClass::Raw super_cls = cls->super();
  if (super_cls.not_null() && !direct) {
    forbid_invoke_interface_optimization(&super_cls, direct);
  }
}

void ROMOptimizer::set_implementing_class(int interf_id, int class_id, bool only_childs, bool direct_only) {
  TypeArray::Raw cache = direct_only ? 
                  direct_interface_implementation_cache()->obj() : interface_implementation_cache()->obj();
  if (!only_childs) {
    if (cache().int_at(interf_id) == NOT_IMPLEMENTED) {
      //this is the first implementing class
      cache().int_at_put(interf_id, class_id);
    } else {
      //we have multiple implementations of the interface
      cache().int_at_put(interf_id, FORBID_TO_IMPLEMENT);
    }
  }
  InstanceClass::Raw cls = Universe::class_from_id(interf_id);
  TypeArray::Raw interfaces = cls().local_interfaces();
  for (int i = 0; i < interfaces().length(); i++) {
    set_implementing_class(interfaces().ushort_at(i), class_id, false, direct_only);
  }
  if (!direct_only) {
    InstanceClass::Raw super_cls = cls().super();
    if (super_cls().not_null()) {
      set_implementing_class(super_cls().class_id(), class_id, true, direct_only);
    }
  }
}
#endif


void ROMOptimizer::initialize_hashtables(ObjArray* symbol_table_input,
                                         ObjArray* string_table_input
                                         JVM_TRAPS) {
  ROMHashtableManager hashtab_mgr;
  hashtab_mgr.initialize(symbol_table_input, string_table_input JVM_CHECK);

  *string_table()          = hashtab_mgr.string_table();
  *symbol_table()          = hashtab_mgr.symbol_table();
  *embedded_table_holder() = hashtab_mgr.embedded_table_holder();
  _embedded_symbols_offset = hashtab_mgr.embedded_symbols_offset();
  _embedded_strings_offset = hashtab_mgr.embedded_strings_offset();
}

bool ROMOptimizer::is_in_public_vtable(InstanceClass *ic, Method *method JVM_TRAPS) {
  UsingFastOops fast_oops;
  int vtable_index = method->vtable_index();
  InstanceClass::Fast klass = ic;

  for (klass = ic->obj(); klass.not_null(); klass = klass().super()) {
    bool hidden = is_in_hidden_package(&klass JVM_CHECK_(1));
    if (hidden) {
      // Not a public base class
      continue;
    }
    if (is_in_restricted_package(&klass) && !klass().is_public()) {
      // Not a public base class
      continue;
    }

    if (vtable_index < klass().vtable_length()) {
      return true;
    }
  }

  return false;
}

bool ROMOptimizer::is_in_public_itable(InstanceClass *ic, Method *method JVM_TRAPS) {
  UsingFastOops fast_oops;
  ClassInfo::Fast ci = ic->class_info();
  InstanceClass::Fast intf;
  for (int index = 0; index < ci().itable_length(); index++) {
    int offset = ci().itable_offset_at(index);
    if (offset > 0) {
      intf = ci().itable_interface_at(index);
      bool hidden = is_in_hidden_package(&intf JVM_CHECK_(1));
      if (hidden) {
        // Not a public interface
        continue;
      }
      if (is_in_restricted_package(&intf) && !intf().is_public()) {
        // Not a public interface
        continue;
      }

      ObjArray::Raw methods = intf().methods();
      for (int i = 0; i < methods().length(); i ++) {
        Method::Raw m = ci().obj_field(offset + i * sizeof(jobject));
        if (m.equals(method)) {
          return true;
        }
      }
    }
  }

  return false;
}

/*======================================================================
 *
 * Field/Method reachability
 *
 * The following table describes what fields/methods may be reachable by
 * user applications. For the unreachable fields/methods, we can perform
 * the following optimizations:
 *
 *        - change their names to .unknown.
 *        - eliminate field from field table
 *        - eliminate method from method table
 *        - remove method from the system entirely
 *
 * Note that these optimizations are subject to further restrictions (e.g.,
 * a <clinit> method should not be renamed). See the following methods
 * for details:
 *
 *        field_may_be_renamed()
 *        method_may_be_renamed()
 *        is_field_removable()
 *
 *----------------------+-------------------+---------------------------
 *                                          |     class access
 *----------------------+-------------------+---------+-----------------
 *                      |field/method access| public  | non-public
 *----------------------+-------------------+---------+-----------------
 * unrestricted package | public            |    Y    |    Y++
 *                      | protected         |    Y    |    Y++
 *                      | package private   |    Y    |    Y++
 *                      | private           |    -    |    -
 *----------------------+-------------------+---------+-----------------
 * restricted package   | public            |    Y    |    -
 *                      | protected         |    Y    |    -
 *                      | package private   |    -    |    -
 *                      | private           |    -    |    -
 *----------------------+-------------------+---------+-----------------
 * hidden package       | public            |    -    |    -
 *                      | protected         |    -    |    -
 *                      | package private   |    -    |    -
 *                      | private           |    -    |    -
 *----------------------+-------------------+---------+-----------------
 *
 * ++: if AgressiveROMSymbolRenaming is true, these fields/methods
 *     are consider unreachable.
 *======================================================================
 */

bool ROMOptimizer::is_member_reachable_by_apps(jint package_flags, 
                                               AccessFlags class_flags,
                                               AccessFlags member_flags) {
  switch (package_flags) {
  case UNRESTRICTED_PACKAGE:
    if (class_flags.is_public()) {
      if (member_flags.is_public() ||
          member_flags.is_protected() ||
          member_flags.is_package_private()) {
        return true;
      }
    } else {
      if (!AggressiveROMSymbolRenaming) {
        if (member_flags.is_public() ||
            member_flags.is_protected() ||
            member_flags.is_package_private()) {
          return true;
        }
      }
    }
    break;

  case RESTRICTED_PACKAGE:
    if (class_flags.is_public()) {
      if (member_flags.is_public() ||
          member_flags.is_protected()) {
        return true;
      }
    }
    break;

  case HIDDEN_PACKAGE:
    break;
  }

  return false;
}

bool ROMOptimizer::field_may_be_renamed(jint package_flags, 
                                        AccessFlags class_flags,
                                        AccessFlags member_flags,
                                        Symbol *name) {
  if (is_member_reachable_by_apps(package_flags, class_flags, member_flags)) {
    return false;
  }
  if (Symbols::is_system_symbol(name)) {  
    return false;
  }
  return true;
}

bool
ROMOptimizer::is_method_reachable_by_apps(InstanceClass *ic, Method *method JVM_TRAPS) {
  jint package_flags = get_package_flags(ic JVM_CHECK_(1));
  AccessFlags class_flags = ic->access_flags();
  AccessFlags method_flags = method->access_flags();
  
  if (is_member_reachable_by_apps(package_flags, class_flags, method_flags)) {
    return true;
  }

  if (method->is_public() && !method->is_static()){
    bool public_vtable = is_in_public_vtable(ic, method JVM_CHECK_(1));
    if (public_vtable) {
      return true;
    }
    bool public_itable = is_in_public_itable(ic, method JVM_CHECK_(1));
    if (public_itable) {
      return true;
    }
  }

  return false;
}

bool ROMOptimizer::method_may_be_renamed(InstanceClass *ic, Method *method JVM_TRAPS) {
  bool reachable = is_method_reachable_by_apps(ic, method JVM_CHECK_(1));
  if (reachable) {
    return false;
  }

  Symbol::Raw name = method->name();
  if (Symbols::is_system_symbol(&name)) {  
    return false;
  }
  return true;
}

// Should this method be added to the initial invocation closure (when we
// start to search for methods that are not dead)
bool
ROMOptimizer::is_invocation_closure_root(InstanceClass *ic, Method *method JVM_TRAPS) {
  if (is_special_method(method)) {
    return true;
  }
  if (method->is_native()) {
    return true; // private finalize, etc
  }
  bool reachable = is_method_reachable_by_apps(ic, method JVM_CHECK_(1));
  if (reachable) {
    return true;
  }

  // default constructor should be here 
  if (method->match(Symbols::object_initializer_name(),
                    Symbols::void_signature())) {
    return true;
  }

  return false;
}

void ROMOptimizer::remove_dead_methods(JVM_SINGLE_ARG_TRAPS) {
#if !USE_SOURCE_IMAGE_GENERATOR
  JVM_IGNORE_TRAPS;
#else
  ROMVector log_vector;
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);

#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print_cr("[Removed dead methods]");
  _log_stream->cr();
#endif

  MethodInvocationClosure mic;
  int removed_count = 0;
  int removed_bytes = 0;
  int removed_bytecodes = 0;

  mic.initialize(JVM_SINGLE_ARG_CHECK);

  // build accessible methods closure
  UsingFastOops level1;
  ObjArray::Fast methods;
  Method::Fast method;
  InstanceClass::Fast klass;
  for (SystemClassStream st; st.has_next_optimizable();) {
    klass = st.next();
    
    methods = klass().methods();

    for (int i=0; i<methods().length(); i++) {
      method = methods().obj_at(i);
      
      if (method.is_null()) {
        // A nulled-out method
        continue;
      }
      bool in_closure = is_invocation_closure_root(&klass, &method JVM_CHECK)
      if (in_closure) {
        mic.add_method(&method JVM_CHECK);
      }
    }
  }

  // remove all methods not in closure
  ClassInfo::Fast ci;  
  for (SystemClassStream stream; stream.has_next_optimizable();) {
    klass = stream.next();
    
    methods = klass().methods();

    for (int i=0; i<methods().length(); i++) {
      method = methods().obj_at(i);
      
      if (method.is_null()) {
        // A nulled-out method
        continue;
      }
      ci = klass().class_info();
      bool is_dead = false;
      
      if (!mic.contains(&method)) {
        is_dead = true;
      }

      if (is_dead) {
        log_vector.add_element(&method JVM_CHECK);

        removed_count ++;
        removed_bytes += method().object_size();
        removed_bytecodes += method().code_size();
        
        int vindex = method().vtable_index();
        // put nulls to method table and vtable if needed
        if (vindex > -1) {
          clean_vtables(&klass, &method, vindex);
          if (ci().is_interface()) {
            clean_itables(&klass, vindex - JavaVTable::base_vtable_size());
          }
        }
        methods().obj_at_clear(i);

        // Mark the method as impossible to compile to guarantee it won't be 
        // precompiled. We don't remove it from _precompile_method_list so
        // that the method is listed in AOT compilation log.
        method().set_impossible_to_compile();
     }
    }
  }

  log_vector.sort();
  for (int i=0; i<log_vector.size(); i++) {
    Method::Raw method = log_vector.element_at(i);
    _log_stream->print("dead method: ");
#ifndef PRODUCT
    method().print_name_on(_log_stream);
#endif

    _log_stream->print(", bytecodes=%d, size=%d", method().code_size(),
                      method().object_size()); 
    if (method().is_abstract()) {
      _log_stream->print(" (abstract)");
    }
    _log_stream->cr();
  }

  _log_stream->cr();
  _log_stream->print_cr("Removed %d methods, bytecodes = %d, size = %d bytes",
                       removed_count, removed_bytecodes, removed_bytes);
  _log_stream->cr();
#endif
}

void ROMOptimizer::inline_exception_constructors() {
  // There are many recursive invocations of Exception constructors
  // that do nothing except for calling Throwable.<init>(). We detect
  // this case and call Throwable.<init>() instead.
  //
  // This optimization eliminates resolved_static_method entries from
  // the merged constant pool.
  Method::Raw throwable_init = Universe::throwable_class()->lookup_method(
      Symbols::object_initializer_name(),
      Symbols::void_signature());

  GUARANTEE(throwable_init.not_null(), "Sanity!");

  for (SystemClassStream stream; stream.has_next();) {
    InstanceClass::Raw klass = stream.next();
    ConstantPool::Raw cp = klass().constants();

    int len = cp().length();
    for (int i=0; i<len; i++) {
      if (cp().tag_at(i).is_resolved_static_method()) {
        Method::Raw method = cp().resolved_static_method_at(i);
        if (is_inlineable_exception_constructor(&method)) {
          cp().resolved_static_method_at_put(i, &throwable_init);
        }
      }
    }
  }
}

bool ROMOptimizer::is_inlineable_exception_constructor(Method *method) {
  // An inlineable_exception_constructor is:
  // (1) Throwable.<init>()V
  // (2) A constructor of a subclass of Throwable that executes the following
  //     bytecodes:
  //
  //     aload_0
  //     fast_invokevirtual_final inlineable exception_constructor.
  //     return

  if (!method->match(Symbols::object_initializer_name(),
                     Symbols::void_signature())) {
    return false;
  }
  InstanceClass::Raw ic = method->holder();
  if (ic.equals(Universe::object_class())) {
    return false;
  }
  else if (ic.equals(Universe::throwable_class())) {
    return true;
  }
  else {
    if (method->code_size() != 5) {
      return false;
    }
    if ((method->bytecode_at(0) != Bytecodes::_aload_0) ||
        (method->bytecode_at(1) != Bytecodes::_fast_invokevirtual_final) ||
        (method->bytecode_at(4) != Bytecodes::_return)) {
      return false;
    }
    ConstantPool::Raw cp = ic().constants();
    jint cp_index = method->get_java_ushort(2);
    if (!cp().tag_at(cp_index).is_resolved_static_method()) {
      return false;
    }
    Method::Raw callee = cp().resolved_static_method_at(cp_index);
    return is_inlineable_exception_constructor(&callee);
  }
}

void ROMOptimizer::inline_short_methods(JVM_SINGLE_ARG_TRAPS) {
#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print_cr("[Inlined methods]");
  _log_stream->cr();
#endif

  ROMInliner inliner;

  inliner.initialize(12 JVM_CHECK);
  int inlined = 0, total_inlined = 0;  

  UsingFastOops level1;
  ObjArray::Fast methods;
  Method::Fast method;
  for (SystemClassStream stream; stream.has_next_optimizable();) {
    InstanceClass::Raw klass = stream.next();
    methods = klass().methods();

    for (int i=0; i<methods().length(); i++) {
      method = methods().obj_at(i);
      
      if (method.is_null()) {
        // A nulled-out method
        continue;
      }
      
      inlined = inliner.try_to_inline_in_method(&method JVM_CHECK);

      if (inlined > 0) {
#if USE_ROM_LOGGING
        _log_stream->print("in method ");
#ifndef PRODUCT
        method().print_name_on(_log_stream);
#endif
        _log_stream->print_cr(" inlined %d calls", inlined);
#endif
        total_inlined += inlined;
      }
    }
  }
}

void ROMOptimizer::clean_vtables(InstanceClass* klass,
                                 Method* method,
                                 int vindex) {
  Oop null_oop;

  // clean our vtable
  ClassInfo::Raw info = klass->class_info();
  info().vtable_at_put(vindex, null_oop);

  // clean subs vtables
  for (SystemClassStream st; st.has_next();) {
    InstanceClass::Raw next_klass = st.next();
    if (next_klass().is_subclass_of(klass)) {
      info = next_klass().class_info();

      GUARANTEE(vindex < info().vtable_length(), "sanity");
      // for additional safety 
      Method::Raw m = info().vtable_method_at(vindex);
      if (m().equals(*method)) {
        info().vtable_at_put(vindex, null_oop);
      }      
    }
  }
}

void ROMOptimizer::clean_itables(InstanceClass* intf_klass,
                                 int itable_index) {
  Oop null_oop;
  
  for (SystemClassStream st; st.has_next();) {
    InstanceClass::Raw klass = st.next();
    ClassInfo::Raw ci = klass().class_info();

    for (int index = 0; index < ci().itable_length(); index++) {
      int offset = ci().itable_offset_at(index);
      if (offset <= 0) {
        continue;
      }
      InstanceClass::Raw intf = ci().itable_interface_at(index);
      if (!intf_klass->equals(intf)) {
        continue;
      }
      jint addr = offset + itable_index * sizeof(jobject);
      ci().obj_field_put(addr, &null_oop);
    }
  }
}

class BytecodeQuickenClosure : public BytecodeClosure {
  int _slow_count;
  Bytecodes::Code _code;
  Thread* THREAD;
  static class BytecodeQuickenClosure *current;
public:
  ObjArray uninit_classes;

  static BytecodeQuickenClosure *getCurrent() {
    return current;
  }

  BytecodeQuickenClosure() : uninit_classes() {
    GUARANTEE(current == NULL, "sanity");
    current = this;
    _slow_count = 0;
    THREAD = Thread::current();
  }

  ~BytecodeQuickenClosure() {
    current = NULL;
  }

  virtual void bytecode_prolog(JVM_SINGLE_ARG_TRAPS) {
    JVM_IGNORE_TRAPS;
    _code = current_bytecode();
  }

  virtual void handle_exception(JVM_SINGLE_ARG_TRAPS) {
    JVM_IGNORE_TRAPS;
    if (CURRENT_HAS_PENDING_EXCEPTION) {
      ROMOptimizer::process_quickening_failure(method());
    }
  }

  virtual void put_static(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_putstatic) {
      _rewrite_static_field_bytecode(*method(), bci(), false,
                                     false JVM_NO_CHECK_AT_BOTTOM);
    }
  }

  virtual void get_static(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_getstatic) {
      _rewrite_static_field_bytecode(*method(), bci(), true,
                                     false JVM_NO_CHECK_AT_BOTTOM);
    }
  }

  virtual void put_field(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_putfield) {
      _rewrite_field_bytecode(*method(), bci(),
                              Bytecodes::_fast_bputfield,
                              false JVM_NO_CHECK_AT_BOTTOM);
    }
  }

  virtual void get_field(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_getfield) {
      _rewrite_field_bytecode(*method(), bci(),
                              Bytecodes::_fast_bgetfield,
                              true JVM_NO_CHECK_AT_BOTTOM);
    }
  }

  virtual void invoke_static(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_invokestatic) {
      _quicken_invokestatic(*method(), bci(), false JVM_NO_CHECK_AT_BOTTOM);
    }
  }

  void quicken() {
    SETUP_ERROR_CHECKER_ARG;
    // This deals with ldc, ldc_w, ldc2_w, invokevirtual,
    // invokespecial and invokeinterface
    _quicken(*method(), bci() JVM_NO_CHECK_AT_BOTTOM);
  }

  void push1() {
    if (_code == Bytecodes::_ldc ||_code == Bytecodes::_ldc_w) {
      quicken();
    }
  }

  void push2() {
    if (_code == Bytecodes::_ldc2_w) {
      quicken();
    }
  }

  virtual void push_int(jint value JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)value;
    push1();
  }
  virtual void push_float(jfloat value JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)value;
    push1();
  }
  virtual void push_obj(Oop* value JVM_TRAPS) { 
    JVM_IGNORE_TRAPS;
    (void)value;
    push1();
  }
  virtual void push_long(jlong value JVM_TRAPS) { 
    JVM_IGNORE_TRAPS;
    (void)value;
    push2();
  }
  virtual void push_double(jdouble value JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)value;
    push2();
  }

  virtual void invoke_interface(int index, int nofArgs JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    (void)nofArgs;
    if (_code == Bytecodes::_invokeinterface) {
      quicken();
    }
  }

  virtual void invoke_special(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_invokespecial) {
      quicken();
    }
  }

  virtual void invoke_virtual(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_invokevirtual) {
      quicken();
    }
  }

  virtual void new_object(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_new) {
      _quicken_new(*method(), bci() JVM_CHECK);
    }
  }

  virtual void new_object_array(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_anewarray) {
      _quicken_anewarray(*method(), bci() JVM_CHECK);
    }
  }

  virtual void check_cast(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_checkcast) {
      _quicken_checkcast(*method(), bci() JVM_CHECK);
    }
  }

  virtual void instance_of(int index JVM_TRAPS) {
    JVM_IGNORE_TRAPS;
    (void)index;
    if (_code == Bytecodes::_instanceof) {
      _quicken_instanceof(*method(), bci() JVM_CHECK);
    }
  }

  bool quickened() {
    return (_slow_count == 0);
  }

  bool slow_count() {
    return _slow_count;
  }

  void trace_failed_quicken(JavaClass *dependency JVM_TRAPS) {
    _slow_count ++;

    if (uninit_classes.is_null()) {
      uninit_classes = 
        Universe::new_obj_array(Universe::class_list()->length() JVM_CHECK);
    }

    for (int i=0; i<uninit_classes.length(); i++) {
      JavaClass::Raw klass = uninit_classes.obj_at(i);
      if (klass.is_null()) {
          uninit_classes.obj_at_put(i, dependency);
        return;
      } else if (klass.equals(dependency)) {
        // we have already recorded this class
        return;
      }
    }
    // There must be enough space in uninit_classes to store <dependency>!
    SHOULD_NOT_REACH_HERE();
  }
};

BytecodeQuickenClosure *BytecodeQuickenClosure::current = NULL;

// This is called by InterpreterRuntime.cpp and ConstantPool.cpp when
// a bytecode quickening operation fails.
void ROMOptimizer::trace_failed_quicken(Method *method,
                                        JavaClass *dependency
                                        JVM_TRAPS) {
  (void)method;
  BytecodeQuickenClosure *current_bq = BytecodeQuickenClosure::getCurrent();
  if (current_bq != NULL) {
    current_bq->trace_failed_quicken(dependency JVM_CHECK);
  }
}

void
ROMOptimizer::process_quickening_failure(Method *method) {
  // We catch and ignore quickening failures when ROMizing an application,
  // since they can be caused by a missed or invalid class.
  // Theses failures must be reported at run-time.
  if (PostponeErrorsUntilRuntime) {
#ifndef PRODUCT
    Thread::print_current_pending_exception_stack_trace();
#endif
    Thread::clear_current_pending_exception();

    // NOTE: we disable some ROM optimizations for this class,
    // so that the failure can be detected at run-time.
    InstanceClass::Raw( method->holder() )().set_is_non_optimizable();

#if USE_ROM_LOGGING
    _log_stream->print_cr("Failed to quicken bytecodes of ");
    method->print_name_on(_log_stream, true);
    _log_stream->cr();
#endif
  }
}

bool
ROMOptimizer::quicken_one_method(Method *method JVM_TRAPS) {
  BytecodeQuickenClosure bq;
  bq.initialize(method);
  method->iterate(0, method->code_size(), &bq JVM_NO_CHECK);
  if (CURRENT_HAS_PENDING_EXCEPTION) {
    GUARANTEE(!PostponeErrorsUntilRuntime,
              "Cannot have exception at this point");
    process_quickening_failure(method);
    return false;
  }

#if USE_ROM_LOGGING
  if (!bq.quickened()) {
    method->print_name_on(_log_stream, true);
    _log_stream->print_cr(":");
    _log_stream->print_cr("\t%d slow byte code(s)", bq.slow_count());

    GUARANTEE(!bq.uninit_classes.is_null(), 
              "at least one dependency must have been recorded");

    for (int i=0; i<bq.uninit_classes.length(); i++) {
      JavaClass::Raw klass = bq.uninit_classes.obj_at(i);
      if (klass.is_null()) {
        break;
      }
      _log_stream->print("\tuses uninitialized class: ");
      klass().print_name_on(_log_stream);
      _log_stream->cr();
    }
    _log_stream->cr();
  }
#endif

  return bq.quickened();
}

void ROMOptimizer::optimize_fast_accessors(JVM_SINGLE_ARG_TRAPS) {
#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print_cr("[Fast accessor methods]");
  _log_stream->cr();

  int count = 0;
  ROMVector log_vector;
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);
#endif

  UsingFastOops level1;
  InstanceClass::Fast klass;
  ObjArray::Fast methods;
  Method::Fast m;
  for (SystemClassStream st; st.has_next_optimizable();) {
    klass = st.next();
    methods = klass().methods();

    for (int i=0; i<methods().length(); i++) {
      m = methods().obj_at(i);

      if (m.not_null() && m().is_fast_get_accessor()) {
        jint old_size = m.object_size();
        m().set_code_size((jushort)0);
        jint new_size = m().object_size();
        ROMTools::shrink_object(&m, old_size, old_size - new_size);
#if USE_ROM_LOGGING
        log_vector.add_element(&m JVM_CHECK);
        count ++;
#endif
      }
    }
  }

#if USE_ROM_LOGGING
  // Print the results
  log_vector.sort();
  for (int i=0; i<log_vector.size(); i++) {
    Method::Raw method = log_vector.element_at(i);
    _log_stream->print("fast_accessor: ");
    method().print_name_on(_log_stream);
    _log_stream->cr();
  }

  _log_stream->print_cr("%d fast accessors", count);
#endif
}

void ROMOptimizer::resize_class_list(JVM_SINGLE_ARG_TRAPS) {
#if ENABLE_MONET
    // Get rid of the unused space at the end of the class_list, since no
    // new classes would be loaded when running from a binary ROM image.
    //
    // IMPL_NOTE: we should change the format of the class_list stored in the
    // binary ROM image so that only the loaded classes are listed.
    int delta = Universe::number_of_java_classes() - 
                Universe::class_list()->length();
    Universe::resize_class_list(delta JVM_CHECK);
#else
    // Make sure there's enough room in Universe::class_list() to load
    // more than a few classes. This way we don't have to immediately expand
    // it at VM start-up time.
    int headroom = Universe::class_list()->length() - 
                   Universe::number_of_java_classes();
    if (headroom < 20) {
      Universe::resize_class_list(20 - headroom JVM_CHECK);
    }
#endif

#if USE_ROM_LOGGING
  _log_stream->print_cr("number of java classes = %d", 
                ROMWriter::number_of_romized_java_classes());
#endif
}

void ROMOptimizer::rename_non_public_symbols(JVM_SINGLE_ARG_TRAPS) {
  int class_count = 0;
  int field_count = 0;
  int method_count = 0;

  UsingFastOops level1;
  InstanceClass::Fast klass;
  ConstantPool::Fast cp;
  for (SystemClassStream st; st.has_next();) {
    // Note: we just have to go over the InstanceClasses. There are no
    // fields/methods in the array classes.
    klass = st.next();

    // ClassParser should have saved an invalid entry at the end
    // of the CP, which we use temporarily to store the .unknown.
    // symbol.
    cp = klass().constants();
    AZZERT_ONLY(ConstantTag tag = cp().tag_at(0));
    GUARANTEE(tag.is_invalid(), "must be invalid!");
    cp().symbol_at_put(0, Symbols::unknown());

    // (1) Rename the class itself
    if (dont_rename_class(&klass)) {
      // Don't rename this class (so that it may be referenced via
      // Class.forName).
    } else {
      if (RenameNonPublicROMClasses) {
        class_count += rename_non_public_class(&klass JVM_CHECK);
      }
    }

    // (2) Rename the fields in this class
    if (dont_rename_fields_in_class(&klass)) {
      // Don't rename fields in this klass because
      // KNI code may use the fileds by name.
    } else {
      field_count += rename_non_public_fields(&klass JVM_CHECK);
    }

    // (2) Rename the methods in this class
    if (dont_rename_methods_in_class(&klass)) {
      // Don't rename methods in this klass if you have
      // the VM-specific native code that use EntryActivation to
      // invoke Java methods by name.
    } else {
      method_count += rename_non_public_methods(&klass JVM_CHECK);
    }
  }

#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print_cr("Renamed non-public classes: %d", class_count);
  _log_stream->print_cr("Renamed non-public fields:  %d", field_count);
  _log_stream->print_cr("Renamed non-public methods: %d", method_count);
  _log_stream->cr();
#endif
}

int ROMOptimizer::rename_non_public_class(InstanceClass *klass JVM_TRAPS) {
  // The following types of classes may be renamed:
  // [1] all classes in hidden packages
  // [2] package-private classes in restricted packages
  bool may_rename = false;
  bool hidden = is_in_hidden_package(klass JVM_CHECK_0;)
  if (hidden) {
    may_rename = true;
  } else if (is_in_restricted_package(klass) && !klass->is_public()) {
    may_rename = true;
  }

  if (may_rename) {
    Symbol::Raw name = klass->name();
    record_original_class_info(klass, &name);
    klass->set_name(Symbols::unknown());
    return 1;
  } else {
    return 0;
  }
}

int ROMOptimizer::rename_non_public_fields(InstanceClass *klass JVM_TRAPS) {
  UsingFastOops level1;
  ConstantPool::Fast cp = klass->constants();
  int unknown_symbol_index = 0;
  TypeArray::Fast fields = klass->fields();
  int count = 0;

  jint package_flags = get_package_flags(klass JVM_CHECK_0);
  const AccessFlags class_flags = klass->access_flags();

  for (int i=0; i<fields().length(); i+= 5) {
    //5-tuples of shorts [access, name index, sig index, initval index, offset]
    AccessFlags field_flags;
    field_flags.set_flags(fields().ushort_at(i + Field::ACCESS_FLAGS_OFFSET));
    jushort name_index = fields().ushort_at(i + Field::NAME_OFFSET);
    Symbol::Raw name = cp().symbol_at(name_index);

    if (field_may_be_renamed(package_flags, class_flags, field_flags, &name)) {
      record_original_field_info(klass, name_index JVM_CHECK_0);
      fields().ushort_at_put(i + Field::NAME_OFFSET, unknown_symbol_index);
      count ++;
    }
  }
  return count;
}

int ROMOptimizer::rename_non_public_methods(InstanceClass *klass JVM_TRAPS) {
  UsingFastOops level1;
  int unknown_symbol_index = 0;
  int count = 0;

  ObjArray::Fast methods = klass->methods();
  Method::Fast method;
  for (int i=0; i<methods().length(); i++) {
    method = methods().obj_at(i);
    if (method.is_null()) {
      // A nulled-out <clinit> method
      continue;
    }

    bool may_be_renamed = method_may_be_renamed(klass, &method JVM_CHECK_0);
    if (may_be_renamed) {
      record_original_method_info(&method JVM_CHECK_0);
      method().set_name_index(unknown_symbol_index);
      count ++;
    }
  }
  return count;
}

void ROMOptimizer::replace_empty_arrays() {
  for (SystemClassStream st; st.has_next(false, true/*we need fake classes here*/);) {
    InstanceClass::Raw klass = st.next();
#if ENABLE_MONET && ENABLE_LIB_IMAGES
    if (ROM::in_any_loaded_bundle(klass.obj()) || (klass.obj() <= ROM::romized_heap_marker())) {
      continue;
    }
#endif
    TypeArray::Raw fields = klass().fields();
    if (fields().length() == 0) {
      // This may have been Universe::empty_short_array() but that can't
      // go into the TEXT section where we put classinfo
      klass().set_fields(empty_short_array());
    }
    TypeArray::Raw interfaces = klass().local_interfaces();
    if (interfaces().length() == 0) {
      klass().set_local_interfaces(empty_short_array());
    }

    ObjArray::Raw methods = klass().methods();
    int length = methods().length();
    if (length == 0) {
      klass().set_methods(empty_obj_array());
    }

#if ENABLE_REFLECTION
    for (int i = 0; i < length; i++) {
      Method::Raw m = methods().obj_at(i);
      if (m.not_null()) {
        TypeArray::Raw exceptions = m().thrown_exceptions();
        if (exceptions.not_null() && exceptions().length() == 0) {
          m().set_thrown_exceptions(empty_short_array());
        }
      }
    }

    TypeArray::Raw inner_classes = klass().inner_classes();
    if (inner_classes().length() == 0) {
      klass().set_inner_classes(empty_short_array());
    }
#endif
  }
}

void ROMOptimizer::compact_field_tables(JVM_SINGLE_ARG_TRAPS) {
#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print("[Field table compaction]");
  _log_stream->cr();
#endif

  int total_removed = 0;
  UsingFastOops level1;
  InstanceClass::Fast klass;
  TypeArray::Fast fields;
  TypeArray::Fast new_fields;
  for (SystemClassStream st; st.has_next_optimizable();) {
    klass = st.next();
    fields = klass().fields();

    if (dont_rename_fields_in_class(&klass)) {
      // The fileds in this class may be accessed by KNI_GetFieldID() --
      // even the private fields. Don't compact it.
      continue;
    }

    int len = fields().length();
    int num_fields = len / Field::NUMBER_OF_SLOTS;
    int num_removed = 0;
    int i, j;

    if (len == 0) { // nothing to do
      continue;
    }

    for (i = 0; i < len; i += Field::NUMBER_OF_SLOTS) {
      bool removable = is_field_removable(&klass, i, true JVM_CHECK);
      if (removable) {
        num_removed ++;
      }
    }

    if (num_removed == 0) { // nothing to do
      continue;
    }

#ifndef PRODUCT
#if USE_SOURCE_IMAGE_GENERATOR
    record_original_fields(&klass JVM_CHECK);
#endif
#endif

    if (num_removed == num_fields) {
      // We have no fields left
      klass().set_fields(empty_short_array());
    } else if (num_removed > 0) {
      int new_len = (num_fields - num_removed) * Field::NUMBER_OF_SLOTS;
      new_fields = Universe::new_short_array(new_len JVM_CHECK);

      for (i = 0, j = 0; i < len; i += Field::NUMBER_OF_SLOTS) {
        bool removable = is_field_removable(&klass, i, true JVM_CHECK);
        if (!removable) {
          for (int x=0; x<Field::NUMBER_OF_SLOTS; x++) {
            jushort val = fields().ushort_at(i+x);
            new_fields().ushort_at_put(j+x, val);
          }
          j += Field::NUMBER_OF_SLOTS;
        }
      }
      klass().set_fields(&new_fields);
    }

    if (num_removed > 0) {
      total_removed += num_removed;
#if USE_ROM_LOGGING
      _log_stream->print("\tremoved %2d field(s) from ", num_removed);
      klass().print_name_on(_log_stream);
      _log_stream->cr();
#endif
    }
  }

#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print_cr("\tTotal removed field(s) = %d (%d bytes)", 
                       total_removed, 
                       total_removed * sizeof(short) * Field::NUMBER_OF_SLOTS);
#endif
}

// Can this field be removed (from the fields table, or from InstanceClass)
bool ROMOptimizer::is_field_removable(InstanceClass *ic, int field_index,
                                      bool from_table JVM_TRAPS) {
  TypeArray::Raw fields = ic->fields();

  AccessFlags flags;
  flags.set_flags(fields().ushort_at(field_index+ Field::ACCESS_FLAGS_OFFSET));
  jushort initval = fields().ushort_at(field_index + Field::INITVAL_OFFSET);

  // Fields may be not removed if any of the following is true
  if (flags.is_static() && !ic->is_initialized() && initval != 0) {
    // This field is needed by InstanceClass::initialize_static_fields()
    return false;
  }
#if ENABLE_ISOLATES
  if (from_table) {
    // In the case of isolates, fields may be not removed even if the
    // class has been initialized because we re-init the class for each task
    // at runtime
    if (flags.is_static() && initval != 0) {
      // This field is needed by InstanceClass::initialize_static_fields()
      return false;
    }
  }
#endif
  if (from_table) {
    if (flags.is_static() && flags.is_final()) {
      // This field is needed by Value::set_immediate_from_static_field()
      return false;
    }
  }

  const jint package_flags = get_package_flags(ic JVM_CHECK_0);
  const AccessFlags class_flags = ic->access_flags();

  if (is_member_reachable_by_apps(package_flags, class_flags, flags)) {
    return false;
  }

  return true;
}

/// Compact the InstanceClass::methods() array -- remove renamed or virtual
/// methods to save footprint. See InstanceClass::lookup_method for more
/// info.
void ROMOptimizer::compact_method_tables(JVM_SINGLE_ARG_TRAPS) {
  ROMVector log_vector;
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);

#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print("[Method table compaction]");
  _log_stream->cr();
#endif

  int total_removed = 0;
  InstanceClass klass;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    if (klass.is_interface()) {
      // Can't remove *any* method entries from an interface's method table --
      // even a NULL-ed out <clinit>.
      // See InstanceClass::lookup_method_in_all_interfaces(),
      // and InstanceClass::interface_method_at().
      continue;
    }

    total_removed += compact_method_table(&klass JVM_CHECK);
  }

#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print_cr("\tTotal removed method table entries(s) = %d "
                        "(%d bytes)", total_removed, total_removed*4);
#endif
}

int ROMOptimizer::compact_method_table(InstanceClass *klass JVM_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast old_methods = klass->methods();
  Method::Fast m;
  int num_old_methods = old_methods().length();
  int num_removed_methods = 0;
  int i;

  // (1) Determine how many method entries can be removed.
  for (i=0; i<num_old_methods; i++) {
    m = old_methods().obj_at(i);
    if (m.is_null() || is_method_removable_from_table(&m)) {
      num_removed_methods ++;
    }
  }

  if (num_removed_methods == 0) {
    return 0;
  }

  if (num_removed_methods == num_old_methods) {
    klass->set_methods(empty_obj_array());
    return num_removed_methods;
  }

  // (2) Allocate a new method table and replace klass->methods();
  int num_new_methods = num_old_methods - num_removed_methods;
  int num_methods_added = 0;
  ObjArray::Fast new_methods =
      Universe::new_obj_array(num_new_methods JVM_CHECK_0);

  for (i=0; i<num_old_methods; i++) {
    m = old_methods().obj_at(i);
    if (m.not_null() && !is_method_removable_from_table(&m)) {
      new_methods().obj_at_put(num_methods_added, &m);
      num_methods_added ++;
    }
  }
  GUARANTEE(num_methods_added == num_new_methods, "sanity");

  klass->set_methods(&new_methods);
  return num_removed_methods;
}

bool ROMOptimizer::is_method_removable_from_table(Method *method) {
#if ENABLE_JVMPI_PROFILE
  // Should not remove the method from the method table. JVMPI interface need
  // the method info.
  return false;
#endif
  
  if (method->is_native() && method->is_overloaded()) {
    // See CR 4969018. If we remove it from the table ROMWriter
    // will fail to generate an overloaded signature for the
    // native "C" function that implements this method.
    return false;
  }
  Symbol::Raw method_name = method->name();
  if (method_name.equals(Symbols::unknown())) {
    // Renamed methods are always safe to remove
    return true;
  }

  if (method->vtable_index() >= 0) {
    // This method is in the vtable, so it can be discovered by the second
    // part of InstanceClass::lookup_method(). Remove it from the method
    // table to save space.a
    return true;
  }

  InstanceClass::Raw klass = method->holder();
  Symbol::Raw class_name = klass().name();
  if (class_name.equals(Symbols::unknown()) && !is_special_method(method)) {
    // This method can be accessed only by romized classes, but romized
    // classes are already fully resolved, so this method will never
    // be looked up by name again.
    return true;
  } 

  return false;
}

void ROMOptimizer::remove_unused_symbols(JVM_SINGLE_ARG_TRAPS) {
#if USE_ROM_LOGGING
  ROMVector log_vector;
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);

  _log_stream->cr();
  _log_stream->print("[Unused symbol removal]");
  _log_stream->cr();
#endif

  int count = 0;
  int bytes = 0;
  {
    UsingFastOops level1;
    ObjArray::Fast live_symbols = get_live_symbols(JVM_SINGLE_ARG_CHECK);
    ObjArray::Fast symbol_table = SymbolTable::current();
    Symbol::Fast symbol;
    const int length = symbol_table().length();
    for (int i=0; i<length; i++) {
      symbol = symbol_table().obj_at(i);
      if (!symbol.is_null() && !ROMWriter::write_by_reference(&symbol) &&
          !is_symbol_alive(&live_symbols, &symbol)) {
        symbol_table().obj_at_clear(i);
        count ++;
        bytes += symbol().length();
#if USE_ROM_LOGGING
        log_vector.add_element(&symbol JVM_CHECK);
#endif
      }
    }
  }

#if USE_ROM_LOGGING
  log_vector.sort();
  {
    for (int i=0; i<log_vector.size(); i++) {
      Symbol::Raw symbol = log_vector.element_at(i);
      _log_stream->print("removed dead symbol: ");
      symbol().print_symbol_on(_log_stream);
      _log_stream->cr();
    }
  }

  _log_stream->print_cr("Removed dead symbols: %d (%d bytes)",  count, bytes);
  _log_stream->cr();
#endif
}

// Compact vtable and itable for InstanceClass that represents interface.
// We can safely remove vtable, but class ids from itable may be used at 
// runtime by Class.isAssignableFrom to determine if one interface extends 
// another. So we keep class ids and remove the rest of itable.
int ROMOptimizer::compact_one_interface(InstanceClass* ic) {
  GUARANTEE(ic->is_interface(), "Sanity");

  ClassInfo::Raw info = ic->class_info();

  {
    GCDisabler no_gc_must_happen_in_this_block; // IMPL_NOTE: shrinking

    const int old_vtable_length = info().vtable_length();
    const size_t old_klass_size = ic->object_size();
    const size_t static_field_size = ic->static_field_size();
    const size_t oop_map_size = 
      ic->nonstatic_map_size() + ic->static_map_size();

    int itable_length = info().itable_length();  
    size_t old_size = info().object_size();
    size_t new_itable_size = 2 * itable_length * sizeof(int);
    size_t new_size = ClassInfoDesc::allocation_size(0, new_itable_size);
    int reduction = old_size - new_size;
  
    GUARANTEE((info().itable_size() + info().vtable_length() * 4 == 
               (int)new_itable_size + reduction) && 
              (reduction >= 0), "Sanity");

    ClassInfoDesc *oopdesc = (ClassInfoDesc*)info.obj();

    char* old_itable_start = 
      (char*)oopdesc + info().itable_offset_from_index(0);
    char* new_itable_start = 
      (char*)oopdesc + ClassInfo::vtable_offset_from_index(0);

    oopdesc->_object_size = new_size;
    oopdesc->_vtable_length = 0;

    if (itable_length > 0) {
      // Move the itable.
      jvm_memmove(new_itable_start, old_itable_start, new_itable_size);

      // Clear bits for new itable, since it is an int array.
      ObjectHeap::clear_bit_range((OopDesc**)new_itable_start,  
                              (OopDesc**)(new_itable_start + new_itable_size));

      // Invalidate offsets, so that they won't be printed by
      // ClassInfo::iterate_tables().
      for (int i = 0; i < itable_length; i++) {
        info().itable_offset_at_put(i, -1);
      }
    }

    ROMTools::shrink_object(&info, old_size, reduction);

    // InstanceClass size can change if we use embedded vtable bitmap
    if (old_vtable_length > 0) {
      const size_t new_klass_size = 
        InstanceClassDesc::allocation_size(static_field_size, oop_map_size, 0);
      const size_t klass_reduction = old_klass_size - new_klass_size;
      GUARANTEE(klass_reduction >= 0, "Sanity");

      if (klass_reduction > 0) {
        InstanceClassDesc *oopdesc = (InstanceClassDesc*)ic->obj();

        oopdesc->_object_size = new_klass_size;

        ROMTools::shrink_object(ic, old_klass_size, klass_reduction);

        GUARANTEE(new_klass_size > 0 && new_klass_size <= 0xFFFF, "Sanity");
        ic->set_object_size((jushort)new_klass_size);

        reduction += klass_reduction;
      }
    }
    return reduction;
  }
}

/// We don't need the vtables in the InstanceClass'es that represent interfaces. 
/// These vtables will never be traversed in run-time.
void ROMOptimizer::compact_interface_classes(JVM_SINGLE_ARG_TRAPS) {
#if USE_ROM_LOGGING
  ROMVector log_vector;
  log_vector.initialize(JVM_SINGLE_ARG_CHECK);

  _log_stream->cr();
  _log_stream->print("[Interface class itable/vtable removal]");
  _log_stream->cr();
#else
  JVM_IGNORE_TRAPS;
#endif

  int num_interfaces = 0;
  int num_removed_bytes = 0;
  InstanceClass klass;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    if (klass.is_interface()) {
      int reduction = compact_one_interface(&klass);

      num_removed_bytes += reduction;
      num_interfaces ++;

#if USE_ROM_LOGGING
      log_vector.add_element(&klass JVM_CHECK);
#endif
    }
  }

#if USE_ROM_LOGGING
  // Print the results
  log_vector.sort();
  for (int i=0; i<log_vector.size(); i++) {
    InstanceClass::Raw klass = log_vector.element_at(i);
    _log_stream->print("compacted interface class: ");
    klass().print_name_on(_log_stream);
    _log_stream->cr();
  }

  AZZERT_ONLY(ObjectHeap::verify()); // It pays to be paranoid
  _log_stream->print_cr("remove %d bytes in %d interfaces", num_removed_bytes, 
                        num_interfaces);
#endif
}

// Returns all the symbols in the current task that are (a) currently live
// and (b) not already in a loaded ROM.
ReturnOop ROMOptimizer::get_live_symbols(JVM_SINGLE_ARG_TRAPS) {
  int max = SymbolTable::current()->length() * 2;
  UsingFastOops level1;
  ObjArray::Fast live_symbols = Universe::new_obj_array(max JVM_CHECK_0);

#if USE_SOURCE_IMAGE_GENERATOR
  int index;
  // (1) Record all Symbols stored in Universe
  for (index = 0; index < Universe::__number_of_persistent_handles; index++) {
    Oop::Raw oop = persistent_handles[index];
    if (oop.not_null() && oop.is_symbol() &&
        !ROMWriter::write_by_reference(&oop)) {
      Symbol::Raw s = oop.obj();      
      record_live_symbol(&live_symbols, &s);
    }
  }

  for (index = 0; index < Universe::resource_names()->length(); index++) {
    Oop::Raw oop = Universe::resource_names()->obj_at(index);
    if (oop.not_null()) {
      Symbol::Raw s = oop.obj();      
      record_live_symbol(&live_symbols, &s);
    }
  }

  for (index = 0; index < Symbols::number_of_system_symbols(); index++) {
    Oop::Raw oop = (OopDesc*)(system_symbols[index]);
    if (oop.not_null() && oop.is_symbol()
        && !ROMWriter::write_by_reference(&oop)) {
      Symbol::Raw s = oop.obj();
      record_live_symbol(&live_symbols, &s);
    }
  }
#endif

  // (2) Record all symbols stored in classes
  InstanceClass::Fast klass;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    if (klass.not_null()) {
      scan_live_symbols_in_class(&live_symbols, &klass);
    }
  }

#if 0  // IMPL_NOTE: why are we scanning symbols that belong to another task!
#if ENABLE_ISOLATES
  TaskList::Raw task_list = Universe::task_list();
  int len = task_list().length();
  int idx;
  for (idx = Task::FIRST_TASK; idx < len; idx++) {
    if (idx == TaskContext::current_task_id()) {
      continue; //we have already added such symbols
    }    
    Task::Raw processed_task = task_list().obj_at(idx);
    if (processed_task.is_null()) {
      continue;
    }    
    ObjArray::Raw class_list = processed_task().class_list();
    for (int class_id = ROM::number_of_system_classes(); 
         class_id < class_list().length(); class_id++) {
      JavaClass::Raw klass = class_list().obj_at(class_id);
      if (klass().not_null()) {
        scan_all_symbols_in_class(&live_symbols, &klass);
      }
    }
  }    
#endif
#endif

  return live_symbols;
}

#if ENABLE_ISOLATES && 0
// No need for this anymore: app-defined symbols are not shared across
// isolates.
void ROMOptimizer::scan_all_symbols_in_class(ObjArray *live_symbols,
                                              JavaClass *klass) {
  ClassInfo::Raw class_info = klass->class_info();
  Symbol::Raw name = class_info().name();
  record_live_symbol(live_symbols, &name);  
  if (klass->is_instance_class()) {    
    InstanceClass::Raw ic = klass->obj();
    ConstantPool::Raw cp = ic().constants();    
    for (int i = 0; i < cp().length(); i++) {
      unsigned char tag_value = cp().tag_value_at(i);            
      if (tag_value == JVM_CONSTANT_Utf8) {
        Symbol::Raw sym = cp().symbol_at(i);       
        record_live_symbol(live_symbols, &sym);
      }
    }
  }
}
#endif

void ROMOptimizer::scan_live_symbols_in_class(ObjArray *live_symbols,
                                              JavaClass *klass) {
  ClassInfo::Raw class_info = klass->class_info();
  Symbol::Raw name = class_info().name();
  record_live_symbol(live_symbols, &name);  

  if (klass->is_instance_class()) {
    InstanceClass ic = klass->obj();
    scan_live_symbols_in_fields(live_symbols, &ic);
    scan_live_symbols_in_methods(live_symbols, &ic);
  }
}

void ROMOptimizer::scan_live_symbols_in_fields(ObjArray *live_symbols,
                                              InstanceClass *klass) {
  UsingFastOops level1;
  ConstantPool::Fast cp = klass->constants();
  TypeArray::Fast fields = klass->fields();

  if (fields.is_null()) {
    return;
  }
  Symbol::Fast name;
  Symbol::Fast sig;
  for (int i=0; i<fields().length(); i+= 5) {
    //5-tuples of shorts [access, name index, sig index, initval index, offset]
    jushort name_index = fields().ushort_at(i + Field::NAME_OFFSET);
    jushort sig_index  = fields().ushort_at(i + Field::SIGNATURE_OFFSET);

    name = cp().symbol_at(name_index);
    sig  = cp().symbol_at(sig_index);

    record_live_symbol(live_symbols, &name);
    record_live_symbol(live_symbols, &sig);
  }
}

void ROMOptimizer::scan_live_symbols_in_methods(ObjArray *live_symbols,
                                                InstanceClass *klass) {
  UsingFastOops level1;
  ObjArray::Fast methods = klass->methods();

  if (methods.is_null()) {
    return;
  }
  Method::Fast method;
  Symbol::Fast name;
  Symbol::Fast sig;
  for (int i=0; i<methods().length(); i++) {
    method = methods().obj_at(i);
    if (method.is_null()) {
      // A nulled-out <clinit> method
      continue;
    }
    name = method().name();
    sig  = method().signature();
   
    record_live_symbol(live_symbols, &name);
    record_live_symbol(live_symbols, &sig);
  }
}

void ROMOptimizer::record_live_symbol(ObjArray *live_symbols, Symbol* s) {
  juint hash_value = SymbolTable::hash(s->utf8_data(), s->length());
  int length = live_symbols->length();
  juint index = hash_value % length;

  AZZERT_ONLY(juint start = index;)

  while (true) {
    Symbol::Raw existing = live_symbols->obj_at(index);
    if (existing.is_null()) {
      live_symbols->obj_at_put(index, s);
      return;
    } else if (existing().equals(s)) {
      // s is already recorded.
      return;
    } else {
      // advance to next slot
      index ++;
      index %= length;
    }

    GUARANTEE(index != start, "table overflow");
  }
}

bool ROMOptimizer::is_symbol_alive(ObjArray *live_symbols, Symbol* s) {
  juint hash_value = SymbolTable::hash(s->utf8_data(), s->length());
  int length = live_symbols->length();
  juint index = hash_value % length;
  juint start = index;

  while (true) {
    Symbol::Raw existing = live_symbols->obj_at(index);
    if (existing.is_null()) {
      return false;
    } else if (existing().equals(s)) {
      return true;
    } else {
      // advance to next slot
      index ++;
      index %= length;
    }

    if (index == start) {
      return false;
    }
  }
}

class DisableCompilationMatcher : public JavaClassPatternMatcher {
  ROMVector *_log_vector;
public:
  DisableCompilationMatcher(ROMVector *log_vector) {
    _log_vector = log_vector;
  }

  virtual void handle_matching_method(Method *m JVM_TRAPS) {
    if (!m->is_impossible_to_compile()) {
      m->set_impossible_to_compile();
      _log_vector->add_element(m JVM_CHECK);
    }
  }
};

void ROMOptimizer::disable_compilation(char * pattern JVM_TRAPS) {
  DisableCompilationMatcher matcher(_disable_compilation_log);
  matcher.run(pattern JVM_CHECK);
}

#if USE_ROM_LOGGING
void ROMOptimizer::write_disable_compilation_log() {
  int size = _disable_compilation_log->size();
  _disable_compilation_log->sort();

  _log_stream->cr();
  _log_stream->print_cr("[Disabled compilation (%d)]", size);
  _log_stream->cr();

  for (int i=0; i<size; i++) {
    Method::Raw method = _disable_compilation_log->element_at(i);
    _log_stream->print("disabled compilation: ");
#ifndef PRODUCT
    method().print_name_on(_log_stream);
#endif
    _log_stream->cr();
  }
}
#endif

ROMTableInfo::ROMTableInfo(ObjArray *array) {
  _heap_table = array;

  int len = _heap_table->length();
  _count = 0;
  for (int i=0; i<len; i++) {
    if (_heap_table->obj_at(i) != NULL) {
      ++ _count;
    }
  }
}

int ROMTableInfo::num_buckets() {
  int num = (_count + ROMHashTableDepth - 1) / ROMHashTableDepth;

  // If the number is small, make it divisible by 2 so that
  // we can use bitwise AND instead of division in SymbolTable::symbol_for()
  // Do this max for bucket size less than 1024.
  for (int i=2; i<=10; i++) {
    int n = 1 << i;
    if (num < (n * 3 / 2)) {
      num = n;
      return num;
    }
  }

  return num;
}

class SymbolTableInfo : public ROMTableInfo {
public:
  SymbolTableInfo(ObjArray *array) : ROMTableInfo(array) {}
  virtual juint hash(Oop *object) {
    Symbol::Raw symbol = object->obj();
    return symbol().hash();
  }
};

class StringTableInfo : public ROMTableInfo {
public:
  StringTableInfo(ObjArray *array) : ROMTableInfo(array) {}
  virtual juint hash(Oop *object) {
    String::Raw string = object->obj();
    return string().hash();
  }
};

void ROMHashtableManager::initialize(ObjArray* symbol_table,
                                     ObjArray* string_table 
                                     JVM_TRAPS) {
  GUARANTEE(_symbol_table.is_null() && _string_table.is_null(),
            "Already initialized");
  _symbol_table = init_symbols(symbol_table JVM_CHECK);
  _string_table = init_strings(string_table JVM_CHECK);
}

ReturnOop ROMHashtableManager::init_symbols(ObjArray* symbol_table 
                                            JVM_TRAPS) {
  SymbolTableInfo symbol_table_info(symbol_table);
  _symbol_count = symbol_table_info.count();
  return init_rom_hashtable(symbol_table_info JVM_CHECK_0);
}

ReturnOop ROMHashtableManager::init_strings(ObjArray* string_table 
                                            JVM_TRAPS) {
  StringTableInfo string_table_info(string_table);
  _string_count = string_table_info.count();
  return init_rom_hashtable(string_table_info JVM_CHECK_0);
}

ReturnOop ROMHashtableManager::init_rom_hashtable(ROMTableInfo &info JVM_TRAPS) {
  int rom_bucket_sizes[4096]; // The size of each bucket
  int num_buckets = info.num_buckets();
  int max = ARRAY_SIZE(rom_bucket_sizes);
  int i;
  if (num_buckets > max) {
    num_buckets = max;
  }
  if (num_buckets < 2) {
    // just in case we have zero objects to save in table
    num_buckets = 2;
  }

  UsingFastOops level1;
  ObjArray::Fast rom_table = Universe::new_obj_array(num_buckets JVM_CHECK_0);

  for (i = 0; i<num_buckets; i++) {
    rom_bucket_sizes[i] = 0;
  }

  Oop::Fast oop;
  // (1) Determine the size of each bucket
  for (i = 0; i < info.heap_table()->length(); i++) {
    oop = info.heap_table()->obj_at(i);
    if (!oop.is_null()) {
      juint index = info.hash(&oop) % num_buckets;
      rom_bucket_sizes[index] ++;
    }
  }

  // (2) Allocate space for each bucket
  ObjArray::Fast bucket;
  for (i = 0; i<num_buckets; i++) {
    bucket = Universe::new_obj_array(rom_bucket_sizes[i] JVM_CHECK_0);
    rom_table().obj_at_put(i, &bucket);
  }

  // (3) Copy all referenced symbols/strings to their destined bucket.
  for (i = 0; i < info.heap_table()->length(); i++) {
    oop = info.heap_table()->obj_at(i);
    if (!oop.is_null()) {
      juint index = info.hash(&oop) % num_buckets;
      add_to_bucket(&rom_table, index, &oop);
    }
  }

  return rom_table;
}

// Add an object (which lives in a rom table) into the bucket that it
// lives in.
void ROMHashtableManager::add_to_bucket(ObjArray *rom_table, int index,
                                        Oop *object) {
  ObjArray::Raw bucket = rom_table->obj_at(index);
  for (int i=0; i<bucket().length(); i++) {
    Oop::Raw oop = bucket().obj_at(i);
    if (oop.is_null()) {
      bucket().obj_at_put(i, object);
      return;
    }
  }

  SHOULD_NOT_REACH_HERE();
}

void JavaClassPatternMatcher::run(char *pattern JVM_TRAPS) {
  initialize(pattern JVM_CHECK);

  if (!_has_wildcards) {
    quick_match(JVM_SINGLE_ARG_CHECK);
  } else {
    wildcard_match(JVM_SINGLE_ARG_CHECK);
  }
}

void JavaClassPatternMatcher::quick_match(JVM_SINGLE_ARG_TRAPS) {
  LoaderContext ctx(&_class, ErrorOnFailure);
  UsingFastOops level1;
  InstanceClass::Fast instance_class = SystemDictionary::find(&ctx,
                                                   /*lookup_only=*/ true,
                                                   /*check_only= */ true
                                                   JVM_NO_CHECK);
  if (instance_class.is_null()) {
    Thread::clear_current_pending_exception();
    return;
  }

  ObjArray::Fast methods = instance_class().methods();
  Method::Fast m;
  Symbol::Fast mname;
  Symbol::Fast msignature;
  for (int i=0; i<methods().length(); i++) {
    m = methods().obj_at(i);
    if (m.not_null()) {
      mname = m().name();
      msignature = m().signature();

      if (match_method(&mname, &msignature)) {
        handle_matching_method(&m JVM_CHECK);
      }
    }
  }
}

void JavaClassPatternMatcher::wildcard_match(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops level1;
  InstanceClass::Fast klass;
  Symbol::Fast cname;
  ObjArray::Fast methods;
  Method::Fast m;
  Symbol::Fast mname;
  Symbol::Fast msignature;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    cname = klass().name();

    if (match_class(&cname)) {
      methods = klass().methods();

      for (int i=0; i<methods().length(); i++) {
        m = methods().obj_at(i);

        if (m.not_null()) {
          mname = m().name();
          msignature = m().signature();

          if (match_method(&mname, &msignature)) {
            handle_matching_method(&m JVM_CHECK);
          }
        }
      }
    }
  }
} 

void JavaClassPatternMatcher::initialize(char* pattern JVM_TRAPS) {
  GUARANTEE(pattern != NULL, "No empty pattern allowed");
  char* delimiter;
  int pos = 0;

  _as_package = false; 
  _has_wildcards = ((char *) jvm_strrchr(pattern, '*')) != NULL;

  if ((delimiter = ((char *) jvm_strrchr(pattern, '.'))) != NULL) {
    pos = delimiter - pattern;
    if (pos == 0 || (size_t) pos == jvm_strlen(pattern)) {
      tty->print_cr("invalid pattern: %s", pattern);
      return;
    }

    // found at least one '.' in the middle    
    _class  = SymbolTable::slashified_symbol_for((utf8) pattern,
                                                 pos JVM_CHECK);
    
    // parse the method name after '.'
    pattern += pos + 1;
    if ((delimiter = (jvm_strchr(pattern, '('))) == NULL) {
      // no method signature specified
      _method = SymbolTable::symbol_for((utf8) pattern, 
                                        jvm_strlen(pattern) JVM_CHECK);
    } else {
      // delimiter points to the start of method's signature
      pos = delimiter - pattern;
      if (pos == 0 || (size_t) pos == jvm_strlen(pattern)) {
        tty->print_cr("invalid pattern: %s", pattern);
        return;
      }
      _method = SymbolTable::symbol_for((utf8) pattern, pos JVM_CHECK);
      _signature = TypeSymbol::parse(pattern + pos JVM_CHECK);
    }
    
  } else {
    // no dots in it whatsoever, the only possible case is '*'
    // maybe generate an error
    if (pattern[0] != '*') {
      tty->print_cr("invalid pattern: %s", pattern);
      return;
    }
    _class = SymbolTable::symbol_for("*" JVM_CHECK);
    _method = SymbolTable::symbol_for("*" JVM_CHECK);
  }

  return;
}

void JavaClassPatternMatcher::initialize_as_package(Symbol* class_name JVM_TRAPS) {
  GUARANTEE(class_name != NULL, "sanity");

  int pos = class_name->strrchr('/');
  GUARANTEE(pos > -1, "ROMizing class without package, are you sure?");

  _as_package = true;  
  
  // forge java.lang.* from java.lang.Object
  TypeArray byte_array = Universe::new_byte_array(pos+2 JVM_CHECK);
  jvm_memcpy(byte_array.data(), class_name->base_address(), pos+1);
  *((jbyte*)byte_array.data()+pos+1) = '*';

  // create symbols for matching
  _class  = SymbolTable::symbol_for(&byte_array JVM_CHECK);
  _method = SymbolTable::symbol_for("*" JVM_CHECK); // just in case
}

bool JavaClassPatternMatcher::match_class(Symbol* symbol) {
  return match(&_class, symbol);
}

bool JavaClassPatternMatcher::match_method(Symbol* name, Symbol* signature) {
  if (_signature.is_null()) {
    return match(&_method, name);
  } else {
    return match(&_method, name) && _signature.matches(signature);
  }
}

bool JavaClassPatternMatcher::match(Symbol* pattern, Symbol* symbol) {
  int i;

  for (i=0; i<symbol->length(); i++) {

    if (i >= pattern->length()) {
      return false;
    }

    char ch1 = *(symbol->base_address() + i);    
    char ch2 = *(pattern->base_address() + i);

    if (ch2 == '*') {
      return true;
    }
    if (ch1 != ch2) {
      return false;
    }
  }

  // maybe just true, but let's be verbose
  return (i == pattern->length());
}

bool ROMOptimizer::is_special_method(Method* method) {
  UsingFastOops level1;
  Symbol::Fast name = method->name();
  Symbol::Fast sig = method->signature();
  InstanceClass::Fast klass = method->holder();
  Symbol::Fast class_name = klass().name();

  if (name.equals(Symbols::finalize_name())) {
    return true;
  }
  if (name.equals(Symbols::internal_exit_name())) {
    return true;
  }
  if (name.equals(Symbols::class_initializer_name())) {
    return true;
  }
  if (class_name.equals(Symbols::java_lang_Class()) && 
      name.equals(Symbols::initialize_name())) {
    return true;
  }
  if (name.equals(Symbols::main_name()) &&
      sig.equals(Symbols::string_array_void_signature())) {
    return true;
  }
  if (name.equals(Symbols::create_app_image_name())) {
    return true;
  }
  if (name.equals(Symbols::create_sys_image_name())) {
    return true;
  }
#if ENABLE_INLINED_ARRAYCOPY
  if (name.equals(Symbols::unchecked_byte_arraycopy_name())) {
    return true;
  }
  if (name.equals(Symbols::unchecked_char_arraycopy_name())) {
    return true;
  }
  if (name.equals(Symbols::unchecked_int_arraycopy_name())) {
    return true;
  }
  if (name.equals(Symbols::unchecked_long_arraycopy_name())) {
    return true;
  }
  if (name.equals(Symbols::unchecked_obj_arraycopy_name())) {
    return true;
  }
#endif

  if (method->equals(Universe::throw_null_pointer_exception_method())) {
    return true;
  }
  if (method->equals(Universe::throw_array_index_exception_method())) {
    return true;
  }

#if ENABLE_ISOLATES
  if (name.equals(Symbols::initialize_name()) &&
      sig.equals(Symbols::int_obj_array_void_signature())) {
    return true;
  }
#endif

#if ENABLE_JAVA_DEBUGGER
  if (name.equals(Symbols::debugger_sync_name())) {
    return true;
  }
#endif
  return false;
}

class ROMUniqueObjectTable: public ROMLookupTable {
public:
  HANDLE_DEFINITION(ROMUniqueObjectTable, ROMLookupTable);

  void initialize(int num_buckets, int num_attributes JVM_TRAPS) {
    ROMLookupTable::initialize(num_buckets, num_attributes JVM_CHECK);
    set_hashcode_func((hashcode_func_type)&ROMUniqueObjectTable::_hashcode);
    set_equals_func(  (equals_func_type)  &ROMUniqueObjectTable::_equals);
  }

  juint _hashcode(Oop *oop) {
    if (oop->is_short_array()) {
      TypeArray::Raw array = oop->obj();
      return hashcode_for_short_array(&array);
    }
    else if (oop->is_stackmap_list()) {
      StackmapList::Raw entry = oop->obj();
      juint hashcode = 0;
      juint n;
      for (int i=0; i<entry().entry_count(); i++) {
        if (entry().is_short_map(i)) {
          n = entry().uint_field(StackmapList::entry_stat_offset(i));
        } else {
          TypeArray::Raw long_map = entry().get_long_map(i);
          n = hashcode_for_short_array(&long_map);
        }
        hashcode *= 37;
        hashcode += n;
      }
    }

    return 0;
  }

  bool _equals(Oop *oop1, Oop *oop2) {
    if (oop1->obj() == oop2->obj()) {
      return true;
    }

    if (oop1->is_short_array() && oop2->is_short_array()) {
      TypeArray::Raw array1 = oop1->obj();
      TypeArray::Raw array2 = oop2->obj();

      return short_arrays_equal(&array1, &array2);
    }
    else if (oop1->is_stackmap_list() && oop2->is_stackmap_list()) {
      StackmapList::Raw entry1 = oop1->obj();
      StackmapList::Raw entry2 = oop2->obj();

      if (entry1().entry_count() != entry2().entry_count()) {
        return false;
      }
      int count = entry1().entry_count();
      for (int i=0; i<count; i++) {
        if (entry1().is_short_map(i) != entry2().is_short_map(i)) {
          return false;
        }
        if (entry1().is_short_map(i)) {
          juint n1 = entry1().uint_field(StackmapList::entry_stat_offset(i));
          juint n2 = entry2().uint_field(StackmapList::entry_stat_offset(i));
          if (n1 != n2) {
            return false;
          }
        } else {
          TypeArray::Raw long_map1 = entry1().get_long_map(i);
          TypeArray::Raw long_map2 = entry2().get_long_map(i);

          if (!short_arrays_equal(&long_map1, &long_map2)) {
            return false;
          }
        }
      }
      return true;
    }

    return false;
  } 

private:
  static juint hashcode_for_short_array(TypeArray *array) {
    juint hashcode = 0;
    for (int i=0; i<array->length(); i++) {
      jushort s = array->short_at(i);
      hashcode *= 37;
      hashcode += (juint)s;
    }
    return hashcode;
  }

  static bool short_arrays_equal(TypeArray *array1, TypeArray *array2) {
    if (array1->length() != array2->length()) {
      return false;
    }
    int len = array1->length();
    for (int i=0; i<len; i++) {
      jshort s1 = array1->short_at(i);
      jshort s2 = array2->short_at(i);
      if (s1 != s2) {
        return false;
      }
    }
    return true;
  }

};

void ROMOptimizer::remove_duplicated_objects(JVM_SINGLE_ARG_TRAPS) {
  remove_duplicated_short_arrays(JVM_SINGLE_ARG_CHECK);
  remove_duplicated_stackmaps(JVM_SINGLE_ARG_CHECK);
}

void ROMOptimizer::use_unique_object_at(Oop *owner, int offset, 
                                        ROMLookupTable *table JVM_TRAPS)
{
  UsingFastOops fast_oops;
  Oop::Fast key = owner->obj_field(offset);
  if (key.is_null() || ROMWriter::write_by_reference(key)) {
    return; // nothing to do
  }

  if (!table->exists(&key)) {
    table->put(&key, &key JVM_CHECK);
  } else {
    Oop::Raw uniq = table->get(&key);
    GUARANTEE(uniq.not_null(), "sanity");
    owner->obj_field_put(offset, &uniq);
  }
}

void ROMOptimizer::remove_duplicated_short_arrays(JVM_SINGLE_ARG_TRAPS) {
  ROMUniqueObjectTable table;
  table.initialize(128, 1 JVM_CHECK);
  table.put(empty_short_array(), empty_short_array() JVM_CHECK);

  for (SystemClassStream st; st.has_next();) {
    UsingFastOops fast_oops;
    InstanceClass::Fast klass = st.next();
    ClassInfo::Fast info = klass().class_info();

    // Make ClassInfo::local_interfaces unique
    use_unique_object_at(&info, ClassInfo::local_interfaces_offset(), &table
                         JVM_CHECK);

    ObjArray::Fast methods = klass().methods();

    if (methods.is_null() || methods().length() == 0) {
      klass().set_methods(empty_obj_array());
    }
  }
  
  MethodIterator method_iterator(this);
  method_iterator.iterate(MethodIterator::REMOVE_DUPLICATED_SHORT_ARRAYS,
                          &table JVM_CHECK);
}

void ROMOptimizer::remove_duplicated_short_arrays(Method *method, void *param
                                                  JVM_TRAPS) {
  UsingFastOops fast_oops;
  StackmapList::Fast maps;
  ROMUniqueObjectTable *table = (ROMUniqueObjectTable*)param;

  // (1) Make Method::exception_table unique
  use_unique_object_at(method, Method::exception_table_offset(), table JVM_CHECK);

#if ENABLE_REFLECTION
  // IMPL_NOTE: Shall we do that?? ConstantPoolRewriter may rewrite these arrays
  // use_unique_object_at(method, Method::thrown_exceptions_offset(), table JVM_CHECK);
#endif

  // (2) Make long maps in StackmapList unique
  maps = method->stackmaps();
  if (maps.not_null()) {
    for (int i=0; i<maps().entry_count(); i++) {
      if (!maps().is_short_map(i)) {
        int offset = StackmapList::entry_stat_offset(i);
        use_unique_object_at(&maps, offset, table JVM_CHECK);
      }
    }
  }
}

// Remove long entries in StackmapLists that are duplicated -- bit-by-bit the
// same as another entry, which may be in the same StackmapList or another
// StackmapList.
void ROMOptimizer::remove_duplicated_stackmaps(JVM_SINGLE_ARG_TRAPS) {
  ROMUniqueObjectTable table;
  table.initialize(128, 1 JVM_CHECK);

  MethodIterator method_iterator(this);
  method_iterator.iterate(MethodIterator::REMOVE_DUPLICATED_STACKMAPS,
                          &table JVM_CHECK);
}

void ROMOptimizer::remove_duplicated_stackmaps(Method *method, void *param
                                               JVM_TRAPS)
{
  ROMUniqueObjectTable *table = (ROMUniqueObjectTable*)param;
  use_unique_object_at(method, Method::stackmaps_offset(), table JVM_CHECK);
}

// Remove (long or short) entries in StackmapLists that are redundant --
// i.e., if the entry may be computed via abstract interpretation.
void ROMOptimizer::remove_redundant_stackmaps(JVM_SINGLE_ARG_TRAPS) {
  _redundant_stackmap_count = 0;
  _redundant_stackmap_bytes = 0;

  MethodIterator method_iterator(this);
  method_iterator.iterate(MethodIterator::REMOVE_REDUNDANT_STACKMAPS,
                          NULL JVM_CHECK);

#if USE_ROM_LOGGING
  _log_stream->print_cr("Redundant maps removed = %d (%d bytes)",
                _redundant_stackmap_count,
                _redundant_stackmap_bytes);
#endif
}

void ROMOptimizer::remove_redundant_stackmaps(Method *method, void* /*dummy*/
                                              JVM_TRAPS)
{
  UsingFastOops fast_oops;
  StackmapList::Fast old_maps = method->stackmaps();
  StackmapList::Fast new_maps;
  TypeArray::Fast long_map;
 
  int num_to_copy = 0;
  int num_copied = 0;

  if (old_maps.is_null()) {
    return;
  }

  for (int pass=0; pass<2; pass++) {
    int count = old_maps().entry_count();
    for (int i=0; i<count; i++) {
      bool redundant = StackmapChecker::is_redundant(method, i JVM_CHECK);
      if (redundant) {
        if (pass == 0) {
          // Do some accounting here
          _redundant_stackmap_count ++;
          if (old_maps().is_short_map(i)) {
            _redundant_stackmap_bytes += sizeof(juint);
          } else {
            long_map = old_maps().get_long_map(i);
            _redundant_stackmap_bytes += long_map().object_size() - 4;
          }
        }
      } else {
        if (pass == 0) {
          num_to_copy ++;
        } else {
          // Copy the old map entry to new_maps.
          int src = i;
          int dst = num_copied;

          if (old_maps().is_short_map(src)) {
            juint val;
            val  = old_maps().uint_field(old_maps().entry_stat_offset(src));
            new_maps().uint_field_put(new_maps().entry_stat_offset(dst), val);
          } else {
            long_map = old_maps().get_long_map(src);
            new_maps().set_long_map(dst, &long_map);
          }
          num_copied ++;
        }
      }
    }

    if (pass == 0) {
      if (count == num_to_copy) {
        // no redundant copies found. No need to copy
        return;
      } else if (num_to_copy == 0) {
        // This method doesn't need stackmaps. Everything can be computed.
        method->clear_stackmaps();
        return;
      } else {
        new_maps = Universe::new_stackmap_list(num_to_copy JVM_CHECK);
      }
    } else {
      method->set_stackmaps(&new_maps);
    }
  }
}

#if ENABLE_REFLECTION
void ROMOptimizer::resolve_constant_pool(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops level1;
  InstanceClass::Fast klass;
  ConstantPool::Fast cp;
  TypeArray::Fast inner_classes;
  ObjArray::Fast methods;
  Method::Fast method;
  TypeArray::Fast thrown_exceptions;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    cp = klass().constants();
    int i, j, length;

    inner_classes = klass().inner_classes();
    length = inner_classes().length();
    for (i = 0; i < length; i++) {
      if ((j = inner_classes().ushort_at(i)) != 0) {
        cp().klass_at(j JVM_CHECK);
      }
    }
    
    methods = klass().methods();
    length = methods().length();
    for (i = 0; i < length; i++) {
      method = methods().obj_at(i);
      if (method.is_null()) {
        continue;
      }
      
      thrown_exceptions = method().thrown_exceptions();
      if (thrown_exceptions.is_null()) {
        continue;
      }

      int exception_count = thrown_exceptions().length();
      cp = method().constants();
      for (j = 0; j < exception_count; j++) {
        cp().klass_at(thrown_exceptions().ushort_at(j) JVM_CHECK);
      }
    }
  }
}
#endif

#if USE_AOT_COMPILATION
void ROMOptimizer::precompile_methods(JVM_SINGLE_ARG_TRAPS) {
  // AOT is supported only on ARM right now.
  // Note that AOT cross-compiler uses different Java Frame layout than
  // JVM runtime with C interpreter does. So, when compiler needs to allocate
  // and causes GC, the conflict of Java Frame layout mismatch occurs.
  // Possible solutions are:
  //  1. Make C interpreter frame layout identical to ARM frame layout;
  //  2. Dynamically change frame layout during compilation and GC;
  //  3. Disable GC at all during AOT compilation.
  // Currently we use the latter solution.
  // Try increasing HeapCapacity if ROMizer fails here.
  GCDisabler gc_during_aot_compilation_not_supported;

#if USE_ROM_LOGGING
  int native_count = 0;
  int impossible_count = 0;
  int removed_count = 0;
  int failed_count = 0;
  int compiled_count = 0;
  int non_optimizable_count = 0;

  _log_stream->cr();
  _log_stream->print_cr("[AOT compilation report]");
  _log_stream->cr();
#endif

  UsingFastOops level1;
  Method::Fast method;
  InstanceClass::Fast holder;
  int precompile_size = precompile_method_list()->size();
  for (int i=0; i < precompile_size; i++) {
    AZZERT_ONLY(int old = jvm_perf_count.uncommon_traps_generated;)
    method = precompile_method_list()->element_at(i);
    holder = method().holder();

#if USE_ROM_LOGGING
    method().print_value_on(_log_stream);
#endif

    if (method().is_impossible_to_compile()) {
#if USE_ROM_LOGGING
      _log_stream->print_cr(" impossible to compile.");
      impossible_count++;
#endif
      continue;
    }

    if (!holder().is_optimizable()) {
#if USE_ROM_LOGGING
      _log_stream->print_cr(" is in a bad class, compilation skipped.");
      non_optimizable_count++;
#endif
      continue;
    }

#if !ENABLE_ISOLATES
    // In SVM mode, <clinit> is removed after the class is initialized.
    if (method().is_static()) {
      if (holder().is_initialized()) {
        Symbol *name = Symbols::class_initializer_name();
        Symbol *signature = Symbols::void_signature();

        if (method().match(name, signature)) {
          GUARANTEE(holder().lookup_method(name, signature) == NULL,
                    "Must be removed");
#if USE_ROM_LOGGING
          _log_stream->print_cr(" removed.");
          removed_count++;
#endif
          continue;
        }
      }
    }
#endif

    method().compile(-1, false JVM_CHECK);

    // IMPL_NOTE: we can't deal with uncommon traps inside AOT-compiled code
    GUARANTEE(jvm_perf_count.uncommon_traps_generated == old,
              "Uncommon trap in AOT-compiled code");

#if 0
    // Methods that are too big might fail to compile
    GUARANTEE(!method().is_impossible_to_compile() && method().has_compiled_code(),
              "Precompilation failed");
#endif

#if USE_ROM_LOGGING
    if (method().is_impossible_to_compile() || !method().has_compiled_code()) {
      _log_stream->print_cr(" compilation failed.");
      failed_count++;
    } else {
      _log_stream->print_cr(" compiled.");
      compiled_count++;
    }
#endif
  }

#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print_cr("[AOT compilation statistics]");
  _log_stream->cr();
  _log_stream->print_cr("   Total methods for precompilation: %5d", 
                        precompile_size);
  _log_stream->print_cr("   Removed <clinit>'s:               %5d", 
                        removed_count);
  _log_stream->print_cr("   Impossible to compile:            %5d", 
                        impossible_count);
  _log_stream->print_cr("   Methods in bad classes:           %5d", 
                        non_optimizable_count);
  _log_stream->print_cr("   Successfully compiled:            %5d", 
                        compiled_count);
  _log_stream->print_cr("   Failed to compile:                %5d", 
                        failed_count);
#endif
}
#endif

/*
 * this function works in two steps to avoid additional memory
 * allocation in java heap!
 */
void ROMOptimizer::initialize_subclasses_cache(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops level1;
  *subclasses_array() =
      Universe::new_obj_array(Universe::number_of_java_classes() JVM_CHECK);
  TypeArray::Fast sizes =
      Universe::new_short_array(Universe::number_of_java_classes() JVM_CHECK);

  //first loop - counting required array sizes
  InstanceClass::Fast klass;  
  for (SystemClassStream st(true); st.has_next(); ) {
    klass = st.next();
    if (klass().is_interface()) {
      continue;
    }    
    while (klass.not_null()) {            
      int class_id = klass().class_id();
      sizes().short_at_put(class_id, sizes().short_at(class_id) + 1);
      klass = klass().super();
    }
  }

  TypeArray::Fast cur_item;
  for (int i = 0; i < Universe::number_of_java_classes(); i++) {
    cur_item = Universe::new_short_array(sizes().short_at(i) JVM_CHECK);
    subclasses_array()->obj_at_put(i, cur_item);
    sizes().short_at_put(i, 0);
  }
  
  for (SystemClassStream st1(true); st1.has_next(); ) {
    klass = st1.next();
    if (klass().is_interface()) {
      continue;
    } 

    int processed_class_id = klass().class_id();        
    while (klass.not_null()) {            
      int class_id = klass().class_id();      
      cur_item = subclasses_array()->obj_at(class_id);
      cur_item().short_at_put(sizes().short_at(class_id), processed_class_id);
      sizes().short_at_put(class_id, sizes().short_at(class_id) + 1);
      klass = klass().super();
    }
  }
}

ReturnOop ROMOptimizer::get_subclass_list(jushort klass_id) {
  return subclasses_array()->obj_at(klass_id);
}

#if ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
void ROMOptimizer::create_profiles_hidden_bitmap(JVM_SINGLE_ARG_TRAPS) {
  const int profiles_count = profiles_vector()->size();
  const int class_count = ROMWriter::number_of_romized_java_classes();
  const int bitmap_row_size = ROMProfile::calc_bitmap_raw_size();

  int p;
  UsingFastOops fastOops;
  ROMProfile::Fast rom_profile;
  JavaClass::Fast klass;
  InstanceClass::Fast ic;
  ROMVector::Fast hidden_classes;
  ROMVector::Fast hidden_packages;

  *profile_hidden_bitmap() = Universe::new_byte_array(
    bitmap_row_size * profiles_count JVM_CHECK);

  for (p = 0; p < profiles_count; p++ ) {
    rom_profile = profiles_vector()->element_at(p);
    hidden_classes = rom_profile().hidden_classes();
    hidden_packages = rom_profile().hidden_packages();

    for (SystemClassStream st(false); st.has_next();) {
      InstanceClass::Raw klass = st.next();
      GUARANTEE(klass.not_null(), "Sanity");

      if (klass().is_instance_class()) {
        ic = klass;       
        
        // We rely on the fact that if the number of all system classes is N,
        // each of a class should have class_id less then N.
        GUARANTEE(ic().class_id() < class_count, "Sanity");        

        // [1] If class is hidden itself.
        bool class_is_hidden = 
          class_matches_classes_list(&ic, &hidden_classes);
        
        // [2] If class belongs to the profile hidden package.
        class_is_hidden |= class_matches_packages_list(&ic, &hidden_packages
                                                       JVM_CHECK);

        // Mark class in profile bitmap if [1] or [2].
        if (class_is_hidden) {
          const int class_id = ic().class_id();        
          const int byte_ind = p * bitmap_row_size + class_id / BitsPerByte;

          GUARANTEE(byte_ind < class_count, "Sanity");

          unsigned char new_byte = profile_hidden_bitmap()->byte_at(byte_ind);
          new_byte |= 1 << (class_id % BitsPerByte);
          profile_hidden_bitmap()->byte_at_put(byte_ind, new_byte);
        }
      }
    }
  }
}
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT && USE_SOURCE_IMAGE_GENERATOR
#endif // ENABLE_ROM_GENERATOR
