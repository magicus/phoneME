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

class ROMizerHashEntryDesc: public MixedOopDesc { 
 private:
  static size_t allocation_size() { 
    return align_allocation_size(sizeof(ROMizerHashEntryDesc));
  }
  static int pointer_count() {
    return 2;
  }

 private:
  // pointers
  OopDesc *_referent;
  ROMizerHashEntryDesc *_next;

  /* All oops must go before here.  If you change the number of oops, 
   * be sure to change pointer_count()
   */
  // non-pointers
  int _seen;
  int _type;
  int _pass;
  int _offset;
  int _skip_words;
  int _num_entries;

#if ENABLE_SEGMENTED_ROM_TEXT_BLOCK
  // _loc_offset is used to evaluate hash code for TEXT object
  // for retrieving its "klass" field from text_klass_table,
  // see TextKlassLookupTable (SourceROMWriter.cpp).
  // _offset is still a "global" TEXT offset.
  // _offset is not suitable for hash code evaluation for separated
  // TEXT, because there is no global TEXT offset when ROM is linked.
  int _loc_offset;
#endif
#if ENABLE_HEAP_NEARS_IN_HEAP && USE_SOURCE_IMAGE_GENERATOR
  //this field is only needed for keeping offset inside HEAP block for 
  // objects which need to be cloned into the both TEXT and HEAP blocks.
  // this is made as a performance optimization, to avoid using references
  // to near objects inside ROM, from objects in HEAP, what lead to complicated
  // near ptr encoding
  int _heap_offset;
#endif
  friend class ROMizerHashEntry;
  friend class ROMWriter;
  friend class BinaryROMWriter;
  friend class OffsetFinder;
  friend class BlockTypeFinder;
};
