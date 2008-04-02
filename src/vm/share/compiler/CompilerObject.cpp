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
#include "incls/_CompilerObject.cpp.incl"

#if ENABLE_COMPILER

#if USE_COMPILER_OBJECT_HEADER
const char* const CompilerObject::_names[] = {
  #define DEFINE_COMPILER_OBJECT_NAME(name) #name,
  COMPILER_OBJECTS_DO( DEFINE_COMPILER_OBJECT_NAME )
  #undef DEFINE_COMPILER_OBJECT_NAME
};

const CompilerObject* CompilerObject::find( const void* ref ) {
  const CompilerObject* p = start();
  if( p == NULL || ref < ((void*)p) || ref >= ((void*)end()) ) {
    return NULL;
  }
  for(;;) {
    const CompilerObject* next = p->next();
    if( ref >= (const void*) next ) {
      break;
    }
    p = next;
  }
  return p;
}
#endif

#endif
