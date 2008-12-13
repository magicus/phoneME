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
# include "incls/_MixedOop.cpp.incl"

HANDLE_CHECK(MixedOop, is_mixed_oop())

#define MIXED_OOP_TYPE_STRING(x, y) \
    case MixedOopDesc::Type_ ## x: return STR(x);

#if !defined(PRODUCT) || USE_DEBUG_PRINTING

const char *MixedOop::type_string() {
  switch (type()) {
#if USE_DEBUG_PRINTING
    MIXED_OOP_TYPES_DO(MIXED_OOP_TYPE_STRING)
#endif
  default:
    SHOULD_NOT_REACH_HERE();
    return NULL;
  }

}

#endif

#ifndef PRODUCT

void MixedOop::iterate(OopVisitor* visitor) {
#if USE_OOP_VISITOR
  Oop::iterate(visitor);
  { 
    NamedField id("size", true);
    visitor->do_ushort(&id, size_offset(), true);
  }
  { 
    NamedField id("type", true);
    visitor->do_ubyte(&id, type_offset(), true);
  }
  { 
    NamedField id("pointer count", true);
    visitor->do_ubyte(&id, pointer_count_offset(), true);
  }
  switch (type()) {
  case MixedOopDesc::Type_JarFileParser:
    ((JarFileParser*)this)->iterate(visitor);
    break;
  case MixedOopDesc::Type_Task:
    ((Task*)this)->iterate(visitor);
    break;
  case MixedOopDesc::Type_Thread:
    ((Thread*)this)->iterate(visitor);
    break;
#if 0 && ENABLE_COMPILER 
  case MixedOopDesc::Type_CompilationQueueElement:
    ((CompilationQueueElement*)this)->iterate(visitor);
    break;    
#if 0 && USE_COMPILER_LITERALS_MAP
  case MixedOopDesc::Type_LiteralPoolElement:
    ((LiteralPoolElement*)this)->iterate(visitor);
    break;
#endif  // USE_COMPILER_LITERALS_MAP
#endif  // ENABLE_COMPILER
  // IMPL_NOTE: fill in the other cases!
  }
#endif
}

#endif /* PRODUCT || USE_PRODUCT_BINARY_IMAGE_GENERATOR */

