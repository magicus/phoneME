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

class ROMInliner {
public:
  // max_len defines maximal length of inlined method, usually should be 
  // something  < 10
  void initialize(int max_len JVM_TRAPS);
  
  // tries to inline all calls in method to methods in _methods
  // list, returns number of inlined calls
  int try_to_inline_in_method(Method* method JVM_TRAPS);
  
  void try_inline(Method* caller, Method* callee, int bci_in_caller);

private:
  // auxiliary methods
  void add(Method* method);
  bool contains(Method* method);
  int hashcode_for_method(Method *method);
  int hashcode_for_symbol(Symbol *symbol);
  static bool may_inline_invokestatic(Method* caller, Method* callee);
  static bool is_inner_class_of(InstanceClass *inner, InstanceClass* outer);

  // those are called when inlining different things
  bool try_inline_empty_method(Method* caller, Method* callee, 
                               int bci_in_caller);
  bool try_inline_empty_constructor(Method* caller, Method* callee, 
                                    int bci_in_caller);
  bool try_inline_arg_return(Method* caller, Method* callee, 
                             int bci_in_caller);
  bool try_inline_static_getter(Method* caller, Method* callee, 
                         int bci_in_caller);
  bool try_inline_getter(Method* caller, Method* callee, 
                         int bci_in_caller);
  bool try_inline_setter(Method* caller, Method* callee, 
                         int bci_in_caller);
  bool try_inline_forwarder(Method* caller, Method* callee, 
                            int bci_in_caller);  
  int  find_or_add_resolved_field_ref(ConstantPool *cp, int tag_value,
                                      int offset, int class_id,
                                      BasicType type, int is_static);



  // pop as much as needed
  bool pop_as_needed(Method* caller, Method* callee, 
                     int bci_in_caller);
#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR
  bool zero_bci_is_branch_target(Method* caller);
#endif

  // return number of stack words consumed by method invocation
  int  num_args(Method* calee);

  ObjArray _methods;

  int _rewritten;
};

#endif
