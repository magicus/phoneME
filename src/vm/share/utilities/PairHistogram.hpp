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

#if !defined(PRODUCT) || USE_DEBUG_PRINTING

extern "C" {
  extern jlong interpreter_pair_counters[];
}

/**
 * Gather frequencies of byte code pair counts during interpretation
 * and print a histogram of those.
 *
 * The interpreter fills the array above with base data.
 */
class PairHistogram : public AllStatic {
 public:
  static void  reset_counters();
  static jlong total_count();
  static void  print(float cutoff = 0);

 private:
  static jlong counter_at(int first, int second)      { return interpreter_pair_counters[first * Bytecodes::number_of_java_codes + second];  }
  static void  counter_at_put(int first, int second, jlong value) { interpreter_pair_counters[first * Bytecodes::number_of_java_codes + second] = value; }
};

#endif
