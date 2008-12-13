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

import java.util.Collection;
import java.util.ArrayList;
import java.util.HashMap;

/**
 * This class is container for Java Object information. It contains all object data.
 *
 * @see com.sun.cldchi.tools.memoryprofiler.data.MPDataProvider
 *
 */
public class JavaObject {
  /**
   * address of the java object
   */  
  public final int address;

  /**
   * unique id of class of the java object
   */  
  final int class_id;

  /**
   * size of the java object
   */  
  public final int size;

  public final int object_type;
  private int root_distance = -1;

  /**
   * these fields are used only for stacks
   */  
  final int _stack_id;
  final HashMap _stack_offsets;

  /**
   * addresses of the objects which are referenced by this object
   */    
  final int[] _references_addresses;
  private Collection _references = new ArrayList();
  private Collection _referees = new ArrayList();

  /**
   * return the array of objects which are referenced by this object
   *
   * @return array of JavaObject
   */    
  public Object[] get_references() {
    return _references.toArray();
  }

  public void add_reference(JavaObject reference) {
    _references.add(reference);
  }

  /**
   * return the array of objects which are reference this object
   *
   * @return array of JavaObject
   */    
  public Object[] get_referees() {
    return _referees.toArray();
  }

  public void add_referee(JavaObject referee) {
      _referees.add(referee);
  }

  JavaObject(int p_address, int p_class_id, int p_size, int[] references, int p_obj_type) {
    this(p_address, p_class_id, p_size, references, p_obj_type, null, -1);
  }

  JavaObject(int p_address, int p_class_id, int p_size, 
                            int[] references, int p_obj_tp, HashMap offsets, int stack_id) {
    address = p_address;
    class_id = p_class_id;
    size = p_size;
    object_type = p_obj_tp;
    _references_addresses = references;
    _stack_offsets = offsets;
    _stack_id = stack_id;
  }
  public String toString() {
    return /*Integer.toString(root_distance) + */"0x" + Integer.toHexString(address);
  }

  /**
   * set the minimal size of a chain of linked objects started from 
   * a root object and finished in this object
   */      
  void setRootDistance(int dist) {
    root_distance = dist;
  }

  /**
   * return minimal size of a chain of linked objects started from 
   * a root object and finished in this object
   */      
  public int getRootDistance() {
    return root_distance;
  }  

  /**
   * return true, if there is a chain of linked objects started from 
   * a root object and finished in this object
   *
   * @return true if it is a live object
   */      
  public boolean alive() {return root_distance != -1;}
}
