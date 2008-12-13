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
# include "incls/_Timer.cpp.incl"

#if !defined(PRODUCT) && ENABLE_PERFORMANCE_COUNTERS

void JvmTimer::add(const JvmTimer& t) {
  _counter += t._counter;
}

void JvmTimer::start() {
  if (!_active) {
    _active = true;
    _counter = Os::elapsed_counter() - _counter; // value of _counter is initially zero
  }
}

void JvmTimer::stop() {
  if (_active) {
    _counter = Os::elapsed_counter() - _counter;
    _active = false;
  }
}

double JvmTimer::seconds() const {
  return counter2seconds(_counter);
}

jlong JvmTimer::counter() const { 
  return _counter;
}

jint JvmTimer::milliseconds() const {
  jlong frequency = Os::elapsed_frequency();
  if (frequency == 1000) {
    // a standard milli-second resolution
    return (jint)_counter;
  } else {
    return jvm_d2i(jvm_dmul(seconds(), 1000.0));
  }
}

const char * JvmTimer::millisecond_fractions() const {
  jlong frequency = Os::elapsed_frequency();
  if (frequency == 1000) {
    // a standard milli-second resolution
    return "0";
  } else {
    jlong nanos = jvm_f2l(jvm_d2f(jvm_dmul(seconds(), 1000000.0)));
    int frac = (int)(nanos % 1000);
    static char buf[4];
    if (frac <= 0) {
      return "0";
    } else {
      if (frac < 10) {
        jvm_sprintf(buf, "00%d", frac);
      } else if (frac < 100) {
        jvm_sprintf(buf, "0%d",  frac);
      } else {
        jvm_sprintf(buf, "%d",   frac);
      }
      return buf;
    }
  }
}

jint JvmTimer::units_per_second(int units) const {
  jlong frequency = Os::elapsed_frequency();
  if (frequency == 1000) {
    // a standard milli-second resolution
    return units * 1000 / (jint)_counter;
  } else {
    return jvm_f2i(jvm_fdiv(jvm_i2f(units), jvm_d2f(seconds())));
  }
}

double JvmTimer::counter2seconds(jlong count) {
  jfloat frequency = jvm_l2f(Os::elapsed_frequency());
  return jvm_f2d(jvm_fdiv(jvm_l2f(count), frequency));
}

#endif // PRODUCT
