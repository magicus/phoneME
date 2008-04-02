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
#include "incls/_MethodInvocationClosure.cpp.incl"

#if ENABLE_ROM_GENERATOR
class BytecodeAnalyzeClosure : public BytecodeClosure {
  Bytecodes::Code           _code;
  Thread*                   THREAD;
  MethodInvocationClosure*  _owner;
  

public:
  BytecodeAnalyzeClosure(MethodInvocationClosure* owner) :
    _owner(owner)
  {   
    THREAD = Thread::current();   
  }

  ~BytecodeAnalyzeClosure() {
  }

  virtual void bytecode_prolog(JVM_SINGLE_ARG_TRAPS) {
    JVM_IGNORE_TRAPS;
    _code = method()->bytecode_at(bci());
  }
  
  virtual void invoke_static(int index JVM_TRAPS) {
    if (_code == Bytecodes::_invokestatic ||
        _code == Bytecodes::_fast_invokestatic ||
        _code == Bytecodes::_fast_init_invokestatic) {
      check_static(index JVM_CHECK);
    }
    BytecodeClosure::invoke_static(index JVM_CHECK);
  }
  
  virtual void invoke_special(int index JVM_TRAPS) {
    if (_code == Bytecodes::_invokespecial) {
      check_virtual(index JVM_CHECK);
    }
    BytecodeClosure::invoke_special(index JVM_CHECK);
  }

  virtual void invoke_virtual(int index JVM_TRAPS) {
    if (_code == Bytecodes::_invokevirtual) {
      check_virtual(index JVM_CHECK);      
    }
    BytecodeClosure::invoke_virtual(index JVM_CHECK);
  } 
 
  virtual void invoke_interface(int index, int num_of_args JVM_TRAPS) {
    UsingFastOops level1;
    ConstantPool::Fast cp = method()->constants();
    Method::Fast m;
    if (cp().tag_at(index).is_resolved_interface_method()) {
       int class_id, itable_index;
       cp().resolved_interface_method_at(index, itable_index, class_id);
       InstanceClass::Raw klass = Universe::class_from_id(class_id);
       m = klass().interface_method_at(itable_index);
      _owner->add_interface_method(&m JVM_CHECK);
    } else {
      // here also should go those uncommon cases, but it shouldn't happen in 
      // ROMized code unless we postpone errors until runtime.
      GUARANTEE(PostponeErrorsUntilRuntime, "Cannot happen");
    }

    BytecodeClosure::invoke_interface(index, num_of_args JVM_CHECK);
  }

  virtual void fast_invoke_special(int index JVM_TRAPS) {
    if (_code == Bytecodes::_fast_invokespecial) {
      check_virtual(index JVM_CHECK);
    }
    BytecodeClosure::fast_invoke_special(index JVM_CHECK);
  }

  virtual void fast_invoke_virtual(int index JVM_TRAPS) {
    if (_code == Bytecodes::_fast_invokevirtual) {
      check_virtual(index JVM_CHECK);      
    }
    BytecodeClosure::fast_invoke_virtual(index JVM_CHECK);
  }
  
  virtual void fast_invoke_virtual_final(int index JVM_TRAPS) {
    check_uncommon_or_static_method(index JVM_CHECK);      

    BytecodeClosure::fast_invoke_virtual_final(index JVM_CHECK);
  }

  //interpreteur put fast_invoke_virtual_final not only for 
  //static methods but also for final methods of Object class!
  void check_uncommon_or_static_method(int index JVM_TRAPS) {
    ConstantPool cp = method()->constants();

    if (!cp.tag_at(index).is_resolved_final_uncommon_interface_method()) {
      check_static(index JVM_CHECK);
    }
  }
  
  void check_virtual(int index JVM_TRAPS) {
    UsingFastOops level1;
    ConstantPool::Fast cp = method()->constants();
    InstanceClass::Fast klass;
    ClassInfo::Fast info;
    Method::Fast m;

    if (cp().tag_at(index).is_resolved_virtual_method()) {
      int class_id, vtable_index;
      cp().resolved_virtual_method_at(index, vtable_index, class_id);
      klass = Universe::class_from_id(class_id);
      info = klass().class_info();
      m = info().vtable_method_at(vtable_index);

      _owner->add_method(&m JVM_NO_CHECK_AT_BOTTOM);
    } else {
      // This could be an element we failed to resolve 
      // when ROMizing an application.
      if (!PostponeErrorsUntilRuntime) {
        SHOULD_NOT_REACH_HERE();
      } else {
        //The following GUARANTEE could trigger if the class is a bogus
        // TCK class and we want to postpone the error until runtime so
        // we have commented it out.
        // GUARANTEE(cp.tag_at(index).is_method(), "Sanity");
        // The class must be marked as unverified or non-optimizable, 
        // since it contains an unresolved entry at this point.
#ifdef AZZERT
        klass = method()->holder();
        GUARANTEE(!klass().is_verified() || !klass().is_optimizable(), "Sanity");
#endif
      }
    }
  }

  void check_static(int index JVM_TRAPS) {
    ConstantPool::Raw cp = method()->constants();      
      if (cp().tag_at(index).is_resolved_static_method()) {
        Method m = cp().resolved_static_method_at(index);       
        _owner->add_method(&m JVM_NO_CHECK_AT_BOTTOM);
      } else {
        // This could be an element we failed to resolve 
        // when ROMizing an application.
        if (!PostponeErrorsUntilRuntime) {
          SHOULD_NOT_REACH_HERE();
        } else {
          GUARANTEE(cp().tag_at(index).is_method(), "Sanity");
          // The class must be marked as unverified or non-optimizable, 
          // since it contains an unresolved entry at this point.
#ifdef AZZERT
          InstanceClass klass = method()->holder();
          GUARANTEE(!klass.is_verified() || !klass.is_optimizable(), 
                    "Sanity");
#endif
        }
      }
  }
};

void  MethodInvocationClosure::initialize(JVM_SINGLE_ARG_TRAPS) {
  int method_count = 0;
  UsingFastOops level1;
  InstanceClass::Fast klass;
  ObjArray::Fast methods;
  for (SystemClassStream st(true); st.has_next();) {
    klass = st.next();
    methods = klass().methods();
    method_count += methods().length();
  }

  _methods = Universe::new_obj_array(method_count * 3 JVM_CHECK);  
}

void MethodInvocationClosure::add_method(Method* method JVM_TRAPS) {
  if (contains(method)) return;

  juint len = (juint)_methods.length();
  juint start = (juint)(hashcode_for_method(method)) % len;

  for (juint i=start; ;) {
    if (_methods.obj_at(i) == NULL) {
      _methods.obj_at_put(i, method);
      add_closure(method JVM_CHECK);
      return;
    }
    i ++;

    if (i >= len) {
      i = 0;
    }
    if (i == start) {
      // _old_methods's length is 3 times the number of methods, so we will
      // always have space.
      SHOULD_NOT_REACH_HERE();
    }
  }
}

void MethodInvocationClosure::add_closure(Method* method JVM_TRAPS) {
  BytecodeAnalyzeClosure ba(this);
  
  // add all methods invoked from this class
  ba.initialize(method);
  method->iterate(0, method->code_size(), &ba JVM_CHECK);

  // add this method in parent and children if virtual
  Oop null_oop;
  int vindex = method->vtable_index();
 
  if (vindex > -1) {
    InstanceClass ci = method->holder();
    add_supers(&ci, vindex JVM_CHECK);
    add_subs(&ci, vindex JVM_CHECK);
  }

  // If this method belongs to an interface, consider all implementation methods
  // reachable.
  InstanceClass::Raw klass = method->holder();
  if (klass().is_interface()) {
    add_interface_method(method JVM_NO_CHECK_AT_BOTTOM);
  }
}

bool MethodInvocationClosure::contains(Method* method) {
  juint len = (juint)_methods.length();
  juint start = (juint)(hashcode_for_method(method)) % len;

  for (juint i=start; ;) {
    Method::Raw m = _methods.obj_at(i);

    if (m.not_null() && m.equals(method)) return true;

     i ++;

     if (i >= len) {
       i = 0;
     }

     if (i == start) {
       return false;
     }     
  }

  return false;
}

int MethodInvocationClosure::hashcode_for_method(Method *method) {
  Symbol::Raw name = method->name();
  Symbol::Raw sig = method->signature();

  return (hashcode_for_symbol(&name) ^ hashcode_for_symbol(&sig));
}

int MethodInvocationClosure::hashcode_for_symbol(Symbol *symbol) {
  return SymbolTable::hash(symbol);
}

void MethodInvocationClosure::add_supers(InstanceClass* ci, int vindex JVM_TRAPS) 
{
  UsingFastOops level1;
  InstanceClass::Fast super = ci;
  Method::Fast m;
  while (super.not_null()) {
    ClassInfo::Raw info = super().class_info();

    if (info().vtable_length() <= vindex) {
      break;
    }
    
    Method m = info().vtable_method_at(vindex);
    add_method(&m JVM_CHECK);

    super = super().super();
  }
}

// find all classes that are subclasses of given and mark method 
// with same index in vtable as callable
void MethodInvocationClosure::add_subs(InstanceClass* ci, int vindex JVM_TRAPS) 
{
  UsingFastOops level1;
  InstanceClass::Fast klass;
  Method::Fast m;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    if (klass().is_subclass_of(ci)) {
      ClassInfo::Raw info = klass().class_info();

      GUARANTEE(vindex < info().vtable_length(), "sanity");
      m = info().vtable_method_at(vindex);
      add_method(&m JVM_CHECK);
    }
  }
}

// find all classes implementing 
void MethodInvocationClosure::add_interface_method(Method* method JVM_TRAPS) 
{
  add_method(method JVM_CHECK);
  UsingFastOops level1;
  InstanceClass::Fast klass;
  Method::Fast m;
  for (SystemClassStream st; st.has_next();) {
    klass = st.next();
    Symbol::Raw method_name = method->name();
    Symbol::Raw method_sig = method->signature();
    int dummy_id, dummy_index;

    // IMPL_NOTE: the following check is too liberal -- we should
    // check if klass implements the interface first
    m = klass().lookup_method_in_all_interfaces(&method_name,
                                                     &method_sig,
                                                     dummy_id, dummy_index);
    if (m.not_null()) {      
      add_method(&m JVM_CHECK);
    }    
  }
}
#endif // ENABLE_ROM_GENERATOR
