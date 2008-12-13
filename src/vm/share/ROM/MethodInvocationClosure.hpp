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

#if ENABLE_ROM_GENERATOR

class MethodInvocationClosure {
public:
  void initialize(JVM_SINGLE_ARG_TRAPS);
  
  void add_method(Method* method JVM_TRAPS);
  void add_interface_method(Method* method JVM_TRAPS);
  bool contains(Method* method);

private:
  void add_closure(Method *method JVM_TRAPS);
  void add_supers(InstanceClass* ci, int vindex JVM_TRAPS);
  void add_subs(InstanceClass* ci, int vindex JVM_TRAPS);

  int hashcode_for_method(Method *method);
  int hashcode_for_symbol(Symbol *symbol);
  
  ObjArray _methods;
};

#endif
