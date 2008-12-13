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

inline int JavaFrame__arg_offset_from_sp(int index) {
  GUARANTEE(JavaStackDirection < 0, "Cannot handle forward stacks");
  // The stack pointer points at the beginning of the last argument
  return BytesPerStackElement * (index + 1) - BytesPerWord;
}

inline int JavaFrame__stack_bottom_pointer_offset() {return  -20;}
inline int JavaFrame__bcp_store_offset()            {return  -16;}
inline int JavaFrame__locals_pointer_offset()       {return  -12;}
inline int JavaFrame__cpool_offset()                {return   -8;}
inline int JavaFrame__method_offset()               {return   -4;}
inline int JavaFrame__caller_fp_offset()            {return    0;} 
inline int JavaFrame__return_address_offset()       {return    4; }

// These two slots are not used in this architecture.
inline int JavaFrame__stored_int_value1_offset()    {return  -99999;}
inline int JavaFrame__stored_int_value2_offset()    {return  -99999;}

inline int JavaFrame__frame_desc_size()             {return 7*BytesPerWord; }  

inline int JavaFrame__end_of_locals_offset()        { return 8;   }
inline int JavaFrame__empty_stack_offset()          { return -20; }


inline int EntryFrame__pending_activation_offset()   { return  -28; }
inline int EntryFrame__pending_exception_offset()    { return  -24; }
inline int EntryFrame__stored_obj_value_offset()     { return  -20; }
inline int EntryFrame__stored_int_value2_offset()    { return  -16; }
inline int EntryFrame__stored_int_value1_offset()    { return  -12; }
inline int EntryFrame__stored_last_sp_offset()       { return -8; }
inline int EntryFrame__stored_last_fp_offset()       { return -4; }
inline int EntryFrame__real_return_address_offset()  { return  0; }
inline int EntryFrame__fake_return_address_offset()  { return +4; }

inline int EntryFrame__frame_desc_size()     { return 9 * BytesPerWord;};

// When an EntryFrame is empty, the sp points at the word above
inline int EntryFrame__empty_stack_offset()          { return -28; }

#if ENABLE_EMBEDDED_CALLINFO
// number of bytes between the return address and the start of the callinfo
inline int JavaFrame__callinfo_offset_from_return_address() { return 1;}
#endif

// number of bytes between a stack tag value and its tag
inline int StackValue__stack_tag_offset() { return -4; }
