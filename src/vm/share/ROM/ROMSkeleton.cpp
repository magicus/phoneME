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

/*
 * This file is is originally src/vm/share/ROM/ROMSkeleton.cpp and
 * is copied to be ROMImage.cpp for non-ROMIZING builds.
 *
 * If this file is named ROMImage.cpp, do not edit -- it will be
 * overwritten by the build process.
 */

#include "jvmconfig.h"

#ifdef ROMIZING
error "wrong ROMImage.cpp file used.";
#endif

#include "ROMImage.hpp"

extern "C" {
  const int              _rom_flags                         =  0;
  const int              _rom_symbol_table[]                = {0};
  const int              _rom_symbol_table_num_buckets      =  0;
  
  const int              _rom_string_table[]                = {0};
  const int              _rom_string_table_num_buckets      =  0;
  
  const int              _rom_has_linked_image              =  0;
  const int              _rom_is_relaunchable               =  0;
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  const int              _rom_text_block0_size              =  0; 
  const int              _rom_text_block1_size              =  0; 
  const int              _rom_text_block2_size              =  0; 
  const int              _rom_text_block3_size              =  0; 
  const int              _rom_text_block4_size              =  0; 
  const int              _rom_text_block5_size              =  0; 
  const int              _rom_text_block6_size              =  0; 
  const int              _rom_text_block7_size              =  0; 
  const int              _rom_text_block8_size              =  0;
  const int              _rom_text_block9_size              =  0; 
#else
  const int              _rom_text_block_size               =  0;
#endif
  const int              _rom_data_block_size               =  0;
  const int              _rom_data_block_scanned_size       =  0;
  const int              _rom_heap_block_size               =  0;
  const int              _rom_heap_block_permanent_size     =  0;
#if ENABLE_HEAP_NEARS_IN_HEAP
  const int              _rom_rom_duplicated_handles_size   =  0;
#endif
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES
  const int              _rom_task_mirrors_size             =  0;
#endif   
  const int              _rom_persistent_handles_size       =  0;
  const int              _rom_method_variable_parts_size    =  0;
  const int              _rom_number_of_java_classes        =  0;
  
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  const int              _rom_text_block0[]                 = {0};
  const int              _rom_text_block1[]                 = {0};
  const int              _rom_text_block2[]                 = {0};
  const int              _rom_text_block3[]                 = {0};
  const int              _rom_text_block4[]                 = {0};
  const int              _rom_text_block5[]                 = {0};
  const int              _rom_text_block6[]                 = {0};
  const int              _rom_text_block7[]                 = {0};
  const int              _rom_text_block8[]                 = {0};
  const int              _rom_text_block9[]                 = {0};
#else
  const int              _rom_text_block[]                  = {0};
#endif
        int              _rom_data_block[]                  = {0};
  const int              _rom_data_block_src[]              = {0};
  const int              _rom_heap_block[]                  = {0};
  const int              _rom_persistent_handles[]          = {0};
#if ENABLE_HEAP_NEARS_IN_HEAP
  const int              _rom_rom_duplicated_handles[]      = {0};
#endif
#if ENABLE_PREINITED_TASK_MIRRORS && ENABLE_ISOLATES
  const int              _rom_task_mirrors[]                = {0};
  const int              _rom_task_mirrors_bitmap[]         = {0};          
#endif 
  const unsigned short   _rom_strings_src[]                 = {0};
  const int              _rom_strings_count                 =  0;
  const int              _rom_method_variable_parts_src[]   = {0};
  const int*             _rom_constant_pool                 =  0;
  int                    _gc_stackmap_size                  =  10;
        int              _rom_method_variable_parts[]       = {0};
  
  // These values are set such that a range check of (start <= p <= end) will
  // always fail. See OopDesc::is_rom_symbol() for more info.
  const int*             _rom_methods_start               = (const int*)(0x10);
  const int*             _rom_methods_end                 = (const int*)(0x00);
  const int*             _rom_symbols_start               = (const int*)(0x10);
  const int*             _rom_symbols_end                 = (const int*)(0x00);
  const int*             _rom_fieldtype_symbols_start     = (const int*)(0x10);
  const int*             _rom_fieldtype_symbols_end       = (const int*)(0x00);
  const int*             _rom_signature_symbols_start     = (const int*)(0x10);
  const int*             _rom_signature_symbols_end       = (const int*)(0x00);
  const int*             _rom_string_chars_addr           = (const int*)(0x00);
  const char             _rom_restricted_packages[]       = {(char)0x0};

#if ENABLE_APPENDED_CALLINFO
  const unsigned int     _rom_compiled_methods[]     = {0};
  const unsigned int     _rom_compiled_methods_count = 0;
#endif // ENABLE_APPENDED_CALLINFO

#if !defined(PRODUCT) || ENABLE_JVMPI_PROFILE || ENABLE_TTY_TRACE
  const int              _rom_system_symbols_src[]        = {0};
  const int               _rom_original_class_info_count  = 0;
  const OriginalClassInfo _rom_original_class_info[] = {
    {(const char*)0x0,
     0, (const OriginalMethodInfo *)0x0, 0,
     (const OriginalFieldInfo *)0x0, },
  };
  const char*             _rom_alternate_constant_pool_src[] = {0};
  const int               _rom_alternate_constant_pool_count = 0;
  const int*              _rom_text_klass_table[]            = {0};
  const int               _rom_text_klass_table_size         = 0;
#endif

#if ENABLE_MULTIPLE_PROFILES_SUPPORT
  const int               _rom_profile_bitmap_row_size       = 0;
  const int               _rom_profiles_count                = 0;
  const char*             _rom_profiles_names[]              = {0};
  const unsigned char     _rom_hidden_classes_bitmaps[]      = {0};
  const unsigned char     _rom_profiles_hidden_packages[]    = {0};
  const char**            _rom_profiles_restricted_packages[]= {0};
#endif // ENABLE_MULTIPLE_PROFILES_SUPPORT


  const int _ROM_LINKCHECK_HLE  = 0;
  const int _ROM_LINKCHECK_MFFL = 0;
  const int _ROM_LINKCHECK_MFFD = 0;

#if ENABLE_ROM_JAVA_DEBUGGER
  const int _rom_is_debuggable = 0;
#endif

  void romgen_check_oopmaps() {}

#ifndef PRODUCT
#define DEFINE_FRAME_OFFSETS_CHECKER(x) \
  const int _rom_check_ ## x = 0;

  const int _rom_compilation_enabled = 0;
  const int _rom_check_JavaFrame__arg_offset_from_sp_0 = 0;
  FRAME_OFFSETS_DO(DEFINE_FRAME_OFFSETS_CHECKER)
  const int _rom_generator_soft_float_enabled = 0;
  const int _rom_generator_target_msw_first_for_double = 0;
#endif
}
