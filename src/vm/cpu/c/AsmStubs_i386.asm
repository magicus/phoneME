        ; 
        ;   
        ;
        ; Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
        ; DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
        ; 
        ; This program is free software; you can redistribute it and/or
        ; modify it under the terms of the GNU General Public License version
        ; 2 only, as published by the Free Software Foundation.
        ; 
        ; This program is distributed in the hope that it will be useful, but
        ; WITHOUT ANY WARRANTY; without even the implied warranty of
        ; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
        ; General Public License version 2 for more details (a copy is
        ; included at /legal/license.txt).
        ; 
        ; You should have received a copy of the GNU General Public License
        ; version 2 along with this work; if not, write to the Free Software
        ; Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
        ; 02110-1301 USA
        ; 
        ; Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
        ; Clara, CA 95054 or visit www.sun.com if you need additional
        ; information or have any questions.
        ;

        .486P
        .MODEL flat, C

CONST	SEGMENT
__lo_dword_of_shifter DD 0h        ; shifter 0x7bf0000000000000r (9.74531e+288)
__hi_dword_of_shifter DD 7bf00000h 
CONST	ENDS
        
CodeSegment segment  para  public   'CODE'
        ALIGN 16

_lvalue$ = 8
_rvalue$ = 16
	PUBLIC JFP_lib_dmul_x86
JFP_lib_dmul_x86:
        ; Load the shifter.
        mov   edx, dword ptr __lo_dword_of_shifter
        mov   dword ptr[esp - 8], edx                   
        mov   edx, dword ptr __hi_dword_of_shifter
        mov   dword ptr[esp - 4], edx
        ; Shift 80-bit values to achieve the 
        ; 64-bit precision of the operation.
        fld1                                       
        fdiv  qword ptr [esp - 8]
        fstp  qword ptr [esp - 16]
        fld   qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                    
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        ; Perform operation with 64-bit precision.
        fmul  qword ptr [esp + 4]                 
        fmul  qword ptr [esp + 12]                 
        ; Prepare shifting back.        
        fld   qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]
        ; Get 80-bit result.
        fmulp st(1), st(0)      
        ; Round 80-bit result to 64-bit value.
        fstp  qword ptr [esp - 16]
        fld   qword ptr [esp - 16]
        ret                                        

	PUBLIC JFP_lib_ddiv_x86
JFP_lib_ddiv_x86:
        ; Load the shifter.
        mov   edx, dword ptr __lo_dword_of_shifter
        mov   dword ptr[esp - 8], edx                   
        mov   edx, dword ptr __hi_dword_of_shifter
        mov   dword ptr[esp - 4], edx
        ; Shift 80-bit values to achieve the 
        ; 64-bit precision of the operation.
        fld1                                       
        fdiv  qword ptr [esp - 8]
        fstp  qword ptr [esp - 16]
        fld   qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                    
        fmul  qword ptr [esp - 16]                 
        fmul  qword ptr [esp - 16]                 
        ; Perform operation with 64-bit precision.
        fmul  qword ptr [esp + 4]                 
        fdiv  qword ptr [esp + 12]                 
        ; Prepare shifting back.
        fld   qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        fmul  qword ptr [esp - 8]                 
        ; Get 80-bit result.
        fmulp st(1), st(0)      
        ; Round the result from 80-bit to 64-bit.
        fstp  qword ptr [esp - 16]
        fld   qword ptr [esp - 16]
        ret                                        
	
        ; mark the end of ASM functions
        PUBLIC end_of_the_world
end_of_the_world:

CodeSegment ends
        END
