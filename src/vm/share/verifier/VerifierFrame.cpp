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

/**
   \class VerifierFrame
   Defines the stack operations performed on the CLDC Verifier stack.
 */

#include "incls/_precompiled.incl"
#include "incls/_VerifierFrame.cpp.incl"

// initialize verifier stack and locals 
void VerifierFrame::initialize(Method *method, ObjArray *stackmaps JVM_TRAPS) {
  // initialize stack and locals bit maps for tracking tag types
  _max_stack  = method->max_stack();
  _max_locals = method->max_locals();

  if (_max_stack <= Verifier::VSTACK_CACHE_SIZE) {
    _vstack_tags    = Universe::verifier_vstack_tags_cache()->obj();
    _vstack_classes = Universe::verifier_vstack_classes_cache()->obj();

    address base = (address)_vstack_classes().base_address();
    jvm_memset(base, 0, _max_stack * sizeof(int));
  } else {
    _vstack_tags    = Universe::new_int_array(_max_stack JVM_CHECK);
    _vstack_classes = Universe::new_obj_array  (_max_stack JVM_CHECK);
  }

  if (_max_locals <= Verifier::VLOCALS_CACHE_SIZE) {
    _vlocals_tags    = Universe::verifier_vlocals_tags_cache()->obj();
    _vlocals_classes = Universe::verifier_vlocals_classes_cache()->obj();

    address base = (address)_vlocals_classes().base_address();
    jvm_memset(base, 0, _max_locals * sizeof(int));
  } else {
    _vlocals_tags    = Universe::new_int_array(_max_locals JVM_CHECK);
    _vlocals_classes = Universe::new_obj_array  (_max_locals JVM_CHECK);
  }

  _vstackpointer = 0;    // initial stack is empty
  _stackmaps = stackmaps;
}

void VerifierFrame::push_category1(StackMapKind kind JVM_TRAPS) {
  int vsp = vstackpointer();
  if (vsp < max_stack()) {
    // push the tag type and the class
    vstack_tags_at_put(vsp, kind);
    vstack_classes_at_clear(vsp);
    _vstackpointer = vsp + 1;
  } else {
    VFY_ERROR(ve_stack_overflow);
  }
}

void VerifierFrame::push_category1(StackMapKind kind, Symbol *name JVM_TRAPS) {
  int vsp = vstackpointer();
  if (vsp < max_stack()) {
    // push the tag type and the class
    vstack_tags_at_put(vsp, kind);
    vstack_classes_at_put(vsp, name);
    _vstackpointer = vsp + 1;
  } else {
    VFY_ERROR(ve_stack_overflow);
  }
}

void VerifierFrame::pop_category1(StackMapKind& kind, Symbol *name JVM_TRAPS) {
  int vsp = vstackpointer();
  if (vsp > 0) {
    vsp --;
    _vstackpointer = vsp;
    // push the tag type and the class
    kind = vstack_tags_at(vsp);
    *name = vstack_classes_at(vsp);
    if (kind == ITEM_Double_2 || kind == ITEM_Long_2) {
      VFY_ERROR(ve_stack_bad_type);
    }
  } else {
    VFY_ERROR(ve_stack_underflow);
  }
}

void VerifierFrame::push_category2(StackMapKind kind1, StackMapKind kind2,
                                   Symbol *name1, Symbol *name2 JVM_TRAPS) {
  int vsp = vstackpointer();
  if (vsp + 1 < max_stack()) {
    vstack_tags_at_put(vsp,      kind1);
    vstack_classes_at_put(vsp,   name1);
    vstack_tags_at_put(vsp+1,    kind2);
    vstack_classes_at_put(vsp+1, name2);
    _vstackpointer = vsp + 2;
  } else {
    VFY_ERROR(ve_stack_overflow);
  }
}

void VerifierFrame::pop_category2(StackMapKind& kind1, StackMapKind& kind2,
                                   Symbol *name1, Symbol *name2 JVM_TRAPS) {
  int vsp = vstackpointer();
  if (vsp <= 1) {
    VFY_ERROR(ve_stack_underflow);
  }
  vsp -=2;
  _vstackpointer = vsp;
  kind1  = vstack_tags_at(vsp);
  kind2  = vstack_tags_at(vsp + 1);
  *name1 = vstack_classes_at(vsp);
  *name2 = vstack_classes_at(vsp + 1);
  if (kind1 == ITEM_Double_2 || kind1 == ITEM_Long_2) {
    VFY_ERROR(ve_stack_bad_type);
  }
}

void VerifierFrame::pop_ref(StackMapKind& kind, Symbol* name JVM_TRAPS) {
  pop_category1(kind, name JVM_CHECK);
  if (kind == ITEM_Object || kind == ITEM_Null || kind == ITEM_InitObject ||
      IS_TAG_TYPE_FOR_NEW_OBJECT(kind)) {
    // do nothing
  } else {
    VFY_ERROR(ve_stack_bad_type);
  }
}

void VerifierFrame::pop_ref(JVM_SINGLE_ARG_TRAPS) {
  UsingFastOops fast_oops;
  StackMapKind kind_dummy;
  Symbol::Fast name_dummy;
  pop_ref(kind_dummy, &name_dummy JVM_NO_CHECK_AT_BOTTOM);
}

void VerifierFrame::pop_object(Symbol* name JVM_TRAPS) {
  UsingFastOops fast_oops;
  StackMapKind kind;
  Symbol::Fast stack_name;
  pop_category1(kind, &stack_name JVM_CHECK);
  switch(kind) {
    case ITEM_Object:
      {
        bool result = compute_is_subtype_of(&stack_name, name JVM_CHECK);
        if (result) {
          return;
        }
      }
      break;

    case ITEM_Null:
      return;
  }
  VFY_ERROR(ve_stack_bad_type);
}

void VerifierFrame::pop(StackMapKind kind JVM_TRAPS) {
  int vsp = vstackpointer();
  if (vsp > 0) {
    vsp --;
    _vstackpointer = vsp;
    StackMapKind kind2 = vstack_tags_at(vsp);
    if (kind != kind2) {
      VFY_ERROR(ve_stack_bad_type);
    }
  } else {
    VFY_ERROR(ve_stack_underflow);
  }
}

void VerifierFrame::set_local(int index, StackMapKind kind JVM_TRAPS) {
  GUARANTEE(kind != ITEM_Double && kind != ITEM_Long, "Bad call");
  if (index >= 0 && index < max_locals()) {
    vlocals_tags_at_put(index, kind);
    vlocals_classes_at_clear(index);
    bogify_surrounding_doublewords(index);
  } else {
    VFY_ERROR(ve_locals_overflow);
  }
}

void VerifierFrame::set_local2(int index, StackMapKind kind JVM_TRAPS) {
  GUARANTEE(kind == ITEM_Double || kind == ITEM_Long, "Bad call");
  if (index < 0 || index + 1 >= max_locals()) {
    VFY_ERROR(ve_locals_overflow);
  }
  vlocals_tags_at_put(index, kind);
  vlocals_tags_at_put(index + 1, kind == ITEM_Long ? ITEM_Long_2 : ITEM_Double_2);
  vlocals_classes_at_clear(index);
  vlocals_classes_at_clear(index+1);
  bogify_surrounding_doublewords(index);
}

void VerifierFrame::set_local_ref(int index, StackMapKind kind, Symbol *name
                                  JVM_TRAPS) {
  if (index < 0 || index >= max_locals()) {
    VFY_ERROR(ve_locals_overflow);
  }
  vlocals_tags_at_put(index, kind);
  vlocals_classes_at_put(index, name);
  bogify_surrounding_doublewords(index);
}

void VerifierFrame::bogify_surrounding_doublewords(int index) {
  if (index >= 1) {
    StackMapKind previousTag = vlocals_tags_at(index - 1);
    if (previousTag == ITEM_Double || previousTag == ITEM_Long) {
      vlocals_tags_at_put(index - 1, ITEM_Bogus);
    }
  }
  // We can ignore if we're overwriting the first word of a long or
  // double.  We will have a useless ITEM_Double2 or ITEM_Long2 sitting
  // in the registers.  But there's nothing you can do with one!
}

void VerifierFrame::check_local(int index, StackMapKind kind JVM_TRAPS) {
  GUARANTEE(kind == ITEM_Integer || kind == ITEM_Float, "Bad call");
  if (index < 0 || index >= max_locals()) {
    VFY_ERROR(ve_locals_overflow);
  }
  if (vlocals_tags_at(index) != kind) {
    VFY_ERROR(ve_locals_bad_type);
  }
}

void VerifierFrame::check_local2(int index,  StackMapKind kind JVM_TRAPS) {
  GUARANTEE(kind == ITEM_Double || kind == ITEM_Long, "Bad call");
  if (index < 0 || index + 1 >= max_locals()) {
    VFY_ERROR(ve_locals_overflow);
  }
  if (vlocals_tags_at(index) != kind) {
    VFY_ERROR(ve_locals_bad_type);
  }
  GUARANTEE(   kind == ITEM_Long && vlocals_tags_at(index + 1) == ITEM_Long_2
            || kind == ITEM_Double && vlocals_tags_at(index + 1) == ITEM_Double_2,
           "Verifier assertions");
}

void VerifierFrame::check_local_ref(int index,
                                    StackMapKind& kind, Symbol*name JVM_TRAPS)
{
  if (index < 0 || index >= max_locals()) {
    VFY_ERROR(ve_locals_overflow);
  }
  kind = vlocals_tags_at(index);
  if (kind == ITEM_Object) {
    *name = vlocals_classes_at(index);
  } else if (kind == ITEM_Null || kind == ITEM_InitObject
             || IS_TAG_TYPE_FOR_NEW_OBJECT(kind)) {
    name->set_null();
  } else {
    VFY_ERROR(ve_locals_bad_type);
  }
}

void VerifierFrame::replace_stack_type_with_real_type(StackMapKind from,
                                                      StackMapKind to,
                                                      Symbol* name) {
  int i;
  for (i = 0; i < max_stack(); i++) {
    if (vstack_tags_at(i) == from) {
      vstack_tags_at_put(i, to);
      vstack_classes_at_put(i, name);
    }
  }
  for (i = 0; i < max_locals(); i++) {
    if (vlocals_tags_at(i) == from) {
      vlocals_tags_at_put(i, to);
      vlocals_classes_at_put(i, name);
    }
  }
}

#if !defined(PRODUCT) || ENABLE_TTY_TRACE
void VerifierFrame::verifier_error(ErrorMsgTag err JVM_TRAPS) {
  if (Thread::current_has_pending_exception() && 
    (TraceExceptions || GenerateROMImage)) {
    tty->print("Exception during verification: ");
  }
  if (TraceExceptions) {
    ps();
  }
  Thread::clear_current_pending_exception();
  Throw::class_format_error(err JVM_THROW);
}
#endif

void VerifierFrame::verifier_error(JVM_SINGLE_ARG_TRAPS) {
  Thread::clear_current_pending_exception();
  Throw::class_format_error(verification_error JVM_THROW);
}


int VerifierFrame::pop_invoke_arguments(Signature* method_signature JVM_TRAPS)
{
  // get the parameter word size, but force "is_static" to true so that
  // the size returned does not add 1 to account for the 'this' pointer
  int param_size = method_signature->parameter_word_size(true);

  // Are these the required number of words on the stack?
  if (vstackpointer() < param_size) {
    VFY_ERROR_0(ve_args_not_enough);
  }

  _vstackpointer = vstackpointer() - param_size;

  // iterate through the SignatureStream to verify its parameter types
  SignatureStream ss(method_signature, true);

  int n = vstackpointer();
  for (; !ss.eos(); ss.next()) {
    BasicType param_type = ss.type();
    switch(param_type) {
       case T_INT: case T_BYTE: case T_CHAR: case T_SHORT: case T_BOOLEAN:
        if (vstack_tags_at(n) != ITEM_Integer) {
          VFY_ERROR_0(ve_stack_bad_type);
        }
        n++;
        break;

      case T_FLOAT:
        if (vstack_tags_at(n) != ITEM_Float) {
          VFY_ERROR_0(ve_stack_bad_type);
        }
        n++;
        break;

      case T_LONG:
        if (vstack_tags_at(n) != ITEM_Long) {
          VFY_ERROR_0(ve_stack_bad_type);
        }
        GUARANTEE(vstack_tags_at(n+1) == ITEM_Long_2, "Verifier assertion");
        n += 2;
        break;

      case T_DOUBLE:
        if (vstack_tags_at(n) != ITEM_Double) {
          VFY_ERROR_0(ve_stack_bad_type);
        }
        GUARANTEE(vstack_tags_at(n+1) == ITEM_Double_2, "Verifier assertion");
        n += 2;
        break;

      case T_OBJECT:
      case T_ARRAY: {
        UsingFastOops fast_oos;
        Symbol::Fast expected_name = ss.type_symbol();
        Symbol::Fast actual_name;
        // pop the class corresponding to the ITEM_Object tag
        StackMapKind actual_kind = vstack_tags_at(n);
        if (actual_kind == ITEM_Null) {
          // do nothing
        } else if (actual_kind == ITEM_Object) {
          actual_name = vstack_classes_at(n);
          bool result = compute_is_subtype_of(&actual_name,
                                              &expected_name JVM_CHECK_0);
          if (!result) {
			  // CLEANUP
			  tty->print("Verifier error: actual name = "); 
			  actual_name().print_symbol_on(tty); 
			  tty->print(", expected name = "); 
			  expected_name().print_symbol_on(tty); 
			  tty->print_cr("");
			  // CLEANUP
            VFY_ERROR_0(ve_stack_bad_type);
          }
        } else {
          VFY_ERROR_0(ve_stack_bad_type);
        }

        n++;
        break;
      }
      default:
        SHOULD_NOT_REACH_HERE();
    }
  }
  return param_size;
}

#ifndef PRODUCT
void VerifierFrame::print_frame() {
  UsingFastOops fast_oops;
  Symbol::Fast name;
  int i;
  for (i = 0; i < max_locals(); i++) {
    StackMapKind kind = vlocals_tags_at(i);
    name = vlocals_classes_at(i);
    print_frame_internal("LOCAL", i, kind, &name);
  }
  if (max_locals() > 0 && vstackpointer() > 0) {
    tty->print_cr("   ---");
  }
  for (i = 0; i < vstackpointer(); i++) {
    StackMapKind kind = vstack_tags_at(i);
    name = vstack_classes_at(i);
    print_frame_internal("STACK", i, kind, &name);
  }
}

void VerifierFrame::print_frame_internal(const char *name, int index,
                                         StackMapKind kind, Symbol* cname) {
  tty->print("   %s[%d] %d: ", name, index, kind);
  switch(kind) {
    case ITEM_Object:     cname->print_value_on(tty); tty->cr(); break;
    case ITEM_Integer:    tty->print_cr("Integer"); break;
    case ITEM_Float:      tty->print_cr("Float"); break;
    case ITEM_Double:     tty->print_cr("Double"); break;
    case ITEM_Double_2:   tty->print_cr("Double_2"); break;
    case ITEM_Long:       tty->print_cr("Long"); break;
    case ITEM_Long_2:     tty->print_cr("Long_2"); break;
    case ITEM_InitObject: tty->print_cr("Uninitialized This"); break;
    case ITEM_Null:       tty->print_cr("Null"); break;
    case ITEM_Bogus:      tty->print_cr("Bogus"); break;
    default:              tty->print_cr(IS_TAG_TYPE_FOR_NEW_OBJECT(kind) ?
                                        "New" : "???"); break;
  }
}
#endif

void VerifierFrame::save_stack_state() {
  _saved_stack_pointer = vstackpointer();
  _saved_tags_0        = vstack_tags_at(0);
  _saved_klass_0       = vstack_classes_at(0);
  _vstackpointer       = 0;
}

void VerifierFrame::restore_stack_state() {
  _vstackpointer = _saved_stack_pointer;
  vstack_tags_at_put(0,    _saved_tags_0);
  vstack_classes_at_put(0, &_saved_klass_0);
}

void VerifierFrame::check_stackmap_match(Method *method, int target_bci, 
                                         int flags,
                                         ErrorMsgTag err JVM_TRAPS) {
  int stackmap_index;
  (void)err;
  if (((juint)target_bci) < ((juint)Verifier::stackmap_cache_max())) {
    stackmap_index = Universe::verifier_stackmap_cache()->int_at(target_bci);
    GUARANTEE(stackmap_index == get_stackmap_index_for_offset(target_bci),
              "cached value must match");
  } else {
    stackmap_index = get_stackmap_index_for_offset(target_bci);
  }

  if (stackmap_index < 0) {
    if (flags & SM_EXIST) {
      // Need stackmap but couldn't find it
      goto error;
    } else {
      return;
    }
  }

  {
    UsingFastOops fast_oops;
    bool result;
    bool target_needs_initialization = false;
    int i;

#ifndef PRODUCT
    if (TraceVerifier) {
      if ((flags & SM_MERGE) == 0) {
        tty->print_cr("Checking against target stackmap at %d", target_bci);
      } else if ((flags & SM_CHECK) == 0) {
        tty->print_cr("Grabbing new stackmap at %d", target_bci);
      } else {
        tty->print_cr("Merging in new stackmap at %d", target_bci);
      }
    }
#endif

    TypeArray::Fast stackmap_scalars = stackmaps()->obj_at(stackmap_index);
    ObjArray::Fast  stackmap_classes = stackmaps()->obj_at(stackmap_index + 1);
    int nlocals = stackmap_scalars().int_at(1);
    int nstack  = stackmap_scalars().int_at(nlocals+2);

    Symbol::Fast  slocal_class, vlocal_class;
    StackMapKind  slocal_tag, vlocal_tag;

    for (i = 0; i < nlocals; i++) {
      slocal_tag = (StackMapKind)stackmap_scalars().int_at(i + 2);
      if (slocal_tag == ITEM_Object) {
        slocal_class = stackmap_classes().obj_at(i + 2);
      } else if (slocal_tag == ITEM_InitObject) { 
        target_needs_initialization = true;
      }
      if (SM_CHECK & flags) {
        vlocal_tag    = vlocals_tags_at(i);
        vlocal_class  = vlocals_classes_at(i);
        result = is_assignable_to(vlocal_tag, slocal_tag,
                                  &vlocal_class, &slocal_class JVM_NO_CHECK);
        if (!result) {
          goto error;
        }
      }
      if (SM_MERGE & flags) {
        vlocals_tags_at_put(i, slocal_tag);
        vlocals_classes_at_put(i, &slocal_class);
      }
    }
    if (SM_MERGE & flags) {
      for (i = nlocals; i < method->max_locals(); i++) {
        vlocals_tags_at_put(i, ITEM_Bogus);
        vlocals_classes_at_clear(i);
      }
    }

    if ((SM_CHECK & flags) && (nstack != vstackpointer())) {
      goto error;
    } else if (SM_MERGE & flags) {
      _vstackpointer = nstack;
    }

    Symbol::Fast  sstack_class, vstack_class;
    StackMapKind  sstack_tag, vstack_tag;
    for (i = 0; i < nstack; i++) {
      sstack_tag = (StackMapKind)stackmap_scalars().int_at(nlocals + i + 3);
      if (sstack_tag == ITEM_Object) {
        sstack_class = stackmap_classes().obj_at(nlocals + i + 3);
      } else if (sstack_tag == ITEM_InitObject) { 
        target_needs_initialization = true;
      }
      if (SM_CHECK & flags) {
        vstack_tag       = vstack_tags_at(i);
        vstack_class     = vstack_classes_at(i);
        result = is_assignable_to(vstack_tag, sstack_tag,
                                  &vstack_class, &sstack_class JVM_NO_CHECK);
        if (!result) {
          goto error;
        }
      }
      if (SM_MERGE & flags) {
        vstack_tags_at_put(i, sstack_tag);
        vstack_classes_at_put(i, &sstack_class);
      }
    }

    if (method->is_object_initializer()) { 
      if (need_initialization() && !target_needs_initialization) { 
          goto error;
      }
      if (SM_MERGE & flags) { 
        set_need_initialization(target_needs_initialization);
      }
    }

#ifndef PRODUCT
    if (TraceVerifier && (flags & SM_MERGE)) {
      tty->print_cr("New stackmap for bci==%d", target_bci);
      print_frame();
    }
#endif
  }

  // Things are fine.
  return;

error:
  VFY_ERROR(err);
}

// Check if a value of a tag or class type can be converted
// to a value of another tag or class type.
bool VerifierFrame::is_assignable_to(StackMapKind from_type, 
                                     StackMapKind to_type,
                                     Symbol *from_class, Symbol *to_class
                                     JVM_TRAPS) {
  if (from_type == to_type) {
    if (from_type == ITEM_Object) {
      return compute_is_subtype_of(from_class, to_class JVM_NO_CHECK_AT_BOTTOM);
    } else {
      return true;
    }
  }
  if (to_type == ITEM_Bogus) {
    return true;
  }
  if (to_type == ITEM_Object && from_type == ITEM_Null) {
    return true;
  }
  return false;
}

bool VerifierFrame::compute_is_subtype_of(Symbol *from_name, Symbol *to_name
                                          JVM_TRAPS) {
  // Let's look at the two easiest ones first
  if (from_name->equals(to_name)) {
    return true;
  }
  if (to_name->equals(Symbols::java_lang_Object())) {
    return true;
  }
  UsingFastOops fast_oops;

  JavaClass::Fast from = SystemDictionary::resolve(from_name, ErrorOnFailure
                                                   JVM_CHECK_0);
  JavaClass::Fast from_ancestor = from().super();
  Symbol::Fast name;
  for(; !from_ancestor.is_null(); from_ancestor = from_ancestor().super()) {
    name = from_ancestor().name();
    if (to_name->equals(&name)) {
      return true;
    }
  }
  JavaClass::Fast to = SystemDictionary::resolve(to_name, ErrorOnFailure
                                                 JVM_CHECK_0);
  return to().is_interface() || from().compute_is_subtype_of(&to);
}
