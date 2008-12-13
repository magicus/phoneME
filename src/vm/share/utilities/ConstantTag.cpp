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

// CopyrightVersion 1.2

# include "incls/_precompiled.incl"
# include "incls/_ConstantTag.cpp.incl"

#ifndef PRODUCT

void ConstantTag::print_on(Stream* st) const {
#if USE_DEBUG_PRINTING 
  switch (_tag) {
    case JVM_CONSTANT_Invalid :
      st->print("Invalid");
      break;
    case JVM_CONSTANT_Class :
      st->print("Class");
      break;
    case JVM_CONSTANT_Fieldref :
      st->print("Field");
      break;
    case JVM_CONSTANT_Methodref :
      st->print("Method");
      break;
    case JVM_CONSTANT_InterfaceMethodref :
      st->print("InterfaceMethod");
      break;
    case JVM_CONSTANT_String :
      st->print("String");
      break;
    case JVM_CONSTANT_Integer :
      st->print("Integer");
      break;
    case JVM_CONSTANT_Float :
      st->print("Float");
      break;
    case JVM_CONSTANT_Long :
      st->print("Long");
      break;
    case JVM_CONSTANT_Double :
      st->print("Double");
      break;
    case JVM_CONSTANT_NameAndType :
      st->print("NameAndType");
      break;
    case JVM_CONSTANT_Utf8 :
      st->print("Utf8");
      break;
    case JVM_CONSTANT_UnresolvedClass :
      st->print("Unresolved class");
      break;
    case JVM_CONSTANT_ClassIndex :
      st->print("Unresolved class index");
      break;
    case JVM_CONSTANT_UnresolvedString :
      st->print("Unresolved string");
      break;
    case JVM_CONSTANT_StringIndex :
      st->print("Unresolved string index");
      break;
    default:
      SHOULD_NOT_REACH_HERE();
      break;
  }         
#endif
}

#if USE_DEBUG_PRINTING 
void ConstantTag::print_definitions() {
  for (jubyte i=0; i<CONSTANT_POOL_TAG_LIMIT; i++) {
    tty->print("%-49s = %2d, 0x", name_for(i, true), i);
    if (i < 0x10) {
      tty->print("0");
    }
    tty->print_cr("%x", i);
    if (i == JVM_CONSTANT_NameAndType) {
      tty->cr();
    }
  }
}

#define JVM_TAG_CASE(x) \
  case JVM_CONSTANT_ ## x: \
    result = (use_prefix) ? ("JVM_CONSTANT_" # x) : (#x); \
    break

char *ConstantTag::name_for(jubyte tag, bool use_prefix) {
  const char *result = "???";
  switch (tag) {
    // Defined by JLS (JvmConst.hpp)
    JVM_TAG_CASE(Utf8);
    JVM_TAG_CASE(Unicode);
    JVM_TAG_CASE(Integer);
    JVM_TAG_CASE(Float);
    JVM_TAG_CASE(Long);      
    JVM_TAG_CASE(Double);
    JVM_TAG_CASE(Class);
    JVM_TAG_CASE(String);
    JVM_TAG_CASE(Fieldref);
    JVM_TAG_CASE(Methodref);
    JVM_TAG_CASE(InterfaceMethodref);
    JVM_TAG_CASE(NameAndType);

    // Defined by the VM (ConstantTag.hpp)
    JVM_TAG_CASE(Invalid);

    JVM_TAG_CASE(UnresolvedClass);
    JVM_TAG_CASE(ClassIndex);
    JVM_TAG_CASE(UnresolvedString);
    JVM_TAG_CASE(StringIndex);

    JVM_TAG_CASE(ResolvedBooleanFieldref);
    JVM_TAG_CASE(ResolvedCharFieldref);
    JVM_TAG_CASE(ResolvedFloatFieldref);
    JVM_TAG_CASE(ResolvedDoubleFieldref);
    JVM_TAG_CASE(ResolvedByteFieldref);
    JVM_TAG_CASE(ResolvedShortFieldref);
    JVM_TAG_CASE(ResolvedIntFieldref);
    JVM_TAG_CASE(ResolvedLongFieldref);
    JVM_TAG_CASE(ResolvedObjectFieldref);
    JVM_TAG_CASE(ResolvedArrayFieldref);

    JVM_TAG_CASE(ResolvedStaticBooleanFieldref);
    JVM_TAG_CASE(ResolvedStaticCharFieldref);
    JVM_TAG_CASE(ResolvedStaticFloatFieldref);
    JVM_TAG_CASE(ResolvedStaticDoubleFieldref);
    JVM_TAG_CASE(ResolvedStaticByteFieldref);
    JVM_TAG_CASE(ResolvedStaticShortFieldref);
    JVM_TAG_CASE(ResolvedStaticIntFieldref);
    JVM_TAG_CASE(ResolvedStaticLongFieldref);
    JVM_TAG_CASE(ResolvedStaticObjectFieldref);
    JVM_TAG_CASE(ResolvedStaticArrayFieldref);

    JVM_TAG_CASE(ResolvedStaticMethod);

    JVM_TAG_CASE(ResolvedBooleanVirtualMethod);
    JVM_TAG_CASE(ResolvedCharVirtualMethod);
    JVM_TAG_CASE(ResolvedFloatVirtualMethod);
    JVM_TAG_CASE(ResolvedDoubleVirtualMethod);
    JVM_TAG_CASE(ResolvedByteVirtualMethod);
    JVM_TAG_CASE(ResolvedShortVirtualMethod);
    JVM_TAG_CASE(ResolvedIntVirtualMethod);
    JVM_TAG_CASE(ResolvedLongVirtualMethod);
    JVM_TAG_CASE(ResolvedObjectVirtualMethod);
    JVM_TAG_CASE(ResolvedArrayVirtualMethod);
    JVM_TAG_CASE(ResolvedVoidVirtualMethod);

    JVM_TAG_CASE(ResolvedBooleanInterfaceMethod);
    JVM_TAG_CASE(ResolvedCharInterfaceMethod);
    JVM_TAG_CASE(ResolvedFloatInterfaceMethod);
    JVM_TAG_CASE(ResolvedDoubleInterfaceMethod);
    JVM_TAG_CASE(ResolvedByteInterfaceMethod);
    JVM_TAG_CASE(ResolvedShortInterfaceMethod);
    JVM_TAG_CASE(ResolvedIntInterfaceMethod);
    JVM_TAG_CASE(ResolvedLongInterfaceMethod);
    JVM_TAG_CASE(ResolvedObjectInterfaceMethod);
    JVM_TAG_CASE(ResolvedArrayInterfaceMethod);
    JVM_TAG_CASE(ResolvedVoidInterfaceMethod);

    JVM_TAG_CASE(ResolvedUncommonInterfaceMethod);
    JVM_TAG_CASE(ResolvedFinalUncommonInterfaceMethod);

  default:
    SHOULD_NOT_REACH_HERE();
  }
  return (char*)result;
}

#undef JVM_TAG_CASE
#endif

#endif // PRODUCT
