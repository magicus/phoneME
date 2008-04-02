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

class ROMizerHashEntry: public MixedOop {
 public:
  HANDLE_DEFINITION(ROMizerHashEntry, MixedOop);

  DEFINE_ACCESSOR_OBJ(ROMizerHashEntry, Oop,                 referent)
  DEFINE_ACCESSOR_OBJ(ROMizerHashEntry, ROMizerHashEntry,    next)
                                      
  DEFINE_ACCESSOR_NUM(ROMizerHashEntry, int,                 seen)
  DEFINE_ACCESSOR_NUM(ROMizerHashEntry, int,                 type)
  DEFINE_ACCESSOR_NUM(ROMizerHashEntry, int,                 pass)
  DEFINE_ACCESSOR_NUM(ROMizerHashEntry, int,                 offset)
  DEFINE_ACCESSOR_NUM(ROMizerHashEntry, int,                 skip_words)
  DEFINE_ACCESSOR_NUM(ROMizerHashEntry, int,                 num_entries)
#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  DEFINE_ACCESSOR_NUM(ROMizerHashEntry, int,                 loc_offset)
#endif
#if ENABLE_HEAP_NEARS_IN_HEAP && USE_SOURCE_IMAGE_GENERATOR
  DEFINE_ACCESSOR_NUM(ROMizerHashEntry, int,                 heap_offset)
#endif
 public:
  static ReturnOop allocate(JVM_SINGLE_ARG_TRAPS);
};
