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
# include "incls/_JarFileParser.cpp.incl"

//----------------------------------------------------------------------
// JAR File Caching 
//
// [1] Overview
//
//     Jar File Caching is done at two levels to speed up the loading
//     of classes and resources from JAR files, especially on
//     embedded devices which may have slow file systems:
//
//     - OsFile_Handles are cached, so that JAR files do not need to be
//       opened repeatedly.
//     - The entries table in a JAR file is cached, so we do need to
//       read the file repeatedly to search for the entries.
//
// [2] Requirements for Cache implementation
//
//     [a] Automatic reclaimation -- no need for explicit reference 
//         counting. This is especially important becasue of VM's
//         TRAPS mechanism. You can easiy forget to decrement a
//         reference count duo to an exception.
//     [b] Deterministic usage -- OsFile_Handles are scarce on embedded
//         devices. We want to keep the number of open file handles
//         below a fixed bound at all times. We cannot depend on 
//         finalizers to free unused handles.
//
//     [a] and [b] are often mutually exclusive.
//
// [3] Data Structures and Organization
//
//     [a] JarFileParser -- it plays the central role in JAR file caching.
//     [b] FileDescriptor -- it's used to free OsFile_Handles when
//         a JarFileParser is garbage collected.
//     [c] FileDecoder and Inflater -- these two classes rely on 
//         JarFileParser to provide a cache for OsFile_Handles.
//
//     When you open a JAR file to search its entries table for the
//     first time, a JarFileParser is created. A fixed number
//     (MAX_CACHED_PARSERS, default = 4) of JarFileParser may be cached,
//     using the ObjectHeap's weak reference mechanism. In the future,
//     when you need to open the same JAR file again, you can use
//     the cached JarFileParser, if it's still available.
//     See JarFileParser::get().
//
//     JarFileParser objects are only (strongly) referenced in Oop
//     handles. They should never be stored anywhere else, including
//     in regular Java objects. As a result, when no C code is active,
//     all cached JarFileParser objects are eligible for GC.
//
//     When JAR file processing is happening (either when JarFileParser
//     is searching the entries table, or when Inflater is decoding
//     a compressed entry), we require that an active Oop handle holds
//     the associated JarFileParser object. As a result, the JarFileParser
//     will not be GC'ed, and thus the OsFile_Handle will be kept open.
//
//     The above requirement is quite trivial for JarFileParser - when a 
//     JarFileParser is active, it must already be in an active Oop handle.
//     However, since Inflater objects may be active outside of the scope
//     of a JarFileParser (see SystemDictionary::fetch_buffer() and
//     Java_com_sun_cldc_io_ResourceInputStream_readBytes), we require that
//     all public APIs that can reach an Inflater (FileDecoder::get_bytes
//     and FileDecoder::read_completely) to explicitly store a JarFileParser
//     in an Oop handle, before attempting to access the OsFile_handle).
//     See FileDecoder::get_jar_parser_if_needed and
//     FileDecoder::guarantee_jar_file_handle.
//
// [4] Reclaimation of OsFile_Handles
//
//     OsFile_Handles may be reclaimed in the following ways:
//     - A currently cached JarFileParser object is
//       GC'ed with the weak reference mechanism. Its FileDescriptor
//       object is GC'ed at the same time. The OsFile_Handle is closed
//       via finalize(). All of this happens within the same GC cycle.
//     - When a JarFileParser is evicted from the cache, its OsFile_Handle
//       is closed immediately.
//     - An explicit call to JarFileParser::flush_caches() force
//       all currently cached JarFileParser objects to close their
//       OsFile_Handles immediately. Note this may happen inside
//       JarFileParser::get(), so that it the system is running low on
//       OsFile_Handles, the VM will limit its use of OsFile_Handles to
//       at most 1. This allows the VM to co-exist with MIDP code that
//       might be less conservative in opening OsFile_Handles.
//
// [5] Only one active JarFileParser at any time
//
//     This requirement is essential for the caching algorithm to work
//     as described. If you hold on to one JarFileParser in an Oop,
//     while processing another Jar file, some or all of the nice
//     properties described above may be defeated.
//----------------------------------------------------------------------

int JarFileParser::_cached_parsers[MAX_CACHED_PARSERS];
int JarFileParser::_timestamp;

void JarFileParser::dispose( const int i ) {
  const int ref = _cached_parsers[i];
  if( ref >= 0 ) {
    JarFileParser::Raw parser = ObjectHeap::get_global_ref_object( ref );
    if( parser.not_null() ) {
      parser().dispose();
    }
    ObjectHeap::unregister_global_ref_object( ref );
    _cached_parsers[i] = -1;
  }
}

// Initialize global data structures used by JarFileParser.
void JarFileParser::initialize() {
  // Set all entries to -1
  jvm_memset(_cached_parsers, 0xff, sizeof _cached_parsers);
  _timestamp = 0;
}

inline void JarFileParser::save_parser_in_cache(JVM_SINGLE_ARG_TRAPS) {
  const int max = MAX_CACHED_PARSERS < MaxCachedJarParsers ?
                  MAX_CACHED_PARSERS : MaxCachedJarParsers;
  int found = 0;
  int oldest = max_jint;

  // (1) Find a location the _cached_parsers[]
  for (int i=0; i<max; i++) {
    int ref = _cached_parsers[i];
    if (ref < 0) { // This slot is not used.
      found = i;
      break;
    } else {
      JarFileParser::Raw parser = ObjectHeap::get_global_ref_object(ref);
      if (parser.is_null()) {
        // The cached parser has been GC'ed
        found = i;
        break;
      }
      if (parser().timestamp() < oldest) {
        // It's not GC'ed, let use it if it's the oldest of the non-yet-GC'ed
        // parsers.
        oldest = parser().timestamp();
        found = i;
      }
    }
  }

  dispose( found );

  const int newref = ObjectHeap::register_global_ref_object(this,
    ObjectHeap::WEAK JVM_MUST_SUCCEED);
  if( newref >= 0 ) {
    _cached_parsers[found] = newref;
  }

#ifndef ENABLE_TTY_TRACE
  if (TraceJarCache) {
    TTY_TRACE(("JAR: parser cached: %d ", found));
    TypeArray::Raw stored_name = pathname();
    JvmPathChar *data = (JvmPathChar*)stored_name().byte_base_address();
    for (int n=0; n<stored_name().length(); n++) {
      TTY_TRACE(("%c", (char)data[n]));
    }
    TTY_TRACE_CR((""));
  }
#endif
}

// Create or retrieve a JarFileParser for the given JAR file.
// As a side effect, it might invalidate any previously obtained
// JarFileParser objects by closing their OsFile_handles.
ReturnOop JarFileParser::get(const JvmPathChar* jar_file_name1,
                             TypeArray * jar_file_name2,
                             bool enable_entry_cache JVM_TRAPS) {
  GUARANTEE((jar_file_name1 != NULL && jar_file_name2 == NULL) ||
            (jar_file_name1 == NULL && jar_file_name2 != NULL), "sanity");

  UsingFastOops fast_oops;
  JarFileParser::Fast parser = get_parser_from_cache(jar_file_name1,
                                                     jar_file_name2);
  if (parser.not_null()) {
    return parser;
  }

  if (jar_file_name1 && !OsFile_exists(jar_file_name1)) {
    return NULL;
  }
  if (jar_file_name2 && 
      !OsFile_exists((JvmPathChar *)jar_file_name2->byte_base_address())) {
    return NULL;
  }

  parser = Universe::new_mixed_oop(MixedOopDesc::Type_JarFileParser,
                                   JarFileParser::allocation_size(),
                                   JarFileParser::pointer_count()
                                   JVM_CHECK_0);
  TypeArray::Fast stored_name;
  if (jar_file_name1 != NULL) {
    size_t name_bytes = (fn_strlen(jar_file_name1)+1) * sizeof(JvmPathChar);
    stored_name = Universe::new_byte_array_raw(name_bytes JVM_CHECK_0);
    JvmPathChar *data = (JvmPathChar *)stored_name().byte_base_address();
    jvm_memcpy(data, jar_file_name1, name_bytes); // copy trailing NUL as well.
  } else {
    stored_name = jar_file_name2->obj();
  }


  BufferedFile::Fast bf = BufferedFile::allocate(JVM_SINGLE_ARG_CHECK_0);
  FileDescriptor::Fast desc = FileDescriptor::allocate(JVM_SINGLE_ARG_CHECK_0);

  OsFile_Handle fh = NULL;
  for (int pass=0; pass<2; pass++) {
    if (jar_file_name1) {
      fh = OsFile_open(jar_file_name1, "rb");
    } else {
      fh = OsFile_open((JvmPathChar *)jar_file_name2->byte_base_address(),
                       "rb");
    }
    if (fh != NULL) {
      break;
    }
    if (pass == 1 && fh == NULL) {
      // The system is running low on OsFile_Handles. Make sure we flush
      // the cache, and free all currently cached OsFile_Handles that belong
      // to other JAR files.
      flush_caches();
    }
  }
  if (fh == NULL) {
    return NULL;
  }

  desc().set_handle(fh);
  bf().set_file_pointer(fh);
  bf().set_file_size(fh == NULL ? 0 : OsFile_length(fh));
  parser().set_file_descriptor(&desc);
  parser().set_enable_entry_cache(enable_entry_cache);
  parser().set_pathname(&stored_name);
  parser().set_buffered_file(&bf);
  parser().set_timestamp(++_timestamp);

  if (!parser().find_end_of_central_header()) {
    // The jar file is corrupted. Stop parsing it.
    return NULL;
  }

  parser().save_parser_in_cache(JVM_SINGLE_ARG_MUST_SUCCEED);
  return parser;
}

#ifndef PRODUCT
bool JarFileParser::parser_cache_is_empty() {
  const int max = MAX_CACHED_PARSERS < MaxCachedJarParsers ?
                  MAX_CACHED_PARSERS : MaxCachedJarParsers;

  for( int i=0; i<max; i++ ) {
    const int ref = _cached_parsers[i];
    if( ref >= 0 && ObjectHeap::get_global_ref_object( ref ) ) {
      return false;
    }
  }

  return true;
}
#endif

// Call when VM shuts down. Close all FileDescriptor objects that
// have not been GC'ed
void JarFileParser::flush_caches() {
  const int max = MAX_CACHED_PARSERS < MaxCachedJarParsers ?
                  MAX_CACHED_PARSERS : MaxCachedJarParsers;

  for( int i=0; i < max; i++ ) {
    dispose( i );
  }
}

ReturnOop
JarFileParser::get_parser_from_cache(const JvmPathChar* jar_file_name1,
                                     TypeArray *jar_file_name2) {
  // Count trailing zero as well
  const int name_bytes = jar_file_name1
    ? (fn_strlen(jar_file_name1)+1) * sizeof(JvmPathChar)
    : jar_file_name2->length();

  const int max = MAX_CACHED_PARSERS < MaxCachedJarParsers ?
                  MAX_CACHED_PARSERS : MaxCachedJarParsers;

#if ENABLE_ISOLATES
  const char current_task_id = TaskContext::current_task_id();
#endif
  for( int i=0; i < max; i++ ) {
    const int ref = _cached_parsers[i];
    if (ref >= 0) {
#if ENABLE_ISOLATES
      const char task_id = ObjectHeap::get_global_reference_owner(ref);
      if( task_id != current_task_id ) {
        // Don't share cached JarFileParser across task ID. See
        // JarFileParser.hpp for why.
        continue;
      }
#endif
      JarFileParser::Raw parser = ObjectHeap::get_global_ref_object(ref);
      if (parser.not_null()) {
        TypeArray::Raw stored_name = parser().pathname();
        JvmPathChar *data = (JvmPathChar*)stored_name().byte_base_address();
        bool match;
        if (jar_file_name1) {
          match = stored_name().length() == name_bytes &&
                  jvm_memcmp(data, jar_file_name1, name_bytes) == 0;
        } else {
          match = jar_file_name2->equals(&stored_name) ||
                  (stored_name().length() == name_bytes &&
                   jvm_memcmp(data, jar_file_name2->byte_base_address(),
                              name_bytes) == 0);
        }

        if (match) {
          if (TraceJarCache) {
            TTY_TRACE(("JAR: parser cache hit: "));
            for (int n=0; n <= name_bytes; n++) {
              TTY_TRACE(("%c", (char)data[n]));
            }
            TTY_TRACE_CR((""));
          }

          // Found a match
          parser().set_timestamp(++_timestamp);
          return parser;
        }
      }
    }
  }

  return NULL;
}

OsFile_Handle JarFileParser::handle() const {
  FileDescriptor::Raw desc = file_descriptor();
  GUARANTEE(desc().valid(), "sanity");
  return desc().handle();
}

void JarFileParser::dispose() const {
  FileDescriptor::Raw desc = file_descriptor();
  desc().dispose();
#ifdef AZZERT
  BufferedFile::Raw bf = buffered_file();
  bf().junk_file_pointer();
#endif
}

#if !ROMIZED_PRODUCT
// This function fixes the dummy object allocated by FileDescriptor::allocate()
// before the Universe was fully initialized.
void JarFileParser::fix_bootstrap() {
  const int max = MAX_CACHED_PARSERS < MaxCachedJarParsers ?
                  MAX_CACHED_PARSERS : MaxCachedJarParsers;

  for (int i=0; i<max; i++) {
    int ref = _cached_parsers[0];
    if (ref >= 0) {
      JarFileParser::Raw parser = ObjectHeap::get_global_ref_object(ref);
      GUARANTEE(parser.not_null(), "must not be GC'ed yet!");
      OopDesc** desc = (OopDesc**)parser().file_descriptor();
      OopDesc* n = Universe::file_descriptor_class()->prototypical_near();

      // Make it a real FileDescriptor object.
      oop_write_barrier(desc, n);

      // This line tests that n is now really a FileDescriptor.
      FileDescriptor::Raw try_it = parser().file_descriptor();
      (void)try_it;
    }
  }

  // We still haven't put the fixed up FileDescriptor into the list of
  // finalizable objects yet. It's easier just to flush the cache now
  // and close the FileDescriptors now ... this code is used only in
  // non-product mode, so there's no need to make it fancy.
  flush_caches();
}
#endif

bool JarFileParser::find_end_of_central_header() {
  DECLARE_STATIC_BUFFER(unsigned char, buffer, TMPBUFFERSIZE);
  BufferedFile::Raw bf = buffered_file();

  /* Get the length of the file */
  const jint length = (int) bf().file_size();

  /* Calculate the smallest possible offset for the end header.  It
   * can be at most 0xFFFF + ENDHDRSIZ bytes from the end of the file, but
   * the file must also have a local header and a central header
   */
  jint minOffset = length - (0xFFFF + ENDHDRSIZ);
  if (minOffset < LOCHDRSIZ + CENHDRSIZ) {
    minOffset = LOCHDRSIZ + CENHDRSIZ;
  }

  /* We assume that "buffer" contains the contents
   * of part of the file. currentOffset contains the offset of buffer[0].
   */

  /* Read in the last ENDHDRSIZ bytes into the buffer.  99% of the time,
   * the file won't have a comment, and this is the only read we'll need */
  if ( (bf().seek(-ENDHDRSIZ, SEEK_END) < 0)
    || (bf().get_bytes(buffer, ENDHDRSIZ) != ENDHDRSIZ)) {
    return false;
  }
  /* Set currentOffset to be the offset of buffer[0] */
  jint currentOffset = length - ENDHDRSIZ;
  /* Set bp to be the location at which to start looking */
  unsigned const char* bp = buffer;

  for (;;) {
    /* "buffer" contains a block of data from the file, starting at
     * currentOffset "position" in the file.
     * We investigate whether   currentOffset + (bp - buffer)  is the start
     * of the end header in the zip file.
     *
     * We use a simplified version of Knuth-Morris-Pratt search algorithm.
     * The header we're looking for is 'P' 'K' 5  6
     */
    switch(bp[0]) {
    case '\006':   /* The header must start at least 3 bytes back */
      bp -= 3; break;
    case '\005':   /* The header must start at least 2 bytes back  */
      bp -= 2; break;
    case 'K':      /* The header must start at least 1 byte back  */
      bp -= 1; break;
    case 'P':      /* Either this is the header, or the header must
                    * start at least 4  back */
      if (bp[1] == 'K' && bp[2] == 5 && bp[3] == 6) {
        /* We have what may be a header.  Let's make sure the
         * implied length of the jar file matches the actual
         * length.
         */
        int endpos = (int) currentOffset + (bp - buffer);
        if (endpos + ENDHDRSIZ + ENDCOM(bp) == length) {
          juint cenOffset = endpos - ENDSIZ(bp);
          juint locOffset = cenOffset - ENDOFF(bp);
          unsigned char sig[4];

          if (bf().seek(locOffset, SEEK_SET) >= 0 &&
              bf().get_bytes(sig, 4) == 4 &&
              sig[0] == (unsigned char)'P' && 
              sig[1] == (unsigned char)'K' && 
              sig[2] == (unsigned char) 3  && 
              sig[3] == (unsigned char) 4) {

            raw_current_entry()->cenOffset = cenOffset;
            raw_current_entry()->nextCenOffset = cenOffset;
            raw_current_entry()->locOffset = locOffset;
#if ENABLE_ROM_GENERATOR
            raw_current_entry()->totalEntryCount = ENDTOT(bp);
#endif
          }
          return true; // Found central header
        }
      }
      /* FALL THROUGH */
    default:
      /* The header must start at least four characters back, since
       * the current character isn't in the header */
      bp -= 4;
    }
    if (bp < buffer) {
      /* We've moved outside our window into the file.  We must
       * move the window backwards */
      size_t count = (size_t) (currentOffset - minOffset); /* Bytes left in file */
      if (((jint)count) <= 0) {
        /* Nothing left to read.  Time to give up */
        return false;
      } else {
        /* up to ((bp - buffer) + ENDHDRSIZ) bytes in the buffer might
         * still be part of the end header, so the most bytes we can
         * actually read are
         *      TMPBUFFERSIZE - ((bp - buffer) + ENDHDRSIZE).
         */
        size_t available = (TMPBUFFERSIZE - ENDHDRSIZ) + (buffer - bp);
        if (count > available) {
          count = available;
        }
      }
      /* Back up, while keeping our virtual currentOffset the same */
      currentOffset -= count;
      bp += count;
      jvm_memmove(buffer + count, buffer, TMPBUFFERSIZE - count);
      if ( bf().seek(currentOffset, SEEK_SET) < 0 ||
           bf().get_bytes(buffer, count) != size_t(count) ) {
        return false;
      }
    }
  } /* end of for loop */
}

#if USE_JAR_ENTRY_ENUMERATOR

typedef struct {
  const char*                  _suffix;
  bool                         should_match;
  JarFileParser*               _jf;
  JarFileParser::do_entry_proc _f;
} SuffixMatchFilterData;

jboolean JarFileParser::suffix_match_filter(const char* name,
                                            void* caller_data) {
  SETUP_ERROR_CHECKER_ARG;
  SuffixMatchFilterData* data = (SuffixMatchFilterData*)caller_data;
  int length = jvm_strlen(name);
  if (name[length-1] == '/') { //this is  directory. skip it
    return KNI_TRUE;
  } 
  if (match(data->_suffix, name, length) == data->should_match) {
    data->_f((char*)name, length, data->_jf JVM_CHECK_0);
  }

  return KNI_TRUE;
}

int JarFileParser::do_next_entries(const JvmPathChar* jar_file_name,
                                   const char *suffix,
                                   bool should_match, 
                                   JarFileParser::do_entry_proc f,
                                   int entry_id, int max_size JVM_TRAPS)
{
  GUARANTEE(entry_id >= 0 && max_size > 0, "Sanity");
  int result = -1;

  // JAR entry iteration is not compatible with CacheJarEntries. The 
  // easiest way make iteration co-exist with CacheJarEntries (especially
  // in an MVM environment where other tasks may be loading class files
  // at the same time) is to flush the JarFileParser caches before and after.

#if ENABLE_JAR_ENTRY_CACHE
  // Make sure we don't have a JarFileParser that has entry caching enabled.
  flush_caches();
#endif
  {
    UsingFastOops fast_oops;
    JarFileParser::Fast parser = get(jar_file_name, 
                                     /*enable_entry_cache=*/false
                                     JVM_NO_CHECK);
    if (parser.not_null()) {
      SuffixMatchFilterData data = { suffix, should_match, &parser, f };
      result =
        parser().filtered_do_next_entries(&JarFileParser::suffix_match_filter, 
                                          NULL, &data, entry_id, max_size 
                                          JVM_NO_CHECK);
    }
  }
#if ENABLE_JAR_ENTRY_CACHE
  // Make sure we don't leave behind a JarFileParser that has entry caching
  // disabled.
  flush_caches();
#endif

  return result;
}

bool JarFileParser::match(const char *suffix, const char *name, int name_len) {
  int suffix_len = jvm_strlen(suffix);
  int skip = name_len - suffix_len;

  if (skip <= 0) {
    // i.e., the suffix ".class" matches with the name "x.class", but not with
    // the name ".class"
    return false;
  }
  name += skip;
  if (jvm_memcmp(name, suffix, suffix_len) == 0) {
    return true;
  } else {
    return false;
  }
}
#endif // USE_JAR_ENTRY_ENUMERATOR

#if ENABLE_JAR_READER_EXPORTS || USE_JAR_ENTRY_ENUMERATOR
int
JarFileParser::filtered_do_next_entries(JvmNameFilterProc entry_filter_proc,
                                        JvmDoJarEntryProc do_jar_entry_proc,
                                        void* caller_data,
                                        int entry_id, int max_size 
                                        JVM_TRAPS) {
  GUARANTEE(entry_id >= 0 && max_size > 0, "Sanity");
  DECLARE_STATIC_BUFFER(char, entry_name,  MAX_ENTRY_NAME);
  unsigned int name_length = 0;
  int total_size = 0;

  if (max_size <= 0) {
    max_size = max_jint;
  }

  if (entry_id > 0) {
    raw_current_entry()->nextCenOffset = entry_id;
  }

  // NOTE: max_entries can be -1 that indicates no limitation on the number of
  // entries processed.
  for (total_size = 0; total_size < max_size;) {
    bool found = find_entry(NULL JVM_MUST_SUCCEED);
    if (!found) {
      break;
    }

    // If find_entry succeeded, centralHeader contains the central directory  
    // header for the found entry.
    unsigned char *cenp = (unsigned char *)raw_current_entry()->centralHeader;
    name_length = CENNAM(cenp);

    if (name_length >= MAX_ENTRY_NAME) {
      return -1;
    }

    // Save the next offset as entry_filter_proc may move raw_current_entry()
    juint nextCenOffset = raw_current_entry()->nextCenOffset +
                          CENHDRSIZ + name_length + CENEXT(cenp) + CENCOM(cenp);

    {
      UsingFastOops fast_oops;
      BufferedFile::Fast bf = buffered_file();
      if (bf().get_bytes((address)entry_name, name_length) != name_length) {
        return -1;
      }
    }
    entry_name[name_length] = '\0';

    if (((entry_filter_proc == NULL) || 
         (entry_filter_proc(entry_name, caller_data) == KNI_TRUE)) &&
        (do_jar_entry_proc != NULL)) {
      UsingFastOops fast_oops;
      Buffer::Fast buffer = load_entry(JVM_SINGLE_ARG_CHECK_0);
      if (buffer.is_null()) {
        return -1;
      }

      if (do_jar_entry_proc((char*)entry_name, buffer().data(), 
                            buffer().length(), caller_data) == KNI_FALSE) {
        return 0;
      }
    }

    total_size += raw_current_entry()->length;

    if (CURRENT_HAS_PENDING_EXCEPTION) {
      // if entry_filter_proc returned NULL, check for exception
      return -1;
    }

    raw_current_entry()->nextCenOffset = nextCenOffset;
  }

  // If we reached the end of JAR return 0.
  // Otherwise return nextCenOffset, it is used as a JAR entry id.
  return (total_size < max_size) ? 0 : raw_current_entry()->nextCenOffset;
}
#endif // ENABLE_JAR_READER_EXPORTS || USE_JAR_ENTRY_ENUMERATOR

#if ENABLE_ROM_GENERATOR
/**
 * Copies the specified number of bytes from the BufferedFile 
 * to the destination file. The data is read from the BufferedFile 
 * and written to the destination file at their current positions.
 * Returns the number of bytes successfully copied.
 */
size_t JarFileParser::block_copy( BufferedFile* buffered_file,
                                  OsFile_Handle destination_file_handle, 
                                  size_t bytes_to_copy) {
  const size_t buffer_size = BufferedFile::BUFFERSIZE;
  DECLARE_STATIC_BUFFER(unsigned char, buffer, buffer_size);
  size_t bytes_left = bytes_to_copy;

  while (bytes_left > 0) {
    size_t block_size = bytes_left;
    size_t bytes_copied = 0;
    if (block_size > buffer_size) {
      block_size = buffer_size;
    }

    bytes_copied = buffered_file->get_bytes(buffer, block_size);
    bytes_copied = OsFile_write(destination_file_handle, buffer,
                                1, bytes_copied);

    bytes_left -= bytes_copied;

    if (bytes_copied != block_size) {
      break;
    }
  }

  return bytes_to_copy - bytes_left;
}

/**
 * Writes all entries except ".class" to the data_file_handle,
 * writes corresponding central directory records and the end of central
 * directory record to the directory_file_handle.
 * Treats .ZIP file format as documented in
 * ftp://ftp.uu.net/pub/archiving/zip/doc/appnote-970311-iz.zip.
 * Assumes that the .ZIP file doesn't span across multiple disks.
 * Assumes that raw_current_entry()->nextCenOffset contains the offset 
 * of the first entry of the central directory (as set in the constructor).
 * Returns true if completes successfully, false otherwise.
 */
bool
JarFileParser::copy_non_class_entries_to(OsFile_Handle data_file_handle,
                                         OsFile_Handle directory_file_handle 
                                         JVM_TRAPS) {
  UsingFastOops fast_oops;
  BufferedFile::Fast jar_buffer = buffered_file();

  const char class_suffix[] = {'.','c','l','a','s','s','\0'};
  juint locOffset = raw_current_entry()->locOffset;
  const juint buffer_size = MAX_ENTRY_NAME;
  DECLARE_STATIC_BUFFER(unsigned char, buffer,  buffer_size);
  unsigned int name_length = 0;
  juint loc_header_offset = 0;
  juint cen_header_offset = 0;
  size_t bytes_to_copy = 0;

  unsigned int written_entry_count = 0;
  unsigned int read_entry_count = 0;
  unsigned int total_entry_count = raw_current_entry()->totalEntryCount;

  // No OsFile_ routine to get the current position, 
  // so keep this information by ourselves.
  juint data_file_position = 0;
  juint directory_file_position = 0;

  GUARANTEE(buffer_size >= LOCHDRSIZ && 
            buffer_size >= EXTHDRSIZ && 
            buffer_size >= ENDHDRSIZ && 
            buffer_size >= MAX_ENTRY_NAME, "Buffer is too small");

  if ((OsFile_seek(data_file_handle, 0L, SEEK_SET) < 0) 
      || (OsFile_seek(directory_file_handle, 0L, SEEK_SET) < 0)) {
    return false;
  }

  for (read_entry_count = 0; read_entry_count < total_entry_count;
       read_entry_count++) {

    // Find the next entry in the JAR.
    bool found = find_entry(NULL JVM_MUST_SUCCEED);
    if (!found) {
      return false;
    }

    cen_header_offset = raw_current_entry()->nextCenOffset;

    // If find_entry succeeded, centralHeader contains the central directory  
    // header for the found entry.
    unsigned char *cenp = (unsigned char *)raw_current_entry()->centralHeader;

    name_length = CENNAM(cenp);

    if (name_length >= MAX_ENTRY_NAME) {
      Throw::error(jarfile_error JVM_THROW_0);
    } else {
      jar_buffer().get_bytes(buffer, name_length);
      if (!JarFileParser::match(class_suffix, (char*)buffer, name_length)) {
        loc_header_offset = locOffset + CENOFF(cenp);

        // Update the relative offset of local header 
        // in the central directory entry.
        // Use little-endian order according to the .ZIP format.
        Bytes::put_Java_u4(cenp + CENOFF_OFFSET,
                           Bytes::swap_u4(data_file_position));

        // Write the central directory entry to the directory file.
        if (OsFile_write(directory_file_handle, 
                         cenp, 1, CENHDRSIZ) != CENHDRSIZ) {
          return false;
        }

        directory_file_position += CENHDRSIZ;

        // Write the file name to the direcory file.
        if (OsFile_write(directory_file_handle, 
                         buffer, 1, name_length) != name_length) {
          return false;
        }

        directory_file_position += name_length;

        // Copy the extra field and file comment to the directory file.
        bytes_to_copy = CENEXT(cenp) + CENCOM(cenp);
        if (block_copy(&jar_buffer, directory_file_handle,
                       bytes_to_copy) != bytes_to_copy) {
          return false;
        }

        directory_file_position += bytes_to_copy;

        // Copy the local file header to the data file.
        if ((jar_buffer().seek(loc_header_offset, SEEK_SET) < 0) 
            || (jar_buffer().get_bytes(buffer, LOCHDRSIZ) != LOCHDRSIZ)
            || (OsFile_write(data_file_handle, 
                             buffer, 1, LOCHDRSIZ) != LOCHDRSIZ)) {
          return false;
        }

        data_file_position += LOCHDRSIZ;

        {
          unsigned char *locp = (unsigned char *)buffer;

          // Copy the file name, extra field and the compressed file data 
          // to the data file.
          bytes_to_copy = LOCNAM(locp) + LOCEXT(locp) + CENSIZ(cenp);
          if (block_copy(&jar_buffer, data_file_handle,
                         bytes_to_copy) != bytes_to_copy) {
            return false;
          }

          data_file_position += bytes_to_copy;

          // Check if the data descriptor exists.
          if ((LOCFLG(locp) & 8) == 8) {
            // Copy the data descriptor to the data file.
            if (jar_buffer().get_bytes(buffer, EXTHDRSIZ) != EXTHDRSIZ) {
              return false;
            }

            // The data descriptor may or may not start with the signature
            // depending on .ZIP file format revision used.
            if (GETSIG(buffer) == EXTSIG) {
              // According to the Info-ZIP Application Note 970311,
              // the data descriptor starts with the signature.
              if (OsFile_write(data_file_handle, 
                               buffer, 1, EXTHDRSIZ) != EXTHDRSIZ) {
                return false;
              }

              data_file_position += EXTHDRSIZ;
            } else {
              // According to the .ZIP format specification version 6.1.0,
              // the data descriptor doesn't start with the signature.
              if (OsFile_write(data_file_handle, 
                               buffer, 1, EXTHDRSIZ - 4) != EXTHDRSIZ - 4) {
                return false;
              }

              data_file_position += EXTHDRSIZ - 4;
            }
          }
        }

        written_entry_count++;
      }
    }

    cen_header_offset += CENHDRSIZ + name_length + CENEXT(cenp) + CENCOM(cenp);
    raw_current_entry()->nextCenOffset = cen_header_offset;
  }

  // Now that all central directory entries are processed, 
  // cen_header_offset points to the end of the central directory.
  if (jar_buffer().seek(cen_header_offset, SEEK_SET) < 0) {
    return false;
  }

  if (jar_buffer().get_bytes(buffer, ENDHDRSIZ) != ENDHDRSIZ) {
    return false;
  }

  {
    unsigned char *endp = (unsigned char *)buffer;

    if (GETSIG(endp) != ENDSIG) {
      return false;
    }

    // Update the record to match the new number of entries.
    // Use little-endian order according to the .ZIP format.

    Bytes::put_Java_u2(endp + ENDSUB_OFFSET,
                       Bytes::swap_u2(written_entry_count)); 
    Bytes::put_Java_u2(endp + ENDTOT_OFFSET,
                       Bytes::swap_u2(written_entry_count)); 

    // The size of the central directory is exactly 
    // the number of bytes written to the directory_file.
    Bytes::put_Java_u4(endp + ENDSIZ_OFFSET,
                       Bytes::swap_u4(directory_file_position)); 
    // The offset of the central directory is exactly 
    // the number of bytes written to the data_file.
    Bytes::put_Java_u4(endp + ENDOFF_OFFSET,
                       Bytes::swap_u4(data_file_position)); 

    if (OsFile_write(directory_file_handle, endp, 1, ENDHDRSIZ) != ENDHDRSIZ) {
      return false;
    }

    if (block_copy(&jar_buffer,
                   directory_file_handle, ENDCOM(endp)) != (size_t)ENDCOM(endp)) {
      return false;
    }
  }

  return true;
}

/**
 * Removes all .class entries from the JAR file with the specified path.
 * Assumes the process has read/write permissions to the JAR directory.
 * Returns true if completes successfully, false otherwise.
 */
bool JarFileParser::remove_class_entries(FilePath *path JVM_TRAPS) {
  const unsigned int buffer_size = 512;
  const unsigned int suffix_length = 5;
  const JvmPathChar data_file_suffix[suffix_length] = {'.','d','a','t',0};
  const JvmPathChar directory_file_suffix[suffix_length] = {'.','d','i','r',0};
  DECLARE_STATIC_BUFFER2(checker1, JvmPathChar, file_name, buffer_size);
  DECLARE_STATIC_BUFFER2(checker2, JvmPathChar, jar_name, buffer_size);
  OsFile_Handle jar_file_handle = NULL;
  OsFile_Handle data_file_handle = NULL;
  OsFile_Handle directory_file_handle = NULL;
  bool result = false;

  path->string_copy(file_name, buffer_size - suffix_length);

  if (!OsFile_exists(file_name)) {
    return false;
  }

  {
    JarFileParser parser = JarFileParser::get(file_name, false JVM_CHECK_0);
    if (parser.is_null()) {
      return false;
    }

    fn_strcat(file_name, directory_file_suffix);

    if ((directory_file_handle = OsFile_open(file_name, "wb")) == NULL) {
      return false;
    }

    file_name[path->length()] = 0;
    fn_strcat(file_name, data_file_suffix);

    if ((data_file_handle = OsFile_open(file_name, "wb")) == NULL) {
      return false;
    }

    result = parser.copy_non_class_entries_to(data_file_handle,
                                              directory_file_handle
                                              JVM_CHECK_0);

  }

  OsFile_close(data_file_handle);
  OsFile_close(directory_file_handle);

  {
    UsingFastOops fast_oops;
    BufferedFile::Fast buffered_file;
    size_t bytes_to_copy;

    if (result) {
      buffered_file = BufferedFile::allocate(JVM_SINGLE_ARG_CHECK_0);

      Buffer::Raw b = Universe::new_byte_array_raw(BufferedFile::BUFFERSIZE 
                                                   JVM_CHECK_0);
      buffered_file().set_data_buffer(&b);
      buffered_file().set_buffer_size(BufferedFile::BUFFERSIZE);

      path->string_copy(jar_name, buffer_size - suffix_length);

      OsFile_remove(jar_name); // just in case it exists
      result = OsFile_rename(file_name, jar_name);

      if (!result) {
        // Rename failed. Fall back to a plain block copy.
        if (buffered_file().open(file_name, "rb") 
            && ((jar_file_handle = OsFile_open(jar_name, "wb")) != NULL)) {

          bytes_to_copy = buffered_file().length();
          result = (block_copy(&buffered_file, 
                               jar_file_handle, bytes_to_copy) == bytes_to_copy);
          OsFile_close(jar_file_handle);
        }

        buffered_file().close();

        // Remove the data file.
        OsFile_remove(file_name);
      }
    }

    // Reconstruct the directory file name.
    file_name[path->length()] = 0;
    fn_strcat(file_name, directory_file_suffix);

    // Append the central directory to the JAR.
    if (result) {
      result = false;

      if (buffered_file().open(file_name, "rb") 
          && (jar_file_handle = OsFile_open(jar_name, "ab")) != NULL) {

        bytes_to_copy = buffered_file().length();
        result = (block_copy(&buffered_file, 
                             jar_file_handle, bytes_to_copy) == bytes_to_copy);
        OsFile_close(jar_file_handle);
      }

      buffered_file().close();
    }

    // Remove the directory file.
    OsFile_remove(file_name);
  }

  return result;
}

#endif // ENABLE_ROM_GENERATOR

#if ENABLE_JAR_ENTRY_CACHE

bool JarFileParser::find_entry_from_cache(const char *match_name) {
  ObjArray::Raw cache = entry_cache();
  TypeArray::Raw cache_line;
  jint size = entry_cache_count();
  jint match_name_len = jvm_strlen(match_name);

  for (int i=0; i<size; i++) {
    cache_line = cache().obj_at(i);
    jint name_len = cache_line().length() - CENHDRSIZ;

    if (name_len == match_name_len) {
      char *p = (char*)cache_line().base_address();
      char *name = p + CENHDRSIZ;

      if (jvm_memcmp(match_name, name, name_len) == 0) {
        unsigned char *cenp = raw_current_entry()->centralHeader;
        jvm_memcpy(cenp, p, CENHDRSIZ);
        raw_current_entry()->length = CENLEN(cenp);

        if (TraceJarCache) {
          TTY_TRACE_CR(("JAR: entry cache hit: %s", match_name));
        }

        return true;
      }
    }
  }

  return false;
}

// This function will never THROW
bool JarFileParser::add_current_entry_to_cache(char * name, int name_len
                                               JVM_TRAPS) {
  if (name_len > 1 && name[name_len-1] == '/') {
    // No need to cache any directory names
    return false;
  }

  bool result = add_current_entry_to_cache0(name, name_len JVM_NO_CHECK);
  if (!result) {
    if (CURRENT_HAS_PENDING_EXCEPTION) {
      Thread::clear_current_pending_exception();
      if (TraceJarCache) {
        TTY_TRACE_CR(("JAR: entry cache OutOfMemory"));
      }
    }
  }
  return result;
}

bool JarFileParser::add_current_entry_to_cache0(char * name, int name_len
                                                JVM_TRAPS) {
  UsingFastOops fast_oops;
  ObjArray::Fast cache = entry_cache();
  ObjArray::Fast new_cache;
  TypeArray::Fast cache_line;
  jint old_size = entry_cache_count();
  jint cache_line_size;

  // (1) Expand header list if necessary
  if (cache.is_null() || old_size == cache().length()) {
    jint new_size = old_size * 5 / 4 + 20;
    if (new_size > MaxJarCacheEntryCount) {
        new_size = MaxJarCacheEntryCount;
    }
    if (new_size <= old_size) {
      return false; // Do not exceed MaxJarCacheEntryCount
    }
    new_cache = Universe::new_obj_array(new_size JVM_CHECK_0);
    if (cache.not_null()) {
      ObjArray::array_copy(&cache, 0, &new_cache, 0, old_size JVM_CHECK_0);
    }
    set_entry_cache(&new_cache);
    cache = new_cache.obj();
  }

  // (2) Allocate the cache line
  cache_line_size = CENHDRSIZ + name_len;
  cache_line = Universe::new_byte_array_raw(cache_line_size JVM_CHECK_0);
  cache().obj_at_put(old_size, &cache_line);
  set_entry_cache_count(old_size+1);

  // (3) Copy information there
  char *p = (char*)cache_line().base_address();
  jvm_memcpy(p, raw_current_entry()->centralHeader, CENHDRSIZ);

  p += CENHDRSIZ;
  jvm_memcpy(p, name, name_len);

  if (TraceJarCache) {
    DECLARE_STATIC_BUFFER(char, tmp, MAX_ENTRY_NAME+1);
    jvm_memcpy(tmp, name, name_len);
    tmp[name_len] = 0;
    TTY_TRACE_CR(("JAR: cached entry [%d]: %s", old_size, tmp));
  }

  return true;
}

#endif // ENABLE_JAR_ENTRY_CACHE

ReturnOop JarFileParser::load_entry(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  FileDecoder::Fast fd = open_entry(0 JVM_CHECK_0);
  if (fd.not_null()) {
    return fd().read_completely(JVM_SINGLE_ARG_NO_CHECK_AT_BOTTOM);
  }
  return NULL;
}

/**
 * Finds an entry and stores the info into the buffer object. When successful,
 * raw_current_entry()->centralHeader[] will be filled with information
 * about the entry that we're looking for. This information will be used by
 * JarFile::load_entry() to seek to the beginning of the entry.
 */
bool JarFileParser::find_entry(const char *match_name JVM_TRAPS)
{
  UsingFastOops fast_oops;
  BufferedFile::Fast jar_buffer = buffered_file();
  const bool use_entry_cache = CacheJarEntries && enable_entry_cache();

  if (use_entry_cache && match_name != NULL && 
      find_entry_from_cache(match_name)) {
    return true;
  }

  DECLARE_STATIC_BUFFER(unsigned char, found_name, MAX_ENTRY_NAME);
  int offset = (int) (raw_current_entry()->nextCenOffset);
  int nextCenOffset = (int) (raw_current_entry()->nextCenOffset);
  unsigned int match_name_len, found_name_len;

  if (match_name == NULL) {
    match_name_len = 0xffffffff;
  } else {
    match_name_len = jvm_strlen(match_name);
  }

  while (true) {
    // raw_current_entry() may be moved by add_current_entry_to_cache(),
    // Must reload it after every iteration.
    unsigned char *cenp = (unsigned char *)raw_current_entry()->centralHeader;

    /* Offset contains the offset of the next central header. Read the
     * header into the temporary buffer */
    /* Go to the header */
    if (jar_buffer().seek(offset, SEEK_SET) < 0) {
      return false;
    }
    /* Read the bytes */
    if (jar_buffer().get_bytes(cenp, CENHDRSIZ) != CENHDRSIZ) {
      return false;
    }
    /* p contains the current central header */
    if (GETSIG(cenp) != CENSIG) {
      /* We've reached the end of the headers */
      return false;
    }

    /*
     * (1) Let's decide whether we want to read the entry name at all.
     *     In some cases, if we know the length of the name doesn't
     *     match, we can reject the name without reading it.
     */
    bool read_name = false;
    if (use_entry_cache) {
      // We're using entry cache. Always read the name.
      read_name = true;
    }

    found_name_len = (juint) CENNAM(cenp);
    if (found_name_len == match_name_len) {
      // The length of the name seems promising; let's read the name.
      read_name = true;
    }

    // Number of bytes to skip over the current entry
    int skip_bytes = (int) (CENHDRSIZ + found_name_len + CENEXT(cenp) + 
                            CENCOM(cenp));

    /*
     * (2) See if we found a matching entry
     */
    bool found = false;

    if (match_name == NULL) {
      // We're doing a wildcard search. Always return the entry. Let
      // the caller do more advanced matching (such as matching with *.class)
      found = true;
      nextCenOffset = offset;
    } else {
      if (read_name && (found_name_len <= MAX_ENTRY_NAME)) {
        if (jar_buffer().get_bytes(found_name, found_name_len)
            != found_name_len) { // I/O error
          return false;
        }
        if (use_entry_cache) {
          bool ok = add_current_entry_to_cache((char*)found_name,
                                               (int)found_name_len
                                               JVM_MUST_SUCCEED);
          if (ok) {
            // We have successfully cached the current entry (and all entries
            // before it). So when we run find_entry() again, we just need to
            // search after <offset> in the JarFile.
            nextCenOffset = offset + skip_bytes;
          }
        }

        // raw_current_entry() may be moved by
        // add_current_entry_to_cache(), Must reload it after every
        // iteration.
        cenp = (unsigned char *)raw_current_entry()->centralHeader;

        // we do length check again to handle caching case 
        if (found_name_len == match_name_len) {
          if (jvm_memcmp(found_name, match_name, match_name_len) == 0) {
            found = true;
          }
        }
      }
    }

    if (found) {
      raw_current_entry()->length = (int) CENLEN(cenp);
      raw_current_entry()->nextCenOffset = nextCenOffset;
      return true;
    }

    /* Advance to to the next central header */
    offset += skip_bytes;
  }
}

ReturnOop JarFileParser::open_entry(int flags JVM_TRAPS) {
  UsingFastOops fast_oops;
  BufferedFile::Fast jar_buffer = buffered_file();
  FileDecoder::Fast result;

  unsigned char *cenp = raw_current_entry()->centralHeader;
  juint decompLen   = CENLEN(cenp); /* the decompressed length */
  juint compLen     = CENSIZ(cenp); /* the compressed length */
  juint method      = CENHOW(cenp); /* how it is stored */
  juint expectedCRC = CENCRC(cenp); /* expected CRC */

  juint locOffset = raw_current_entry()->locOffset;
  DECLARE_STATIC_BUFFER(unsigned char, locp, TMPBUFFERSIZE);

  /* Make sure file is not encrypted */
  if ((CENFLG(cenp) & 1) == 1) {
    return NULL;
  }

  /* Go to the beginning of the LOC header */
  if (jar_buffer().seek(locOffset + CENOFF(cenp), SEEK_SET) < 0) {
    return NULL;
  }
  /* Read it */
  if (jar_buffer().get_bytes(locp, LOCHDRSIZ) != LOCHDRSIZ) {
    return NULL;
  }
  /* Skip over name and extension, if any */
  if (jar_buffer().seek(LOCNAM(locp) + LOCEXT(locp), SEEK_CUR) < 0) {
    return NULL;
  }

  OsFile_Handle handle = jar_buffer().file_pointer();
  int pos = jar_buffer().last_unread_file_pos();

  switch (method) {
  case STORED:
    if (compLen != decompLen) {
      return NULL;
    }
    result = FileDecoder::allocate(handle, pos, decompLen, flags JVM_CHECK_0);
    break;

  case DEFLATED:
    result = Inflater::allocate(handle, pos, decompLen, compLen, expectedCRC,
                                flags JVM_CHECK_0);
    break;
  }

  if (result.not_null()) {
    TypeArray::Raw jar_name = pathname();
    result().set_jar_file_name(&jar_name);
  }

  return result.obj();
}

#ifndef PRODUCT

#if USE_DEBUG_PRINTING
#define PRINT_JAR(x, p) tty->print_cr("%s = 0x%08x %10d", STR(x), x(p), x(p))

void pjfend(char*p) {
  PRINT_JAR(ENDSUB, p);
  PRINT_JAR(ENDTOT, p);
  PRINT_JAR(ENDSIZ, p);
  PRINT_JAR(ENDOFF, p);
  PRINT_JAR(ENDCOM, p);
}
void pjfcen(char*p) {
  PRINT_JAR(CENVEM, p);
  PRINT_JAR(CENVER, p);
  PRINT_JAR(CENFLG, p);
  PRINT_JAR(CENHOW, p);
  PRINT_JAR(CENTIM, p);
  PRINT_JAR(CENCRC, p);
  PRINT_JAR(CENSIZ, p);
  PRINT_JAR(CENLEN, p);
  PRINT_JAR(CENNAM, p);
  PRINT_JAR(CENEXT, p);
  PRINT_JAR(CENCOM, p);
  PRINT_JAR(CENDSK, p);
  PRINT_JAR(CENATT, p);
  PRINT_JAR(CENATX, p);
  PRINT_JAR(CENOFF, p);
}

void pjfloc(char*p) {
  PRINT_JAR(LOCVER, p);
  PRINT_JAR(LOCFLG, p);
  PRINT_JAR(LOCHOW, p);
  PRINT_JAR(LOCTIM, p);
  PRINT_JAR(LOCCRC, p);
  PRINT_JAR(LOCSIZ, p);
  PRINT_JAR(LOCLEN, p);
  PRINT_JAR(LOCNAM, p);
  PRINT_JAR(LOCEXT, p);
}

#undef PRINT_JAR
#endif // USE_DEBUG_PRINTING

void JarFileParser::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  {
    NamedField id("pathname", true);
    visitor->do_oop(&id, pathname_offset(), true);
  }
  {
    NamedField id("file_descriptor", true);
    visitor->do_oop(&id, file_descriptor_offset(), true);
  }
  {
    NamedField id("buffered_file", true);
    visitor->do_oop(&id, buffered_file_offset(), true);
  }

#if ENABLE_JAR_ENTRY_CACHE
  {
    NamedField id("entry_cache", true);
    visitor->do_oop(&id, entry_cache_offset(), true);
  }
  { 
    NamedField id("entry_cache_count", true);
    visitor->do_int(&id, entry_cache_count_offset(), true);
  }
#endif
  { 
    NamedField id("timestamp", true);
    visitor->do_int(&id, timestamp_offset(), true);
  }
  { 
    NamedField id("locOffset", true);
    visitor->do_int(&id, FIELD_OFFSET(JarFileParserDesc,
                                      _current_entry.locOffset), true);
  }
  { 
    NamedField id("cenOffset", true);
    visitor->do_int(&id, FIELD_OFFSET(JarFileParserDesc,
                                      _current_entry.cenOffset), true);
  }
  { 
    NamedField id("nextCenOffset", true);
    visitor->do_int(&id, FIELD_OFFSET(JarFileParserDesc,
                                      _current_entry.nextCenOffset), true);
  }
  { 
    NamedField id("length", true);
    visitor->do_int(&id, FIELD_OFFSET(JarFileParserDesc,
                                      _current_entry.length), true);
  }
#if ENABLE_ROM_GENERATOR
  { 
    NamedField id("totalEntryCount", true);
    visitor->do_int(&id, FIELD_OFFSET(JarFileParserDesc,
                                      _current_entry.totalEntryCount), true);
  }
#endif
  { 
    NamedField id("enable_entry_cache", true);
    visitor->do_int(&id, enable_entry_cache_offset(), true);
  }
#endif
}

#endif // !PRODUCT

ReturnOop FileDescriptor::allocate(JVM_SINGLE_ARG_TRAPS) {
  if (!UseROM && Universe::before_main()) {
    // Just put in a dummy object with enough space to hold all the fields
    // of a FileDescriptor. This will be fixed up in
    // JarFileParser::fix_bootstrap().
    return Universe::new_byte_array_raw(4 JVM_NO_CHECK_AT_BOTTOM);
  } else {
    return Universe::new_instance(Universe::file_descriptor_class()
                                  JVM_NO_CHECK_AT_BOTTOM);
  }
}

void FileDescriptor::set_handle(OsFile_Handle value) {
  int_field_put(handle_offset(), (int)value);
  set_valid(true);
}

void FileDescriptor::dispose() {
  if (valid()) {
    OsFile_close(handle());
    set_valid(false);
#ifdef AZZERT
    int_field_put(handle_offset(), -1);
#endif
  }
}

KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_cldchi_jvm_FileDescriptor_finalize() {
  FileDescriptor::Raw thisObj = GET_PARAMETER_AS_OOP(0);
  thisObj().dispose();
}
