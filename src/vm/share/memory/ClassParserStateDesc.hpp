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

class ClassParserStateDesc: public MixedOopDesc { 
 private:
  static size_t allocation_size() { 
    return align_allocation_size(sizeof(ClassParserStateDesc));
  }
  static int pointer_count() {
    return 6;
  }

 private:
  // Initializes the object after allocation
  void initialize(OopDesc* name) {
    _class_name = (SymbolDesc*)name;
    _stage = 0;
  }

  // pointers
  ClassParserStateDesc *_next;
  SymbolDesc *_class_name;
  TypeArrayDesc * _buffer;
  ConstantPoolDesc* _cp;
  TypeArrayDesc * _interface_indices;
  InstanceClassDesc *_result;

  /* All oops must go before here.  If you change the number of oops, be
   * sure to change pointer_count()
   */

  // non-pointers
  int _stage;
  int _access_flags;
  int _super_class_index;
  int _buffer_pos;
  int _major_version;
  int _minor_version;

  friend class ClassParserState;
  friend class Universe;
  friend class OopDesc;
};
