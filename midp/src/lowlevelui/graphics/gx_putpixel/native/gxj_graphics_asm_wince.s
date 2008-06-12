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
; r0:  dstRaster - short* aligned pointer into destination
; r1:  dstSpan   - number of bytes per scanline of dstRaster (must be even)
; r2:  srcRaster - short* aligned pointer into source of pixels
; r3:  srcSpan   - number of bytes per scanline of srcRaster (must be even)
; r12: width     - number of bytes to copy per scanline (must be even)
; lr:  height    - number of scanlines to copy
;
; r4-r11: spare registers used in data transfer

	EXPORT  unclipped_blit
unclipped_blit  PROC
	stmfd	sp, {r4 - r11, lr}
	ldr	r12, [sp, #4]      ; load width from stack
	ldr	lr, [sp]           ; load height from stack

	; check for a special case: width == 2 (one column to copy)
	cmp	r12, #2
	beq	one_column
	; check for another special case: dstSpan == srcSpan == width
	sub	r3, r3, r12
	sub	r1, r1, r12
	orrs	r4, r3, r1
	beq	through_blit

start_row
        ; is dstRaster word-aligned?
	tst	r0, #2
	bne	unaligned_dst
	; is srcRaster word-aligned?
	tst	r2, #2
	bne	unaligned_to_aligned

aligned_to_aligned
	subs	r12, r12, #32
	blo	blit_small

blit_32
        ; 32-byte copying loop
	ldmia	r2!, {r4 - r11}
	subs	r12, r12, #32
	stmia	r0!, {r4 - r11}
	bhs	blit_32

blit_small
        ; a single pass to copy up to 30 remaining bytes
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

	; decrease row count and repeat the above procedure
	subs	lr, lr, #1
	ldrne	r12, [sp, #4]
	addne	r2, r2, r3
	addne	r0, r0, r1
	bne	start_row
	ldmeqea	sp, {r4 - r11, pc}

unaligned_dst
        ; copy a single pixel and thus make dstRaster word-aligned
	ldrh	r4, [r2], #2
	sub	r12, r12, #2
	strh	r4, [r0], #2
	tst	r2, #2
	beq	aligned_to_aligned

unaligned_to_aligned
	ldrh	r4, [r2], #2
	subs	r12, r12, #18
	blo	blit_8

blit_16
        ; the loop copies unaligned 16-byte blocks
        ; the half-words need to be recombined
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
	bhs	blit_16

        ; a single pass to copy unaligned blocks of 8, 4 and 2 bytes
blit_8
	tst	r12, #8
	beq	blit_4
	ldmia	r2!, {r6, r7}
	orr	r4, r4, r6, lsl #16
	mov	r5, r6, lsr #16
	orr	r5, r5, r7, lsl #16
	stmia	r0!, {r4, r5}
	mov	r4, r7, lsr #16
blit_4
	tst	r12, #4
	beq	blit_2
	ldr	r6, [r2], #4
	orr	r5, r4, r6, lsl #16
	str	r5, [r0], #4
	mov	r4, r6, lsr #16
blit_2
	strh	r4, [r0], #2
	tst	r12, #2
	ldrneh	r4, [r2], #2
	strneh	r4, [r0], #2

	; go the next row
	subs	lr, lr, #1
	ldrne	r12, [sp, #4]
	addne	r2, r2, r3
	addne	r0, r0, r1
	bne	start_row
	ldmeqea	sp, {r4 - r11, pc}

one_column
	ldrh	r4, [r2], +r3
	subs	lr, lr, #1
	strh	r4, [r0], +r1
	bne	one_column
	ldmeqea	sp, {r4 - r11, pc}

through_blit
        ; there is no interval between rows -
        ; we may consider that width = width * height and height = 1
	mul	r12, lr, r12
	mov	lr, #1

	; usualy we come here when blitting the whole screen,
	; that's why we prefer to count by the large blocks of 512 bytes
	subs	r12, r12, #512
	blo	through_blit_small

through_blit_512
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	stmia	r0!, {r4 - r11}
	ldmia	r2!, {r4 - r11}
	subs	r12, r12, #512
	stmia	r0!, {r4 - r11}
	bhs	through_blit_512

through_blit_small
        ; smaller blocks can be easily handled by the general routine
	adds	r12, r12, #512
	ldmeqea	sp, {r4 - r11, pc}
	bne	start_row

        LTORG
unclipped_blit	ENDP

; void asm_draw_rgb(jint* src, int srcSpan, unsigned short* dst,
;    int dstSpan, int width, int height);
;
; r0:  src     - source RGB data pointer
; r1:  srcSpan - source line span value, width+srcSpan is source scanline length
; r2:  dst     - dest pointer
; r3:  dstSpan - dest line span value, width+dstSpan is dest scanline length
; r10: width   - width to draw
; r11: height  - height to draw

	EXPORT  asm_draw_rgb
asm_draw_rgb    PROC
	stmfd	sp, {r4 - r11, lr}
	ldmfd	sp, {r10, r11}

row_loop
        ;  is dst word-aligned?
	tst	r2, #2
	bne	unaligned_row

	;  width < 4 ?
	subs	lr, r10, #4
	blt	pixels2

	;  this loop converts four 32-bit values
	;  to four 16-bit pixels and puts them to word-aligned dst
pixels4_loop
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
	bge	pixels4_loop

pixels2
        ;  here we convert and put two pixels to word-aligned dst
	tst	lr, #2
	beq	pixel1
	;  load two pixels
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

pixel1
        ;  the final pixel in row (if any)
	tst	lr, #1
	beq	row_done
	ldr	r4, [r0], #4
	and	r12, r4, #248
	mov	r8, r12, lsr #3
	and	r12, r4, #0xfc00
	orr	r8, r8, r12, lsr #5
	and	r12, r4, #0xf80000
	orr	r8, r8, r12, lsr #8
	strh	r8, [r2], #2

row_done
        ;  decrease row count, update src & dst and repeat from the beginning
	subs	r11, r11, #1
	add	r0, r0, r1, lsl #2
	add	r2, r2, r3, lsl #1
	bne	row_loop
	ldmeqea	sp, {r4 - r11, pc}

unaligned_row
        ;  convert and put the first pixel thus making dst word-aligned
	ldr	r4, [r0], #4
	subs	lr, r10, #5
	and	r12, r4, #248
	mov	r8, r12, lsr #3
	and	r12, r4, #0xfc00
	orr	r8, r8, r12, lsr #5
	and	r12, r4, #0xf80000
	orr	r8, r8, r12, lsr #8
	strh	r8, [r2], #2
	bge	pixels4_loop
	blt	pixels2

        LTORG
asm_draw_rgb	ENDP

	END
