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
# include "incls/_AccessFlags.cpp.incl"

#if USE_DEBUG_PRINTING

AccessFlags::FlagInfo AccessFlags::flag_info[] = {
 {JVM_ACC_PUBLIC,                 AccessFlags::CMF, "public"},
 {JVM_ACC_PRIVATE,                AccessFlags::MF,  "private"},
 {JVM_ACC_PROTECTED,              AccessFlags::MF,  "protected"},
 {JVM_ACC_STATIC,                 AccessFlags::MF,  "static"},
 {JVM_ACC_FINAL,                  AccessFlags::CMF, "final"},
 {JVM_ACC_SYNCHRONIZED,           AccessFlags::M,   "synchronized"},
 {JVM_ACC_SUPER,                  AccessFlags::C,   "ACC_SUPER"},
 {JVM_ACC_VOLATILE,               AccessFlags::F,   "volatile"},
 {JVM_ACC_TRANSIENT,              AccessFlags::F,   "transient"},
 {JVM_ACC_NATIVE,                 AccessFlags::M,   "native"},
 {JVM_ACC_INTERFACE,              AccessFlags::C,   "interface"},
 {JVM_ACC_ABSTRACT,               AccessFlags::CM,  "abstract"},
 {JVM_ACC_STRICT,                 AccessFlags::M,   "strict"},
 {JVM_ACC_SYNTHETIC,              AccessFlags::CMF, "synthetic"}, // unused!!
 {JVM_ACC_HAS_MONITOR_BYTECODES,  AccessFlags::M,   "HAS_MONITOR_BYTECODES"},
 {JVM_ACC_HAS_INVOKE_BYTECODES,   AccessFlags::M,   "HAS_INVOKE_BYTECODES"},
 {JVM_ACC_HAS_COMPRESSED_HEADER,  AccessFlags::M,   "HAS_COMPRESSED_HEADER"},
 {JVM_ACC_HAS_NO_STACKMAPS,       AccessFlags::M,   "HAS_NO_STACKMAPS"},
 {JVM_ACC_HAS_NO_EXCEPTION_TABLE, AccessFlags::M,   "HAS_NO_EXCEPTION_TABLE"},
 {JVM_ACC_DOUBLE_SIZE,            AccessFlags::M,   "JVM_ACC_DOUBLE_SIZE"},
 {JVM_ACC_HAS_VANILLA_CONSTRUCTOR,AccessFlags::C,   "HAS_VANILLA_CONSTRUCTOR"},
 {JVM_ACC_ARRAY_CLASS,            AccessFlags::C,   "ARRAY_CLASS"},
 {JVM_ACC_FAKE_CLASS,             AccessFlags::C,   "FAKE_CLASS"},
 {JVM_ACC_HAS_FINALIZER,          AccessFlags::C,   "HAS_FINALIZER"},
 {JVM_ACC_PRELOADED,              AccessFlags::C,   "PRELOADED"},
 {JVM_ACC_CONVERTED,              AccessFlags::C,   "CONVERTED"},
 {JVM_ACC_NON_OPTIMIZABLE,        AccessFlags::C,   "NON_OPTIMIZABLE"},
 {JVM_ACC_VERIFIED,               AccessFlags::C,   "VERIFIED"},
 {JVM_ACC_HIDDEN,                 AccessFlags::C,   "HIDDEN"}
};

void AccessFlags::print_to_buffer(char *buff, int type) const {
  const char *prefix = "";
  buff[0] = 0;
  for (int i=0; i<sizeof(flag_info)/sizeof(FlagInfo); i++) {
    if ((_flags & flag_info[i].flag) != 0 && (type & flag_info[i].type) != 0) {
      jvm_strcat(buff, prefix);
      jvm_strcat(buff, flag_info[i].name);
      prefix = " ";
    }
  }
}

void AccessFlags::p() const {
  char buff[1024];
  print_to_buffer(buff, CMF);
  tty->print_cr(buff);
}

void AccessFlags::print_definitions() {
  tty->print_cr("    Class access flags:");
  print_definitions(AccessFlags::CLASS_FLAGS);

  tty->print_cr("    Method access flags:");
  print_definitions(AccessFlags::METHOD_FLAGS);

  tty->print_cr("    Field access flags:");
  print_definitions(AccessFlags::FIELD_FLAGS);
}

void AccessFlags::print_definitions(int type) {
  int total = 0;
  for (int n=0; n<32; n++) {
    juint mask = (juint)(1 << n);
    bool found = false;

    tty->print("    1 << %2d ", n);
    for (int i=0; i<sizeof(flag_info)/sizeof(FlagInfo); i++) {
      if ((type & flag_info[i].type) != 0 && (mask & flag_info[i].flag) != 0) {
        // Two flags can't have the same value for the same type
        GUARANTEE(!found, "duplicated flag value");
        tty->print_cr("%s", flag_info[i].name);
        found = true;
        total ++;
      }
    }
    if (!found) {
      tty->print_cr("%s", "-- unused --");
    }
  }
  tty->print_cr("       Total number of used flags = %d", total);
  tty->cr();
}


#endif
