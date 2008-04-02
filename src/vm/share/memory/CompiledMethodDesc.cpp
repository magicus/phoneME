/*
 *   
 *
 * Portions Copyright  2000-2008 Sun Microsystems, Inc. All Rights
 * Reserved.  Use is subject to license terms.
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
 *
 *!c<
 * Copyright 2006 Intel Corporation. All rights reserved.
 *!c>
 */

# include "incls/_precompiled.incl"
# include "incls/_CompiledMethodDesc.cpp.incl"

#if USE_COMPILER_STRUCTURES

#if ENABLE_REMOTE_TRACER
jint CompiledMethodDesc::total_code = 0;
jint CompiledMethodDesc::total_items = 0;
#endif
#if ENABLE_TRAMPOLINE


BranchItem* BranchTable::_b_items = NULL;
int BranchTable::_item_index;



 void  BranchTable::init()
  {
    _b_items = 
      (BranchItem*) jvm_malloc(BRANCH_TABLE_LENGTH *  sizeof(BranchItem));
    GUARANTEE(_b_items, "sanity");
    jvm_memset(_b_items, 0, BRANCH_TABLE_LENGTH * sizeof(BranchItem));
    _item_index = 0;
  }

  
  void BranchTable::append(address inst_addr, address caller_addr, 
    address callee_addr, jint old_inst)
  {
    TTY_TRACE_IF_VERBOSE(("entering append\n"));
    GUARANTEE(inst_addr >= (address) _compiler_area_start && 
      inst_addr <= (address) _compiler_area_top, "inst_addr sansity\n");
    GUARANTEE(caller_addr, "sanity check");
    GUARANTEE(callee_addr, "sanity check");
    GUARANTEE(old_inst, "sanity check");
    /*remove old element*/
    if (_b_items[_item_index].inst_addr() != 0)
   {
      *(int*)(_b_items[_item_index].inst_addr()) =
              _b_items[_item_index].old_inst();
      OsMisc_flush_icache(_b_items[_item_index].inst_addr(), 4);      
      TTY_TRACE_IF_VERBOSE(("cover an old branch item\n"));
    }
    
    /*new value*/
    _b_items[_item_index].set_inst_addr(inst_addr);
    _b_items[_item_index].set_caller_addr(caller_addr);
    _b_items[_item_index].set_callee_addr(callee_addr);
    _b_items[_item_index].set_old_inst(old_inst);

    TTY_TRACE_IF_VERBOSE(("inst addr = 0x%x, caller addr = 0x%x\n",
                          *(int*) inst_addr, (int)caller_addr));

    address target = callee_addr + CompiledMethodDesc::entry_offset();
    int value;
    value =( target - (inst_addr + 8)) >> 2;/*assumption: log(target)<24*/
    *(int*)(inst_addr) = (0xE << 28 | 0xA << 24 | (value & ((1<<24) - 1)));
#if AZZERT      
    GUARANTEE(*(juint*)(inst_addr) >> 24 ==0xEA, "sanity");
#endif
    OsMisc_flush_icache(inst_addr, 4);

    ++_item_index;
    _item_index &= BRANCH_TABLE_LENGTH - 1;
    TTY_TRACE_IF_VERBOSE(("leaving append\n"));
  }

  void  BranchTable::revoke(address caller)
  {
    TTY_TRACE_IF_VERBOSE(("entering revoke\n"));
    TTY_TRACE_IF_VERBOSE(("caller addr = 0x%x\n", (int)caller));
    int i = _item_index - 1;
    while(true)
    {
      if (_b_items[i].inst_addr() != 0 && _b_items[i].caller_addr() == caller)
      {
        TTY_TRACE_IF_VERBOSE(("revoke index = %d\n", i));
        jvm_memset(&_b_items[i], 0, sizeof(BranchItem));
        i--;
        if (i < 0)
        {
          i = BRANCH_TABLE_LENGTH - 1;
        }
      }else
        {
        break;
        }
    }
    TTY_TRACE_IF_VERBOSE(("leaving revoke\n"));
  }
  
 void BranchTable::remove_only(address cm)
 {
	 for (int i = 0; i < BRANCH_TABLE_LENGTH; i++)
	 {
	 	if (_b_items[i].inst_addr() != 0 && 
			_b_items[i].callee_addr() == cm)
	 	{
	              *(int*)(_b_items[i].inst_addr()) = _b_items[i].old_inst();	 		
                     OsMisc_flush_icache(_b_items[i].inst_addr(), 4);		
       		jvm_memset(&_b_items[i], 0, sizeof(BranchItem));
	 	}
	 }
 }

  void BranchTable::remove()
  {
    TTY_TRACE_IF_VERBOSE(("entering remove\n"));
    for (int i = 0; i < BRANCH_TABLE_LENGTH; i++)
    {
      if (_b_items[i].inst_addr() != 0)
      {
        const CompiledMethodDesc* callee = (CompiledMethodDesc*)_b_items[i].callee_addr();
        const CompiledMethodDesc* caller = (CompiledMethodDesc*)_b_items[i].caller_addr();
        const int callee_index = callee->get_cache_index();
        const int caller_index = caller->get_cache_index();
        GUARANTEE(callee_index >= 0, "sanity");
        GUARANTEE(caller_index >= 0, "sanity");

          if (callee_index > CompiledMethodCache::get_upb() ||
               caller_index > CompiledMethodCache::get_upb() ||  
              CompiledMethodCache::get_item(callee_index) != callee ||
              CompiledMethodCache::get_item(caller_index) != caller)//means removed
          {

            GUARANTEE(_b_items[i].inst_addr() >= (address) _compiler_area_start 
              && _b_items[i].inst_addr() <= (address) _compiler_area_top, "sansity\n");
            {
              TTY_TRACE_IF_VERBOSE(("static removed\n"));
              TTY_TRACE_IF_VERBOSE(("remove: inst_addr = 0x%x,  index = %d\n",
                                    (int) _b_items[i].inst_addr(), i));
              GUARANTEE(*(juint*)(_b_items[i].inst_addr()) >> 24 ==0xEA,
                        "sanity");
              *(int*)(_b_items[i].inst_addr()) = _b_items[i].old_inst();
              OsMisc_flush_icache(_b_items[i].inst_addr(), 4);
            }
            jvm_memset(&_b_items[i], 0, sizeof(BranchItem));
            TTY_TRACE_IF_VERBOSE(("removed index %d\n", i));
          } else {
            GUARANTEE(_b_items[i].inst_addr() >= (address) _compiler_area_start 
              && _b_items[i].inst_addr() <= (address) _compiler_area_top, "sansity\n");
            address old_caller_addr = _b_items[i].caller_addr();
            address old_callee_addr = _b_items[i].callee_addr();
            jint caller_shift = (int) ((MethodDesc*) old_caller_addr)->_klass;
            if ( caller_shift!= 0)
            {
              _b_items[i].set_caller_addr(old_caller_addr + caller_shift);
              TTY_TRACE_IF_VERBOSE(("adjusted caller\n"));
            }
            jint callee_shift = (int) ((MethodDesc*) old_callee_addr)->_klass;
            if ( callee_shift != 0)
           {
             _b_items[i].set_callee_addr(old_callee_addr + callee_shift);
             TTY_TRACE_IF_VERBOSE(("adjusted callee\n"));
           }
            
            {
              int offset = callee_shift - caller_shift;
              TTY_TRACE_IF_VERBOSE(("adjust: inst_addr = 0x%x, index = %d\n",
                                    (int) _b_items[i].inst_addr(),  i));
              GUARANTEE(*(juint*)(_b_items[i].inst_addr()) >> 24 ==0xEA, 
                        "sanity");

                    if (offset != 0)
                    {  
                        TTY_TRACE_IF_VERBOSE(("adjusted index %d\n", i));
                        int* inst_addr = (int*) (_b_items[i].inst_addr());

                        int current_target = *inst_addr & ((1<<24) - 1);
                        current_target <<= 8; /*lsl*/
                        current_target >>= 6; /*asr*/
                        current_target += offset;
                        current_target >>=2;
                        current_target &= ((1<<24) - 1);
                        *inst_addr &= ~((1<<24) - 1);
                        *inst_addr |= current_target;
                        TTY_TRACE_IF_VERBOSE(("static adjusted\n"));
                       OsMisc_flush_icache((address) inst_addr, 4);    
                        
                    }
                   _b_items[i].set_inst_addr(_b_items[i].inst_addr() + caller_shift);
                    TTY_TRACE_IF_VERBOSE(("adjusted index %d\n", i));
            }
          
          }

      }
      
    }
    TTY_TRACE_IF_VERBOSE(("leaving remove\n"));
  }

  

#endif

void CompiledMethodDesc::variable_oops_do(void do_oop(OopDesc**)) {
#if ENABLE_COMPILER
  // Create a fake handle (only used during GC).
  // Note that this temporary handle will not be visited by GC.
  CompiledMethod::Raw cm = this;
  if (cm().size() > 0) {
    // Compiled methods of size 0 are discarded compiled methods that we're
    // in the middle of discarding.  Attempting to use a RelocationReader
    // on them fails.

    // Relocation entries are ordered so that oop_type entries are at the
    // beginning (possibly padded with comment_type entries).
    for (RelocationReader stream(&cm); !stream.at_end(); stream.advance()) {
      const Relocation::Kind kind = stream.kind();
      if (kind == Relocation::oop_type) {
        OopDesc** p = (OopDesc**) (cm().entry() + stream.code_offset());
        do_oop(p);
      } else if (kind != Relocation::comment_type) {
        break;
      }
    }
  }
#endif
}

#ifndef PRODUCT
void CompiledMethodDesc::guarantee_unused() {
  // IMPL_NOTE: check that there's no reference to this
  // CompiledMethodDesc.
}
#endif

inline ReturnOop MethodDesc::compiled_code(void) const {
  GUARANTEE(has_compiled_code(), "sanity check");
  int offset = CompiledMethodDesc::entry_offset();
#if ENABLE_THUMB_COMPILER
  // The low bit is set to 0x1 so that BX will automatically switch into
  // THUMB mode.
  offset += 1;
#endif

  ReturnOop result = DERIVED(ReturnOop, execution_entry(), -offset);

#if USE_INDIRECT_EXECUTION_SENSOR_UPDATE
  GUARANTEE(ENABLE_THUMB_COMPILER, "sanity")
  if( int(result) & 0x02 ) {
    result = DERIVED(ReturnOop, result, 
                     - CompiledMethodDesc::NUMBER_OF_EXECUTION_SENSOR_BYTES);
    GUARANTEE_R(result->is_compiled_method(), "sanity");
  }
#endif

  return result;
}

#endif
