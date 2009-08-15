/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
class BytecodeAnalyzeClosure: public BytecodeClosure {
  AZZERT_ONLY( Bytecodes::Code _code; )
  MethodInvocationClosure*  _owner;  

public:
  BytecodeAnalyzeClosure(MethodInvocationClosure* owner) :
    _owner(owner) {}
 ~BytecodeAnalyzeClosure(void) {}

  virtual void bytecode_prolog(JVM_SINGLE_ARG_TRAPS);
  
  virtual void invoke_static(int index JVM_TRAPS);  
  virtual void invoke_special(int index JVM_TRAPS);
  virtual void invoke_virtual(int index JVM_TRAPS); 
  virtual void invoke_interface(int index, int num_of_args JVM_TRAPS);
  virtual void fast_invoke_special(int index JVM_TRAPS);
  virtual void fast_invoke_virtual(int index JVM_TRAPS);
  virtual void fast_invoke_virtual_final(int index JVM_TRAPS);

  //interpreteur put fast_invoke_virtual_final not only for 
  //static methods but also for final methods of Object class!
  void check_uncommon_or_static_method(const int index) const {
    ConstantPool::Raw cp = method()->constants();
    if (!cp().tag_at(index).is_resolved_final_uncommon_interface_method()) {
      check_static(index);
    }
  }
  
  void check_virtual(const int index) const;

  void check_static(const int index) const {
    ConstantPool::Raw cp = method()->constants();      
      if (cp().tag_at(index).is_resolved_static_method()) {
        Method::Raw m = cp().resolved_static_method_at(index);       
        InstanceClass::Raw holder = m().holder();
        _owner->add_method(&m, &holder);
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
          InstanceClass::Raw klass = method()->holder();
          GUARANTEE(!klass().is_verified() || !klass().is_optimizable(), 
                    "Sanity");
#endif
        }
      }
  }
};

void BytecodeAnalyzeClosure::check_virtual(const int index) const {
  ConstantPool::Raw cp = method()->constants();
  if (cp().tag_at(index).is_resolved_virtual_method()) {
    int class_id, vtable_index;
    cp().resolved_virtual_method_at(index, vtable_index, class_id);
    InstanceClass::Raw klass = Universe::class_from_id(class_id);
    ClassInfo::Raw info = klass().class_info();
    Method::Raw m = info().vtable_method_at(vtable_index);
    _owner->add_method(&m, &klass);
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
      InstanceClass::Raw klass = method()->holder();
      GUARANTEE(!klass().is_verified() || !klass().is_optimizable(), "Sanity");
#endif
    }
  }
}

void BytecodeAnalyzeClosure::bytecode_prolog(JVM_SINGLE_ARG_TRAPS) {
  JVM_IGNORE_TRAPS;
  AZZERT_ONLY( _code = method()->bytecode_at(bci()); )
}

void BytecodeAnalyzeClosure::invoke_static(int index JVM_TRAPS) {
  GUARANTEE(_code == Bytecodes::_invokestatic ||
            _code == Bytecodes::_fast_invokestatic ||
            _code == Bytecodes::_fast_init_invokestatic, "Sanity");
  check_static(index);
  BytecodeClosure::invoke_static(index JVM_CHECK);
}

void BytecodeAnalyzeClosure::invoke_special(int index JVM_TRAPS) {
  GUARANTEE(_code == Bytecodes::_invokespecial, "Sanity");
  check_virtual(index);
  BytecodeClosure::invoke_special(index JVM_CHECK);
}

void BytecodeAnalyzeClosure::invoke_virtual(int index JVM_TRAPS) {
  GUARANTEE(_code == Bytecodes::_invokevirtual, "Sanity");
  check_virtual(index);      
  BytecodeClosure::invoke_virtual(index JVM_CHECK);
} 

void
BytecodeAnalyzeClosure::invoke_interface(int index, int num_of_args JVM_TRAPS) {
  ConstantPool::Raw cp = method()->constants();
  if (cp().tag_at(index).is_resolved_interface_method()) {
     int class_id, itable_index;
     cp().resolved_interface_method_at(index, itable_index, class_id);
     InstanceClass::Raw klass = Universe::class_from_id(class_id);
     Method::Raw m = klass().interface_method_at(itable_index);
    _owner->add_interface_method(&m);
  } else {
    // here also should go those uncommon cases, but it shouldn't happen in 
    // ROMized code unless we postpone errors until runtime.
    GUARANTEE(PostponeErrorsUntilRuntime, "Cannot happen");
  }

  BytecodeClosure::invoke_interface(index, num_of_args JVM_CHECK);
}

void BytecodeAnalyzeClosure::fast_invoke_special(int index JVM_TRAPS) {
  GUARANTEE(_code == Bytecodes::_fast_invokespecial, "Sanity");
  check_virtual(index);
  BytecodeClosure::fast_invoke_special(index JVM_CHECK);
}

void BytecodeAnalyzeClosure::fast_invoke_virtual(int index JVM_TRAPS) {
  GUARANTEE(_code == Bytecodes::_fast_invokevirtual, "Sanity");
  check_virtual(index);      
  BytecodeClosure::fast_invoke_virtual(index JVM_CHECK);
}

void BytecodeAnalyzeClosure::fast_invoke_virtual_final(int index JVM_TRAPS) {
  check_uncommon_or_static_method(index);
  BytecodeClosure::fast_invoke_virtual_final(index JVM_CHECK);
}

void  MethodInvocationClosure::initialize(JVM_SINGLE_ARG_TRAPS) {
  int method_count = 0;
  for (SystemClassStream st(true); st.has_next();) {
    InstanceClass::Raw klass = st.next();
    ObjArray::Raw methods = klass().methods();
    method_count += methods().length();
  }

  _methods = Universe::new_obj_array(method_count * 3 JVM_NO_CHECK_AT_BOTTOM);
}

inline int MethodInvocationClosure::hashcode_for_symbol(Symbol *symbol) {
  return SymbolTable::hash(symbol);
}

int MethodInvocationClosure::hashcode_for_method(Method *method) {
  Symbol::Raw name = method->name();
  Symbol::Raw sig = method->signature();

  return (hashcode_for_symbol(&name) ^ hashcode_for_symbol(&sig));
}

void MethodInvocationClosure::add_method(Method* method,
                                         InstanceClass* receiver_klass) {
  {
    const juint len = juint(_methods.length());
    const juint start = juint(hashcode_for_method(method)) % len;

    for (juint i=start; ;) {
      Method::Raw m = _methods.obj_at(i);
      if (m.equals(method)) {
        return;
      }
      if (m.is_null()) {
        // add this method
        _methods.obj_at_put(i, method);
        break;
      }
      if (++i >= len) {
        i = 0;
      }
      GUARANTEE(i != start, "Sanity" );
      // _old_methods's length is 3 times the number of methods,
      // so we will always have space.
    }
  }

  // add all methods invoked from this method
  {
    BytecodeAnalyzeClosure ba(this);    
    ba.initialize(method);

    SETUP_ERROR_CHECKER_ARG;
    method->iterate(0, method->code_size(), &ba JVM_CHECK);
  }

  // find all subclasses of the method class and
  // add methods with the same index in vtable
  {
    const int vindex = method->vtable_index(); 
    if (vindex > -1) {
      for (SystemClassStream st; st.has_next();) {
        InstanceClass::Raw klass = st.next();
        if (klass().is_strict_subclass_of(receiver_klass)) {
          ClassInfo::Raw info = klass().class_info();
          GUARANTEE(vindex < info().vtable_length(), "sanity");

          Method::Raw m = info().vtable_method_at(vindex);
          add_method(&m, &klass);
        }
      }
    }
  }

  // If this method belongs to an interface,
  // consider all implementation methods reachable.
  if (receiver_klass->is_interface()) {
    add_interface_method(method);
  }
}

// find all classes implementing 
void MethodInvocationClosure::add_interface_method(Method* method) {
  Symbol::Raw method_name = method->name();
  Symbol::Raw method_sig = method->signature();

  for (SystemClassStream st; st.has_next();) {
    InstanceClass::Raw klass = st.next();
    int dummy_id, dummy_index;

    // IMPL_NOTE: the following check is too liberal -- we should
    // check if klass implements the interface first
    Method::Raw m =
      klass().lookup_method_in_all_interfaces(&method_name,&method_sig,
                                              dummy_id, dummy_index);
    if (m.not_null()) {
      // Cannot use klass here because of too liberal check above
      InstanceClass::Raw holder = m().holder();
      add_method(&m, &holder);
    }    
  }
}
#endif // ENABLE_ROM_GENERATOR
