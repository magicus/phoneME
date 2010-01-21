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

#include <gxj_putpixel.h>

#include "gxj_intern_image.h"

/*
#pragma O0
*/
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * fast pixel set - perform fast consecutive setting of pixel on raster
 *                  32-bits at a time
 * this function is similar to memset, with the exception that the value is
 * 16bit wide instead of 8bit, and that number of pixels is counter is in 
 * 16bits, instead of bytes/
 *
 * mem:              address of raster to fill
 * value:            uint16 color value to fill 
 * number_of_pixels: number of pixels to fill (total bytes set is 
 *                   2*number_of_pixels)
 */

#if UNDER_ADS
void fast_pixel_set(void* mem, int value, int number_of_pixels) {
__asm {

    mov     r0,mem
    mov     r2,number_of_pixels
    add     r2, r0, r2, lsl #1
    add     r3, value, value, lsl #0x10

    and     r1, r0, #0x3
    cmp     r1,#0
    strneh  r3, [r0],#2
    subne   r2, r2, #1

    sub     r1, r2, #0x1f

    cmp     r0, r1
    bge     loop2


#if (__ARMCC_VERSION < 120848)
    stmfd sp, {r4-r11}
#endif

    mov r4,  r3
    mov r5,  r3
    mov r6,  r3
    mov r7,  r3
    mov r8,  r3
    mov r9,  r3
    mov r10, r3
    

loop:
    stmia r0!, {r3-r10}
    cmp r0, r1
    blt loop

#if (__ARMCC_VERSION < 120848)
    ldmfd sp, {r4-r11}
#endif 

 loop2:
    cmp    r0,r2;
    strlth r3, [r0],#2
    blt    loop2

    } 
}
#elif defined(__GNUC__) && defined(ARM)
// For more info on GNU/ARM inline-asm, see
// http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html
// http://lists.arm.linux.org.uk/pipermail/linux-arm/2005-July/010365.html
void fast_pixel_set(void* mem, int value, int number_of_pixels) {
asm volatile(
   "mov     r0, %0\n\t"   // %0 = mem
   "mov     r2, %2\n\t"   // %2 = number_of_pixels
   "add     r2, r0, r2, lsl #1\n\t"
   "add     r3, %1, %1, lsl #0x10\n\t" // %1 = value

   "and     r1, r0, #0x3\n\t"
   "cmp     r1, #0\n\t"
   "strneh  r3, [r0], #2\n\t"
   "subne   r2, r2, #1\n\t"

   "sub     r1, r2, #0x1f\n\t"
   "cmp     r0, r1\n\t"
   "bge     loop2\n\t"

   // GCC doesn't allow us to use too many registers, so we can't
   // use r7-r10 as in the ADS version.
   "mov     r4,  r3\n\t"
   "mov     r5,  r3\n\t"
   "mov     r6,  r3\n\t"

"\nloop:\n\t"
   "stmia   r0!, {r3-r6}\n\t"
   "stmia   r0!, {r3-r6}\n\t"
   "cmp     r0, r1\n\t"
   "blt     loop\n\t"

"\nloop2:\n\t"
   "cmp    r0, r2\n\t"
   "strlth r3, [r0], #2\n\t"
   "blt    loop2\n\t"
  : 
  : "r" (mem), "r" (value), "r" (number_of_pixels)
  : "r0", "r1", "r2", "r3", "r4", "r5", "r6"
  );
}
#endif

/**
 * unclippedBlit - low level simple blit of 16bit pixels from src to dst
 * srcRaster - short* aligned pointer into source of pixels
 * dstRaster - short* aligned pointer into destination
 * srcSpan   - number of bytes per scanline of srcRaster (must be even)
 * dstSpan   - number of bytes per scanline of dstRaster (must be even)
 * width     - number of bytes to copy per scanline (must be even)
 * height    - number of scanlines to copy
 * Note: There is a special case for blitting a 16x16 image to an aligned dst
 */


#if UNDER_ADS
#define ASM_BLIT         1 /* use ARM assembler blit */
#define ASM_BLIT16x16    1 /* special case 16x16 blit */
#define ASM_LOOPOPTIMIZE 1 /* collapse multiple scanlines to 1 */
#endif 

#if (!UNDER_CE)
void unclipped_blit(unsigned short *dstRaster, int dstSpan,
		    unsigned short *srcRaster, int srcSpan,
		    int height, int width, gxj_screen_buffer *dst) {
    (void)dst;

#if ASM_BLIT
    __asm {
        mov    r0, dstRaster;
        mov    r1, srcRaster;
        mov    r2, width; 

#if ASM_BLIT16x16
        tst    r1, #2;
        bne    height_test;
        tst    r0, #2;
        bne    height_test;
        mov    r4, dstSpan;
        tst    r4, #2;
        bne    height_test;
        cmp    r2, #32;
        bne    height_test;
        cmp    r2, srcSpan; 
        bne    height_test;
        cmp    height, #16;
        bne    copy_16xHEIGHT;

    /* copy 16x16 pixels */
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11};
        b      done;

    copy_16xHEIGHT:
        cmp    height, #0;
        ble    done;

    copy_16xHEIGHT_loop:
        ldmia  r1!,{r4-r11}; stmia  r0,{r4-r11}; add    r0, r0, dstSpan;
        subs   height, height, #1;
        bgt    copy_16xHEIGHT_loop;
        b      done;
#endif

    height_test:
        cmp    height, #0;
        ble    done;
//    height_test_nonzero:
#if ASM_LOOPOPTIMIZE
        cmp    dstSpan, srcSpan  /* dstSpan == srcSpan == width */
        bne    no_optimize;
        cmp    srcSpan, width;
        bne    no_optimize;
        mul    width, height, width;
        mov    height, #1;
    no_optimize:
#endif

    height_loop:
        mov    r2, width;
        cmp    r2, #32;
        blt    copy_small;

//    alignment_test:
        tst    r1, #2;
        bne    unaligned_to_aligned; /* srcRaster is unaligned,
                                        dstRaster is aligned? */
        tst    r0, #2;
        bne    aligned_to_unaligned; /* srcRaster is aligned,
                                        dstRaster is aligned? */

    aligned_to_aligned_loop:         /* copy 16 pixels per iteration */
        ldmia  r1!,{r4-r11};
        stmia  r0!,{r4-r11};
        subs   r2, r2, #32;
        beq    span_done;
        cmp    r2, #32;
        blt    copy_small;

        ldmia  r1!,{r4-r11};         /* unroll loop */
        stmia  r0!,{r4-r11};
        subs   r2, r2, #32;
        beq    span_done;
        cmp    r2, #32;
        bge    aligned_to_aligned_loop;

    copy_small:
        cmp    r2, #0;
        ble    span_done;

    copy_small_loop:                 /* copy 1 pixel per iteration */
        ldrh   r4, [r1], #2;
        strh   r4, [r0], #2;
        subs   r2, r2, #2;
        ldrgth r4, [r1], #2;         /* unroll loop */
        strgth r4, [r0], #2;
        subgts r2, r2, #2;
        bgt    copy_small_loop;

    span_done:                       /* advance to next scanline */
        subs   height, height, #1;
        ble    done;
        sub    r0, r0, width;
        add    r0, r0, dstSpan;
        sub    r1, r1, width;
        add    r1, r1, srcSpan;
        b      height_loop;

    unaligned_to_unaligned:
        ldrh   r4, [r1], #2;         /* read 1 pixel */
        strh   r4, [r0], #2;         /* store 1 pixel */
        subs   r2, r2, #2;
        cmp    r2, #32
        bge    aligned_to_aligned_loop;
        b      copy_small;

    unaligned_to_aligned:
        tst    r0, #2;
        bne    unaligned_to_unaligned;

    unaligned_to_aligned_loop:
        /* (first half of 16 pixels) */
        ldrh   r4, [r1], #2;         /* read 1 pixel */
        ldmia  r1!,{r5,r6,r7,r8};    /* read 8 pixels */
        orr    r4, r4, r5, lsl #16;
        mov    r5, r5, lsr #16;
        orr    r5, r5, r6, lsl #16;
        mov    r6, r6, lsr #16;
        orr    r6, r6, r7, lsl #16;
        mov    r7, r7, lsr #16;
        orr    r7, r7, r8, lsl #16;
        stmia  r0!,{r4,r5,r6,r7};

        /* (second half of 16 pixels) */
        ldmia  r1!,{r9,r10,r11};     /* read 6 pixels */
        ldrh   r4, [r1], #2;         /* read 1 pixel  */
        mov    r8, r8, lsr #16;
        orr    r8, r8, r9, lsl #16;
        mov    r9, r9, lsr #16;
        orr    r9, r9, r10, lsl #16;
        mov    r10, r10, lsr #16;
        orr    r10, r10, r11, lsl #16;
        mov    r11, r11, lsr #16;
        orr    r11, r11, r4, lsl #16;
        stmia  r0!,{r8,r9,r10,r11};
        subs   r2, r2, #32;
        beq    span_done;
        cmp    r2, #32;
        bge    unaligned_to_aligned_loop;
        b      copy_small;

    aligned_to_unaligned:
        /* srcRaster is aligned, dstRaster is unaligned */

    aligned_to_unaligned_loop:
        /* (first half of 16 pixels) */
        ldmia  r1!,{r4,r5,r6,r7};    /* read 8 pixels */
        strh   r4, [r0], #2;         /* 1 pixel */
        mov    r4, r4, lsr #16;   
        orr    r4, r4, r5, lsl #16;
        mov    r5, r5, lsr 16;
        orr    r5, r5, r6, lsl #16;
        mov    r6, r6, lsr #16;
        orr    r6, r6, r7, lsl #16;
        stmia  r0!,{r4,r5,r6};       /* 1 + 2+2+2 = 7 pixels */

        /* (second half of 16 pixels) */
        ldmia  r1!,{r8,r9,r10,r11};  /* read 8 pixels */
        mov    r7, r7, lsr #16; 
        orr    r7, r7, r8, lsl #16;
        mov    r8, r8, lsr #16;
        orr    r8, r8, r9, lsl #16;
        mov    r9, r9, lsr #16;
        orr    r9, r9, r10, lsl #16;
        mov    r10, r10, lsr #16;
        orr    r10, r10, r11, lsl #16;
        stmia  r0!,{r7,r8,r9,r10};   /* 7 + 2+2+2+2 = 15 pixels */
        mov    r11, r11, lsr #16;
        strh   r11, [r0], #2;        /* the last pixel */
        subs   r2, r2, #32;
        beq    span_done;
        cmp    r2, #32;
        bge    aligned_to_unaligned_loop;
        b      copy_small;

    done:
    }
#else
  dstSpan >>= 1; srcSpan >>= 1;
  if (((unsigned int)dstRaster | (unsigned int)srcRaster | 
       dstSpan<<1 | srcSpan<<1) & 0x2) {
    for ( ; height > 0; height--) {
      CHECK_PTR_CLIP(dst, dstRaster); CHECK_PTR_CLIP(dst, dstRaster+(width>>1)-1);
      if (((unsigned int)dstRaster | (unsigned int)srcRaster) & 0x2) {
        memcpy(dstRaster, srcRaster, width);
      } else {
#ifdef USE_RT_MEMCPY_W
        __rt_memcpy_w(dstRaster, srcRaster, width);
#else
          memcpy(dstRaster, srcRaster, width);
#endif
      }
      dstRaster += dstSpan;
      srcRaster += srcSpan;
    }
  } else {
    if (width == 32) {
      int *dstPtr = (int*) dstRaster;
      int *srcPtr = (int*) srcRaster;
      dstSpan >>= 1; srcSpan >>= 1;
      if (height == 16) {
        CHECK_PTR_CLIP(dst, dstPtr); CHECK_PTR_CLIP(dst, dstPtr+(width>>2)-1);
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        CHECK_PTR_CLIP(dst, dstPtr); CHECK_PTR_CLIP(dst, dstPtr+(width>>2)-1);
        memcpy(dstPtr, srcPtr, 32); // dstPtr += dstSpan; srcPtr += srcSpan;
      } else {
        for ( ; height > 0; height -= 1) {
          CHECK_PTR_CLIP(dst, dstPtr); CHECK_PTR_CLIP(dst, dstPtr+(width>>2)-1);
          memcpy(dstPtr, srcPtr, 32); dstPtr += dstSpan; srcPtr += srcSpan;
        }
      }
    } else {
      /* single memcpy optimization */
      if (((width>>1) == srcSpan) && (srcSpan == dstSpan)) {
        width = (dstSpan<<1) * height;
        height = 1;
      }
      for ( ; height > 0; height -= 1) {
        CHECK_PTR_CLIP(dst, dstRaster); CHECK_PTR_CLIP(dst, dstRaster+(width>>1)-1);
#ifdef USE_RT_MEMCPY_W
        __rt_memcpy_w(dstRaster, srcRaster, width);
#else
        memcpy(dstRaster, srcRaster, width);
#endif
        dstRaster += dstSpan;
        srcRaster += srcSpan;
      }
    }
  }
#endif
}

#endif // !UNDER_CE

#ifdef __cplusplus
}
#endif
