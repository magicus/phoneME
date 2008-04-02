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
#include "incls/_BytecodeHistogram.cpp.incl"

#if USE_DEBUG_PRINTING

void BytecodeHistogram::reset_counters() {
  for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
    counter_at_put(i, 0);
  }
}

jlong BytecodeHistogram::total_count() {
  jlong value = 0;
  for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
    value += counter_at(i);
  }
  return value;
}

struct SortedHistogramEntry {
  Bytecodes::Code code;
  jlong count;
};

static int __cdecl compare_entries(const void* e1, const void* e2) {
  jlong delta = ((SortedHistogramEntry*)e2)->count - ((SortedHistogramEntry*)e1)->count;
  return delta < 0 ? -1 : (delta == 0 ? 0 : 1);
}

#ifdef LINUX
#define FLL "%14lld"
#else
#define FLL "%14I64d"
#endif

void BytecodeHistogram::print(float cutoff) {
  jlong total        = total_count();
  jlong absolute_sum = 0;

  SortedHistogramEntry* sorted_histogram = 
    NEW_GLOBAL_HEAP_ARRAY(SortedHistogramEntry, Bytecodes::number_of_java_codes, "BytecodeHistogram");
  for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
    sorted_histogram[i].count = counter_at(i);
    sorted_histogram[i].code = (Bytecodes::Code) i;
  }
  jvm_qsort(sorted_histogram, Bytecodes::number_of_java_codes, sizeof(SortedHistogramEntry), compare_entries);

  tty->cr();
  tty->print_cr("Histogram of executed bytecodes:");
  tty->cr();
  tty->print_cr("      absolute  relative    code    name");
  tty->print_cr("----------------------------------------------------------------------");
  if (total > 0) {
    for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
      Bytecodes::Code code = sorted_histogram[i].code;
      jlong count          = sorted_histogram[i].count;
      if (Bytecodes::is_defined(code)) {
        jlong absolute = count;
        float relative = jvm_fdiv(jvm_fmul(jvm_l2f(absolute), 100.0F),
				  jvm_l2f(total));
        if (!jvm_fcmpg(cutoff, relative)) {
          tty->print_cr(FLL "  %7.2f%%    0x%02x    %s", 
			absolute, 
			jvm_f2d(relative), 
			code, 
			Bytecodes::name(Bytecodes::cast(code)));
          absolute_sum += absolute;
        }
      }
    }
  }
  tty->print_cr("----------------------------------------------------------------------");
  if (total > 0) {  
    float relative_sum = jvm_fdiv(jvm_fmul(jvm_l2f(absolute_sum), 100.0F),
				  jvm_l2f(total));
    tty->print_cr(FLL " %7.2f%%    cutoff = %.2f%%", 
		  absolute_sum, 
		  jvm_f2d(relative_sum), 
		  jvm_f2d(cutoff));
    tty->print_cr(FLL " %7.2f%%    total", total, 100.0, 0.0);
  }
  tty->cr();

  FREE_GLOBAL_HEAP_ARRAY(sorted_histogram, "BytecodeHistogram");
}

#endif
