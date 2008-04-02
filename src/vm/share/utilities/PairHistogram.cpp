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
#include "incls/_PairHistogram.cpp.incl"

#if USE_DEBUG_PRINTING

void PairHistogram::reset_counters() {
  for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
    for (int j = 0; j < Bytecodes::number_of_java_codes; j++) {
      counter_at_put(i, j, 0);
    }
  }
}

jlong PairHistogram::total_count() {
  jlong value = 0;
  for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
    for (int j = 0; j < Bytecodes::number_of_java_codes; j++) {
      value += counter_at(i, j);
    }
  }
  return value;
}

struct SortedPairHistogramEntry {
  Bytecodes::Code first;
  Bytecodes::Code second;
  jlong count;
};

static int __cdecl pair_compare_entries(const void* e1, const void* e2) {
  jlong delta = ((SortedPairHistogramEntry*)e2)->count - ((SortedPairHistogramEntry*)e1)->count;
  return delta < 0 ? -1 : (delta == 0 ? 0 : 1);
}

#ifdef LINUX
#define FLL "%14lld"
#else
#define FLL "%14I64d"
#endif

void PairHistogram::print(float cutoff) {
  jlong total        = total_count();
  jlong absolute_sum = 0;

  SortedPairHistogramEntry* sorted_histogram = 
    NEW_GLOBAL_HEAP_ARRAY(SortedPairHistogramEntry, Bytecodes::number_of_java_codes * Bytecodes::number_of_java_codes, "PairHistogram");
  for (int i = 0; i < Bytecodes::number_of_java_codes; i++) {
    for (int j = 0; j < Bytecodes::number_of_java_codes; j++) {
      sorted_histogram[i * Bytecodes::number_of_java_codes + j].count = counter_at(i, j);
      sorted_histogram[i * Bytecodes::number_of_java_codes + j].first = (Bytecodes::Code) i;
      sorted_histogram[i * Bytecodes::number_of_java_codes + j].second = (Bytecodes::Code) j;
    }
  }
  jvm_qsort(sorted_histogram, Bytecodes::number_of_java_codes * Bytecodes::number_of_java_codes, sizeof(SortedPairHistogramEntry), pair_compare_entries);

  tty->cr();
  tty->print_cr("Histogram of executed bytecode pairs:");
  tty->cr();
  tty->print_cr("      absolute  relative    (code,code)    (name,name)");
  tty->print_cr("----------------------------------------------------------------------");
  if (total > 0) {
    for (int i = 0; i < Bytecodes::number_of_java_codes * Bytecodes::number_of_java_codes; i++) {
      Bytecodes::Code first  = sorted_histogram[i].first;
      Bytecodes::Code second = sorted_histogram[i].second;
      jlong count            = sorted_histogram[i].count;
      if (Bytecodes::is_defined(first) && Bytecodes::is_defined(second)) {
        jlong absolute = count;
	float relative = jvm_fdiv(jvm_fmul(jvm_l2f(absolute), 100.0F),
				  jvm_l2f(total));
        if (!jvm_fcmpg(cutoff, relative)) {
          tty->print_cr(FLL " %7.2f%%    (0x%02x,0x%02x)    (%s,%s)", 
			absolute, 
			jvm_f2d(relative), 
			first, 
			second, 
                        Bytecodes::name(Bytecodes::cast(first)), 
			Bytecodes::name(Bytecodes::cast(second)));
          absolute_sum += absolute;
        }
      }
    }
  }
  tty->print_cr("----------------------------------------------------------------------");
  if (total > 0) {
    // relative_sum = (absolute_sum * 100.0) / total
    float relative_sum = jvm_fdiv(jvm_fmul(jvm_l2f(absolute_sum), 100.0F),
				  jvm_l2f(total));
    tty->print_cr(FLL " %7.2f%%    cutoff = %.2f%%", 
		  absolute_sum, 
		  jvm_f2d(relative_sum), 
		  jvm_f2d(cutoff));
    tty->print_cr(FLL " %7.2f%%    total", total, 100.0, 0.0);
  }
  tty->cr();

  FREE_GLOBAL_HEAP_ARRAY(sorted_histogram, "PairHistogram");
}

#endif
