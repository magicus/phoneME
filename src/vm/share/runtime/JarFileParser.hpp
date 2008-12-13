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

class FileDescriptor : public Instance {
public:
  HANDLE_DEFINITION(FileDescriptor, Instance);

  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  static int handle_offset() {
    return header_size() + 0;
  } 
  static int valid_offset() {
    return header_size() + sizeof(jobject);
  }

public:
  jint valid() const {
    return int_field(valid_offset());
  }
  void set_valid(jint value) {
    int_field_put(valid_offset(), value);
  }

  OsFile_Handle handle() const {
    return (OsFile_Handle)int_field(handle_offset());
  }
  void set_handle(OsFile_Handle value);

  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS);
  void dispose();
};

class JarFileParserDesc : public MixedOopDesc {
 /*
  * Holds the OsFile_Handle associated with this JarFileParser. This
  * is an instance of com.sun.cldchi.jvm.FileDescriptor class, so that we
  * can use a finalizer to close the handle during GC.
  */
  OopDesc *          _file_descriptor;

 /*
  * The Jar file being handled by this JarFileParser object. This
  * information is used in JarFileParser::get() to cache the last Jar
  * processed file. It's a byte array.
  */
  TypeArrayDesc *    _pathname;

  BufferedFileDesc*  _buffered_file;

#if ENABLE_JAR_ENTRY_CACHE
  /**
   * Cache for the JAR file's header table. It's used to speed up
   * entry searching in JarFileParser.cpp. This is an ObjArray. Each
   * element is a byte TypeArray:
   *
   *        [centralHeader][name ...]
   *
   * This ObjArray is grown automatically, up to the size specified in
   * the global MaxJarCacheEntryCount.
   */
  OopDesc *         _entry_cache;


  //
  // All non-oop fields must appear below here.
  //


  /*
   * There may be empty slots at the end of _entry_cache. Only the first
   * _entry_cache_count slots are in use.
   */
  int               _entry_cache_count;
#endif

  int               _timestamp;

  /*
   * Information about the current JAR entry being processed
   */
  JarInfoEntry      _current_entry;

  /*
   * In some cases it's undesirable to cache the entries, such as in
   * JarFileParser::remove_class_entries().
   */
  bool              _enable_entry_cache;

  friend class JarFileParser;
};

class JarFileParser : public MixedOop {
public:
  HANDLE_DEFINITION(JarFileParser, MixedOop);

  static size_t allocation_size() {
    return align_allocation_size(sizeof(JarFileParserDesc));
  }
  static size_t pointer_count() {
#if ENABLE_JAR_ENTRY_CACHE
    return 4;
#else
    return 3;
#endif
  }

  // Initialize static data structures used for caching.
  static void initialize();

public:
  // To avoid endless lists of friends the static offset computation
  // routines are all public.
  static jint timestamp_offset() {
    return FIELD_OFFSET(JarFileParserDesc, _timestamp);
  }
  static jint enable_entry_cache_offset() {
    return FIELD_OFFSET(JarFileParserDesc, _enable_entry_cache);
  }
  static jint pathname_offset() {
    return FIELD_OFFSET(JarFileParserDesc, _pathname);
  }
  static jint file_descriptor_offset() {
    return FIELD_OFFSET(JarFileParserDesc, _file_descriptor);
  }
  static jint buffered_file_offset() {
    return FIELD_OFFSET(JarFileParserDesc, _buffered_file);
  }
  static jint current_entry_offset() {
    return FIELD_OFFSET(JarFileParserDesc, _current_entry);
  }

  JarInfoEntry * raw_current_entry() const {
    return (JarInfoEntry *)(obj()->int_field_addr(current_entry_offset()));
  }

  ReturnOop file_descriptor() const {
    return obj_field(file_descriptor_offset());
  }
  void set_file_descriptor(Oop *value) {
    obj_field_put(file_descriptor_offset(), value);
  }

  ReturnOop pathname() const {
    return obj_field(pathname_offset());
  }
  void set_pathname(TypeArray *value) {
    obj_field_put(pathname_offset(), value);
  }

  ReturnOop buffered_file() const {
    return obj_field(buffered_file_offset());
  }
  void set_buffered_file(BufferedFile *value) {
    obj_field_put(buffered_file_offset(), value);
  }

  void dispose() const;

  int timestamp() const {
    return int_field(timestamp_offset());
  }
  void set_timestamp(int value) {
    int_field_put(timestamp_offset(), value);
  }

  bool enable_entry_cache() const {
    return bool_field(enable_entry_cache_offset());
  }
  void set_enable_entry_cache(bool value) {
    bool_field_put(enable_entry_cache_offset(), value);
  }

#if ENABLE_JAR_ENTRY_CACHE
  static jint entry_cache_offset() {
    return FIELD_OFFSET(JarFileParserDesc, _entry_cache);
  }
  static jint entry_cache_count_offset() {
    return FIELD_OFFSET(JarFileParserDesc, _entry_cache_count);
  }

  ReturnOop entry_cache() {
    return obj_field(entry_cache_offset());
  }
  void set_entry_cache(ObjArray *value) {
    obj_field_put(entry_cache_offset(), value);
  }

  int entry_cache_count() const {
    return int_field(entry_cache_count_offset());
  }
  void set_entry_cache_count(int value) {
    int_field_put(entry_cache_count_offset(), value);
  }
#endif

private:
  static ReturnOop get_parser_from_cache(const JvmPathChar* jar_file_name1,
                                         TypeArray* jar_file_name2);
  void save_parser_in_cache(JVM_SINGLE_ARG_TRAPS);

  static ReturnOop get(const JvmPathChar* jar_file_name1, 
                       TypeArray* jar_file_name12,
                       bool enable_entry_cache JVM_TRAPS);
public:
#if !ROMIZED_PRODUCT
  static void fix_bootstrap();
#endif

  OsFile_Handle handle() const;

  static ReturnOop get(const JvmPathChar* jar_file_name,
                       bool enable_entry_cache JVM_TRAPS) {
    return get(jar_file_name, NULL, enable_entry_cache JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop get(const JvmPathChar* jar_file_name JVM_TRAPS) {
    return get(jar_file_name, true JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop get(TypeArray* jar_file_name,
                       bool enable_entry_cache JVM_TRAPS) {
    return get(NULL, jar_file_name, enable_entry_cache JVM_NO_CHECK_AT_BOTTOM);
  }
  static ReturnOop get(TypeArray* jar_file_name JVM_TRAPS) {
    return get(jar_file_name, true JVM_NO_CHECK_AT_BOTTOM);
  }

  static bool parser_cache_is_empty() PRODUCT_RETURN0;

#ifndef PRODUCT
  void iterate(OopVisitor* visitor);
#endif

private:
  static jboolean suffix_match_filter(const char* name, void* caller_data);
  bool find_end_of_central_header();

public:
  bool find_entry(const char* entry_name JVM_TRAPS);
  ReturnOop open_entry(int flags JVM_TRAPS);
  ReturnOop load_entry(JVM_SINGLE_ARG_TRAPS);

  static void flush_caches();

#if USE_JAR_ENTRY_ENUMERATOR
  typedef void (*do_entry_proc)(char* name, int length, JarFileParser *jf
                                JVM_TRAPS);
  static void do_entries(const JvmPathChar* jar_file_name, const char*suffix,
                         bool should_match, 
                         do_entry_proc f JVM_TRAPS) {
    do_next_entries(jar_file_name, suffix, should_match, f, /*entry_id=*/0, 
                    /*max_size=*/max_jint
                    JVM_NO_CHECK_AT_BOTTOM);
  }

  /*
   * Iterates over entries in the specified JAR file.
   * entry_id specifies the next entry to load.
   * entry_id of the first entry in the JAR is 0.
   * chunk_size defines the total compressed size of entries
   * to load from the path within this chunk.
   *
   * The routine iterates over entries until either it reaches the end of JAR
   * or the total compressed size of loaded entries is greater than 
   * chunk_size.
   *
   * Returns 0 if successfully reaches the end of the JAR. 
   * Returns -1 if failed to iterate some entry in the JAR.
   * Otherwise, returns entry_id of the next entry.
   */
  static int do_next_entries(const JvmPathChar* jar_file_name, 
                             const char*suffix, bool should_match,
                             do_entry_proc f, int entry_id, int max_size
                             JVM_TRAPS);
  static bool match(const char *suffix, const char *name, int name_len);
#endif // USE_JAR_ENTRY_ENUMERATOR

#if ENABLE_JAR_READER_EXPORTS || USE_JAR_ENTRY_ENUMERATOR
  bool filtered_do_entries(JvmNameFilterProc entry_filter_proc,
                           JvmDoJarEntryProc do_jar_entry_proc,
                           void* caller_data JVM_TRAPS) {
    int ret_code = filtered_do_next_entries(entry_filter_proc,
                                            do_jar_entry_proc,
                                            caller_data,
                                            0, max_jint
                                            JVM_NO_CHECK_AT_BOTTOM);
    return ret_code == 0 ? true : false;
  }

  int filtered_do_next_entries(JvmNameFilterProc entry_filter_proc,
                               JvmDoJarEntryProc do_jar_entry_proc,
                               void* caller_data,
                               int entry_id, int max_size JVM_TRAPS);
#endif

#if ENABLE_JAR_ENTRY_CACHE
  bool find_entry_from_cache(const char *entryname);
  bool add_current_entry_to_cache(char * name, int name_len JVM_TRAPS);
  bool add_current_entry_to_cache0(char * name, int name_len JVM_TRAPS);
#else
  inline bool find_entry_from_cache(const char* /*entryname*/) {
    return false;
  }
  inline bool add_current_entry_to_cache(char* /*name*/, int /*name_len*/
                                         JVM_TRAPS) {
    return false;
  }
#endif

#if ENABLE_ROM_GENERATOR
  // Removes all .class entries from the JAR file with the specified path.
  static bool remove_class_entries(FilePath *path JVM_TRAPS);
 private:
  bool copy_non_class_entries_to(OsFile_Handle data_file_handle,
                                 OsFile_Handle directory_file_handle 
                                 JVM_TRAPS);
  static size_t block_copy(BufferedFile* buffered_file,
                           OsFile_Handle destination_file_handle, 
                           size_t bytes_to_copy);
#endif

private:
  enum {
    // Max length of JAR entry names. If your JAR file contains an entry
    // whose name is longer than 256 bytes, it will fail to be read.
    MAX_ENTRY_NAME = 256,

    // Max number of JarFileParser objects to be cached. Change this number
    // if your application is likely to access a large number of Jar files
    // at the same time.
    //
    // See the global MaxCachedJarParsers. The number of actual parsers
    // cached are min(MaxCachedJarParsers, MAX_CACHED_PARSERS)
    MAX_CACHED_PARSERS = 4
  };

  static int _cached_parsers [MAX_CACHED_PARSERS];
  static int _timestamp;

  static void dispose( const int i );
};
