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

// This file contains ROM optimizations that are too expensive to be include
// in the binary ROM generator

#if ENABLE_ROM_GENERATOR && USE_SOURCE_IMAGE_GENERATOR

class StringCounter : public ObjectHeapVisitor {
private:
  int _num_strings;
public:
  /**
   * Overrides ObjectHeapVisitor::do_obj(Oop* obj). This method is called
   * once for each object in the ObjectHeap.
   */
  virtual void do_obj(Oop* obj) {
    if (obj->is_string()) {
#if USE_BINARY_IMAGE_GENERATOR
      if (obj->obj() > ROM::romized_heap_marker())
#endif
        _num_strings ++;
    }
  }

  /**
   * Count the number of all String objects in the ObjectHeap.
   */
  int count() {
    _num_strings = 0;
    ObjectHeap::iterate(this);
    return _num_strings;
  }
};

class StringGatherer : public ObjectHeapVisitor {
private:
  ROMVector* _strings;
  int _num_chars;
public:
  /**
   * Overrides ObjectHeapVisitor::do_obj(Oop* obj). This method is called
   * once for each object in the ObjectHeap.
   */
  virtual void do_obj(Oop* obj) {
    if (obj->is_string()) {
#if USE_BINARY_IMAGE_GENERATOR
      if (obj->obj() > ROM::romized_heap_marker()) 
#endif
      {
        _strings->add_no_expand(obj);
        _num_chars += ((String*)obj)->count();
      }
    }
  }

  /**
   * Gather all String objects in the ObjectHeap into the given
   * vector.
   */
  int gather(ROMVector* strings) {
    _strings = strings;
    _num_chars = 0;
    ObjectHeap::iterate(this);
    return _num_chars;
  }
};


class QuickNativesMatcher : public JavaClassPatternMatcher {
  ROMVector *_log_vector;
public:
  QuickNativesMatcher(ROMVector *log_vector) {
    _log_vector = log_vector;
  }

  virtual void handle_matching_method(Method *m JVM_TRAPS) {
    if (m->is_quick_native()) {
      return;
    }
    if (!m->is_native()) {
      return;
    }
    if (m->uses_monitors()) {
      // Synchronized method must use interpreter method entry.
      return;
    }
    address quick_entry = NULL;

    if (!TaggedJavaStack && UseQuickNativeMethods) {
      Signature::Raw signature = m->signature();
      switch (signature().return_type()) {
      case T_VOID:
        quick_entry = (address)quick_void_native_method_entry;
        break;
      case T_BOOLEAN:
      case T_CHAR:
      case T_BYTE:
      case T_SHORT:
      case T_INT:
        quick_entry = (address)quick_int_native_method_entry;
        break;
      case T_OBJECT:
        quick_entry = (address)quick_obj_native_method_entry;
        break;
      }
    }

    if (quick_entry != NULL) {
      m->set_quick_native_code(m->get_native_code());
      m->set_fixed_entry(quick_entry);

      jint old_size = m->object_size();
      m->set_code_size((jushort)0);
      jint new_size = m->object_size();
      ROMTools::shrink_object(m, old_size, old_size - new_size);
      _log_vector->add_element(m JVM_CHECK);
    }
  }
};

int ROMOptimizer::get_max_alternate_constant_pool_count() {
  // The maximum number of entries needed in the alternate constant
  // pool (which stores names and signatures of renamed fields) is the
  // number of total fields in the ROM image times 2 (1 for name and 1
  // for signature).

  int max = 0;
  for (SystemClassStream st; st.has_next();) {
    InstanceClass::Raw klass = st.next();
    TypeArray::Raw fields = klass().fields();
    if (fields.not_null()) {
      max += (fields().length() / 5) * 2;
    }
  }
  return max;
}

void ROMOptimizer::read_config_file(JVM_SINGLE_ARG_TRAPS) {
  ROMVector disable_compilation_log;
  ROMVector quick_natives_log;
  ROMVector precompile_log;
  ROMVector kvm_natives_log;
  ROMVector kvm_native_methods_vector;

#if USE_ROM_LOGGING
  disable_compilation_log.initialize(JVM_SINGLE_ARG_CHECK);
  quick_natives_log.initialize(JVM_SINGLE_ARG_CHECK);
  precompile_log.initialize(JVM_SINGLE_ARG_CHECK);
  kvm_natives_log.initialize(JVM_SINGLE_ARG_CHECK);
#endif

#if ENABLE_KVM_COMPAT
  kvm_native_methods_vector.initialize(JVM_SINGLE_ARG_CHECK);
#endif

  _disable_compilation_log   = &disable_compilation_log;
  _quick_natives_log         = &quick_natives_log;
  _kvm_natives_log           = &kvm_natives_log;
  _kvm_native_methods_vector = &kvm_native_methods_vector;

  const JvmPathChar *config_file = Arguments::rom_config_file();
  if (config_file == NULL) {
    return;
  }
  
  read_hardcoded_config(JVM_SINGLE_ARG_CHECK);

  read_config_file(config_file JVM_CHECK);

#if USE_ROM_LOGGING
  write_disable_compilation_log();
  write_quick_natives_log();
  write_kvm_natives_log();
#endif

#if ENABLE_KVM_COMPAT
  update_kvm_natives_table(JVM_SINGLE_ARG_CHECK);
#endif
}

void ROMOptimizer::read_config_file(const JvmPathChar * config_file JVM_TRAPS)
{
#if defined(WIN32) || defined(LINUX)
  char buff[1024];
  OsFile_Handle f = OsFile_open(config_file, "r");
  if (f == NULL) {
    tty->print_cr("Error: ROM configuration file not found: %s", config_file);
    JVM::exit(0);
  }
#if USE_ROM_LOGGING
  _log_stream->print_cr("Reading ROM configuration file %s", config_file);
#endif

  char * s;

  // Initialize the config parsing context.
  set_config_parsing_file(config_file);
  set_config_parsing_line_number(0);
  set_config_parsing_active(true);
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  set_config_parsing_in_profile(false);
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

  for (;;) {
    // Read a single line from the file, up to 1024 bytes. We don't have
    // fgets() in the OsFile API, so let's roll one on our own. Efficiency is
    // not an issue here because this code is only used by the romizer, 
    // and never touched in PRODUCT mode (including for Monet).
    char c;
    s = buff;
    int max = (sizeof(buff) / sizeof(char)) - 1;
    int n = 0;
    while (((s - buff) < max) && (n = OsFile_read(f, &c, 1, 1)) == 1) {
      if (c == '\r') {
        continue;
      } else if (c == '\n') {
        break;
      }
      *s++ = c;
    }
    if (s == buff && n < 1) {
      // All input has exhausted
      break;
    }
    *s = 0;
    s = buff;

    process_config_line(s JVM_CHECK);
  }

  // Reset the config parsing context.
  set_config_parsing_file(NULL);
  set_config_parsing_line_number(0);
  set_config_parsing_active(true);
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  set_config_parsing_in_profile(false);
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

  OsFile_close(f);
#endif
}

static const char * const hardcoded_rom_config[] = {
  "DontCompile = com.sun.cldchi.jvm.JVM.*",
};

void ROMOptimizer::read_hardcoded_config(JVM_SINGLE_ARG_TRAPS) {
  const int buffer_size = 1024;
  char buffer[buffer_size];

  const JvmPathChar hardcoded_configuration[] = {
    'H','a','r','d','c','o','d','e','d',' ',
    'c','o','n','f','i','g','u','r','a','t','i','o','n', 0 // 0-terminated
  };

  // Initialize the config parsing context.
  set_config_parsing_file(hardcoded_configuration);
  set_config_parsing_line_number(0);
  set_config_parsing_active(true);
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  set_config_parsing_in_profile(false);
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

  for (int index = 0; index < ARRAY_SIZE(hardcoded_rom_config); index++) {
    // Make a copy to avoid modification of the source.
    jvm_strncpy(buffer, hardcoded_rom_config[index], buffer_size - 1);
    process_config_line(buffer JVM_CHECK);
  }

  // Reset the config parsing context.
  set_config_parsing_file(NULL);
  set_config_parsing_line_number(0);
  set_config_parsing_active(true);
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  set_config_parsing_in_profile(false);
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
}

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
int ROMOptimizer::find_profile(char * name) {  
  const int vector_size = profiles_vector()->size();
  for (int p = 0; p < vector_size; p++) {
    ROMProfile::Raw rom_profile = profiles_vector()->element_at(p);
    Symbol::Raw profile_name = rom_profile().profile_name();
    if (profile_name().eq(name)) {
      return p;
    }
  }
  return Universe::DEFAULT_PROFILE_ID;
}
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT

// Parse and process the config line.
void ROMOptimizer::process_config_line(char * s JVM_TRAPS) {
  set_config_parsing_line_number(config_parsing_line_number() + 1);
  char *name, *value;
  if (parse_config(s, &name, &value)) {

    if (jvm_strcmp(name, "If") == 0) {
      if (jvm_strcmp(value, "CLDC_11") == 0) {
        if (!ENABLE_CLDC_11) {
          set_config_parsing_active(false);
        }
      } else if (jvm_strcmp(value, "ISOLATES") == 0) {
        if (!ENABLE_ISOLATES) {
          set_config_parsing_active(false);
        }
      } else if (jvm_strcmp(value, "MULTIPLE_PROFILES_SUPPORT") == 0) {
        if (!ENABLE_MULTIPLE_PROFILES_SUPPORT) {
          set_config_parsing_active(false);
        }
      } else {
        tty->print_cr("Error: line %d in %s", config_parsing_line_number(), 
                      config_parsing_file());
        tty->print_cr("       Only \"If CLDC_11\" and \"If ISOLATES\" "
                      "       and \"MULTIPLE_PROFILES_SUPPORT\" "
                      "are allowed");
        JVM::exit(0);
      }
      return;
    }
    if (jvm_strcmp(name, "EndIf") == 0) {
      set_config_parsing_active(true);
      return;
    }

    if (!config_parsing_active()) {
      // We're in an "If XXX" block but CLDC 1.1 is disabled
      return;
    }

#if ENABLE_MULTIPLE_PROFILES_SUPPORT       
    if (jvm_strcmp(name, "BeginProfile") == 0) {      
      set_config_parsing_in_profile(true);      
      const int profile_ind = find_profile(value);
      if (profile_ind == Universe::DEFAULT_PROFILE_ID) {
        current_profile()->initialize(value JVM_CHECK);
        profiles_vector()->add_element(current_profile() JVM_CHECK);
      } else {
        ROMProfile::Raw existing_profile = 
          profiles_vector()->element_at(profile_ind);
        set_current_profile(&existing_profile);
      }
    }
    else if (jvm_strcmp(name, "HiddenClass") == 0) {
      if (!config_parsing_in_profile()) {
        tty->print_cr("Error: line %d in %s", 
                      config_parsing_line_number(), config_parsing_file());
        tty->print_cr("       HiddenClass only in BeginProfile-EndProfile "
                      "scope is allowed");
        JVM::exit(0);
      }
      UsingFastOops fastOops;
      Symbol::Fast class_pattern = SymbolTable::slashified_symbol_for(value JVM_CHECK);
      ROMVector::Fast this_profile_hidden_classes = 
        current_profile()->hidden_classes();
      this_profile_hidden_classes().add_element(&class_pattern JVM_CHECK);
      GUARANTEE(this_profile_hidden_classes().contains(&class_pattern), "Sanity");
    }
    else if (jvm_strcmp(name, "EndProfile") == 0) {
      set_config_parsing_in_profile(false);
    } else 
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT      
    if (jvm_strcmp(name, "Include") == 0) {
      include_config_file(value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "InitAtBuild") == 0) {
      add_class_to_list(init_at_build_classes(), name, value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "InitAtLoad") == 0) {
      add_class_to_list(init_at_load_classes(), name, value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "DontRenameClass") == 0) {
      add_class_to_list(dont_rename_classes(), name, value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "DontRenameNonPublicFields") == 0) {
      add_class_to_list(dont_rename_fields_classes(), name, value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "DontRenameNonPublicMethods") == 0) {
      add_class_to_list(dont_rename_methods_classes(), name,value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "HiddenPackage") == 0) {
      UsingFastOops fastOops;
      ROMVector::Fast hidden_list;
      ROMVector::Fast restricted_list;
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
      if (config_parsing_in_profile()) {        
        hidden_list = current_profile()->hidden_packages();        
        restricted_list = current_profile()->restricted_packages();
      } else 
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
      {
        hidden_list = hidden_packages();
        restricted_list = restricted_packages();
      }

      // Hidden packages are also restricted.
      add_package_to_list(&hidden_list, value JVM_CHECK);
      add_package_to_list(&restricted_list, value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "ReservedWord") == 0) {
      UsingFastOops fastOops;
      ROMVector::Fast reserved_words_list = reserved_words();

      Symbol::Fast word = 
                 SymbolTable::slashified_symbol_for(value JVM_CHECK);      
      if (!reserved_words_list().contains(&word)) {
        reserved_words_list().add_element(&word JVM_CHECK);
      }
    }
    else if (jvm_strcmp(name, "RestrictedPackage") == 0) {
      UsingFastOops fastOops;
      ROMVector::Fast packages_list;
#if ENABLE_MULTIPLE_PROFILES_SUPPORT
      if (config_parsing_in_profile()) {
        packages_list = current_profile()->restricted_packages();
      } else 
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
      {
        packages_list = restricted_packages();        
      }

      add_package_to_list(&packages_list, value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "DontCompile") == 0) {
      disable_compilation(value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "KvmNative") == 0) {
      enable_kvm_natives(value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "QuickNative") == 0) {
      enable_quick_natives(value JVM_CHECK);
    }
    else if (jvm_strcmp(name, "Precompile") == 0) {
#if USE_AOT_COMPILATION
      enable_precompile(value JVM_CHECK);
#endif
    }

    else {
      tty->print_cr("Unknown command \"%s\" on line %d of %s", name, 
                    config_parsing_line_number, config_parsing_file());
      tty->print_cr("Note: ROM configuration commands are case sensitive.");
      JVM::exit(0);
    }
  }
}

void ROMOptimizer::include_config_file(char *config_file JVM_TRAPS) {
#if defined(WIN32) || defined(LINUX)

#if USE_UNICODE_FOR_FILENAMES
  JvmPathChar fn_config_file[1024+1];
  {
    int len = jvm_strlen(config_file);
    if (len > 1024) {
        len = 1024;
    }
    for (int i=0; i<len; i++) {
      fn_config_file[i] = (JvmPathChar)config_file[i];
    }
    fn_config_file[len] = 0;
  }
#else
  char *fn_config_file = config_file;
#endif

  // (1) Check if it's in current path
  if (OsFile_exists(fn_config_file)) {
    read_config_file(fn_config_file JVM_CHECK);
    return;
  }

  // (2) Check if it's in any path specified by -romincludepath
  Arguments::ROMIncludePath *ptr;
  for (ptr=Arguments::rom_include_paths(); ptr; ptr=ptr->next()) {
    JvmPathChar buf[1024] = {0};
    JvmPathChar sep[] = {'/', 0};
    fn_strcat(buf, ptr->path());
    fn_strcat(buf, sep);
    fn_strcat(buf, fn_config_file);
    if (OsFile_exists(buf)) {
      read_config_file(buf JVM_CHECK);
      return;
    }
  }

  // (3) The file doesn't exist, complain!
  tty->print_cr("Error: Cannot find included ROM configuration file: %s",
                config_file);
  tty->print_cr("Please check you -romincludepath arguments");
  JVM::exit(0);
#endif
}

bool ROMOptimizer::parse_config(char *line, char**name, char **value) {
  if (*line == '#') {
    return false;
  }

  char * p;

  *name = "";
  *value = "";

  for (p = line; *p && isspace(*p);) {
    p++;
  }
  if (*p == 0) {
      return false; // empty line
  } else {
    *name = p;
  }

  for (; *p && *p != '=' && !isspace(*p);) {
    p++;
  }

  for (; *p && ((*p == '=') || isspace(*p));) {
    *p = 0;
    p ++;
  }
  *value = p;
  for (; *p && !isspace(*p);) {
    p++;
  }
  *p = 0;

  return true;
}

void ROMOptimizer::add_class_to_list(ObjArray *list, char *flag, 
                                     char *classname JVM_TRAPS) {
  UsingFastOops level1;
  Symbol::Fast symbol = SymbolTable::slashified_symbol_for((utf8)classname JVM_CHECK);
  
  LoaderContext top_ctx(&symbol, ErrorOnFailure);
  InstanceClass::Fast instance_class = SystemDictionary::find(&top_ctx,
                                                   /*lookup_only=*/ true,
                                                   /*check_only= */ true
                                                   JVM_NO_CHECK);
  if (instance_class.is_null()) {
#if USE_ROM_LOGGING
    _log_stream->print_cr("WARNING: %s class doesn't exist: %s", flag, 
                          classname);
#endif
    Thread::clear_current_pending_exception();
    return;
  }

  for (int i=0; i<list->length(); i++) {
    if (list->obj_at(i) == NULL) {
      list->obj_at_put(i, &instance_class);
      return;
    }
  }
  SHOULD_NOT_REACH_HERE();
}

// IMPL_NOTE: Make list ROMVector, not ObjArray. Here and everywhere!!!
void ROMOptimizer::add_package_to_list(ROMVector *vector, char *pkgname 
                                       JVM_TRAPS) {
  int len = jvm_strlen(pkgname) + 1;
  if (len > 255) {
    tty->print_cr("error: restricted or hidden package name may not exceed "
                  "255 bytes");
    tty->print_cr("(%s)", pkgname);
    Throw::array_index_out_of_bounds_exception(empty_message JVM_THROW);
  }

  UsingFastOops fast_oops;
  Symbol::Fast package = SymbolTable::slashified_symbol_for(pkgname JVM_CHECK);
  if (!vector->contains(&package)) {
    vector->add_element(&package JVM_CHECK);
  }
}


bool ROMOptimizer::class_list_contains(ObjArray *list, InstanceClass *klass) {
  Oop::Raw oop;
  for (int i=0; i<list->length(); i++) {
    oop = list->obj_at(i);
    if (oop.is_null()) {
      // reached end of list
      return false;
    } else if (oop.equals(klass)) {
      return true;
    } else {
      continue;
    }
  }
  return false;
}

bool 
ROMOptimizer::class_matches_classes_list(InstanceClass *klass, 
                                        ROMVector *patterns) {
  Symbol::Raw class_name = klass->name();
  return name_matches_patterns_list(&class_name, patterns);
}

bool 
ROMOptimizer::class_matches_packages_list(InstanceClass *klass, 
                                        ROMVector *patterns JVM_TRAPS) {
  Symbol::Raw package_name = klass->package_name(JVM_SINGLE_ARG_CHECK_0);
  return name_matches_patterns_list(&package_name, patterns);
}

bool
ROMOptimizer::name_matches_patterns_list(Symbol* checking_name, ROMVector *patterns_list) {
  GUARANTEE(patterns_list != NULL, "Sanity");

  if (patterns_list->is_null()) {
    // Do nothing for null patterns lists.
    return false;
  }

  const int list_len = patterns_list->size();
  Symbol pattern;
  for (int i=0; i<list_len; i++) {
    pattern = patterns_list->element_at(i);
    if (pattern.is_null()) { // end of list
      return false;
    }    

    const int pattern_len = pattern.length();
    const int name_len = checking_name->length();

    if (pattern_len == 0) {
      continue;
    }
    utf8 c_name = checking_name->utf8_data();    
    utf8 p_name = pattern.utf8_data();

    const bool name_matches_pattern = 
      Universe::name_matches_pattern(c_name, name_len, p_name, pattern_len);
    if (name_matches_pattern) {
      return true;
    }
  }
  return false;
}


bool ROMOptimizer::is_init_at_build(InstanceClass *klass) {
  int len = init_at_build_classes()->length();
  bool in_list = false;

  for (int i=0; i<len; i++) {
    InstanceClass::Raw init_at_build = init_at_build_classes()->obj_at(i);
    if (init_at_build.is_null()) {
        break;
    } else if (init_at_build.equals(klass)) {
      in_list = true;
      break;
    }
  }
  
  return in_list;
}

// This optimization is too expensive for binary rom image.
//
// IMPL_NOTE: we may have some dead strings in ConstantPools!
void ROMOptimizer::merge_string_bodies(JVM_SINGLE_ARG_TRAPS) {
  // Get rid of all Strings that are now garbage.
  ObjectHeap::full_collect(JVM_SINGLE_ARG_CHECK);

  /*
   * (1) See how many strings we have in the heap, and the max number
   *     of chars needed by them
   */
  StringCounter sc;
  StringGatherer sg;

  int num_strings = sc.count();
  ROMVector all_strings;
  all_strings.initialize(num_strings JVM_CHECK);
  int num_chars = sg.gather(&all_strings);

  /*
   * (2) Merge the body of all Strings into a single array.
   */
  UsingFastOops level1;
  TypeArray::Fast body = Universe::new_char_array(num_chars JVM_CHECK);
  TypeArray::Fast new_body;
  int new_num_chars = compress_and_merge_strings(&all_strings, &body);

  if (new_num_chars < num_chars) {
    new_body = Universe::new_char_array(new_num_chars JVM_CHECK);
    TypeArray::array_copy(&body, 0, &new_body, 0, new_num_chars);
    replace_string_bodies(&all_strings, &new_body);
  }

#if USE_ROM_LOGGING
  _log_stream->print_cr("Merged the body of %d Strings (%d chars, saved "
                        "%d bytes)",
                all_strings.size(), num_chars, (num_chars-new_num_chars)*2);
#endif
}

int ROMOptimizer::compress_and_merge_strings(ROMVector *all_strings,
                                             TypeArray* body) {
  int offset = 0;

  all_strings->set_sort_strings_by_descending_size(true);
  all_strings->sort();
  for (int i=0; i<all_strings->size(); i++) {
    String::Raw str = all_strings->element_at(i);
    TypeArray::Raw old_body = str().value();
    int old_offset = str().offset();
    int num_chars = str().count();
    
    jchar * old_ptr = ((jchar*)old_body().base_address()) + old_offset;
    jchar * new_ptr = (jchar*)body->base_address();
    int dup_offset = -1;
    dup_offset = find_duplicate_chars(new_ptr, offset, old_ptr, num_chars);

    if (dup_offset >= 0) {
      // Found a duplicate in the merged body
      str().set_offset(dup_offset);
    } else {
      // No duplicate found. Let's copy to the end.
      TypeArray::array_copy(&old_body, old_offset, body, offset, num_chars);
      str().set_offset(offset);
      offset += num_chars;
    }
    str().set_value(body);
  }

  return offset;
}

jint ROMOptimizer::find_duplicate_chars(jchar *pool, jint pool_size, 
                                        jchar *match, jint num_chars) {
  int limit = pool_size - num_chars;
  if (limit < 0) {
    return -1;
  }

  jchar m1 = *match;
  jint num_bytes = num_chars*sizeof(jchar);

  for (int i=0; i<=limit; i++) {
    if (*pool == m1 && jvm_memcmp(pool, match, num_bytes) == 0) {
      return i;
    }
    pool ++;
  }

  return -1;
}

void ROMOptimizer::replace_string_bodies(ROMVector *all_strings,
                                         TypeArray* body) {
  for (int i=0; i<all_strings->size(); i++) {
    String::Raw str = all_strings->element_at(i);
    str().set_value(body);
  }
}

void ROMOptimizer::record_original_class_info(InstanceClass *klass,
                                              Symbol *name) {
  int class_id = klass->class_id();
  romizer_original_class_name_list()->obj_at_put(class_id, name);

  _log_stream->print("renamed Class  ");
  name->print_symbol_on(_log_stream);
  _log_stream->cr();
}


void ROMOptimizer::record_original_field_info(InstanceClass *klass,
                                              int name_index JVM_TRAPS) {
  UsingFastOops fast_oops;
  ConstantPool::Fast cp = klass->constants();
  Symbol::Fast name = cp().symbol_at(name_index);

  _log_stream->print("renamed Field  ");
  klass->print_name_on(_log_stream);
  _log_stream->print(".");
  name().print_symbol_on(_log_stream);
  _log_stream->cr();

  record_original_fields(klass JVM_CHECK);
}

void ROMOptimizer::record_original_fields(InstanceClass *klass JVM_TRAPS) {
  UsingFastOops fast_oops;
  int class_id = klass->class_id();
  TypeArray::Fast orig_fields = 
      romizer_original_fields_list()->obj_at(class_id);
  TypeArray::Fast fields = klass->fields();

  if (orig_fields.is_null()) {
    int len = fields().length();
    orig_fields = Universe::new_short_array(len JVM_CHECK);
    romizer_original_fields_list()->obj_at_put(class_id, &orig_fields);

    for (int i=0; i<len; i+=5) {
      // Note: initval should never be read from the original fields
      // array at run time. We should always read it from 
      // InstanceClass::fields().
      jushort initval    = 0xffff; // <- This should never be used at run-time

      jushort flags      = fields().ushort_at(i + Field::ACCESS_FLAGS_OFFSET);
      jushort name_index = fields().ushort_at(i + Field::NAME_OFFSET);
      jushort sig_index  = fields().ushort_at(i + Field::SIGNATURE_OFFSET);
      jushort offset     = fields().ushort_at(i + Field::OFFSET_OFFSET);

      name_index = get_index_from_alternate_constant_pool(klass, name_index);
      sig_index  = get_index_from_alternate_constant_pool(klass, sig_index);

      orig_fields().ushort_at_put(i + Field::ACCESS_FLAGS_OFFSET, flags);
      orig_fields().ushort_at_put(i + Field::NAME_OFFSET,         name_index);
      orig_fields().ushort_at_put(i + Field::SIGNATURE_OFFSET,    sig_index);
      orig_fields().ushort_at_put(i + Field::OFFSET_OFFSET,       offset);
      orig_fields().ushort_at_put(i + Field::INITVAL_OFFSET,      initval);
    }
  }
}

ReturnOop ROMOptimizer::original_fields(InstanceClass *klass, bool &is_orig) {
  is_orig = false;
  if (romizer_original_fields_list()->is_null()) {
    if (UseROM) {
      return klass->original_fields();
    } else {
      return klass->fields();
    }
  }

  ClassInfo::Raw info = klass->class_info();
  ObjArray::Raw orig_list = romizer_original_fields_list()->obj();
  ReturnOop orig = orig_list().obj_at(info().class_id());
  if (orig != NULL) {
    is_orig = true;
    return orig;
  } else {
    if (UseROM) {
      return klass->original_fields();
    } else {
      return klass->fields();
    }
  }
}

jushort
ROMOptimizer::get_index_from_alternate_constant_pool(InstanceClass *klass,
                                                     jushort symbol_index) {
  ConstantPool::Raw cp = klass->constants();
  Symbol::Raw symbol = cp().symbol_at(symbol_index);
  int len = romizer_alternate_constant_pool()->length();
  for (int i=0; i<len; i++) {
    ConstantTag tag = romizer_alternate_constant_pool()->tag_at(i);

    if (tag.is_invalid()) {
      // We have searched all the existing entries and found no match.
      romizer_alternate_constant_pool()->symbol_at_put(i, &symbol);
      return (jushort)i;
    }

    Symbol::Raw s = romizer_alternate_constant_pool()->symbol_at(i);
    if (s.equals(&symbol)) {
      return (jushort)i;
    }
  }

  SHOULD_NOT_REACH_HERE(); // We should have allocated enough space.
  return 0xffff;
}


void ROMOptimizer::record_original_method_info(Method *method JVM_TRAPS) {
  int class_id = method->holder_id();
  UsingFastOops level1;
  ObjArray::Fast old  = romizer_original_method_info()->obj_at(class_id);
  ObjArray::Fast info = Universe::new_obj_array(3 JVM_CHECK);
  Symbol::Fast name = method->name();

  info().obj_at_put(ROM::INFO_OFFSET_METHOD,    method);
  info().obj_at_put(ROM::INFO_OFFSET_NAME,      &name);
  info().obj_at_put(ROM::INFO_OFFSET_NEXT,      &old);

  romizer_original_method_info()->obj_at_put(class_id, &info);

  _log_stream->print("renamed Method ");
#ifndef PRODUCT
  method->print_name_on(_log_stream);
#endif
  if (method->is_native()) {
    _log_stream->print(" (native)");
  } else if (method->is_abstract()) {
    _log_stream->print(" (abstract)");
  }
  _log_stream->cr();
}


void ROMOptimizer::enable_quick_natives(char * pattern JVM_TRAPS) {
  QuickNativesMatcher matcher(_quick_natives_log);
  matcher.run(pattern JVM_CHECK);
}

#if USE_ROM_LOGGING
void ROMOptimizer::write_quick_natives_log() {
  int size = _quick_natives_log->size();
  _quick_natives_log->sort();

  _log_stream->cr();
  _log_stream->print_cr("[Quick native methods (%d)]", size);
  _log_stream->cr();

  for (int i=0; i<size; i++) {
    Method::Raw method = _quick_natives_log->element_at(i);
    _log_stream->print("quick native: ");
#ifndef PRODUCT
    method().print_name_on(_log_stream);
#endif
    _log_stream->cr();
  }
}
#endif

#if USE_AOT_COMPILATION

class PrecompileMatcher : public JavaClassPatternMatcher {
  ROMVector *_log_vector;
public:
  PrecompileMatcher(ROMVector *log_vector) {
    _log_vector = log_vector;
  }
  virtual void handle_matching_method(Method *m JVM_TRAPS) {
    if (m->is_quick_native() 
          || m->is_impossible_to_compile()
          || m->is_fast_get_accessor()) { 
      return;
    }
    _log_vector->add_element(m JVM_NO_CHECK_AT_BOTTOM);
  }
};

void ROMOptimizer::enable_precompile(char * pattern JVM_TRAPS) {
  PrecompileMatcher matcher(precompile_method_list());
  matcher.run(pattern JVM_NO_CHECK_AT_BOTTOM);
}

#endif

class KvmNativesMatcher : public JavaClassPatternMatcher {
  ROMVector *_log_vector;
public:
  KvmNativesMatcher(ROMVector *log_vector) {
    _log_vector = log_vector;
  }

  virtual void handle_matching_method(Method *m JVM_TRAPS) {
    if (!m->is_native()) {
      return;
    }

    _log_vector->add_element(m JVM_CHECK);
  }
};


void ROMOptimizer::enable_kvm_natives(char * pattern JVM_TRAPS) {
  KvmNativesMatcher matcher(_kvm_natives_log);
  matcher.run(pattern JVM_CHECK);
}

void ROMOptimizer::write_kvm_natives_log() {
#if USE_ROM_LOGGING
  int size = _kvm_natives_log->size();
  _kvm_natives_log->sort();

  _log_stream->cr();
  _log_stream->print_cr("[Kvm native methods (%d)]", size);
  _log_stream->cr();

  for (int i=0; i<size; i++) {
    Method::Raw method = _kvm_natives_log->element_at(i);
    _log_stream->print("kvm native: ");
    method().print_name_on(_log_stream);
    _log_stream->cr();
  }
#endif
}

void ROMOptimizer::update_kvm_natives_table(JVM_SINGLE_ARG_TRAPS) {
  int size = _kvm_natives_log->size();
  *kvm_native_methods_table() = Universe::new_obj_array(size * 3 JVM_CHECK);

  for (int i=0; i<size; i++) {
    Method::Raw method = _kvm_natives_log->element_at(i);
    InstanceClass::Raw klass = method().holder();
    Symbol::Raw class_name = klass().name();
    Symbol::Raw method_name = method().name();
    Symbol::Raw method_sig = method().signature();

    kvm_native_methods_table()->obj_at_put(i * 3 + 0, &class_name);
    kvm_native_methods_table()->obj_at_put(i * 3 + 1, &method_name);
    kvm_native_methods_table()->obj_at_put(i * 3 + 2, &method_sig);
  }
}

// In MIDP there are a large number of non-public static final int fields.
// These fields are usually in-lined by javac into bipush bytecodes, so
// there are no corresponding bytecodes that actually attempts to read
// the field itself from the InstanceClass (or TaskMirror), so it's safe
// to remove these fields.
//
// Don't confuse this optimization with compact_field_tables() -- here
// we compact the InstanceClass (or TaskMirror) objects, where
// compact_field_tables() compacts the InstanceClass::fields() table.
void ROMOptimizer::remove_unused_static_fields(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  int num_classes = Universe::number_of_java_classes();
  ObjArray::Fast directory = Universe::new_obj_array(num_classes JVM_CHECK);
  InstanceClass::Fast klass;
  TypeArray::Fast reloc_info;

  if (VerifyGC) {
    ObjectHeap::verify();
  }

  // (1) Count the word of static fields, allocate 4 bytes of reloc_info
  //     storage per word of static fields. Note: each LONG/DOBLE field
  //     is basically treated as two int fields.
  {
    for (SystemClassStream st; st.has_next_optimizable();) {
      klass = st.next();
      int nbytes = klass().static_field_size();
      if (nbytes > 0 && !dont_rename_fields_in_class(&klass)) {
        reloc_info = Universe::new_int_array(nbytes / BytesPerWord JVM_CHECK);
        directory().obj_at_put(klass().class_id(), &reloc_info);
      }
    }
  }

  // (2) Mark all static fields that cannot be removed -- either if they
  //     are referenced by FieldRef (which means they are accessed by
  //     getstatic/putstatic bytecodes), or if the field is accessible
  //     by applications.
  mark_static_fieldrefs(&directory);
  mark_unremoveable_static_fields(&directory JVM_CHECK);

  {
    // Inside this block the heap may be in inconsistent state
    GCDisabler no_gc_must_happen_in_this_block;

    // (3) Determine the new location of the static fields
    compact_static_field_containers(&directory);

    // (4) Fix all field tables to 
    //     (a) remove dead entries and 
    //     (b) fix offset of entries that remain
    fix_field_tables_after_static_field_removal(&directory);

    // (5) Update all fieldrefs
    fix_static_fieldrefs(&directory);
  }

  if (VerifyGC) {
    ObjectHeap::verify();
  }
}

void ROMOptimizer::mark_static_fieldrefs(ObjArray *directory) {
  for (SystemClassStream st; st.has_next_optimizable();) {
    InstanceClass::Raw klass = st.next();
    ConstantPool::Raw cp = klass().constants();
    TypeArray::Raw reloc_info;
    int n = cp().length();
    int offset, class_id, idx;
    BasicType type;

    for (int i=0; i<n; i++) {
      ConstantTag tag = cp().tag_at(i);
      switch (tag.value()) {
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
        // Note: field offset starts from beginning of InstanceClass/TaskMirror
        type = cp().resolved_field_type_at(i, offset, class_id);
        reloc_info = directory->obj_at(class_id);
        idx = (offset - JavaClass::static_field_start()) / BytesPerWord;
        if (reloc_info().not_null()) {
          reloc_info().int_at_put(idx, 1); // Mark the field at this offset
          if (is_two_word(type)) {
            reloc_info().int_at_put(idx+1, 1);
          }
        }
        break;
      case JVM_CONSTANT_Fieldref:
        // This looks like unused Fieldref
        break;
      }
    }
  }
}

void ROMOptimizer::fix_static_fieldrefs(ObjArray *directory) {
  const int static_field_start = JavaClass::static_field_start();

  for (SystemClassStream st; st.has_next_optimizable();) {
    InstanceClass::Raw klass = st.next();
    ConstantPool::Raw cp = klass().constants();
    TypeArray::Raw reloc_info;
    int n = cp().length();
    int offset, class_id, idx;
    BasicType type;

    for (int i=0; i<n; i++) {
      ConstantTag tag = cp().tag_at(i);
      switch (tag.value()) {
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
        // Note: field offset starts from beginning of InstanceClass/TaskMirror
        type = cp().resolved_field_type_at(i, offset, class_id);
        reloc_info = directory->obj_at(class_id);
        idx = (offset - static_field_start) / BytesPerWord;
        if (reloc_info().not_null()) {
          int dst = reloc_info().int_at(idx);
          int new_offset = (dst * BytesPerWord) + static_field_start;
          GUARANTEE(dst != DEAD_FIELD, "sanity");
          GUARANTEE(new_offset <= offset, "sanity");
          cp().resolved_field_at_put(i, class_id, new_offset, type, true);
        }
        break;
      }
    }
  }
}

void ROMOptimizer::mark_unremoveable_static_fields(ObjArray *directory JVM_TRAPS) {
  UsingFastOops fast_oops;
  InstanceClass::Fast klass;
  TypeArray::Fast fields;
  TypeArray::Fast reloc_info;

  for (SystemClassStream st; st.has_next_optimizable();) {
    klass = st.next();
    fields = klass().fields();
    int id = klass().class_id();

    reloc_info = directory->obj_at(klass().class_id());
    if (reloc_info.is_null()) {
      continue;
    } else {
      GUARANTEE(!dont_rename_fields_in_class(&klass), "sanity");
    }

    for (int i = 0; i < fields().length(); i += Field::NUMBER_OF_SLOTS) {
      Field f(&klass, i);
      bool removable = is_field_removable(&klass, i, false JVM_CHECK);
      if (f.is_static() && !removable) {
        int offset = f.offset();
        BasicType type = f.type();
        int idx = (offset - JavaClass::static_field_start()) / BytesPerWord;
        reloc_info().int_at_put(idx, 1); // Mark the field at this offset
        if (is_two_word(type)) {
          reloc_info().int_at_put(idx+1, 1);
        }
      }
    }
  }
}

void ROMOptimizer::compact_static_field_containers(ObjArray *directory) {
#if USE_ROM_LOGGING
  _log_stream->cr();
  _log_stream->print("[Unused Static Field Removal]");
  _log_stream->cr();
#endif

  // We are overwritting obj fields with integers, fixing static oopmaps, etc.
  // GC should not happen in the middle.
  GCDisabler no_gc_must_happen_in_this_function;

  UsingFastOops fast_oops;
  InstanceClass::Fast klass;
  TypeArray::Fast reloc_info;
  TypeArray::Fast fields;
  Oop::Fast holder;
  const int static_field_start = JavaClass::static_field_start();

  for (SystemClassStream st; st.has_next_optimizable();) {
    klass = st.next();
    fields = klass().fields();

    int fields_shrink_size = 0;
    size_t old_klass_size = klass().object_size();

    reloc_info = directory->obj_at(klass().class_id());
    if (reloc_info.is_null()) {
      // nothing to compact for this class.
      continue;
    }
#if ENABLE_ISOLATES
    // We have only one task_mirror to fix, since romization runs in
    // single-task mode.
    if (!klass().is_initialized()) { // Hmmmmm ...
      directory->obj_at_clear(klass().class_id());
      continue;
    }
    holder = klass().task_mirror();
#else
    holder = klass.obj();
#endif

    const size_t old_static_field_size = klass().static_field_size();
#if USE_EMBEDDED_VTABLE_BITMAP
    const size_t old_vtable_bitmap_start = klass().vtable_bitmap_start();    
#endif

    // (1) Move the fields
    {
      int dst, src;
      for (dst=src=0; src<reloc_info().length(); src++) {
        if (reloc_info().int_at(src) != 0) {
          if (dst != src) {
            int srcoffset = static_field_start + src * BytesPerWord;
            int dstoffset = static_field_start + dst * BytesPerWord;
            // Don't worry about setting bitvector now. It will be handled
            // when we rewrite the statics oopmap.
            // Note: we might be writing half of a LONG/DOUBLE now.
            int n = *(holder().obj()->int_field_addr(srcoffset));
            *(holder().obj()->int_field_addr(dstoffset)) = n;
          }
          reloc_info().int_at_put(src, dst); // remember new location
          dst ++;
        } else {
          reloc_info().int_at_put(src, DEAD_FIELD); // mark this field as dead
#if USE_ROM_LOGGING
          int srcoffset = static_field_start + src * BytesPerWord;
          bool is_orig;
          TypeArray::Raw ofields = original_fields(&klass, is_orig);

          for (int i=0; i<ofields().length(); i+= 5) {
            AccessFlags f;
            f.set_flags(ofields().ushort_at(i + Field::ACCESS_FLAGS_OFFSET));
            int offset = ofields().ushort_at(i + Field::OFFSET_OFFSET);
            if (f.is_static() && offset == srcoffset) {
              jushort name_index = ofields().ushort_at(i + Field::NAME_OFFSET);
              ConstantPool::Raw cp;
              if (is_orig) {
                cp = ROM::alternate_constant_pool(&klass);
              } else {
                cp = klass().constants();
              }
              // Note. we may not come to here for ever <src>, since <src>
              // may refer to second word of a LONG/DOUBLE.
              _log_stream->print("removed static field ");
              klass().print_name_on(_log_stream);
              _log_stream->print(".");
              Symbol::Raw name = cp().symbol_at(name_index);
              name().print_symbol_on(_log_stream);
              _log_stream->cr();
              break;
            }
          }
#endif
        }
      }

      fields_shrink_size = (src - dst) * BytesPerWord;
      if (fields_shrink_size == 0) {
        // No field was removed. Do need to worry about this class anymore
        directory->obj_at_clear(klass().class_id());
        continue;
      }
    }

#if !ENABLE_ISOLATES
    // (2) Update InstanceClassDesc::_oop_map._embedded_start, and copy
    //     the instance oop map.
    //     Note: this is not necessary in MVM case because the instance
    //     oopmap always stays at the same offset.
    {
      size_t oop_map_start = klass().embedded_oop_map_start();
      size_t size = klass().nonstatic_map_size();
      jubyte *map_src = klass().embedded_oop_map();
      oop_map_start -= fields_shrink_size;
      klass().set_embedded_oop_map_start(oop_map_start);
      jubyte *map_dst = klass().embedded_oop_map();

      jvm_memmove(map_dst, map_src, size);
    }
#endif

    // (3) - Relocate and compact the static oopmaps, 
    //     - relocate and compact the fields within the holder
    //     - Relocate and compact write-barrier bits.
    //     - Relocate vtable bitmap (SVM-only).
    //     - Shrink the holder.
    {
      // Clear all bits in the holder, in case a stale bit that used to
      // cover an Oop field now covers an INT slot. We'll set the Oop bits
      // later.
      OopDesc **start =DERIVED(OopDesc**, holder.obj(), static_field_start);
      OopDesc **ex_end=DERIVED(OopDesc**, holder.obj(), holder().object_size());
      ObjectHeap::clear_bit_range(start, ex_end);

      // Write a new static oopmap according to the remaining static fields
      int offset = -1;
      int prev_oop_offset = 0;
      jubyte *map_ptr = klass().embedded_oop_map() +
                        klass().nonstatic_map_size();
      jubyte *map_start = map_ptr;

      for (int i = 0; i < fields().length(); i += 5) {
        Field f(&klass, i);
        if (f.is_static()) {
          GUARANTEE((f.offset() > offset), 
                    "static fields offsets must be ascending");
          BasicType type = f.type();
          offset = f.offset();
          int idx = (offset - static_field_start) / BytesPerWord;
          int dst = reloc_info().int_at(idx);
      
          if ((dst != DEAD_FIELD) && (type == T_OBJECT || type == T_ARRAY)) {
            int new_offset = dst * BytesPerWord + static_field_start;
            // Compute delta in words from previous oop offset
            int word_delta = (new_offset - prev_oop_offset) / oopSize;
            GUARANTEE(word_delta > 0, "sanity check");
            while (word_delta >= OopMapEscape) {
              // Fill in escape value for ranges larger than 0x7f
              *map_ptr++ = OopMapEscape;
              word_delta -= (OopMapEscape - 1);
            }
            // Fill in oop map entry
            *map_ptr++ = (jubyte)word_delta;
            prev_oop_offset = new_offset;

            // Set the barrier bit for each remaining oop field.
            OopDesc** loc = DERIVED(OopDesc**, holder.obj(), new_offset);
            ObjectHeap::set_bit_for(loc);
          }
        }
      }
      
      *map_ptr++ = OopMapSentinel;   // Terminate oop map
      int static_map_size = (map_ptr - map_start);
      
      // Shrink InstanceClass object -- note: even in MVM case, the
      // InstaneClass may shrink because we may have removed some oopmap entries
      const size_t new_static_field_size = 
        old_static_field_size - fields_shrink_size;
      const size_t new_oop_map_size = 
        klass().nonstatic_map_size() + static_map_size;
      const size_t new_klass_size = 
        InstanceClassDesc::allocation_size(new_static_field_size,
                                           new_oop_map_size,
                                           klass().vtable_length());

#if !ENABLE_ISOLATES && USE_EMBEDDED_VTABLE_BITMAP
      // Move vtable bitmap
      const size_t new_vtable_bitmap_start = klass().vtable_bitmap_start();
      GUARANTEE(old_vtable_bitmap_start > new_vtable_bitmap_start, 
                "Must decrease: some static fields were removed");

      const size_t vtable_bitmap_size = 
        new_klass_size - new_vtable_bitmap_start;
      GUARANTEE(vtable_bitmap_size >= 0, "Sanity");

      address map_src = (address)klass().obj() + old_vtable_bitmap_start;
      address map_dst = (address)klass().obj() + new_vtable_bitmap_start;
      
      jvm_memmove(map_dst, map_src, vtable_bitmap_size);
#endif

      GUARANTEE(new_klass_size <= old_klass_size, "sanity");
      size_t reduction = old_klass_size - new_klass_size;

      if (reduction > 0) {
        // reduction may be 0 in the MVM case.
        ROMTools::shrink_object(&klass, old_klass_size, reduction);
      }
      klass().set_object_size((jushort)new_klass_size);
    }

#if ENABLE_ISOLATES
    // (4) Shrink the TaskMirror object and update ClassInfo
    {
      TaskMirror::Raw mirror = holder.obj();
      size_t old_size = mirror().object_size();
      int end_offset = klass().static_field_end();

#if ENABLE_ISOLATES && USE_EMBEDDED_VTABLE_BITMAP
      // Move vtable bitmap
      const size_t new_vtable_bitmap_start = 
        old_vtable_bitmap_start - fields_shrink_size;
      GUARANTEE(old_vtable_bitmap_start > new_vtable_bitmap_start, 
                "Must decrease: some static fields were removed");

      const size_t vtable_bitmap_size = old_size - end_offset;
      GUARANTEE(vtable_bitmap_size >= 0, "Sanity");

      address map_src = (address)mirror().obj() + old_vtable_bitmap_start;
      address map_dst = (address)mirror().obj() + new_vtable_bitmap_start;
      
      jvm_memmove(map_dst, map_src, vtable_bitmap_size);
#endif

      end_offset -= fields_shrink_size;
      GUARANTEE(end_offset >= 0, "sanity");
      mirror().set_object_size(old_size - fields_shrink_size);
      
      ROMTools::shrink_object(&mirror, old_size, fields_shrink_size);

      // info().static_field_end() is used by JavaClass.cpp to calculate the
      // number of fields inside a TaskMirror -- IMPL_NOTE: can this field
      // be removed?
      ClassInfo::Raw info = klass().class_info();
      info().set_static_field_end(end_offset);
    }
#endif
  }
}

void
ROMOptimizer::fix_field_tables_after_static_field_removal(ObjArray *directory)
{
  UsingFastOops fast_oops;
  InstanceClass::Fast klass;
  TypeArray::Fast reloc_info;
  TypeArray::Fast fields;

  for (SystemClassStream st; st.has_next_optimizable();) {
    klass = st.next();
    fields = klass().fields();

    reloc_info = directory->obj_at(klass().class_id());
    if (reloc_info.is_null()) {
      // nothing to compact for this class.
      continue;
    }    
    fix_one_field_table(&klass, &fields, &reloc_info);

    bool is_orig;
    fields = original_fields(&klass, is_orig);
    if (is_orig) {
      // This class also has an original fields table. Fix that one as well
      fix_one_field_table(&klass, &fields, &reloc_info);
    }
  }
}

void ROMOptimizer::fix_one_field_table(InstanceClass *klass, TypeArray *fields,
                                       TypeArray *reloc_info) {
  const int static_field_start = JavaClass::static_field_start();
  int i, j;

  for (i=j=0; i<fields->length(); i+= 5) {
    bool copyit = true;

    AccessFlags flags;
    flags.set_flags(fields->ushort_at(i + Field::ACCESS_FLAGS_OFFSET));
    if (flags.is_static()) {
      int offset = fields->ushort_at(i + Field::OFFSET_OFFSET);
      int idx = (offset - static_field_start) / BytesPerWord;
      int dst = reloc_info->int_at(idx);
      if (dst == DEAD_FIELD) {
        copyit = false;
      } else {
        int new_offset = dst * BytesPerWord + static_field_start;
        GUARANTEE(new_offset <= offset, "sanity");
        fields->ushort_at_put(i + Field::OFFSET_OFFSET, new_offset);
      }
    }
    if (copyit) {
      if (i != j) {
        fields->ushort_at_put(j + 0, fields->ushort_at(i + 0));
        fields->ushort_at_put(j + 1, fields->ushort_at(i + 1));
        fields->ushort_at_put(j + 2, fields->ushort_at(i + 2));
        fields->ushort_at_put(j + 3, fields->ushort_at(i + 3));
        fields->ushort_at_put(j + 4, fields->ushort_at(i + 4));
      }
      j += 5;
    }
  }
  if (i != j) {
    size_t old_size = fields->object_size();
    fields->set_length(j);
    size_t new_size = fields->object_size();
    size_t reduction = old_size - new_size;
    GUARANTEE(new_size <= old_size, "sanity");
    if (reduction > 0) {
      ROMTools::shrink_object(fields, old_size, reduction);
    }
  }
}

#endif
