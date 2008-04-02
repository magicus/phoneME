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


package com.sun.cldchi.tools.memoryprofiler.data;

/**
 * This class is container for all statististics for a Java Class.
 * It provides the following information: number of objects, total size of all object of the class,
 * percentage of live objects, percentage of old generation objects
 *
 * @see com.sun.cldchi.tools.memoryprofiler.data.MPDataProvider
 *
 */
public class ClassStatistics {
  public final String _class_name;
  private int _count;
  private int _total_size; 
  private static int _total_heap_size;
  private int _dead_size;
  private int _old_gen_size;
 
  public ClassStatistics(String class_name) {
    _class_name = class_name;
    _count = _total_size = _dead_size = _old_gen_size = 0;
  }

  public int getHeapPercentage() {
    if (_total_heap_size == 0) return 0;
    return (10000*_total_size) / _total_heap_size;
  }

  public int getAverageSize() {
    if (_count== 0) return 0;
    return _total_size / _count;
  }

  public int getLivePercentage() {
    if (_total_size == 0) return 0;
    return (10000*(_total_size - _dead_size)) / _total_size;
  }

  public int getOldGenPercentage() {
    if (_total_size == 0) return 0;
    return (10000*_old_gen_size) / _total_size;
  }

  public void add(JavaObject obj, int old_gen_end) {
    _total_heap_size += obj.size;
    _count++;
    _total_size += obj.size;
    if (!obj.alive()) {
      _dead_size += obj.size;
    }
    if (obj.address < old_gen_end) {
      _old_gen_size += obj.size;
    }
  }
 
  public static void reset() {
    _total_heap_size= 0;
  }

  public int getCount() {return _count;}
  public int getTotalSize() {return _total_size;}
}
