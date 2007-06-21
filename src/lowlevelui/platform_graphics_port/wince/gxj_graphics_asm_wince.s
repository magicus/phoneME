; Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

; void fast_pixel_set(void* mem, int value, int number_of_pixels);
;
; Perform fast consecutive setting of pixel on raster 32-bits at a time,
; this function is similar to memset, with the exception that the value is
; 16bit wide instead of 8bit, and that number of pixels is counter is in
; 16bits, instead of bytes
;
; mem              - address of raster to fill
; value            - uint16 color value to fill
; number_of_pixels - number of pixels to fill
;                    (total bytes set is 2*number_of_pixels)

	EXPORT	fast_pixel_set
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


; void unclipped_blit(unsigned short *dstRaster, int dstSpan,
;   unsigned short *srcRaster, int srcSpan, int height, int width,
;   gxj_screen_buffer *dst);
;
; Low level simple blit of 16bit pixels from src to dst
;
; srcRaster - short* aligned pointer into source of pixels
; dstRaster - short* aligned pointer into destination
; srcSpan   - number of bytes per scanline of srcRaster (must be even)
; dstSpan   - number of bytes per scanline of dstRaster (must be even)
; width     - number of bytes to copy per scanline (must be even)
; height    - number of scanlines to copy
; Note: There is a special case for blitting a 16x16 image to an aligned dst

	EXPORT  unclipped_blit
unclipped_blit  PROC
	stmfd	sp, {r4 - fp, lr}
	ldr	r12, [sp, #4]
	ldr	lr, [sp]
	cmp	r12, #2
	beq	L340
	sub	r3, r3, r12
	sub	r1, r1, r12
	orrs	r4, r3, r1
	beq	L351
L341
	tst	r0, #2
	bne	L344
	tst	r2, #2
	bne	L343
L342
	subs	r12, r12, #32
	bcc	L346
L345
	ldmia	r2!, {r4 - fp}
	subs	r12, r12, #32
	stmia	r0!, {r4 - fp}
	bcs	L345
L346
	tst	r12, #16
	ldmneia	r2!, {r4 - r7}
	stmneia	r0!, {r4 - r7}
	tst	r12, #8
	ldmneia	r2!, {r4, r5}
	stmneia	r0!, {r4, r5}
	tst	r12, #4
	ldrne	r4, [r2], #4
	strne	r4, [r0], #4
	tst	r12, #2
	ldrneh	r4, [r2], #2
	strneh	r4, [r0], #2
	subs	lr, lr, #1
	ldrne	r12, [sp, #4]
	addne	r2, r2, r3
	addne	r0, r0, r1
	bne	L341
	ldmeqea	sp, {r4 - fp, pc}
L344
	ldrh	r4, [r2], #2
	sub	r12, r12, #2
	strh	r4, [r0], #2
	tst	r2, #2
	beq	L342
L343
	ldrh	r4, [r2], #2
	subs	r12, r12, #18
	bcc	L348
L347
	ldmia	r2!, {r6 - r9}
	subs	r12, r12, #16
	orr	r4, r4, r6, lsl #16
	mov	r5, r6, lsr #16
	orr	r5, r5, r7, lsl #16
	mov	r6, r7, lsr #16
	orr	r6, r6, r8, lsl #16
	mov	r7, r8, lsr #16
	orr	r7, r7, r9, lsl #16
	stmia	r0!, {r4 - r7}
	mov	r4, r9, lsr #16
	bcs	L347
L348
	tst	r12, #8
	beq	L349
	ldmia	r2!, {r6, r7}
	orr	r4, r4, r6, lsl #16
	mov	r5, r6, lsr #16
	orr	r5, r5, r7, lsl #16
	stmia	r0!, {r4, r5}
	mov	r4, r7, lsr #16
L349
	tst	r12, #4
	beq	L350
	ldr	r6, [r2], #4
	orr	r5, r4, r6, lsl #16
	str	r5, [r0], #4
	mov	r4, r6, lsr #16
L350
	strh	r4, [r0], #2
	tst	r12, #2
	ldrneh	r4, [r2], #2
	strneh	r4, [r0], #2
	subs	lr, lr, #1
	ldrne	r12, [sp, #4]
	addne	r2, r2, r3
	addne	r0, r0, r1
	bne	L341
	ldmeqea	sp, {r4 - fp, pc}
L340
	ldrh	r4, [r2], +r3
	subs	lr, lr, #1
	strh	r4, [r0], +r1
	bne	L340
	ldmeqea	sp, {r4 - fp, pc}
L351
	mul	r12, lr, r12
	mov	lr, #1
	subs	r12, r12, #0x200
	bcc	L353
L352
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	stmia	r0!, {r4 - fp}
	ldmia	r2!, {r4 - fp}
	subs	r12, r12, #0x200
	stmia	r0!, {r4 - fp}
	bcs	L352
L353
	adds	r12, r12, #0x200
	ldmeqea	sp, {r4 - fp, pc}
	bne	L341

        LTORG
unclipped_blit	ENDP

; void asm_draw_rgb(jint* src, int srcSpan, unsigned short* dst,
;    int dstSpan, int width, int height);
;
; src       - source RGB data pointer
; srcSpan   - source line span value, width+srcSpan is source scanline length
; dst       - dest pointer
; dstSpan   - dest line span value, width+dstSpan is dest scanline length
; width     - width to draw
; height    - height to draw

	EXPORT  asm_draw_rgb
asm_draw_rgb    PROC
	stmfd	sp, {r4 - fp, lr}
	ldmfd	sp, {r10, fp}
L355
	tst	r2, #2
	bne	L354
	subs	lr, r10, #4
	blt	L357
L356
	;  load rgb1, rgb2, rgb3, rgb4
	ldmia	r0!, {r4 - r7}
	subs	lr, lr, #4
	;  convert rgb1
	and	r12, r4, #248
	mov	r8, r12, lsr #3
	and	r12, r4, #0xfc00
	orr	r8, r8, r12, lsr #5
	and	r12, r4, #0xf80000
	orr	r8, r8, r12, lsr #8
	;  convert rgb2
	and	r12, r5, #248
	orr	r8, r8, r12, lsl #13
	and	r12, r5, #0xfc00
	orr	r8, r8, r12, lsl #11
	and	r12, r5, #0xf80000
	orr	r8, r8, r12, lsl #8
	;  convert rgb3
	and	r12, r6, #248
	mov	r9, r12, lsr #3
	and	r12, r6, #0xfc00
	orr	r9, r9, r12, lsr #5
	and	r12, r6, #0xf80000
	orr	r9, r9, r12, lsr #8
	;  convert rgb4
	and	r12, r7, #248
	orr	r9, r9, r12, lsl #13
	and	r12, r7, #0xfc00
	orr	r9, r9, r12, lsl #11
	and	r12, r7, #0xf80000
	orr	r9, r9, r12, lsl #8
	;  store four pixels
	stmia	r2!, {r8, r9}
	bge	L356
L357
	tst	lr, #2
	beq	L358
	ldmia	r0!, {r4, r5}
	;  convert rgb1
	and	r12, r4, #248
	mov	r8, r12, lsr #3
	and	r12, r4, #0xfc00
	orr	r8, r8, r12, lsr #5
	and	r12, r4, #0xf80000
	orr	r8, r8, r12, lsr #8
	;  convert rgb2
	and	r12, r5, #248
	orr	r8, r8, r12, lsl #13
	and	r12, r5, #0xfc00
	orr	r8, r8, r12, lsl #11
	and	r12, r5, #0xf80000
	orr	r8, r8, r12, lsl #8
	;  store two pixels
	str	r8, [r2], #4
L358
	tst	lr, #1
	beq	L359
	ldr	r4, [r0], #4
	and	r12, r4, #248
	mov	r8, r12, lsr #3
	and	r12, r4, #0xfc00
	orr	r8, r8, r12, lsr #5
	and	r12, r4, #0xf80000
	orr	r8, r8, r12, lsr #8
	strh	r8, [r2], #2
L359
	subs	fp, fp, #1
	add	r0, r0, r1, lsl #2
	add	r2, r2, r3, lsl #1
	bne	L355
	ldmeqea	sp, {r4 - fp, pc}
L354
	ldr	r4, [r0], #4
	subs	lr, r10, #5
	and	r12, r4, #248
	mov	r8, r12, lsr #3
	and	r12, r4, #0xfc00
	orr	r8, r8, r12, lsr #5
	and	r12, r4, #0xf80000
	orr	r8, r8, r12, lsr #8
	strh	r8, [r2], #2
	bge	L356
	blt	L357

        LTORG
asm_draw_rgb	ENDP

	END
