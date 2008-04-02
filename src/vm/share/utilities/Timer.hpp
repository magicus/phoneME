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

#if !defined(PRODUCT) && ENABLE_PERFORMANCE_COUNTERS

/**
 * Timer for simple VM internal measurements.
 */
class JvmTimer: public StackObj {
 private:
  jlong   _counter;
  bool    _active;
 public:
  JvmTimer() { _counter = 0; _active = false; }
  // JvmTimer start, stop, accumulate
  void start();
  void stop();
  void add(const JvmTimer& t);

  // Seconds elapsed since timer start
  double seconds() const;
  jlong counter() const;

  // whole milliseconds since timer start
  jint milliseconds() const;
  // millisecond fractions since timer start. To print,
  // use printf("%d.%s",  milliseconds(), millisecond_fractions
  const char * millisecond_fractions() const;

  jint units_per_second(int units) const;

  static double counter2seconds(jlong count);
};

#endif
