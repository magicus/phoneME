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
#include "incls/_ROMInliner.cpp.incl"

#if ENABLE_ROM_GENERATOR

#define VERBOSE_INLINING 0

class BytecodeInlinerClosure : public BytecodeClosure {
  Bytecodes::Code           _code;
  ROMInliner*               _owner;
  
public:
  BytecodeInlinerClosure(ROMInliner* owner) :
    _owner(owner)
  {    
  }

  ~BytecodeInlinerClosure() {
  }
  
  virtual void bytecode_prolog(JVM_SINGLE_ARG_TRAPS) {
    JVM_IGNORE_TRAPS;
    _code = method()->bytecode_at(bci());
  }
  
  virtual void invoke_static(int index JVM_TRAPS) {
    if (_code == Bytecodes::_invokestatic ||
        _code == Bytecodes::_fast_invokestatic ||
        _code == Bytecodes::_fast_init_invokestatic) {
      check_static(index);
    }
    BytecodeClosure::invoke_static(index JVM_CHECK);
  }
  
  virtual void invoke_special(int index JVM_TRAPS) {
    if (_code == Bytecodes::_invokespecial) {
      check_virtual_or_static(index);
    }
    BytecodeClosure::invoke_special(index JVM_CHECK);
  }

  virtual void invoke_virtual(int index JVM_TRAPS) {
    if (_code == Bytecodes::_invokevirtual) {
      check_virtual_or_static(index);      
    }
    BytecodeClosure::invoke_virtual(index JVM_CHECK);
  } 
 
  virtual void fast_invoke_special(int index JVM_TRAPS) {
    if (_code == Bytecodes::_fast_invokespecial) {
      check_virtual_or_static(index);
    }
    BytecodeClosure::fast_invoke_special(index JVM_CHECK);
  }

  virtual void fast_invoke_virtual(int index JVM_TRAPS) {
    if (_code == Bytecodes::_fast_invokevirtual) {
      check_virtual_or_static(index);      
    }
    BytecodeClosure::fast_invoke_virtual(index JVM_CHECK);
  }
  
  virtual void fast_invoke_virtual_final(int index JVM_TRAPS) {
    check_uncommon_or_static_method(index);

    BytecodeClosure::fast_invoke_virtual_final(index JVM_CHECK);
  }

  //invoke_special could invoke <init> methods which are resolved as ResolvedStaticMethod
  void check_virtual_or_static(int index) {
    AllocationDisabler no_allocation_should_happen;
    ConstantPool::Raw cp = method()->constants();
    if (!cp().tag_at(index).is_resolved_static_method()) {
      check_virtual(index);
    } else {
      check_static(index);
    }
  }
  //interpreteur put fast_invoke_virtual_final not only for 
  //static methods but also for final methods of Object class!
  void check_uncommon_or_static_method(int index) {
    AllocationDisabler no_allocation_should_happen;
    ConstantPool::Raw cp = method()->constants();

    if (!cp().tag_at(index).is_resolved_final_uncommon_interface_method()) {
      check_static(index);
    }
  }
  
  void check_virtual(int index) {
    AllocationDisabler no_allocation_should_happen;
    UsingFastOops level1;
    ConstantPool::Fast cp = method()->constants();
    InstanceClass::Fast klass;
    ClassInfo::Fast info;

    if (cp().tag_at(index).is_resolved_virtual_method()) {
      int class_id, vtable_index;
      cp().resolved_virtual_method_at(index, vtable_index, class_id);
      klass = Universe::class_from_id(class_id);
      info = klass().class_info();
      Method::Raw callee = info().vtable_method_at(vtable_index);
      _owner->try_inline(method(), &callee, bci());
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
        klass = method()->holder();
        GUARANTEE(!klass().is_verified() || !klass().is_optimizable(), "Sanity");
#endif
      }
    }
  }

  void check_static(int index) {
    AllocationDisabler no_allocation_should_happen;
    ConstantPool::Raw cp = method()->constants();      
    if (cp().tag_at(index).is_resolved_static_method()) {
      Method::Raw callee = cp().resolved_static_method_at(index);
      _owner->try_inline(method(), &callee, bci());
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
        GUARANTEE(!klass().is_verified() || !klass().is_optimizable(), "Sanity");
#endif
      }
    }
  }
};

void ROMInliner::initialize(int max_len JVM_TRAPS) {

  // create array able to hold all methods
  int method_count = 0;
  for (SystemClassStream st; st.has_next();) {
    InstanceClass::Raw klass = st.next();
    ObjArray::Raw methods = klass().methods();
    method_count += methods().length();
  }
  _methods = Universe::new_obj_array(method_count * 3 JVM_CHECK);

  // iterate over all methods and put potentially inlineable methods in hash
  UsingFastOops level1;
  Method::Fast method;
  ObjArray::Fast methods;
  for (SystemClassStream stream; stream.has_next();) {
    InstanceClass::Raw klass = stream.next();
    
    methods = klass().methods();
    
    for (int i=0; i<methods().length(); i++) {
      method = methods().obj_at(i);
      
      if (method.is_null()) {
        // A nulled-out method
        continue;
      }
      
      // callees like these can't really be inlined
      if (method().is_abstract()         ||
                method().is_native()           ||
                method().code_size() >= max_len) {
              continue;
      }

      add(&method);
    }
  }
}

void ROMInliner::add(Method* method) {
  AllocationDisabler raw_pointers_used_in_this_function;

  OopDesc *p = method->obj();
  juint len = (juint)_methods.length();
  juint start = (juint)(hashcode_for_method(method)) % len;

  for (juint i=start; ;) {
    OopDesc *o = _methods.obj_at(i);
    if (o == p) { // Already added
      return;
    } else if (o == NULL) {
      _methods.obj_at_put(i, method);
      return;
    }
    i ++;

    if (i >= len) {
      i = 0;
    }
    if (i == start) {
      // _methods's length is 3 times the number of methods, so we will
      // always have space.
      SHOULD_NOT_REACH_HERE();
    }
  }
}

bool ROMInliner::contains(Method* method) {
  AllocationDisabler raw_pointers_used_in_this_function;

  OopDesc *p = method->obj();
  juint len = (juint)_methods.length();
  juint start = (juint)(hashcode_for_method(method)) % len;

  for (juint i=start; ;) {
    OopDesc *o = _methods.obj_at(i);
    if (o == p) {
      return true;
    } else if (o == NULL) {
      return false;
    }

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

int ROMInliner::hashcode_for_method(Method *method) {
  Symbol::Raw name = method->name();
  Symbol::Raw sig = method->signature();

  int code = (hashcode_for_symbol(&name) ^ hashcode_for_symbol(&sig));
  code *= method->code_size();
  code ^= method->access_flags().as_int();

  return code;
}

int ROMInliner::hashcode_for_symbol(Symbol *symbol) {
  return SymbolTable::hash(symbol);
}

int ROMInliner::try_to_inline_in_method(Method* method JVM_TRAPS) {
  BytecodeInlinerClosure bic(this);
  
  _rewritten = 0;

  // check every invocation and inline as needed
  bic.initialize(method);
  method->iterate(0, method->code_size(), &bic JVM_CHECK_0);

  return _rewritten;
}

// Well, we assume that all classes are built with javac. If you include
// a hand-writter class with $ inside its name, it will be unpredictable.
//
// IMPL_NOTE: write a better checker to make sure inner class has a field like:
// private final synthetic Field this$0:<outer> ... but really, there's
// no guarantee.

bool ROMInliner::is_inner_class_of(InstanceClass *inner, InstanceClass* outer)
{
  Symbol::Raw inner_name = inner->name();
  Symbol::Raw outer_name = outer->name();
  int len = outer_name().length();
  int i;

  if (inner_name().length() <= len) {
    return false;
  }
  for (i=0; i<len; i++) {
    if (inner_name().byte_at(i) != outer_name().byte_at(i)) {
      return false;
    }
  }
  if (i < inner_name().length() && inner_name().byte_at(i) == '$') {
    return true;
  } else {
    return false;
  }
}

bool ROMInliner::may_inline_invokestatic(Method* caller, Method* callee) {
  // Assuming caller is a method of class A and callee is a static method
  // of class B. In some cases invokestatic is safe to inline:
  //
  // [1] caller is a non-static method, and and A is an inner class of B.
  // [2] A is a subclass of B.
  InstanceClass::Raw A = caller->holder();
  InstanceClass::Raw B = callee->holder();

  if (A().is_subclass_of(&B)) {
    return true;
  }

#if USE_SOURCE_IMAGE_GENERATOR
  // We do this only for source generator, where we can assume all .class
  // files are generated by Javac. It's complex to guarantee, in
  // the context of an supposed inner class, that an outer class is 
  // initialized. Therefore, it would be unsafe to do this for downloaded
  // classes.
  Symbol::Raw callee_name = callee->get_original_name();
  if (callee_name().length() < 8 || 
      jvm_memcmp(callee_name().base_address(), "access$", 7) != 0) {
    return false;
  }
  if (!caller->is_static() && is_inner_class_of(&A, &B)) {
    // This looks very much like an accessor method generated by Javac. The
    // outer class is most likely initialized now.
    return true;
  }
#endif

  return false;
}

void ROMInliner::try_inline(Method* caller, Method* callee, int bci_in_caller)
{
  if (!contains(callee)) {
    // It doesn't seem callee can be inlined
    return;
  }

  Bytecodes::Code call_bc = caller->bytecode_at(bci_in_caller);
  bool is_invokestatic = false;

  switch (call_bc) {
  case Bytecodes::_fast_invokestatic:
  case Bytecodes::_fast_init_invokestatic:
    is_invokestatic = true;
    break;
  case Bytecodes::_fast_invokespecial:
  case Bytecodes::_fast_invokevirtual_final:
    // These bytecodes are always OK to inline
    break;
  case Bytecodes::_invokestatic:
  case Bytecodes::_invokevirtual:
  case Bytecodes::_invokespecial:
    // These bytecodes should have been already quickened
    SHOULD_NOT_REACH_HERE();
  default:
    // All other invoke bytecodes are not safe --
    // fast_invokevirtual   - need revisit: which actual method is invoked
    // fast_invokeinterface - ditto
    return;
  }
  
  // don't inline synchronized methods
  if (callee->uses_monitors()) {
    return;
  }

#if VERBOSE_INLINING
  tty->print("will try to inline: ");
  caller->print_name_on(tty);
  tty->print("=>");
  callee->print_name_on(tty);
  tty->print_cr(" at %d", bci_in_caller);

  tty->print_cr("caller:");
  caller->print_bytecodes(tty);
  tty->print_cr("callee:");
  callee->print_bytecodes(tty);
#endif  
 
#if VERBOSE_INLINING
  const int old_rewritten = _rewritten;
#endif

  bool inlined = false;

  if (is_invokestatic) {
    inlined = try_inline_static_getter(caller, callee, bci_in_caller);
    if (!inlined && !may_inline_invokestatic(caller, callee)) {
      return;
    }
  }
  if (!inlined) {
    inlined = try_inline_empty_method(caller, callee, bci_in_caller);
  }
  if (!inlined) {
    inlined = try_inline_getter(caller, callee, bci_in_caller);
  }
  if (!inlined) {
    inlined = try_inline_setter(caller, callee, bci_in_caller);
  }
  if (!inlined) {
    inlined = try_inline_empty_constructor(caller, callee, bci_in_caller);
  }

  // disable for now
  //try_inline_arg_return(caller, callee, bci_in_caller);
  //try_inline_forwarder(caller, callee, bci_in_caller);
  
#if VERBOSE_INLINING
  if (_rewritten != old_rewritten) {
    tty->print_cr("rewritten caller:");
    caller->print_bytecodes(tty);
  }
#endif
}

static bool is_load(Bytecodes::Code code, int at) {
  switch (at) {
  case 0:
    return (code == Bytecodes::_aload_0 ||
            code == Bytecodes::_iload_0 ||
            code == Bytecodes::_lload_0 ||
            code == Bytecodes::_fload_0 ||
            code == Bytecodes::_dload_0);
  case 1:
    return (code == Bytecodes::_aload_1 ||
            code == Bytecodes::_iload_1 ||
            code == Bytecodes::_lload_1 ||
            code == Bytecodes::_fload_1 ||
            code == Bytecodes::_dload_1);
  case 2:
    return (code == Bytecodes::_aload_2 ||
            code == Bytecodes::_iload_2 ||
            code == Bytecodes::_lload_2 ||
            code == Bytecodes::_fload_2 ||
            code == Bytecodes::_dload_2);
  case 3:
    return (code == Bytecodes::_aload_3 ||
            code == Bytecodes::_iload_3 ||
            code == Bytecodes::_lload_3 ||
            code == Bytecodes::_fload_3 ||
            code == Bytecodes::_dload_3);
  }
  
   SHOULD_NOT_REACH_HERE();
   return false;
}

bool ROMInliner::pop_as_needed(Method* caller, 
                               Method* callee,
                               int bci_in_caller) {  
  int num_params = 0;
  const bool is_static = callee->is_static();
  Signature signature = callee->signature();
  for (SignatureStream ss(&signature, is_static, true); !ss.eos(); ss.next()) {
    // couldn't handle more than 3 args with pop only
    // don't bother to handle two-word args case
    if (++num_params > 3 || is_two_word(ss.type())) {
      return false;
    }
  }

  bool need_nullcheck = true;
#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR
  // IMPL_NOTE: why !USE_PRODUCT_BINARY_IMAGE_GENERATOR ???
  if (!caller->is_static() && bci_in_caller == 1 && 
      caller->bytecode_at(0) == Bytecodes::_aload_0) {
    // This is a common pattern in vanilla constructors:
    //   bci=0 aload_0;
    //   bci=1 invokespecial super.<init>()V    
    // Here we need check if only bci 0 is a branch target -- if so, local_0
    // may become null. Otherwise we can guarantee that local_0 is non-null
    /// and can elide the null check.
    need_nullcheck = zero_bci_is_branch_target(caller);
  } 
#endif

  for (int i=0; i<3; i++) {
    Bytecodes::Code code = i < num_params ? Bytecodes::_pop : Bytecodes::_nop;
    // add null check so that behavior is the same for this == null
    if (!is_static && (i == num_params - 1)) {
      if (need_nullcheck) {
        code = Bytecodes::_pop_and_npe_if_null;
      } else {
        code = Bytecodes::_pop;
      }
    }
    caller->bytecode_at_put_raw(bci_in_caller + i, code);
  }
  return true;
}

#if !USE_PRODUCT_BINARY_IMAGE_GENERATOR
bool ROMInliner::zero_bci_is_branch_target(Method* method) {
  int codesize = method->code_size();
  register jubyte *bcptr   = (jubyte*)method->code_base();
  register jubyte *bcend   = bcptr + codesize;
  register int bci = 0;

  while (bcptr < bcend) {
    const Bytecodes::Code code = (Bytecodes::Code)(*bcptr);
    GUARANTEE(code >= 0, "sanity: unsigned value");

    const int length = method->bytecode_length_for(bci);

    switch (code) {
    case Bytecodes::_ifeq:
    case Bytecodes::_ifne:
    case Bytecodes::_iflt:
    case Bytecodes::_ifge:
    case Bytecodes::_ifgt:
    case Bytecodes::_ifle:
    case Bytecodes::_if_icmpeq:
    case Bytecodes::_if_icmpne:
    case Bytecodes::_if_icmplt:
    case Bytecodes::_if_icmpge:
    case Bytecodes::_if_icmpgt:
    case Bytecodes::_if_icmple:
    case Bytecodes::_if_acmpeq:
    case Bytecodes::_if_acmpne:
    case Bytecodes::_ifnull:
    case Bytecodes::_ifnonnull:
    case Bytecodes::_goto:
      {
        int dest = bci + (jshort)(Bytes::get_Java_u2(bcptr+1));
        if (dest == 0) return true;
        GUARANTEE(dest >= 0 && dest < codesize, "sanity");
      }
      break;
    case Bytecodes::_goto_w:
      {
        int dest = bci + (jint)(Bytes::get_Java_u4(bcptr+1));
        GUARANTEE(dest >= 0 && dest < codesize, "sanity");
        if (dest == 0) return true;
      }
      break;
    case Bytecodes::_lookupswitch:
      {
        int table_index  = align_size_up(bci + 1, sizeof(jint));
        int default_dest = bci + method->get_java_switch_int(table_index + 0);
        int num_of_pairs = method->get_java_switch_int(table_index + 4);

        if (default_dest == 0) return true;
        for (int i = 0; i < num_of_pairs; i++) {
          int dest =
            bci + method->get_java_switch_int(8 * i + table_index + 12);
          if (dest == 0) return true;
        }
      }
      break;
    case Bytecodes::_tableswitch:
      {
        int table_index  = align_size_up(bci + 1, sizeof(jint));
        int default_dest = bci + method->get_java_switch_int(table_index + 0);
        int low          = method->get_java_switch_int(table_index + 4);
        int high         = method->get_java_switch_int(table_index + 8);

        if (default_dest == 0) return true;
        for (int i = 0; i < (high - low + 1); i++) {
          int dest =
            bci + method->get_java_switch_int(4 * i + table_index + 12);
          if (dest == 0) return true;
        }
      }
      break;
    default:
      break;
    }
    bci   += length;
    bcptr += length;
  }
  GUARANTEE(bcptr == bcend, "sanity");

  return false;
}
#endif

int ROMInliner::num_args(Method* callee) {
  Signature::Raw method_signature = callee->signature();
  return method_signature().parameter_word_size(callee->is_static());  
}

bool ROMInliner::try_inline_empty_method(Method* caller, 
                                         Method* callee, 
                                         int bci_in_caller) 
{  
  if (callee->code_size() != 1) {
    return false;
  }

  Bytecodes::Code ret_bc = callee->bytecode_at(0);
  if (ret_bc != Bytecodes::_return) {
    return false;
  }
  
  if (pop_as_needed(caller, callee, bci_in_caller)) {
    _rewritten++;
    return true;
  }
  return false;
}

bool ROMInliner::try_inline_empty_constructor(Method* caller, 
                                              Method* callee, 
                                              int bci_in_caller) 
{
  if (callee->code_size() != 5) {
    return false;
  }

  if (!callee->match(Symbols::object_initializer_name(),
                     Symbols::void_signature())) {
    return false;
  }

  bool is_empty = false;

  if ((callee->bytecode_at(0) == Bytecodes::_aload_0) &&
      (callee->bytecode_at(1) == Bytecodes::_fast_invokevirtual_final) &&
      (callee->bytecode_at(4) == Bytecodes::_return)) {
    InstanceClass::Raw callee_class = callee->holder();
    if (callee_class().access_flags().has_vanilla_constructor()) {
      is_empty = true;
    }
  }
  else if ((callee->bytecode_at(0) == Bytecodes::_aload_0) &&
           (callee->bytecode_at(1) == Bytecodes::_pop) &&
           (callee->bytecode_at(2) == Bytecodes::_nop) &&
           (callee->bytecode_at(3) == Bytecodes::_nop) &&
           (callee->bytecode_at(4) == Bytecodes::_return)) {
    is_empty = true;
  }

  if (is_empty) {
    if (pop_as_needed(caller, callee, bci_in_caller)) {
      _rewritten++;
      return true;
    }
  }

  return false;
}

#if NOT_CURRENTLY_USED
static bool is_return(Bytecodes::Code code, bool args_only) {
   return (code == Bytecodes::_ireturn ||
           code == Bytecodes::_lreturn ||
           code == Bytecodes::_freturn ||
           code == Bytecodes::_dreturn ||
           code == Bytecodes::_areturn ||
           (!args_only && code == Bytecodes::_return));
}

bool ROMInliner::try_inline_arg_return(Method* caller, 
                                       Method* callee, 
                                       int bci_in_caller) 
{
  if (callee->code_size() != 2) {
    return false;
  }

  Bytecodes::Code ret_bc = callee->bytecode_at(1);
  if (is_return(callee->bytecode_at(1), true /* don't include _return */)) {
    return false;
  }

  // NOTE: We never come to here for all CLCD/MIDP classes. Also, the
  // inlining done here seems unsafe. So I am disabling this function
  // 

  if (pop_as_needed(caller, callee, bci_in_caller)) {
    _rewritten++;
    return true;
  }
  return false;
}
#endif

// This pattern happens quite often with synthetic $access methods for
// inner classes to access private static fields in an outer class.
bool ROMInliner::try_inline_static_getter(Method* caller, Method* callee, 
                                          int bci_in_caller) {
  if (callee->code_size() != 4 || callee->size_of_parameters() != 0) {
    return false;
  }
  UsingFastOops fast_oops;

  Bytecodes::Code get_bc = callee->bytecode_at(0);
  if (get_bc != Bytecodes::_fast_1_getstatic &&
      get_bc != Bytecodes::_fast_2_getstatic &&
      get_bc != Bytecodes::_fast_init_1_getstatic &&
      get_bc != Bytecodes::_fast_init_2_getstatic) {
    return false;
  }
  Bytecodes::Code ret_bc = callee->bytecode_at(3);
  if (ret_bc != Bytecodes::_ireturn &&
      ret_bc != Bytecodes::_areturn &&
      ret_bc != Bytecodes::_freturn &&
      ret_bc != Bytecodes::_dreturn &&
      ret_bc != Bytecodes::_lreturn)  {
    // not a getter (this could be a throw bytecode: CR 6197045)
    return false;
  }
  
  int callee_cpindex = callee->get_java_ushort(1);
  InstanceClass::Fast receiver = callee->holder();
  ConstantPool::Fast caller_cp = caller->constants();
  ConstantPool::Fast callee_cp = callee->constants();
  if (!callee_cp().tag_at(callee_cpindex).is_resolved_static_field()) {
    return false;
  }

  int offset, class_id;
  BasicType type = callee_cp().resolved_field_type_at(callee_cpindex, offset,
                                                      class_id);
  if (receiver().class_id() != class_id) {
    // Can't inline -- we may fail to initialize receiver class.
    return false;
  }

  // We should have resolved all field/method/interfacemethod
  // references now, so we can re-use the NameAndType entries in
  // find_or_add_resolved_field_ref()
  GUARANTEE(!caller_cp().needs_name_and_type_entries(), "sanity");

  int tag_value = callee_cp().tag_value_at(callee_cpindex);
  int caller_cpindex = find_or_add_resolved_field_ref(&caller_cp, tag_value,
                                                      offset, class_id,
                                                      type, true);
  if (caller_cpindex < 0) {
     return false;
  }

  switch (caller->bytecode_at(bci_in_caller)) {
  case Bytecodes::_fast_invokestatic:
    get_bc = is_two_word(type) ? Bytecodes::_fast_2_getstatic :
                                 Bytecodes::_fast_1_getstatic;
    break;
  case Bytecodes::_fast_init_invokestatic:
    get_bc = is_two_word(type) ? Bytecodes::_fast_init_2_getstatic :
                                 Bytecodes::_fast_init_1_getstatic;
    break;
  default:
    SHOULD_NOT_REACH_HERE();
  }

  GUARANTEE(caller->bytecode_length_for(bci_in_caller) == 3, "sanity");
  caller->bytecode_at_put_raw(bci_in_caller, get_bc);
  caller->put_java_ushort(bci_in_caller+1, caller_cpindex);
  return true;
}


int ROMInliner::find_or_add_resolved_field_ref(ConstantPool *cp, int tag_value,
                                               int offset, int class_id,
                                               BasicType type, int is_static) {
  int i;
  int len = cp->length();
  for (i = 1; i < len; i++) {
    ConstantTag tag = cp->tag_at(i);
    if (tag.value() == tag_value) {
      int old_offset;
      int old_class_id;
      cp->resolved_field_type_at(i, old_offset, old_class_id);
      if (old_offset == offset && old_class_id == class_id) {
        return i;
      }
    }

    switch (tag.value()) {
    case JVM_CONSTANT_Long:
    case JVM_CONSTANT_Double:
      i++;
      break;
    case JVM_CONSTANT_NameAndType:
    case JVM_CONSTANT_Invalid:
      cp->resolved_field_at_put(i, class_id, offset, type, is_static);
      return i;
    }
  }

  return -1;
}

bool ROMInliner::try_inline_getter(Method* caller, Method* callee, 
                                   int bci_in_caller) {
  if (!callee->is_fast_get_accessor()) {
    return false;
  }

  // IMPL_NOTE: do fast_agetfield_1 as well (although this won't
  // happen yet, as fast_agetfield_1 bytecodes are only generated inside
  // ConstantPoolRewriter, which happens after ROMInliner.

  // simple check for getter, size and loading this first
  if (callee->code_size() != 5 || 
      callee->bytecode_at(0) != Bytecodes::_aload_0) {
    return false;
  }
  
  // check if only thing on stack is instance
  if (num_args(callee) != 1) {
    return false;
  }

  // IMPL_NOTE: the following check is a bit redundant now. Change to a GUARANTEE.
  Bytecodes::Code get_bc = callee->bytecode_at(1);
  if (get_bc != Bytecodes::_fast_bgetfield &&
      get_bc != Bytecodes::_fast_sgetfield &&
      get_bc != Bytecodes::_fast_igetfield &&
      get_bc != Bytecodes::_fast_lgetfield &&
      get_bc != Bytecodes::_fast_fgetfield &&
      get_bc != Bytecodes::_fast_dgetfield &&
      get_bc != Bytecodes::_fast_agetfield &&
      get_bc != Bytecodes::_fast_cgetfield &&
      get_bc != Bytecodes::_fast_bgetfield)  {
    // not a getter
    return false;
  }

  // IMPL_NOTE: the following check is a bit redundant now. Change to a GUARANTEE.
  Bytecodes::Code ret_bc = callee->bytecode_at(4);
  if (ret_bc != Bytecodes::_ireturn &&
      ret_bc != Bytecodes::_areturn &&
      ret_bc != Bytecodes::_freturn &&
      ret_bc != Bytecodes::_dreturn &&
      ret_bc != Bytecodes::_lreturn)  {
    // not a getter (this could be a throw bytecode: CR 6197045)
    return false;
  }

  _rewritten++;
  caller->bytecode_at_put_raw(bci_in_caller, get_bc);
  caller->bytecode_at_put_raw(bci_in_caller+1, callee->bytecode_at(2));
  caller->bytecode_at_put_raw(bci_in_caller+2, callee->bytecode_at(3));
  return true;
}

bool ROMInliner::try_inline_setter(Method* caller, Method* callee, 
                                   int bci_in_caller) {
  // We must have this form of bytecodes
  // [0] aload_0
  // [1] [ailfd]load_1
  // [2] [bsailfd]putfield
  //     cpindex_byte1
  //     cpindex_byte2
  // [5] return // Must check: this could be a throw! (CR 6197045)

  // "this" and size check
   if (callee->code_size() != 6 || 
      callee->bytecode_at(0) != Bytecodes::_aload_0 ||
      callee->bytecode_at(5) != Bytecodes::_return) {
    return false;
  }   

   // value check
   if (!is_load(callee->bytecode_at(1), 1)) {
     return false;
   }

   // setfield check
   // check if only thing on stack is instance and parameter of correct length
   // ( 2 words for long and double)
   int args = num_args(callee);
   Bytecodes::Code set_bc = callee->bytecode_at(2);
   if (!(args == 2 && (set_bc == Bytecodes::_fast_bputfield ||
                       set_bc == Bytecodes::_fast_sputfield ||
                       set_bc == Bytecodes::_fast_iputfield ||
                       set_bc == Bytecodes::_fast_fputfield ||
                       set_bc == Bytecodes::_fast_aputfield)) &&
       !(args == 3 && (set_bc == Bytecodes::_fast_dputfield || 
                       set_bc == Bytecodes::_fast_lputfield))) {
     // not a setter
     return false;
   }

   _rewritten++;
   caller->bytecode_at_put_raw(bci_in_caller, set_bc);
   caller->bytecode_at_put_raw(bci_in_caller+1, callee->bytecode_at(3));
   caller->bytecode_at_put_raw(bci_in_caller+2, callee->bytecode_at(4));
   return true;
}

#if NOT_CURRENTLY_USED
static bool is_invoke(Bytecodes::Code code, bool is_fast) {  
  return (!is_fast && code == Bytecodes::_invokevirtual   ||
          !is_fast && code == Bytecodes::_invokespecial   ||
          !is_fast && code == Bytecodes::_invokestatic    ||
          code == Bytecodes::_fast_invokevirtual ||
          code == Bytecodes::_fast_invokestatic  ||
          code == Bytecodes::_fast_invokenative ||
          code == Bytecodes::_fast_invokevirtual_final
          );
}

// detect and inline invocations like this one:
// ..  f(x, y);
// and int f(x, y) { return g(x, y); } - replace it with g(x, y) - 
// it turns out to be rather frequent pattern
bool ROMInliner::try_inline_forwarder(Method* caller, Method* callee, 
                                      int bci_in_caller) {
  Signature method_signature = callee->signature();
  int num_params = method_signature.parameter_word_size(callee->is_static());
  int num_loads = 0;

  while (is_load(callee->bytecode_at(num_loads), num_loads)) {
    if (++num_loads > 3) return;
  }

  // if method is nontrivial or doesn't look like forwarder 
  if (num_loads + 4 != callee->code_size()) {
    return;
  }

  // if we modify stack state or not fast invoking method right now - don't do anything
  if (num_loads != num_params                             ||
      !is_invoke(callee->bytecode_at(num_loads), true)    ||
      !is_return(callee->bytecode_at(num_loads+3), false)) 
    {    
      return;
    }

#if 0  
  // as static methods still refers to CP, we don't want to inline this case
  // the same's applicable for _fast_invokevirtual_final, as it's invoked like
  // statics
  if (callee->is_static() ||
      caller->bytecode_at(bci_in_caller) == Bytecodes::_fast_invokevirtual_final) {
    return;
  }
#else
  // for now we follow safe path: only inline when constant pools coincides
  if (caller->constants() != callee->constants()) {
    return;
  }
#endif

  _rewritten++;
  caller->bytecode_at_put_raw(bci_in_caller,   callee->bytecode_at(num_loads));
  caller->bytecode_at_put_raw(bci_in_caller+1, callee->bytecode_at(num_loads+1));
  caller->bytecode_at_put_raw(bci_in_caller+2, callee->bytecode_at(num_loads+2));
}
#endif // NOT_CURRENTLY_USED

#undef VERBOSE_INLINING
#endif // ENABLE_ROM_GENERATOR
