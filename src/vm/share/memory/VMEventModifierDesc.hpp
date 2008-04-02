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

typedef struct {
  jint          _thread_id;
  jint          _clazz_id;
  jlong          _method_id;
  jlong         _offset;
  Bytecodes::Code _opcode;
} locationModifier;

typedef  struct {
  jboolean            _sig_caught;
  jboolean            _sig_uncaught;
} exception_mod;

typedef  struct  {
  jint                _step_size;
  jint                _step_depth;
  jlong               _step_starting_offset;
  jlong               _dup_current_line_offset;
  jlong               _post_dup_line_offset;
  address             _step_starting_fp;
  locationModifier    _step_target;
} single_step_mod;

class VMEventModifierDescPointers : public MixedOopDesc {
protected:
  OopDesc*         _class_name;
  OopDesc*         _next;
  OopDesc*         _rom_debug_method;

  friend class OopDesc;
  friend class VMEventModifier;
  friend class Universe;
  friend class VMEvent;
};

class VMEventModifierDesc : public VMEventModifierDescPointers {

  locationModifier _location_mod;

#if defined(UNDER_ADS) && (__ARMCC_VERSION < 200000)
  // ADS 1.2 wants this
  public:
#endif

  exception_mod _exception_mod;
  single_step_mod _singlestep_mod;

#if defined(UNDER_ADS) && (__ARMCC_VERSION < 200000)
  // ADS 1.2 wants this
  private:
#endif

  // If this is in the heap we will not restore it into the method
  // Instead we will just set the entry to the 'default'
  // Hence we don't bother with GC protection for it.
  address         _saved_method_entry;

  jint            _event_count;
  jboolean        _compile_state;
  jbyte           _mod_kind;

  static size_t allocation_size() {
      return align_allocation_size(sizeof(VMEventModifierDesc));
  }
    
  static size_t pointer_count() {
    return (sizeof(VMEventModifierDescPointers) - sizeof(MixedOopDesc)) /
           sizeof(OopDesc*);
  }
  friend class VMEventModifier;

};
