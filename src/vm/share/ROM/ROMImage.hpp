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

/*
 * This header contains declarations of all the variables that should be
 * defined in a ROM image used by the VM.
 */

#ifndef _ROM_IMAGE_HPP_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * This data structure describes non-public romized methods that have
 * been renamed to ".unknown." to save space.
 */
struct OriginalMethodInfo {
  int          method;    /* MethodDesc*, points the method (must be in
                             TEXT_BLOCK) */
  const char * name;      /* 0-terminated UTF8 string of the original name */
};

struct OriginalFieldInfo {
  int flags;      /* Access flags of renamed field. */
  int offset;     /* Offset of the renamed field. */
  int name_index; /* index of name in ROM::alternate_constants_pool() */
  int sig_index;  /* index of signature in ROM::alternate_constants_pool()*/
};

struct OriginalClassInfo {
  const char *name;
  int num_methods;
  const OriginalMethodInfo *methods;
  int num_fields;
  const OriginalFieldInfo *fields;
};

extern const int              _rom_flags;
extern const int              _rom_symbol_table[];
extern const int              _rom_symbol_table_num_buckets;
       
extern const int              _rom_string_table[];
extern const int              _rom_string_table_num_buckets;
       
extern const int              _rom_has_linked_image;
extern const int              _rom_is_relaunchable;
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
extern const int              _rom_text_block0_size;           // in bytes
extern const int              _rom_text_block1_size;           // in bytes
extern const int              _rom_text_block2_size;           // in bytes
extern const int              _rom_text_block3_size;           // in bytes
extern const int              _rom_text_block4_size;           // in bytes
extern const int              _rom_text_block5_size;           // in bytes
extern const int              _rom_text_block6_size;           // in bytes
extern const int              _rom_text_block7_size;           // in bytes
extern const int              _rom_text_block8_size;           // in bytes
extern const int              _rom_text_block9_size;           // in bytes
#else
extern const int              _rom_text_block_size;            // in bytes
extern       int              _rom_text_block_size_fast;       // in bytes
#endif
extern const int              _rom_data_block_size;            // in bytes
extern       int              _rom_data_block_size_fast;       // in bytes
extern const int              _rom_data_block_scanned_size;    // in bytes
extern const int              _rom_heap_block_size;            // in bytes
extern const int              _rom_heap_block_permanent_size;  // in bytes
extern const int              _rom_persistent_handles_size;    // in bytes
#if ENABLE_HEAP_NEARS_IN_HEAP
extern const int              _rom_rom_duplicated_handles_size;    // in bytes
#endif
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES
extern const int              _rom_task_mirrors_size;          // in bytes
#endif 
extern const int              _rom_method_variable_parts_size; // in bytes
extern const int              _rom_number_of_java_classes;
       
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
extern const int              _rom_text_block0[];
extern const int              _rom_text_block1[];
extern const int              _rom_text_block2[];
extern const int              _rom_text_block3[];
extern const int              _rom_text_block4[];
extern const int              _rom_text_block5[];
extern const int              _rom_text_block6[];
extern const int              _rom_text_block7[];
extern const int              _rom_text_block8[];
extern const int              _rom_text_block9[];
#else
extern const int              _rom_text_block[];
#endif
extern       int              _rom_data_block[];
extern const int              _rom_data_block_src[];
extern const int              _rom_heap_block[];
extern const int              _rom_persistent_handles[];
#if ENABLE_HEAP_NEARS_IN_HEAP
extern const int              _rom_rom_duplicated_handles[];
#endif
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES
extern const int              _rom_task_mirrors[];
extern const int              _rom_task_mirrors_bitmap[];          
#endif 
extern const unsigned short   _rom_strings_src[];
extern const int              _rom_strings_count;
extern const int              _rom_method_variable_parts_src[];
extern const int*             _rom_constant_pool;
extern       int              _gc_stackmap_size;
extern       int              _rom_method_variable_parts[];

extern const int*             _rom_symbols_start;
extern const int*             _rom_fieldtype_symbols_start;
extern const int*             _rom_fieldtype_symbols_end; // inclusive
extern const int*             _rom_signature_symbols_start;
extern const int*             _rom_signature_symbols_end; // inclusive
extern const int*             _rom_symbols_end; // inclusive
extern const int*             _rom_methods_start;
extern const int*             _rom_methods_end; // inclusive

extern const char             _rom_restricted_packages[];

#if ENABLE_APPENDED_CALLINFO
extern const unsigned int     _rom_compiled_methods[];
extern const unsigned int     _rom_compiled_methods_count;
#endif // ENABLE_APPENDED_CALLINFO

#if  !defined(PRODUCT) || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
extern const int               _rom_system_symbols_src[];
extern const OriginalClassInfo _rom_original_class_info[];
extern const int               _rom_original_class_info_count;
extern const char*             _rom_alternate_constant_pool_src[];
extern const int               _rom_alternate_constant_pool_count;
extern const int*              _rom_text_klass_table[];
extern const int               _rom_text_klass_table_size;
#endif

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
extern const int               _rom_profile_bitmap_row_size;
extern const int               _rom_profiles_count;
extern const char*             _rom_profiles_names[];
extern const unsigned char     _rom_hidden_classes_bitmaps[];
extern const char**            _rom_profiles_restricted_packages[];
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT
/*
 * Flags that describes the options used by this the ROM image.
 */
#define ROM_SYSTEM_IMAGE (1 << 31)
#define ROM_HAS_COMPACT_METHOD_TABLE     (1 << 30)
#define ROM_IMAGE_LINKED (1 << 29)
#define ROM_IS_RELAUNCHABLE (1 << 28)
#define ROM_FLAG_MASK (ROM_SYSTEM_IMAGE | ROM_HAS_COMPACT_METHOD_TABLE | ROM_IS_RELAUNCHABLE)
#define ROM_LINKED_MASK (ROM_FLAG_MASK | ROM_IMAGE_LINKED)
#define ROM_BINARY_MAGIC 0xCAFEBEBE

#if defined (__SYMBIAN32__) && defined(__GNUC__)
/* IMPL_NOTE: should this be defined(ARM)? */
#ifndef MSW_FIRST_FOR_DOUBLE
#define MSW_FIRST_FOR_DOUBLE 1
#endif
#endif

#ifndef HARDWARE_LITTLE_ENDIAN
#define HARDWARE_LITTLE_ENDIAN 1
#endif

#ifndef MSW_FIRST_FOR_DOUBLE
#define MSW_FIRST_FOR_DOUBLE (!HARDWARE_LITTLE_ENDIAN)
#endif

#ifndef MSW_FIRST_FOR_LONG
#define MSW_FIRST_FOR_LONG (!HARDWARE_LITTLE_ENDIAN)
#endif

#ifndef TARGET_MSW_FIRST_FOR_DOUBLE
#define TARGET_MSW_FIRST_FOR_DOUBLE MSW_FIRST_FOR_DOUBLE
#endif

#ifndef TARGET_MSW_FIRST_FOR_LONG
#define TARGET_MSW_FIRST_FOR_LONG MSW_FIRST_FOR_LONG
#endif

#if MSW_FIRST_FOR_DOUBLE
#define ROM_DOUBLE(msw, lsw) msw, lsw
#else
#define ROM_DOUBLE(msw, lsw) lsw, msw
#endif

#if MSW_FIRST_FOR_LONG
#define ROM_LONG(msw, lsw) msw, lsw
#else
#define ROM_LONG(msw, lsw) lsw, msw
#endif

/* ROM_BL = big- or little-endian */

#if HARDWARE_LITTLE_ENDIAN
#define ROM_BL(be_word, le_word, comment)  (le_word)
#else
#define ROM_BL(be_word, le_word, comment)  (be_word)
#endif

#if ENABLE_JAZELLE && ENABLE_NATIVE_ORDER_REWRITING
// This overridding is done in BuildFlags.hpp, but since we don't
// include BuildFlags.hpp when building the ROMImage, we need to
// override it here manually.
#undef  ENABLE_NATIVE_ORDER_REWRITING
#define ENABLE_NATIVE_ORDER_REWRITING 0
#endif

#if HARDWARE_LITTLE_ENDIAN
#if ENABLE_NATIVE_ORDER_REWRITING
#define ROM_BLX(be_word, le_word_java, le_word_native, comment) (le_word_native)
#else
#define ROM_BLX(be_word, le_word_java, le_word_native, comment) (le_word_java)
#endif
#else 
#define ROM_BLX(be_word, le_word_java, le_word_native, comment) (be_word)
#endif
/*
 * The following #define's are used to product the name of a global
 * variable. It's used to make sure we the ROMImage.o is compiled
 * with the same set of flags as the VM.
 */

#if HARDWARE_LITTLE_ENDIAN
#define _ROM_LINKCHECK_HLE _rom_linkcheck_hle_true
#else
#define _ROM_LINKCHECK_HLE _rom_linkcheck_hle_false
#endif

#if MSW_FIRST_FOR_LONG
#define _ROM_LINKCHECK_MFFL _rom_linkcheck_mffl_true
#else
#define _ROM_LINKCHECK_MFFL _rom_linkcheck_mffl_false
#endif

#if MSW_FIRST_FOR_DOUBLE
#define _ROM_LINKCHECK_MFFD _rom_linkcheck_mffd_true
#else
#define _ROM_LINKCHECK_MFFD _rom_linkcheck_mffd_false
#endif

extern const int _ROM_LINKCHECK_HLE;
extern const int _ROM_LINKCHECK_MFFL;
extern const int _ROM_LINKCHECK_MFFD;
#if ENABLE_ROM_JAVA_DEBUGGER
extern const int _rom_is_debuggable;
#endif

#ifndef PRODUCT
#define __FRAME_OFFSETS_DO(template) \
  template(JavaFrame__stack_bottom_pointer_offset) \
  template(JavaFrame__bcp_store_offset) \
  template(JavaFrame__locals_pointer_offset) \
  template(JavaFrame__cpool_offset) \
  template(JavaFrame__method_offset) \
  template(JavaFrame__caller_fp_offset) \
  template(JavaFrame__return_address_offset) \
  template(JavaFrame__stored_int_value1_offset) \
  template(JavaFrame__stored_int_value2_offset) \
  template(JavaFrame__frame_desc_size) \
  template(JavaFrame__end_of_locals_offset) \
  template(JavaFrame__empty_stack_offset) \
  template(EntryFrame__pending_activation_offset) \
  template(EntryFrame__pending_exception_offset) \
  template(EntryFrame__stored_obj_value_offset) \
  template(EntryFrame__stored_int_value2_offset) \
  template(EntryFrame__stored_int_value1_offset) \
  template(EntryFrame__stored_last_sp_offset) \
  template(EntryFrame__stored_last_fp_offset) \
  template(EntryFrame__real_return_address_offset) \
  template(EntryFrame__fake_return_address_offset) \
  template(EntryFrame__frame_desc_size) \
  template(EntryFrame__empty_stack_offset) \
  template(StackValue__stack_tag_offset) \

#if ENABLE_EMBEDDED_CALLINFO
#define __CALLINFO_OFFSET_DO(template) \
  template(JavaFrame__callinfo_offset_from_return_address)
#else
#define __CALLINFO_OFFSET_DO(template)
#endif // ENABLE_EMBEDDED_CALLINFO

#define FRAME_OFFSETS_DO(template) \
  __CALLINFO_OFFSET_DO(template) \
  __FRAME_OFFSETS_DO(template)

#define DECLARE_FRAME_OFFSETS_CHECKER(x) \
  extern const int _rom_check_ ## x;

extern const int _rom_check_JavaFrame__arg_offset_from_sp_0;

FRAME_OFFSETS_DO(DECLARE_FRAME_OFFSETS_CHECKER)

extern const int _rom_compilation_enabled;
extern const int _rom_generator_soft_float_enabled;
extern const int _rom_generator_target_msw_first_for_double;
#endif

#ifdef __cplusplus
}
#endif

#endif /* _ROM_IMAGE_HPP_ */
