; Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
; This file is necessary because the VC++/VS2005 tools do not support
; ARM in-line assembler in C source files.

	AREA |.text|, CODE
	EXPORT	fast_pixel_set

; ARGS(void* mem, int value, int number_of_pixels)

fast_pixel_set PROC
	add     r2, r0, r2, lsl #1
	add     r3, r1, r1, lsl #0x10

	and     r1, r0, #0x3
	cmp     r1,#0
	strneh  r3, [r0],#2
	subne   r2, r2, #1

	sub     r1, r2, #0x1f

	cmp     r0, r1
	bge     loop2

        stmfd   sp!, {r4-r11}

	mov     r4,  r3
	mov     r5,  r3
	mov     r6,  r3
	mov     r7,  r3
	mov     r8,  r3
	mov     r9,  r3
	mov     r10, r3

loop
        stmia   r0!, {r3-r10}
	cmp     r0, r1
	blt     loop
        ldmfd   sp!, {r4-r11} 

loop2
	cmp     r0,r2;
	strlth  r3, [r0],#2
	blt     loop2

        mov     pc, lr
    
	EXPORT fast_rect_8x8
fast_rect_8x8 PROC           ; args (void*first_pixel, int ypitch, int pixel)
        stmfd   sp!, {r4-r5}

	add     r2, r2, r2, lsl #0x10
        mov     r3, r2
        mov     r4, r2
        mov     r5, r2

        stmia   r0, {r2,r3,r4,r5} ; 0
        add     r0, r0, r1
        stmia   r0, {r2,r3,r4,r5} ; 1
        add     r0, r0, r1
        stmia   r0, {r2,r3,r4,r5} ; 2
        add     r0, r0, r1
        stmia   r0, {r2,r3,r4,r5} ; 3
        add     r0, r0, r1
        stmia   r0, {r2,r3,r4,r5} ; 4
        add     r0, r0, r1
        stmia   r0, {r2,r3,r4,r5} ; 5
        add     r0, r0, r1
        stmia   r0, {r2,r3,r4,r5} ; 6
        add     r0, r0, r1
        stmia   r0, {r2,r3,r4,r5} ; 7

        ldmfd   sp!, {r4-r5} 
        mov     pc, lr


; is guaranteed to render more than one line
; void quick_render_background_16_240(pixelType *dst, pixelType *src,
;                                     int skip_bytes, int lines);

	EXPORT  quick_render_background_16_240
quick_render_background_16_240 PROC
        stmdb     sp!, {r4 - r10, lr}
        ldmia     r1, {r4 - r10, lr}   ; load 16 pixels into 8 regs
        b         loop_16_240

        ; 240 pixels = 16 pixels stored 15 times
again_16_240
        stmia     r0!, {r4 - r10, lr}   ; 0 
        stmia     r0!, {r4 - r10, lr}   ; 1  
        stmia     r0!, {r4 - r10, lr}   ; 2  
        stmia     r0!, {r4 - r10, lr}   ; 3  
        stmia     r0!, {r4 - r10, lr}   ; 4  
        stmia     r0!, {r4 - r10, lr}   ; 5  
        stmia     r0!, {r4 - r10, lr}   ; 6  
        stmia     r0!, {r4 - r10, lr}   ; 7  
        stmia     r0!, {r4 - r10, lr}   ; 8  
        stmia     r0!, {r4 - r10, lr}   ; 9  
        stmia     r0!, {r4 - r10, lr}   ;10  
        stmia     r0!, {r4 - r10, lr}   ;11  
        stmia     r0!, {r4 - r10, lr}   ;12  
        stmia     r0!, {r4 - r10, lr}   ;13  
        stmia     r0!, {r4 - r10, lr}   ;14  
        sub       r3, r3, #1
        add       r0, r0, r2

loop_16_240
        cmp       r3, #0
        bgt       again_16_240
        ldmia     sp!, {r4 - r10, pc}  ; ldmfd

	EXPORT  quick_render_background_16_176
quick_render_background_16_176 PROC
        stmdb     sp!, {r4 - r10, lr}
        ldmia     r1, {r4 - r10, lr}   ; load 16 pixels into 8 regs
        b         loop_16_176

        ; 176 pixels = 16 pixels stored 11 times
again_16_176
        stmia     r0!, {r4 - r10, lr}   ; 0 
        stmia     r0!, {r4 - r10, lr}   ; 1  
        stmia     r0!, {r4 - r10, lr}   ; 2  
        stmia     r0!, {r4 - r10, lr}   ; 3  
        stmia     r0!, {r4 - r10, lr}   ; 4  
        stmia     r0!, {r4 - r10, lr}   ; 5  
        stmia     r0!, {r4 - r10, lr}   ; 6  
        stmia     r0!, {r4 - r10, lr}   ; 7  
        stmia     r0!, {r4 - r10, lr}   ; 8  
        stmia     r0!, {r4 - r10, lr}   ; 9  
        stmia     r0!, {r4 - r10, lr}   ;10  
        sub       r3, r3, #1
        add       r0, r0, r2

loop_16_176
        cmp       r3, #0
        bgt       again_16_176
        ldmia     sp!, {r4 - r10, pc}  ; ldmfd

        LTORG
	ENDP
	END
