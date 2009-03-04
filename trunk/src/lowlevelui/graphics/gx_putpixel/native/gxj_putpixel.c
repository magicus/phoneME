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

#include <kni.h>
#include <midpMalloc.h>
#include <midp_logging.h>

#include "gxj_intern_putpixel.h"
#include "gxj_intern_graphics.h"

typedef struct _registers_4 {
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
} registers_4;

typedef struct _dotted_draw_state { /* the draw state */
    int solidcount; /* how many dots are drawn in the solid fragment */
    int emptycount; /* how many dots are skipped in the empty fragment */
    int drawing;    /* are we drawing (1) or skipping (0) now? */
} dotted_draw_state;

static const dotted_draw_state HALF_STROKE = { DOTTED_SOLID_SIZE/2, 0, TRUE };
static const dotted_draw_state START_STROKE = { 0, 0, TRUE };

int aTangents[91] = {
  /*        _0    _1    _2    _3    _4    _5     _6     _7     _8     _9  */
  /* 0_ */  0,    17,   35,   52,   70,   87,    105,   123,   141,   158,
  /* 1_ */  176,  194,  213,  231,  249,  268,   287,   306,   325,   344,
  /* 2_ */  364,  384,  404,  424,  445,  466,   488,   510,   532,   554,
  /* 3_ */  577,  601,  625,  649,  675,  700,   727,   754,   781,   810,
  /* 4_ */  839,  869,  900,  933,  966,  1000,  1036,  1072,  1111,  1150,
  /* 5_ */  1192, 1235, 1280, 1327, 1376, 1428,  1483,  1540,  1600,  1664,
  /* 6_ */  1732, 1804, 1881, 1963, 2050, 2145,  2246,  2356,  2475,  2605,
  /* 7_ */  2747, 2904, 3078, 3271, 3487, 3732,  4011,  4331,  4705,  5145,
  /* 8_ */  5671, 6314, 7115, 8144, 9514, 11430, 14301, 19081, 28636, 57290,
  /* 9_ */  MAXINT32
  };

/* degrees must be between 0 and 360
 * returns abs(int(tan(degrees) * 1000)) */
static int
absIntTan1000(int degrees)
{
    if (degrees > 180) {
      degrees = degrees - 180;
    }
    if (degrees > 90) {
      degrees = 180 - degrees;
    }
    return aTangents[degrees];
}

/**
  Overview of the SetPixel based implementation of the lcdui graphics functions

  This implementation is meant to be usable as the basis for implementing lcdui graphics
  on any platform which offers SetPixel(color, x, y) or better graphics function.
  It is implemented so that with minimal finessing, it can be optimized for speed 
  for any level of graphics support.
  This is achieved by providing a number of routines which are prefixed with prim
  (for primitive). These routines are written in terms of SetPixel but are often
  available directly on systems with graphics support. They do no clipping.
  These routines (or the calls to them) can be replaced with direct calls to the
  underlying system routines where available.
  
  THe code is currently also written so that the lcdui functions themselves are
  kept short, handling only the peculiarities of the midp spec and other system
  dependent things such as setting up device contexts, redrawing the screen, 
  and color.
  Also, where it's simple, we check to see if there is no 
  clipping and then immediately call primitive routines. The clipped case (and
  the case of partial arcs) are handled in separate routines.

  The code currently has been somewhat biased in favor of readability and debuggability -
  there is no use of macros, and there is fairly heavy breakdown into meaningful 
  functions. After profiling we will have a better idea of exactly where we need to
  make things "dirtier", though some seem obvious immediately (such as putting
  drawSymmetricPixel inside drawClippedOutlineArc).

  Note that lines which are not vertical/horizontal and all arcs cannot be implemented as
  primitive functions because of the demand of the midp spec that partial lines 
  draw the same pixels as the complete line, and the fact that they use the Midpoint
  algorithm for calculating their points - that means you always have to start from
  the beginning point even when you know most of the line is clipped, because you
  can't predict which point will be drawn at any point along the line without calculating
  from the beginning. This means that all of the relevant code would have to be replaced
  as an atomic whole by code which provided *all* of the relevant functionality.

  There are currently some known bugs in the code in certain cases arcs, and filled partial
  arcs in particular.
 */

/**
 * draw pattern pixel at pixelData[y*width+x]
 * partial clipping to sbuf[width,height] and (y*width+x < 0)
 * pattern is (0->SOLID) then (SOLID->(SOLID+EMPTY)) then repeat
 */
static
void
drawDottedPixel(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x, int y, dotted_draw_state * dds) {

  if (dds->solidcount >= DOTTED_SOLID_SIZE) {
    dds->drawing = FALSE;
    dds->solidcount = 0;
  } else { 
    if (dds->emptycount >= DOTTED_EMPTY_SIZE) {
      dds->drawing = TRUE;
      dds->emptycount = 0;
    }
  }

  if (dds->drawing) {
    PRIMDRAWPIXEL(sbuf, color, x, y);
    ++(dds->solidcount);
  } else {
    ++(dds->emptycount);
  }
}

#define SWAP(_x,_y)	do {_x ^= _y; _y ^= _x; _x ^= _y;} while (0)

/* all line drawing routines, including prim routines,
 * draw both endpoints.  if replacing these routines by
 * system routines which don't draw the second endpoint,
 * remember to add one to x2 and y2 */

/**
 * draw pixels from <x1,y1) through (x1,y2)
 * y1 should be <= y2, x1 == x2
 */
static void
primDrawVertLine(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x1, int y1, int x2, int y2) {

  int width = sbuf->width;
  int height = sbuf->height;
  int count;
  gxj_pixel_type* pPtr;

    (void)x2;
#if PRIM_CLIPPING
  if ((x1 < 0) || (x1 >= width))
    return;
  y1 = (y1 < 0) ? 0 : ((y1 >= height) ? height-1 : y1);
  y2 = (y2 < 0) ? 0 : ((y2 >= height) ? height-1 : y2);
#endif
  if ((count=y2-y1) < 0) {
    y1 = y2;
    count = -count;
  }
		
  pPtr = &(sbuf->pixelData[y1 * width + x1]);
  while (count & ~0x7) {
    CHECK_PTR_CLIP(sbuf,pPtr);
    *pPtr = color; pPtr += width; *pPtr = color; pPtr += width;
    *pPtr = color; pPtr += width; *pPtr = color; pPtr += width;
    *pPtr = color; pPtr += width; *pPtr = color; pPtr += width;
    *pPtr = color; pPtr += width;
    *pPtr = color; CHECK_PTR_CLIP(sbuf,pPtr); pPtr += width;
    count -= 8;
  }
  while (count >= 0) {
    CHECK_PTR_CLIP(sbuf,pPtr);
    *pPtr = color; pPtr += width;
    count -= 1;
  }
}

/**
 * draw pixels from <x1,y1) through (x2,y1)
 * x1 should be <= x2, y1 == y2
 */
static void
primDrawHorzLine(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x1, int y1, int x2, int y2) {

#if 0  
  int width = sbuf->width;
  int height = sbuf->height;
  int count;
  gxj_pixel_type* pPtr;

#if PRIM_CLIPPING
  if ((y1 < 0) || (y1 >= height))
    return;
  x1 = (x1 < 0) ? 0 : ((x1 >= width) ? width-1 : x1);
  x2 = (x2 < 0) ? 0 : ((x2 >= width) ? width-1 : x2);
#endif
  if ((count=x2-x1) < 0) {
    x1 = x2;
    count = -count;
  }
		
  pPtr = &(sbuf->pixelData[y1 * width + x1]);
  while (count & ~0x7) {
    CHECK_PTR_CLIP(sbuf,pPtr);
    *pPtr = color; pPtr += 1; *pPtr = color; pPtr += 1;
    *pPtr = color; pPtr += 1; *pPtr = color; pPtr += 1;
    *pPtr = color; pPtr += 1; *pPtr = color; pPtr += 1;
    *pPtr = color; pPtr += 1;
    *pPtr = color; CHECK_PTR_CLIP(sbuf,pPtr); pPtr += 1;
    count -= 8;
  }
  while (count >= 0) {
    CHECK_PTR_CLIP(sbuf,pPtr);
    *pPtr = color; pPtr += 1;
    count -= 1;
  }
#else
  int width = sbuf->width;
  int height = sbuf->height;
  int count;
  gxj_pixel_type* pPtr;
  unsigned int c = ((unsigned int)color) << 16 | ((unsigned int)color);
  jlong lcol = ((jlong) c) << 32 | ((jlong) c);
  registers_4 regs;
  regs.r0 = regs.r1 = regs.r2 = regs.r3 = c;

    (void)y2;

#if PRIM_CLIPPING
  if ((y1 < 0) || (y1 >= height))
    return;
  x1 = (x1 < 0) ? 0 : ((x1 >= width) ? width-1 : x1);
  x2 = (x2 < 0) ? 0 : ((x2 >= width) ? width-1 : x2);
#endif
  y1 *= width;
  if ((count=x2-x1) < 0) {
    x1 = x2;
    count = -count;
  }
  pPtr = &(sbuf->pixelData[y1 + x1]);
  if (((unsigned int)pPtr & 0x3) && (count > 0)) {
    CHECK_PTR_CLIP(sbuf,pPtr);
    *pPtr++ = color;
    --count;
  }

  while (count >= 16) {
    CHECK_LLPTR_CLIP(sbuf,pPtr);
    *((registers_4 *)pPtr) = regs;
    pPtr += 8;
    CHECK_LLPTR_CLIP(sbuf,pPtr);
    *((registers_4 *)pPtr) = regs;
    pPtr += 8;
    count -= 16;
  }
  if (count >= 8) {
    CHECK_LLPTR_CLIP(sbuf,pPtr);
    *((registers_4 *)pPtr) = regs;
    pPtr += 8;
    count -= 8;
  }
  if (count >= 4) {
    CHECK_LLPTR_CLIP(sbuf,pPtr);
    *((jlong *)pPtr) = lcol;
    pPtr += 4;
    count -= 4;
  }
  while (count >= 0) {
    CHECK_PTR_CLIP(sbuf,pPtr);
    *pPtr = color; pPtr += 1;
    count -= 1;
  }
#endif
}




/**
 * draw dotted line pixels from (x1,y1) through (x1,y2)
 * y1 should be <= y2, x1 == x2 starting from specified
 * stroke state, and returning final stroke state
 */
static void
drawDottedVertLine(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x1, int y1, int x2, int y2, dotted_draw_state dds) {

  int y;
  /* Surpress unused parameter warnings */
  (void)x2;
  
#if PRIM_CLIPPING
  if ((x1 < 0) || (x1 >= sbuf->width))
    return;
  y1 = (y1 < 0) ? 0 : ((y1 >= sbuf->height) ? sbuf->height-1 : y1);
  y2 = (y2 < 0) ? 0 : ((y2 >= sbuf->height) ? sbuf->height-1 : y2);
#endif
  for (y = y1; y <= y2; ++y) {
    drawDottedPixel(sbuf, color, x1, y, &dds);
  }
}

/**
 * draw dotted line pixels from (x1,y1) through (x2,y1)
 * x1 should be <= x2, y1 == y2 starting from specified
 * stroke state, and returning final stroke state
 */
static void
drawDottedHorzLine(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x1, int y1, int x2, int y2, dotted_draw_state dds) {

  int x;
  /* Surpress unused parameter warnings */
  (void)y2;

#if PRIM_CLIPPING
  if ((y1 < 0) || (y1 >= sbuf->height))
    return;
  x1 = (x1 < 0) ? 0 : ((x1 >= sbuf->width) ? sbuf->width-1 : x1);
  x2 = (x2 < 0) ? 0 : ((x2 >= sbuf->width) ? sbuf->width-1 : x2);
#endif
  for (x = x1; x <= x2; ++x) {
    drawDottedPixel(sbuf, color, x, y1, &dds);
  }
}

/**
 * draw pixels from (x1,y1) through (x1,y2)
 * y1 should be <= y2, x1 == x2 starting from solid stroke
 */
static void
primDrawDottedVertLine(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x1, int y1, int x2, int y2) {

  drawDottedVertLine(sbuf, color, x1, y1, x2, y2, START_STROKE);
}

/**
 * draw dotted line pixels from (x1,y1) through (x2,y1)
 * x1 should be <= x2, y1 == y2 starting from solid stroke
 */
static void
primDrawDottedHorzLine(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x1, int y1, int x2, int y2) {

  drawDottedHorzLine(sbuf, color, x1, y1, x2, y2, START_STROKE);
}

#define REFLECT    16 /* ymajor octants (2/3/6/7) mapped to xmajor (1/4/5/8) */
#define CLIPPED     8 /* some leading pixels clipped (x1,y1 changed) */
#define POINT       4 /* delta x and y are zero */
#define HORIZONTAL  2 /* delta x or y is zero (see REFLECT) */
#define INSIDE      1 /* some pixels inside clip */
#define OUTSIDE     0 /* all pixles are outside clip */

/**
 * Perform clip testing and clipping of the line segment x1,y1 to x2,y2.
 * If all the pixels of the line segment are beyond the bounds of the
 * clip this function returns OUTSIDE. When SetUpClippedLineParams
 * returns INSIDE it has computed and updated the starting pixel x1,y1,
 * computed a new mayor axis ending value xyEnd, computed a Bresenham
 * decision variable, decision variable increments incX,incY, octant
 * specifier (REFLECT) and quadrant specifiers adjIncX,adjIncY.  This
 * function also looks for empty clip bounds and returns OUTSIDE.
 *
 * If the line is both horizontal (no deltax) and vertical (no deltay)
 * the return value is (POINT) with x1,y1 left untouched.
 * If the line is horizontal (deltay is zero) the return value is (HORIZONTAL)
 * with the x1,y1 and xyEnd clipped.  If the segment is vertical (deltax is
 * zero) the value (HORIZONTAL|REFLECT) is returned with x1,y1 swapped
 * then clipped (CLIPPED) and xyEnd clipped.  Both horizontal and vertical lines
 * have the correct Bresenham decision, increments and x,y adjustment
 * values computed.
 *
 * For all other angle lines the correct quadrant is computed (adjIncX,adjIncY),
 * the exact pixel locations x1,y1 is computed if the leading pixels are clipped
 * (CLIPPED), the correct major axis ending pixel (+1) is computed xyEnd,
 * the exact initial Bresenham decision variable and increments (incX,incY) are
 * computed for clipping and finally an octant flag (REFLECT) is returned if
 * the segment is on a major Y axis.
 *
 * NOTE: Some values (adjIncX,adjIncY) may be changed before OUTSIDE is returned.
 * IMPL_NOTE: Does MIDP require reversibility, a line (x1,y1)<->(x2,y2) produce
 *       the same set of pixels in both directions?
 */
static int
SetUpClippedLineParams(int *x1, int *y1, int x2, int y2,
    int *xyEnd, int *decision, int *incrY, int *incrX,
    int *adjIncX, int *adjIncY, const jshort *clip) {

  int lineX, lineY, lineEnd, deltaX, deltaY, deltaXtimes2, deltaYtimes2;
  int lineX1 = *x1; int lineX2 = x2;
  int lineY1 = *y1; int lineY2 = y2;
  int clipX1 = clip[0]; int clipX2 = clip[2]-1;
  int clipY1 = clip[1]; int clipY2 = clip[3]-1;
  int ret = INSIDE;
  int tmp, remainder, error;

  /* empty clip test */
  if ((clipX1 > clipX2) || (clipY1 > clipY2))
    return OUTSIDE;
  /* start clip testing X axis */
  if (lineX1 < lineX2) {
    if ((lineX1 > clipX2) || (lineX2 < clipX1))
      return OUTSIDE;
    /* line is in octant 1,2,7 or 8 */
    *adjIncX = 1;
  } else {
    if ((lineX2 > clipX2) || (lineX1 < clipX1))
      return OUTSIDE;
    /* line is in octant 3,4,5, or 6 */
    *adjIncX = -1;
    lineX1 = -lineX1; lineX2 = -lineX2;
    clipX1 = -clipX1; clipX2 = -clipX2;
    SWAP(clipX1, clipX2);
  }
  /* start clip testing Y axis */
  if (lineY1 < lineY2) {
    if ((lineY1 > clipY2) || (lineY2 < clipY1))
      return OUTSIDE;
    /* line is in octant 1 or 2 */
    *adjIncY = 1;
  } else {
    if ((lineY2 > clipY2) || (lineY1 < clipY1))
      return OUTSIDE;
    /* line is in octant 7 or 8 */
    *adjIncY = -1;
    lineY1 = -lineY1; lineY2 = -lineY2;
    clipY1 = -clipY1; clipY2 = -clipY2;
    SWAP(clipY1, clipY2);
  }
  /* line has been rotated into octant 1 or 2
   * now calculate the bresenham terms */
  deltaX = lineX2 - lineX1;
  deltaY = lineY2 - lineY1;
  if (deltaX < deltaY) {
    /* line is in octant 2, reflect into octant 1 */
    ret |= REFLECT;
    SWAP(lineX1,lineY1); SWAP(lineX2,lineY2); SWAP(deltaX,deltaY);
    SWAP(clipX1,clipY1); SWAP(clipX2,clipY2); SWAP(*adjIncX,*adjIncY);
  }
  if (deltaX == 0) {
    /* point at x1, y1 */
    return POINT;
  }
  if (deltaY == 0) {
    ret |= HORIZONTAL;
    lineX = (lineX1 < clipX1) ? clipX1 : lineX1;
    lineY = lineY1;
    lineEnd = lineX2;
    deltaXtimes2 = 0; deltaYtimes2 = 0;
    error = -1;
    goto SimpleClipTail;
  }
  /* IMPL_NOTE: Adjust error by bias (per octant 0/-1)
   * to produce symetric line patterns */
  deltaXtimes2 = 2*deltaX; deltaYtimes2 = 2*deltaY;
  lineX = lineX1; lineY = lineY1;
  error = deltaYtimes2 - deltaX;
  lineEnd = lineX2;
  /* determine where line enters clip */
  if (lineY1 < clipY1) {
    ret |= CLIPPED;
    tmp = deltaXtimes2*(clipY1-lineY1)-deltaX;
    lineX += tmp/deltaYtimes2;
    remainder = tmp%deltaYtimes2;
    if (lineX > clipX2)
      return OUTSIDE;
    if (lineX/*+1*/ >= clipX1) {
      lineY = clipY1;
      error -= remainder + deltaX;
      if (remainder > 0) {
        lineX += 1;
        error += deltaYtimes2;
      }
      goto ClipTail;
    }
  }
  if (lineX1 < clipX1) {
    ret |= CLIPPED;
    tmp = deltaYtimes2*(clipX1-lineX1);
    lineY += tmp/deltaXtimes2;
    remainder = tmp%deltaXtimes2;
    if ((lineY > clipY2) || (lineY == clipY2 && remainder >= deltaX))
      return OUTSIDE;
    lineX = clipX1;
    error += remainder;
    if (remainder >= deltaX) {
      lineY += 1;
      error -= deltaXtimes2;
    }
  }
ClipTail:
  /* clip the tail of the line */
  if (lineY2 > clipY2) {
    tmp = deltaXtimes2*(clipY2-lineY1)+deltaX;
    lineEnd = lineX1+tmp/deltaYtimes2;
    remainder = tmp%deltaYtimes2;
    if (remainder == 0) /* direct hit on pixel */
      lineEnd -= 1;
  }
  deltaXtimes2 -= deltaYtimes2;
SimpleClipTail:
  if (lineEnd > clipX2)
    lineEnd = clipX2;
  lineEnd += 1;
  /* reflect line coordinates back into original quadrant */
  if (*adjIncY < 0)
    lineY = -lineY;
  if (*adjIncX < 0) {
    lineX = -lineX;
    lineEnd = -lineEnd;
  }
  *x1 = lineX; *y1 = lineY; *xyEnd = lineEnd;
  *incrY = deltaXtimes2; *incrX = deltaYtimes2;
  *decision = error;
  return ret;
}

/**
 * Generates pixels for line segments that match the MIDP
 * specification.  This function generates the set of pixels between
 * (x1,y1) -> (x2,y2) inclusive, in the order specified.  The
 * function uses the Bresenham midpoint algorithm.  The set of
 * pixels generated is within the bounds (CLIP_X1,CLIP_Y1) ->
 * (CLIP_X2-1,CLIP_Y2-1).  For DOTTED lines the set of generated
 * pixels is a subset of the pixels generated for SOLID.  The
 * affect of clipping a DOTTED line is to generate the set of pixels
 * unclipped and then to remove the pixels beyond the clip bounds.
 * The maximum and minumum values of pixels and clip bounds are
 * constrained to avoid an overflow during line segment to clip
 * bounds calculations.
 *
 * @param sbuf Virtual Device Context passed to primDrawPixel
 * @param color gxj_pixel_type value for generated pixels
 * @param lineStyle SOLID to generate all pixels or DOTTED for a subset
 * @param x1,y1 coordinate of first pixel generated must be
 *              between (-32768 to 32767) inclusive
 * @param x2,y2 coordinate of the last pixel generated must be
 *              between (-32768 to 32767) inclusive
 * @param clip ->clipX1,clipY1,clipX2,clipY2 bounds must be
 *              between (0 to 32767) inclusive
 *
 * Clip constraints: (per the MIDP specification)
 * if clipX2-1 is less than clipX1 or
 * if clipY2-1 is less than clipY1 then
 * the clip bounds are empty and no pixels are generated
 *
 */
void
draw_clipped_line(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int lineStyle, const jshort *clip, int x1, int y1, int x2, int y2) {

  int incrY, incrX, decision;
  int adjIncX, adjIncY;
  int xyEnd;
  int ret;
  int *ptrX, *ptrY;
  dotted_draw_state dds; /* for lineStyle == DOTTED */
  int x0 = x1;
  int y0 = y1;

  CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);

  ret = SetUpClippedLineParams(&x1, &y1, x2, y2, &xyEnd, &decision,
                               &incrY, &incrX, &adjIncX, &adjIncY, clip);

  if (ret == OUTSIDE)
    return;
  if (ret & POINT) {
    PRIMDRAWPIXEL(sbuf, color, x1, y1);
    return;
  }
  if (ret & REFLECT) {
    ptrX = &y1; ptrY = &x1;
  } else {
    ptrX = &x1; ptrY = &y1;
  }
  if (lineStyle == SOLID) {
    if ((ret & (HORIZONTAL|REFLECT)) == (HORIZONTAL|REFLECT)) {
      CHECK_XY_CLIP(sbuf, *ptrX, *ptrY);
      CHECK_XY_CLIP(sbuf, *ptrX, xyEnd-adjIncX);
      primDrawVertLine(sbuf, color, *ptrX, *ptrY, *ptrX, xyEnd-adjIncX);
      return;
    } 
    if (ret & HORIZONTAL) {
      CHECK_XY_CLIP(sbuf, *ptrX, *ptrY);
      CHECK_XY_CLIP(sbuf, xyEnd-adjIncX, *ptrY);
      primDrawHorzLine(sbuf, color, *ptrX, *ptrY, xyEnd-adjIncX, *ptrY);
      return;
    }
    /* The combination of adjIncX/adjIncY (+1 or -1)
     * and REFLECT allow us to use the single octant
     * version of the loop for all octants. */
    while (x1 != xyEnd) {
      PRIMDRAWPIXEL(sbuf, color, *ptrX, *ptrY);
      if (decision >= 0) {
        y1 += adjIncY; decision -= incrY;
      } else {
        decision += incrX;
      }
      x1 += adjIncX;
    }
    return;
  }
  dds = START_STROKE;
  if (ret & CLIPPED) {
    /* compute the offset into pattern compensating for clip */
    dds.solidcount = (ret & REFLECT) ? y1 - y0 :  x1 - x0;
    if (dds.solidcount < 0)
      dds.solidcount = -dds.solidcount;
    dds.solidcount %= (DOTTED_SOLID_SIZE+DOTTED_EMPTY_SIZE);
    if (dds.solidcount >= DOTTED_SOLID_SIZE) {
      dds.drawing = FALSE; dds.emptycount = dds.solidcount-DOTTED_SOLID_SIZE; dds.solidcount = 0;
    }
  }
  while (x1 != xyEnd) {
    drawDottedPixel(sbuf, color, *ptrX, *ptrY, &dds);
    if (decision >= 0) {
      y1 += adjIncY; decision -= incrY;
    } else {
      decision += incrX;
    }
    x1 += adjIncX;
  }
  return;
}

/**
 * draw pixels from (x1,y1) through (x1,y2)
 * y1 should be <= y2, x1 == x2
 * coordinates are clipped against the clip
 */
static void
drawClippedVertLine(const jshort *clip, gxj_screen_buffer *sbuf,
    gxj_pixel_type color, int x1, int y1, int x2, int y2) {

  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];
  (void)x2;

  if (y1 >= clipY2 || y2 <  clipY1 ||
      x2 <  clipX1 || x1 >= clipX2)
    return;
  y1 = (y1 <  clipY1) ? clipY1 : y1;
  y2 = (y2 >= clipY2) ? clipY2-1 : y2;
  CHECK_XY_CLIP(sbuf, x1, y1); CHECK_XY_CLIP(sbuf, x1, y2);
  primDrawVertLine(sbuf, color, x1, y1, x1, y2);
}

/**
 * draw pixels from (x1,y1) through (x2,y1)
 * x1 should be <= x2, y1 == y2
 * coordinates are clipped against the clip
 */
static void
drawClippedHorzLine(const jshort *clip, gxj_screen_buffer *sbuf,
    gxj_pixel_type color, int x1, int y1, int x2, int y2) {

  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];
  (void)y2;

  if (x1 >= clipX2 || x2 <  clipX1 ||
      y1 <  clipY1 ||y1 >= clipY2)
    return;
  x1 = (x1 <  clipX1) ? clipX1 : x1;
  x2 = (x2 >= clipX2) ? clipX2-1 : x2;
  CHECK_XY_CLIP(sbuf, x1, y1); CHECK_XY_CLIP(sbuf, x2, y1);
  primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
}

/**
 * Evaluate dotted stroke parameters for the point distant
 * from the current one by specified number of pixels.
 * The current point is presented with its stroke state,
 * i.e. with dds structure (solidcount, emptycount & drawing).
 *
 * Note, that stroke can be either solid, or empty line,
 * while unstroke is considered as a line of the opposite type,
 * i.e. empty or solid accordingly.
 *
 * @param nPixels number of pixels from the current point
 * @param stroke pointer to the number of pixels drawn already for the
 *   current stroke, on exit from the method will be evaluated for the
 *   distant point
 * @param unstroke if the distant point belongs to a stroke of the
 *   opposite type than the current point has, this parameter will
 *   refer to number of pixels already drawn in the distant stroke,
 *   otherwise it will refer to 0.
 * @param drawing refers to the stroke type of the current point,
 *   on return refers to the stroke type of the distant point
 * @param DOTTED_STROKE_SIZE fixed stroke size of the current stroke type
 * @param DOTTED_UNSTROKE_SIZE fixed size of the stroke type opposite
 *   to the current stroke type
 */
static void
SetUpDottedStrokeParamsByPixels(int nPixels,
    /*INOUT*/ int *stroke, /*OUT*/ int *unstroke, /*INOUT*/ int *drawing,
    const int DOTTED_STROKE_SIZE, const int DOTTED_UNSTROKE_SIZE) {

  if (nPixels > DOTTED_STROKE_SIZE - *stroke) {
    int offset = (nPixels - DOTTED_STROKE_SIZE + *stroke) %
      (DOTTED_STROKE_SIZE + DOTTED_UNSTROKE_SIZE);
    if (offset < DOTTED_UNSTROKE_SIZE) {
      *drawing = !(*drawing);
      *unstroke = offset;
      *stroke = 0;
    } else {
      *stroke = offset - DOTTED_UNSTROKE_SIZE;
      *unstroke = 0;
    }
  } else {
    *stroke += nPixels;
    *unstroke = 0;
  }
}

/**
 * Init dotted stroke params for the point distant from
 * the current point by specified number of pixels.
 * The current point state is presented by dds.
 */
static void
SetUpDottedParamsByPixels(int nPixels, dotted_draw_state * dds) {

    if (dds->drawing) {
      SetUpDottedStrokeParamsByPixels(nPixels, &dds->solidcount,  &dds->emptycount,  &dds->drawing,
          DOTTED_SOLID_SIZE, DOTTED_EMPTY_SIZE);
    } else {
      SetUpDottedStrokeParamsByPixels(nPixels,  &dds->emptycount,  &dds->solidcount,  &dds->drawing,
          DOTTED_EMPTY_SIZE, DOTTED_SOLID_SIZE);
    }
}

/**
 * draw dotted pixels from (x1,y1) through (x1,y2)
 * y1 should be <= y2, x1 == x2
 * coordinates are clipped against the clip
 */
static void
drawClippedDottedVertLine(const jshort *clip, gxj_screen_buffer *sbuf,
    gxj_pixel_type color, int x1, int y1, int x2, int y2,
    dotted_draw_state dds) {

  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];
  (void)x2;

  if (y1 >= clipY2 || y2 <  clipY1 ||
      x2 <  clipX1 || x1 >= clipX2)
    return;
  if (y1 < clipY1) {
    SetUpDottedParamsByPixels(clipY1 - y1, &dds);
    y1 = clipY1;
  }
  y2 = (y2 >= clipY2) ? clipY2-1 : y2;
  CHECK_XY_CLIP(sbuf, x1, y1); CHECK_XY_CLIP(sbuf, x1, y2);
  drawDottedVertLine(sbuf, color, x1, y1, x1, y2, dds);
}

/**
 * draw dotted pixels from (x1,y1) through (x2,y1)
 * x1 should be <= x2, y1 == y2
 * coordinates are clipped against the clip
 */
static void
drawClippedDottedHorzLine(const jshort *clip, gxj_screen_buffer *sbuf,
    gxj_pixel_type color, int x1, int y1, int x2, int y2,
    dotted_draw_state dds) {

  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];
  (void)y2;

  if (x1 >= clipX2 || x2 <  clipX1 ||
      y1 <  clipY1 || y1 >= clipY2)
    return;
  if (x1 < clipX1) {
    SetUpDottedParamsByPixels(clipX1 - x1, &dds);
    x1 = clipX1;
  }
  x2 = (x2 >= clipX2) ? clipX2-1 : x2;
  CHECK_XY_CLIP(sbuf, x1, y1); CHECK_XY_CLIP(sbuf, x2, y1);
  drawDottedHorzLine(sbuf, color, x1, y1, x2, y1, dds);
}

/**
 * draw rectangle outline, without painting corner pixels twice
 * 
 * x2 >= x1, y2 >= y1
 */
static void
primDrawSolidRect(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x1, int y1, int x2, int y2) {

  primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
  primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
  if (y2-1 > y1+1) {
    primDrawVertLine(sbuf, color, x1, y1 + 1, x1, y2 - 1);
    primDrawVertLine(sbuf, color, x2, y1 + 1, x2, y2 - 1);
  }
}

/**
 * draw patterned rectangle outline, without painting corner pixels twice
 * NOTE: pattern is reset for each segment
 * 
 * x2 >= x1, y2 >= y1
 */
static void
primDrawDottedRect(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x1, int y1, int x2, int y2) {

  primDrawDottedHorzLine(sbuf, color, x1, y1, x2, y1);
  primDrawDottedHorzLine(sbuf, color, x1, y2, x2, y2);
  if (y2-1 > y1+1) {
    primDrawDottedVertLine(sbuf, color, x1, y1 + 1, x1, y2 - 1);
    primDrawDottedVertLine(sbuf, color, x2, y1 + 1, x2, y2 - 1);
  }
}

/**
 * draw filled rectangle, this does NOT paint the pixels
 * along the x2 or y2 edges.
 * 
 * x2 > x1, y2 > y1
 */
static void
primDrawFilledRect(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int x1, int y1, int x2, int y2) {

#if 0  
    int width = sbuf->width;
    int height = sbuf->height;
    int count;
    gxj_pixel_type* pPtr;
    gxj_pixel_type* lPtr;

#if PRIM_CLIPPING
    x1 = (x1 < 0) ? 0 : ((x1 > width) ? width : x1);
    x2 = (x2 < 0) ? 0 : ((x2 > width) ? width : x2);
    y1 = (y1 < 0) ? 0 : ((y1 > height) ? height : y1);
    y2 = (y2 < 0) ? 0 : ((y2 > height) ? height : y2);
#endif
    if ((count=x2-x1) < 0) {
      SWAP(x2, x1);
      count = -count;
    }
		
    pPtr = &(sbuf->pixelData[y1 * width + x1]);
    for (lPtr = pPtr; y1 < y2; y1++) {
      while (count & ~0x7) {
        CHECK_PTR_CLIP(sbuf,pPtr);
        *pPtr = color; pPtr += 1; *pPtr = color; pPtr += 1;
        *pPtr = color; pPtr += 1; *pPtr = color; pPtr += 1;
        *pPtr = color; pPtr += 1; *pPtr = color; pPtr += 1;
        *pPtr = color; pPtr += 1;
        *pPtr = color; CHECK_PTR_CLIP(sbuf,pPtr); pPtr += 1;
        count -= 8;
      }
      while (count > 0) {
        CHECK_PTR_CLIP(sbuf,pPtr);
        *pPtr = color; pPtr += 1;
        count -= 1;
      }
      lPtr += width;
      count = x2 - x1;
      pPtr = lPtr;
    }
#else
    int width = sbuf->width;
    int height = sbuf->height;
    int count;
    gxj_pixel_type* pPtr;
    gxj_pixel_type* lPtr;
    registers_4	regs;

	unsigned int c = ((unsigned int)color) << 16 | ((unsigned int)color);
    jlong lcol = ((jlong) c) << 32 | ((jlong) c);
	regs.r0 = regs.r1 = regs.r2 = regs.r3 = c;

#if PRIM_CLIPPING
    y1 = (y1 < 0) ? 0 : ((y1 > height) ? height : y1);
    y2 = (y2 < 0) ? 0 : ((y2 > height) ? height : y2);
#endif
    pPtr = &(sbuf->pixelData[y1 * width]);
#if PRIM_CLIPPING
    x1 = (x1 < 0) ? 0 : ((x1 > width) ? width : x1);
    x2 = (x2 < 0) ? 0 : ((x2 > width) ? width : x2);
#endif
    if ((count=x2-x1) < 0) {
      SWAP(x2, x1);
      count = -count;
    }
    pPtr += x1;
    if (x1 == 0 && x2 == (width)) {
      count = width*(y2-y1);
      y2 = y1+1;
    }
    for (lPtr = pPtr; y1 < y2; y1++) {
      if (((unsigned int)pPtr & 0x3) && (count > 0)) {
        CHECK_PTR_CLIP(sbuf,pPtr);
        *pPtr++ = color;
        --count;
      }
      while (count >= 16) {
        CHECK_LLPTR_CLIP(sbuf,pPtr);
        *((registers_4 *)pPtr) = regs;
		pPtr += 8;
		CHECK_LLPTR_CLIP(sbuf,pPtr);
		*((registers_4 *)pPtr) = regs;
		pPtr += 8;
        count -= 16;
      }
      if (count >= 8) {
        CHECK_LLPTR_CLIP(sbuf,pPtr);
        *((registers_4 *)pPtr) = regs;
        pPtr += 8;
        count -= 8;
      }
      if (count >= 4) {
        CHECK_LLPTR_CLIP(sbuf,pPtr);
        *((jlong *)pPtr) = lcol;
        pPtr += 4;
        count -= 4;
      }
      while (count > 0) {
        CHECK_PTR_CLIP(sbuf,pPtr);
        *pPtr++ = color;
        count -= 1;
      }
      lPtr += width;
      count = x2 - x1;
      pPtr = lPtr;
    }
#endif
}

static void
drawClippedRect(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int lineStyle, int filled, const jshort *clip,
    int x, int y, int width, int height) {

  int x1, y1, x2, y2, x2UnClipped, y2UnClipped;

  if (clip == NULL) {
    x1 = x;
    y1 = y;
    x2 = x + width;
    y2 = y + height;
    if (filled) {
      primDrawFilledRect(sbuf, color, x1, y1, x2, y2);
    } else {
      if (lineStyle == DOTTED) {
        primDrawDottedVertLine(sbuf, color, x1, y1, x1, y2);
        primDrawDottedHorzLine(sbuf, color, x1, y1, x2, y1);
        primDrawDottedVertLine(sbuf, color, x2, y1, x2, y2);
        primDrawDottedHorzLine(sbuf, color, x1, y2, x2, y2);
      } else {
        primDrawVertLine(sbuf, color, x1, y1, x1, y2);
        primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
        primDrawVertLine(sbuf, color, x2, y1, x2, y2);
        primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
      }
    }
    return;
  }
  CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);

  {
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    if (clipX1 == clipX2 || clipY1 == clipY2)
      return;

    x1 = max(x, clipX1);
    y1 = max(y, clipY1);
    x2UnClipped = x + width;
    y2UnClipped = y + height;
    x2 = min(x2UnClipped, clipX2);
    y2 = min(y2UnClipped, clipY2);
  }

  if (filled) {
    /* check that it's still there after clipping */
    if ((x1 > x2) || (y1 > y2)) {
      return;
    }
    primDrawFilledRect(sbuf, color, x1, y1, x2, y2);
  } else {
    /* have we been clipped out of existence? */
    if (((x1 > x) && (x2 < x2UnClipped) &&
         (y1 > y) && (y2 < y2UnClipped)) ||
        (x1 > x2) ||
        (y1 > y2)) {
      return;
    }

    /* is the left side still there? */
    if (x1 == x) {
      if (lineStyle == DOTTED) {
        primDrawDottedVertLine(sbuf, color, x1, y1, x1, y2);
      } else {
        primDrawVertLine(sbuf, color, x1, y1, x1, y2);
      }
    }
    /* is the top still there? */
    if (y1 == y) {
      if (lineStyle == DOTTED) {
        primDrawDottedHorzLine(sbuf, color, x1, y1, x2, y1);
      } else {
        primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
      }
    }
    /* is the left side still there? */
    if (x2 == x2UnClipped) {
      if (lineStyle == DOTTED) {
        primDrawDottedVertLine(sbuf, color, x2, y1, x2, y2);
      } else {
        primDrawVertLine(sbuf, color, x2, y1, x2, y2);
      }
    }
    /* and how about the bottom? */
    if (y2 == y2UnClipped) {
      if (lineStyle == DOTTED) {
        primDrawDottedHorzLine(sbuf, color, x1, y2, x2, y2);
      } else {
        primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
      }
    }
  }
}

/* IMPL_NOTE easy optimization is to break drawSymmetricPixels into
   separate routines for the different cases, then use function pointers
   to call the appropriate one

   IMPL_NOTE another one is to make the params to drawSymmetricPixels
   sit in a struct and just pass the
   pointer to the struct instead of all the params - this will probably
   help, depending on number of references

   taking xOrigin and yOrigin as the origin in Windows coordinates, and
   x, y as Cartesian coordinates, this draws the Windows points for all
   4 quadrants.
   (remember that in Windows bigger y goes down, opposite of Cartesian)
   this function is not used for filled partial or filled clipped arcs
   it is used for all non-filled arcs, and unclipped,full filled arcs.

   dds is used only if lineStyle == DOTTED */
static void
drawSymmetricPixels(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int lineStyle, int filled, const jshort *clip, int startRatio,
    int startQuadrant, int endRatio, int endQuadrant,
    int quadrantStatus[4], dotted_draw_state * dds,
    int xOrigin, int yOrigin, int evenXOffset, int evenYOffset,
    int x, int y) {

  int x1, y1, x2, y2;
  int curRatio;
  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];

  CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);
 
  /* Surpress unused parameter warnings */
  (void)filled;

  x1 = xOrigin + x;
  y1 = yOrigin - y;
  x2 = xOrigin - x - evenXOffset;
  y2 = yOrigin + y + evenYOffset;

  /* check for case of unclipped, complete ellipse */
  if (startQuadrant == -1) {
  }
  /* take care of unfilled partial or clipped (or both) arc */
  else {
    if (x == 0) {       
      curRatio = MAXINT32;
    } else {
      curRatio = (y * 1000) / x;
    }

    /* this is for an unfilled,  partial or clipped (or both) arc  */
            
    if (((quadrantStatus[0] & QUADRANT_STATUS_FULL_ARC) ||
         ((quadrantStatus[0] & QUADRANT_STATUS_PARTIAL_ARC) &&
          (((startQuadrant == 1) &&
            (endQuadrant != 1) &&
            (curRatio >= startRatio)) ||
           ((endQuadrant == 1) &&
            (startQuadrant != 1) &&
            (curRatio <= endRatio)) ||
           ((startQuadrant == 1) &&
            (endQuadrant == 1) && 
            (startRatio >= endRatio) &&
            ((curRatio >= endRatio) ||
             (curRatio <= startRatio))) ||
           ((startQuadrant == 1) &&
            (endQuadrant == 1) &&
            (startRatio < endRatio) &&
            (curRatio >= startRatio) &&
            (curRatio <= endRatio))))) && 
        ((quadrantStatus[0] & QUADRANT_STATUS_UNCLIPPED) ||
         ((quadrantStatus[0] & QUADRANT_STATUS_PARTIALLY_CLIPPED) &&
          (x1 >= clipX1) &&
          (y1 >= clipY1) &&
          (x1 < clipX2) &&
          (y1 < clipY2)))) {
      if (lineStyle == DOTTED) {
        drawDottedPixel(sbuf, color, x1, y1, &dds[0]);
      } else {
        PRIMDRAWPIXEL(sbuf, color, x1, y1);
      }
    }
    if (((quadrantStatus[1] & QUADRANT_STATUS_FULL_ARC) ||
         ((quadrantStatus[1] & QUADRANT_STATUS_PARTIAL_ARC) &&
          (((startQuadrant == 2) &&
            (endQuadrant != 2) &&
                (curRatio <= startRatio)) ||
           ((endQuadrant == 2) &&
            (startQuadrant != 2) &&
            (curRatio >= endRatio)) ||
           ((startQuadrant == 2) &&
            (endQuadrant == 2) && 
            (startRatio <= endRatio) &&
            ((curRatio <= endRatio) ||
             (curRatio >= startRatio))) ||
           ((startQuadrant == 2) &&
            (endQuadrant == 2) &&
            (startRatio > endRatio) &&
            (curRatio <= startRatio) &&
            (curRatio >= endRatio))))) && 
        ((quadrantStatus[1] & QUADRANT_STATUS_UNCLIPPED) ||
         ((quadrantStatus[1] & QUADRANT_STATUS_PARTIALLY_CLIPPED) &&
          (x2 >= clipX1) &&
          (y1 >= clipY1) &&
          (x2 < clipX2) &&
          (y1 < clipY2)))) {
      if (lineStyle == DOTTED) {
        drawDottedPixel(sbuf, color, x2, y1, &dds[1]);
      } else {
        PRIMDRAWPIXEL(sbuf, color, x2, y1);
      }
    }
    if (((quadrantStatus[2] & QUADRANT_STATUS_FULL_ARC) ||
         ((quadrantStatus[2] & QUADRANT_STATUS_PARTIAL_ARC) &&
          (((startQuadrant == 3) &&
            (endQuadrant != 3) &&
            (curRatio >= startRatio)) ||
           ((endQuadrant == 3) &&
            (startQuadrant != 3) &&
            (curRatio <= endRatio)) ||
           ((startQuadrant == 3) &&
            (endQuadrant == 3) && 
            (startRatio >= endRatio) &&
            ((curRatio >= endRatio) ||
             (curRatio <= startRatio))) ||
           ((startQuadrant == 3) &&
            (endQuadrant == 3) &&
            (startRatio < endRatio) &&
            (curRatio >= startRatio) &&
            (curRatio <= endRatio))))) && 
        ((quadrantStatus[2] & QUADRANT_STATUS_UNCLIPPED) ||
         ((quadrantStatus[2] & QUADRANT_STATUS_PARTIALLY_CLIPPED) &&
          (x2 >= clipX1) &&
          (y2 >= clipY1) &&
          (x2 < clipX2) &&
          (y2 < clipY2)))) {
      if (lineStyle == DOTTED) {
        drawDottedPixel(sbuf, color, x2, y2, &dds[2]);
      } else {
        PRIMDRAWPIXEL(sbuf, color, x2, y2);
      }
    }
    if (((quadrantStatus[3] & QUADRANT_STATUS_FULL_ARC) ||
         ((quadrantStatus[3] & QUADRANT_STATUS_PARTIAL_ARC) &&
          (((startQuadrant == 4) &&
            (endQuadrant != 4) &&
            (curRatio <= startRatio)) ||
           ((endQuadrant == 4) &&
            (startQuadrant != 4) &&
            (curRatio >= endRatio)) ||
           ((startQuadrant == 4) &&
            (endQuadrant == 4) && 
            (startRatio <= endRatio) &&
            ((curRatio <= endRatio) ||
             (curRatio >= startRatio))) ||
           ((startQuadrant == 4) &&
            (endQuadrant == 4) &&
            (startRatio > endRatio) &&
            (curRatio <= startRatio) &&
            (curRatio >= endRatio))))) && 
        ((quadrantStatus[3] & QUADRANT_STATUS_UNCLIPPED) ||
         ((quadrantStatus[3] & QUADRANT_STATUS_PARTIALLY_CLIPPED) &&
          (x1 >= clipX1) &&
          (y2 >= clipY1) &&
          (x1 < clipX2) &&
          (y2 < clipY2)))) {
      if (lineStyle == DOTTED) {
        drawDottedPixel(sbuf, color, x1, y2, &dds[3]);
      } else {
        PRIMDRAWPIXEL(sbuf, color, x1, y2);
      }
    }
  }
}

static void
SetUpEllipseParams(int x1, int y1, int x2, int y2,
    int *a, int *b, int *a2, int *b2, int *S, int *T,
    int *xCenter, int *yCenter, int *evenXOffset, int *evenYOffset) {

    (*a) = (x2 - x1) / 2;
    (*evenXOffset) = ((x2 - x1) % 2) == 0 ? 0 : 1;
    (*b) = (y2 - y1) / 2;
    (*evenYOffset) = ((y2 - y1) % 2) == 0 ? 0 : 1;
    (*a2) = (*a)*(*a);
    (*b2) = (*b)*(*b);
    /* x and y are cartesian coordinates around the origin (0, 0)
     * other coordinates are Windows style  */

    (*S) = (*a2)*(1-2*(*b)) + 2*(*b2);
    (*T) = (*b2) - 2*(*a2)*(2*(*b)-1);
    (*xCenter) = x1 + (*a) + (*evenXOffset);
    (*yCenter) = y1 + (*b);
}

static void
GetNextEllipsePoint(int a2, int b2, int *S, int *T, int *x, int *y) {

    if ((*S)<0) {
      (*S) += 2*b2*(2*(*x)+3);
      (*T) += 4*b2*((*x)+1);
      (*x)++;
    } else if ((*T)<0) {
      (*S) += 2*b2*(2*(*x)+3) - 4*a2*((*y)-1);
      (*T) += 4*b2*((*x)+1) - 2*a2*(2*(*y)-3);
      (*x)++;
      (*y)--;
    } else {
      (*S) -= 4*a2*((*y)-1);
      (*T) -= 2*a2*(2*(*y)-3);
      (*y)--;
    }
}

static void
SetUpDottedLineParams(int nPixels, int evenXOffset, int evenYOffset,
    dotted_draw_state * dds) {

    int offset;
    int nPixelsTwoQuadrants;

    nPixelsTwoQuadrants = (2 * nPixels) + ((evenYOffset == 0) ? -1 : 0);

    /* 1st quadrant starts drawing the pattern from the beginning */
    dds[0] = START_STROKE;

    /* the following is the magic formula for figuring out how to draw
     * the fourth quadrant backwards so that when it's done it meets
     * the first quadrant in such a way that it continues the dotted
     * line pattern cleanly */
    offset = (DOTTED_SOLID_SIZE - nPixelsTwoQuadrants) %
        (DOTTED_SOLID_SIZE + DOTTED_EMPTY_SIZE);
    if (offset < 0) {
        /* Really need to just remember proper mod operator! */
        offset += (DOTTED_SOLID_SIZE + DOTTED_EMPTY_SIZE);
    }
    dds[3].solidcount = (offset >= DOTTED_SOLID_SIZE) ? DOTTED_SOLID_SIZE : offset;
    dds[3].emptycount = (offset >= DOTTED_SOLID_SIZE) ? offset - DOTTED_SOLID_SIZE :
        DOTTED_EMPTY_SIZE;
    dds[3].drawing = dds[3].solidcount < DOTTED_SOLID_SIZE;

    /* same idea for making drawing of third quadrant match the adjacent
     * part of the pattern in the fourth quadrant */
    offset = nPixelsTwoQuadrants % (DOTTED_SOLID_SIZE + DOTTED_EMPTY_SIZE);
    offset += ((evenXOffset == 0) ? -1 : 0);
    dds[2].solidcount = (offset >= DOTTED_SOLID_SIZE) ? DOTTED_SOLID_SIZE : offset;
    dds[2].emptycount = (offset >= DOTTED_SOLID_SIZE) ? offset - DOTTED_SOLID_SIZE :
        DOTTED_EMPTY_SIZE;
    dds[2].drawing = dds[3].solidcount < DOTTED_SOLID_SIZE;

    /* same idea for meeting of second and first quadrants */
    offset = DOTTED_SOLID_SIZE + ((evenXOffset == 0) ? -1 : 0);
    dds[1].solidcount = (offset >= DOTTED_SOLID_SIZE) ? DOTTED_SOLID_SIZE : offset;
    dds[1].emptycount = (offset >= DOTTED_SOLID_SIZE) ? offset - DOTTED_SOLID_SIZE :
        DOTTED_EMPTY_SIZE;
    dds[1].drawing = dds[3].solidcount < DOTTED_SOLID_SIZE;

    /* alas, the meeting of the second and third
     * quadrants can't be made exact */
}

static int
getQuadrantStatus(int quadrant, int x1, int y1, int x2, int y2,
    const jshort *clip, int startRatio, int startQuadrant,
    int endRatio, int endQuadrant) {

  int quadrantStatus;
  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];

  quadrantStatus = QUADRANT_STATUS_UNDEFINED;

  if ((x1 >= clipX1) &&
      (x2 < clipX2) &&
      (y1 >= clipY1) &&
      (y2 < clipY2)) {
      quadrantStatus |= QUADRANT_STATUS_UNCLIPPED;
  } else if ((x1 >= clipX2) ||
           (x2 < clipX1) ||
           (y1 >= clipY2) ||
           (y2 <= clipY1)) {
      quadrantStatus |= QUADRANT_STATUS_TOTALLY_CLIPPED;
      return quadrantStatus;
  } else {
      quadrantStatus |= QUADRANT_STATUS_PARTIALLY_CLIPPED;
  }

  if (startQuadrant == -1) {
      quadrantStatus |= QUADRANT_STATUS_FULL_ARC;
  } else if (((quadrant == startQuadrant) && (startRatio > 0) &&
                (startRatio < MAXINT32)) ||
      ((quadrant == endQuadrant) && (endRatio > 0) &&
         (endRatio < MAXINT32))) {
      quadrantStatus |= QUADRANT_STATUS_PARTIAL_ARC;
  } else if ((quadrant == startQuadrant) && (quadrant != endQuadrant)) {
      quadrantStatus |= QUADRANT_STATUS_FULL_ARC;
  } else if ((startQuadrant < endQuadrant) && 
           (quadrant > startQuadrant) && (quadrant < endQuadrant)) {
      quadrantStatus |= QUADRANT_STATUS_FULL_ARC;
  } else if ((startQuadrant > endQuadrant) && ((quadrant > startQuadrant) ||
            (quadrant < endQuadrant))) {
      quadrantStatus |= QUADRANT_STATUS_FULL_ARC;
  } else if ((startQuadrant == endQuadrant) && 
           ((((startQuadrant == 1) || (startQuadrant == 3)) &&
                (startRatio > endRatio)) ||
            (((startQuadrant == 2) || (startQuadrant == 4)) &&
                (startRatio < endRatio))) &&
           (quadrant != startQuadrant)) {
      quadrantStatus |= QUADRANT_STATUS_FULL_ARC;
  } else {
      quadrantStatus |= QUADRANT_STATUS_NO_ARC;
  }

  return quadrantStatus;
}

static void
GetArcHelperParams(int x, int y, int w, int h,
    int startAngle, int endAngle, const jshort *clip,
    int *startQuadrant, int *startRatio,
    int *endQuadrant, int *endRatio, int *quadrantStatus) {

    int a, b, a2, b2, S, T;
    int evenXOffset, evenYOffset;
    int x2, y2;
    int xCenter, yCenter;

    x2 = x + w;
    y2 = y + h;

    /* only need some of these params, but for
     * now haven't separated out the code */
    SetUpEllipseParams(x, y, x2, y2, &a, &b, &a2, &b2, &S, &T,
                       &xCenter, &yCenter, &evenXOffset, &evenYOffset);
    
    if (startAngle != endAngle) {
      *startQuadrant = (startAngle / 90) + 1;
      *endQuadrant = (endAngle / 90) + 1;

      *startRatio = absIntTan1000(startAngle);
      if (*startRatio < MAXINT32) {
        *startRatio = (h * *startRatio) / w;
      }
      *endRatio = absIntTan1000(endAngle);
      if (*endRatio < MAXINT32) {
        *endRatio = (h * *endRatio) / w;
      }
    } else {
      *startRatio = 0;
      *endRatio = 0;
    }
    
    /* now check each quadrant's status */
    quadrantStatus[0] = getQuadrantStatus(1, xCenter, y, x2, yCenter, clip, 
        *startRatio, *startQuadrant, *endRatio, *endQuadrant);
    quadrantStatus[1] = getQuadrantStatus(2, x, y, xCenter, yCenter, clip, 
        *startRatio, *startQuadrant, *endRatio, *endQuadrant);
    quadrantStatus[2] = getQuadrantStatus(3, x, yCenter, xCenter, y2, clip, 
        *startRatio, *startQuadrant, *endRatio, *endQuadrant);
    quadrantStatus[3] = getQuadrantStatus(4, xCenter, yCenter, x2, y2, clip, 
        *startRatio, *startQuadrant, *endRatio, *endQuadrant);
}

static void
SetUpArcEndPoints(int x, int y, int w, int h,
    int startQuadrant, int startRatio, int endQuadrant, int endRatio,
    int *start_x, int *start_y, int *end_x, int *end_y,
    int *start_x1, int *start_y1, int *end_x1, int *end_y1,
    int *start_x2, int *start_y2, int *end_x2, int *end_y2) {

    int a,b;            
    int a2,b2, S, T;
    int xCenter, yCenter;
    int evenXOffset, evenYOffset;       
    int x_point, y_point;       
    int curRatio;

    int boundary_x1 = 0, boundary_x2 = 0, boundary_x3 = 0, boundary_x4 = 0;
    int boundary_y1 = 0, boundary_y2 = 0, boundary_y3 = 0, boundary_y4 = 0;

    /* we need to set up start and end points for partial, filled arcs
     * we do this by getting 4 pairs of coordinates, right before and
     * after each exact start and end point, and afterwards choosing
     * the correct two pairs depending on which quadrant they're in */

    boundary_x1 = -1;
    boundary_x3 = -1;

    SetUpEllipseParams(x, y, x + w, y + h, &a, &b, &a2, &b2, &S, &T,
        &xCenter, &yCenter, &evenXOffset, &evenYOffset);
    x_point = 0;
    y_point = b;
    while(y_point >= 0) {
      curRatio = (x_point == 0) ? MAXINT32 : ((y_point * 1000) / x_point);

      if ((boundary_x1 == -1) && (curRatio <= startRatio)) {
        boundary_x1 = x_point;
        boundary_y1 = y_point;
      }
      if (curRatio >= startRatio) {
        boundary_x2 = x_point;
        boundary_y2 = y_point;
      }
      if ((boundary_x3 == -1) && (curRatio <= endRatio)) {
        boundary_x3 = x_point;
        boundary_y3 = y_point;
      }
      if (curRatio >= endRatio) {
        boundary_x4 = x_point;
        boundary_y4 = y_point;
      }
      GetNextEllipsePoint(a2, b2, &S, &T, &x_point, &y_point);
    }

    /* first establish where the start and end points of the partial arc are */
    if ((startQuadrant == 1) || (startQuadrant == 3)) {        
      *start_x = boundary_x1;
      *start_y = boundary_y1;
    } else {
      *start_x = boundary_x2;
      *start_y = boundary_y2;
    }
    if ((endQuadrant == 1) || (endQuadrant == 3)) {        
      *end_x = boundary_x3;
      *end_y = boundary_y3;
    } else {
      *end_x = boundary_x4;
      *end_y = boundary_y4;
    }

    *start_x1 = xCenter + *start_x;
    *start_y1 = yCenter - *start_y;
    *start_x2 = xCenter - *start_x - evenXOffset;
    *start_y2 = yCenter + *start_y + evenYOffset;
    *end_x1 = xCenter + *end_x;
    *end_y1 = yCenter - *end_y;
    *end_x2 = xCenter - *end_x - evenXOffset;
    *end_y2 = yCenter + *end_y + evenYOffset;
}

static int
calcNPixelsInQuadrant(int x, int y, int w, int h)
{
    int a, b, a2, b2, S, T;
    int evenXOffset, evenYOffset;
    int xCenter, yCenter;    
    int x_point, y_point;
    int nPixels;

    SetUpEllipseParams(x, y, x + w, y + h, &a, &b, &a2, &b2, &S, &T, 
                       &xCenter, &yCenter, &evenXOffset, &evenYOffset);
    x_point = 0;
    y_point = b;

    for (nPixels = 0; (y_point >= 0); ++nPixels)
    {
        GetNextEllipsePoint(a2, b2, &S, &T, &x_point, &y_point);
    }

    return nPixels;
}

static void
drawClippedOutlineArc(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int lineStyle, const jshort *clip, int startQuadrant, int startRatio,
    int endQuadrant, int endRatio, int nPixelsInQuadrant,
    int quadrantStatus[4], int x, int y, int w, int h) {

    int a,b;            
    int a2,b2, S, T;
    int xCenter, yCenter;
    int curRatio;
    int evenXOffset, evenYOffset;
    dotted_draw_state dds[4]; /* used for dotted lines only */
    int x_point, y_point;

    CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);

    SetUpEllipseParams(x, y, x + w, y + h, &a, &b, &a2, &b2,
        &S, &T, &xCenter, &yCenter, &evenXOffset, &evenYOffset);

    if (lineStyle == DOTTED) {
      SetUpDottedLineParams(nPixelsInQuadrant, evenXOffset,
          evenYOffset, dds);
    }

    x_point = 0;
    y_point = b;
    while(y_point >= 0) {
      curRatio = (x_point == 0) ? MAXINT32 : ((y_point * 1000) / x_point);

      drawSymmetricPixels(sbuf, color, lineStyle, 0, clip,
          startRatio, startQuadrant, endRatio, endQuadrant,
          quadrantStatus, dds,
          xCenter, yCenter, evenXOffset, evenYOffset,
          x_point, y_point);

      GetNextEllipsePoint(a2, b2, &S, &T, &x_point, &y_point);
    }
}

/* drawFilledRightTriangle is called with the two points that aren't the
 * square corner the corner is located at x2, y1 */
static void
drawFilledRightTriangle(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    const jshort *clip, int x1, int y1, int x2, int y2) {

    fill_triangle(sbuf, color, clip, x1, y1, x2, y1, x2, y2);
}

#define STEPx(_x,_xFRACT,_dx,_dy,_signDX) \
  do { \
    _xFRACT += _dx; \
    while (_xFRACT >= _dy) { \
      _x += _signDX; \
      _xFRACT -= _dy; \
    } \
  } while (0)

/**
 * Generates pixels for filled triangles that match the MIDP
 * specification.  This function generates the set of pixels contained
 * within (x1,y1)->(x2,y2)->(x3,y3).   The pixels selected lie within
 * the lines drawn along the edge of the triangle with drawLine.  This
 * function uses an exact slope algorithm.  The set of
 * pixels generated is within the bounds (CLIP_X1,CLIP_Y1) ->
 * (CLIP_X2-1,CLIP_Y2-1).
 *
 * @param sbuf Virtual Device Context
 * @param color gxj_pixel_type value for generated pixels
 * @param lineStyle SOLID to generate all pixels or DOTTED for a subset
 * @param x1,y1 coordinate of first pixel generated must be
 *              between (-32768 to 32767) inclusive
 * @param x2,y2 coordinate of the last pixel generated must be
 *              between (-32768 to 32767) inclusive
 * @param x3,y3 coordinate of first pixel generated must be
 *              between (-32768 to 32767) inclusive
 * @param clip  ->clipX1,clipY1,clipX2,clipY2 must be
 *              between (0 to 32767) inclusive
 *
 * Clip constraints: (per the MIDP specification)
 * if clipX2-1 is less than clipX1 or
 * if clipY2-1 is less than clipY1 then
 * the clip bounds are empty and no pixels are generated
 *
 */
void
fill_triangle(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    const jshort *clip, int x1, int y1, int x2, int y2, int x3, int y3) {

    int y;

    int dxa, dya, signDXa;
    int dxb, dyb, signDXb;
    int dxc, dyc, signDXc;
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);

    /* sort axis points (y1<=y2<=y3) */
    if (y1 > y2) {
      SWAP(y1, y2); SWAP(x1, x2);
    }
    if (y1 > y3) {
      SWAP(y1, y3); SWAP(x1, x3);
    }
    if (y2 > y3) {
      SWAP(y2, y3); SWAP(x2, x3);
    }
    /* compute dx,dy and step directions */
    signDXa = 1;
    if ((dxa = x2 - x1) < 0) {
      dxa = -dxa; signDXa = -1;
    }
    dya = y2 - y1;

    signDXb = 1;
    if ((dxb = x3 - x1) < 0) {
      dxb = -dxb; signDXb = -1;
    }
    dyb = y3 - y1;

    signDXc = 1;
    if ((dxc = x3 - x2) < 0) {
      dxc = -dxc; signDXc = -1;
    }
    dyc = y3 - y2;

    /* look for edge case horizontal line
     * (sort above and triangle angles allow simple test) */
    if (/* dya == 0 && */ dyb == 0 /* && dyc == 0 */) {
      draw_clipped_line(sbuf, color, 0 /*lineStyle*/, clip,
                      x1, y1, x2, y2);
      draw_clipped_line(sbuf, color, 0 /*lineStyle*/, clip,
                      x2, y2, x3, y3);
      return;
    }
    /* y2 not checked because of sort on y1,y2,y3 */
    if ((y1 >= clipY1) && (y1 < clipY2) &&
        (y3 >= clipY1) && (y3 < clipY2) &&
        (x1 >= clipX1) && (x1 < clipX2) &&
        (x2 >= clipX1) && (x2 < clipX2) &&
        (x3 >= clipX1) && (x3 < clipX2)) {
      /* handle first edge x1,y1->x2,y2 */
      int xa = x1; int xb = x1;
      int xFRACTa = 0; int xFRACTb = 0;

      for (y=0; y < dya; y++) {
        CHECK_XY_CLIP(sbuf, xa, y1+y); CHECK_XY_CLIP(sbuf, xb, y1+y);
        primDrawHorzLine(sbuf, color, xa, y1 + y, xb, y1 + y);
        STEPx(xa,xFRACTa,dxa,dya,signDXa);
        STEPx(xb,xFRACTb,dxb,dyb,signDXb);
      }
      /* handle second edge x2,y2->x3,y3 */
      xa = x2;
      xFRACTa = 0;
      /* handle edge case (STEPx cannot take dyc == 0)
       * bottom of triangle is horizontal */
      if (dyc == 0) {
        CHECK_XY_CLIP(sbuf, x2, y1+y); CHECK_XY_CLIP(sbuf, x3, y1+y);
        primDrawHorzLine(sbuf, color, x2, y1 + y, x3, y1 + y);
        goto done;
      }
      for (y=dya; y <= dyb; y++) {
        CHECK_XY_CLIP(sbuf, xa, y1+y); CHECK_XY_CLIP(sbuf, xb, y1+y);
        primDrawHorzLine(sbuf, color, xa, y1 + y, xb, y1 + y);
        STEPx(xa,xFRACTa,dxc,dyc,signDXc);
        STEPx(xb,xFRACTb,dxb,dyb,signDXb);
      }
    return;
    }
/*clip: */
    {
      /* handle first edge x1,y1->x2,y2 */
      int xFRACTa = 0; int xFRACTb = 0;
      int xa, xa1, xb, xb1;
      xa = xa1 = x1;
      xb = xb1 = x1;

      for (y=0; y < dya; y++) {
        /* don't draw outside the clip */
        if (y1 + y < clipY1) {
          /* above the clip */
          goto nextStepTop;
        }
        if (y1 + y >= clipY2) {
          /* below the clip */
          goto done;
        }
        { /* complication is xa,xb are not ordered and cannot be swapped */
          int xaChanged, xbChanged;
          if ((xaChanged = (xa < clipX1)))
            xa1 = clipX1;
          if ((xbChanged = (xb < clipX1)))
            xb1 = clipX1;
          if (xaChanged && xbChanged)  /* both to left */
            goto nextStepTop;

          if ((xaChanged = !xaChanged))  /* toggle changed */
            if ((xaChanged = (xa >= clipX2)))
              xa1 = clipX2-1;
          if ((xbChanged = !xbChanged))
            if ((xbChanged = (xb >= clipX2)))
              xb1 = clipX2-1;
          if (xaChanged && xbChanged)  /* both to right */
            goto nextStepTop;
        }
        CHECK_XY_CLIP(sbuf, xa1, y1+y); CHECK_XY_CLIP(sbuf, xb1, y1+y);
        primDrawHorzLine(sbuf, color, xa1, y1 + y, xb1, y1 + y);
nextStepTop:
        STEPx(xa,xFRACTa,dxa,dya,signDXa);
        STEPx(xb,xFRACTb,dxb,dyb,signDXb);
        xa1 = xa;
        xb1 = xb;
      }
      /* handle second edge x2,y2->x3,y3 */
      xa = xa1 = x2;
      xFRACTa = 0;
      /* handle edge case (STEPx cannot take dyc == 0)
       * bottom of triangle is horizontal */
      if (dyc == 0) {
        if ((clipY1 < (y1 + y)) || ((y1 + y) >= clipY2) ||
            (clipX1 < (x2    )) || ((x2    ) >= clipX2) ||
            (clipX1 < (x3    )) || ((x3    ) >= clipX2))
          goto done;
        CHECK_XY_CLIP(sbuf, x2, y1+y); CHECK_XY_CLIP(sbuf, x3, y1+y);
        primDrawHorzLine(sbuf, color, x2, y1 + y, x3, y1 + y);
        goto done;
      }
      for (y=dya; y <= dyb; y++) {
        /* don't draw outside the clip */
        if (y1 + y < clipY1) {
          /* above the clip */
          goto nextStepBottom;
        }
        if (y1 + y >= clipY2) {
          /* below the clip */
          goto done;
        }
        { /* complication is xa,xb are not ordered and cannot be swapped */
          int xaChanged, xbChanged;
          if ((xaChanged = (xa < clipX1)))
            xa1 = clipX1;
          if ((xbChanged = (xb < clipX1)))
            xb1 = clipX1;
          if (xaChanged && xbChanged)  /* both to left */
            goto nextStepBottom;

          if ((xaChanged = !xaChanged))  /* toggle (changed) */
            if ((xaChanged = (xa >= clipX2)))
              xa1 = clipX2-1;
          if ((xbChanged = !xbChanged))
            if ((xbChanged = (xb >= clipX2)))
              xb1 = clipX2-1;
          if (xaChanged && xbChanged)  /* both to right */
            goto nextStepBottom;
        }
        CHECK_XY_CLIP(sbuf, xa1, y1+y); CHECK_XY_CLIP(sbuf, xb1, y1+y);
        primDrawHorzLine(sbuf, color, xa1, y1 + y, xb1, y1 + y);
nextStepBottom:
        STEPx(xa,xFRACTa,dxc,dyc,signDXc);
        STEPx(xb,xFRACTb,dxb,dyb,signDXb);
        xa1 = xa;
        xb1 = xb;
      }
done: ;
     }
}

static void
drawClippedFilledArc(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    const jshort *clip, int startQuadrant, int startRatio,
    int endQuadrant, int endRatio, int quadrantStatus[4],
    int x, int y, int w, int h) {

    int a,b;            
    int a2,b2, S, T;
    int xCenter, yCenter;
    int evenXOffset, evenYOffset;
    int x_point, y_point;
    
    int start_x, start_y, end_x, end_y, start_x1, start_x2,
        start_y1, start_y2, end_x1, end_x2, end_y1, end_y2;
    int nQuadrantsToDraw, curQuadrant, quadrantsToDraw[4];
    int point_x1, point_x2, point_y1, point_y2;
    int i,j;

    CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);

    SetUpArcEndPoints(x, y, w, h,
        startQuadrant, startRatio, endQuadrant, endRatio,
        &start_x, &start_y, &end_x, &end_y,
        &start_x1, &start_y1, &end_x1, &end_y1,
        &start_x2, &start_y2, &end_x2, &end_y2);

    SetUpEllipseParams(x, y, x + w, y + h, &a, &b, &a2, &b2, &S, &T,
        &xCenter, &yCenter, &evenXOffset, &evenYOffset);

    /* now check for fully clipped quadrants or quadrants outside of the arc
     * we won't be drawing them, so we won't check them on every pixel
     * Also - draw the triangles in the quadrants which are partially filled */
    nQuadrantsToDraw = 0;
    for (i = 0; i < 4; ++i) {
      if ((quadrantStatus[i] & QUADRANT_STATUS_NO_ARC) ||
          (quadrantStatus[i] & QUADRANT_STATUS_TOTALLY_CLIPPED)) {
        continue;
      } else {
        curQuadrant = i + 1;
        quadrantsToDraw[nQuadrantsToDraw++] = curQuadrant;
        if (quadrantStatus[i] & QUADRANT_STATUS_PARTIAL_ARC) {
          if ((curQuadrant == startQuadrant) &&
              (curQuadrant == endQuadrant) &&
              ((((curQuadrant == 1) || (curQuadrant == 3)) &&
                 (startRatio <= endRatio)) ||
                 (((curQuadrant == 2) || (curQuadrant == 4)) &&
                   (startRatio >= endRatio)))) {
            /* need to make sure that the first point
             * is the one further from the x-axis */
            if (curQuadrant == 1) {
              fill_triangle(sbuf, color, clip, 
			    end_x1, end_y1, start_x1, start_y1, 
			    xCenter, yCenter);
            } else if (curQuadrant == 2) {
              fill_triangle(sbuf, color, clip, 
			    start_x2, start_y1, end_x2, end_y1, 
			    xCenter, yCenter);
            } else if (curQuadrant == 3) {
              fill_triangle(sbuf, color, clip, 
			    end_x2, end_y2, start_x2, start_y2, 
			    xCenter, yCenter);
            } else {
              fill_triangle(sbuf, color, clip, 
			    start_x1, start_y2, end_x1, end_y2, 
			    xCenter, yCenter);
            }
          } else {
            if (curQuadrant == startQuadrant) {
              /* draw the appropriate triangle */
              if (curQuadrant == 1) {
                drawFilledRightTriangle(sbuf, color, clip,
                    start_x1, start_y1, xCenter, yCenter);
              } else if (curQuadrant == 2) {
                drawFilledRightTriangle(sbuf, color, clip,
                    xCenter - evenXOffset, yCenter, start_x2, start_y1);
              } else if (curQuadrant == 3) {
                drawFilledRightTriangle(sbuf, color, clip,
                    start_x2, start_y2, xCenter - evenXOffset,
                    yCenter + evenYOffset);
              } else {
                drawFilledRightTriangle(sbuf, color, clip,
                    xCenter, yCenter + evenYOffset, start_x1, start_y2);
              }
            }
            if (curQuadrant == endQuadrant) {
              /* draw the appropriate triangle */
              if (curQuadrant == 1) {
                drawFilledRightTriangle(sbuf, color, clip,
                    xCenter, yCenter, end_x1, end_y1);
              } else if (curQuadrant == 2) {
                drawFilledRightTriangle(sbuf, color, clip,
                    end_x2, end_y1, xCenter - evenXOffset, yCenter);
              } else if (curQuadrant == 3) {
                drawFilledRightTriangle(sbuf, color, clip,
                    xCenter - evenXOffset, yCenter + evenYOffset,
                    end_x2, end_y2);
              } else {
                drawFilledRightTriangle(sbuf, color, clip,
                    end_x1, end_y2, xCenter, yCenter + evenYOffset);
              }
            }
          }
        }
      }
    }

    /* now we loop through the ellipse co-ordinates again
     * and finish drawing what's left of the arcs after the triangles
     * we already did SetUpEllipseParams above and didn't use
     * the variables which change */
    x_point = 0;
    y_point = b;
    while (y_point >= 0) {
      point_x1 = xCenter + x_point;
      point_y1 = yCenter - y_point;
      point_x2 = xCenter - x_point - evenXOffset;
      point_y2 = yCenter + y_point + evenYOffset;

      for (j = 0; j < nQuadrantsToDraw; ++j) {
        curQuadrant = quadrantsToDraw[j];
        if (quadrantStatus[curQuadrant - 1] & QUADRANT_STATUS_FULL_ARC) {
          if (curQuadrant == 1) {
            draw_clipped_line(sbuf, color, SOLID, clip, 
                point_x1, point_y1, point_x1, yCenter);
          } else if (curQuadrant == 2) {
            draw_clipped_line(sbuf, color, SOLID, clip, 
                point_x2, point_y1, point_x2, yCenter);
          } else if (curQuadrant == 3) {
            draw_clipped_line(sbuf, color, SOLID, clip, 
                point_x2, point_y2, point_x2, yCenter + evenYOffset);
          } else {
            /* (curQuadrant == 4) */
            draw_clipped_line(sbuf, color, SOLID, clip, 
                point_x1, point_y2, point_x1, yCenter + evenYOffset);
          }
        } else {
          /* must be partial arc
           * case of pie slice fully inside quadrant */
          if ((curQuadrant == startQuadrant) &&
              (curQuadrant == endQuadrant) &&
              ((((curQuadrant == 1) || (curQuadrant == 3)) &&
                 (startRatio <= endRatio)) ||
                 (((curQuadrant == 2) || (curQuadrant == 4)) &&
                   (startRatio >= endRatio)))) {
            if ((((curQuadrant == 1) || (curQuadrant == 3)) &&
                 ((x_point <= start_x) && (y_point >= start_y)) &&
                 ((x_point >= end_x) && (y_point <= end_y))) ||
                 (((curQuadrant == 2) || (curQuadrant == 4)) &&
                  ((x_point >= start_x) && (y_point <= start_y)) &&
                  ((x_point < end_x) || (y_point > end_y)))) {
              if (curQuadrant == 1) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x1, point_y1, point_x1, yCenter - start_y);
              } else if (curQuadrant == 2) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x2, point_y1, point_x2, yCenter - end_y);
              } else if (curQuadrant == 3) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x2, point_y2, point_x2,
                    yCenter + evenYOffset + start_y);
              } else if (curQuadrant == 4) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x1, point_y2, point_x1,
                    yCenter + evenYOffset + end_y);
              }
            }
          } else {
            /* case of pie slice overlapping end of quadrant */
            if ((curQuadrant == startQuadrant) &&
               ((((curQuadrant == 1) || (curQuadrant == 3)) &&
               ((x_point <= start_x) && (y_point >= start_y))) ||
               (((curQuadrant == 2) || (curQuadrant == 4)) &&
               ((x_point >= start_x) && (y_point <= start_y))))) {

              if (curQuadrant == 1) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x1, point_y1, point_x1, yCenter - start_y);
              } else if (curQuadrant == 2) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x2, point_y1, point_x2, yCenter);
              } else if (curQuadrant == 3) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x2, point_y2, point_x2,
                    yCenter + evenYOffset + start_y);
              } else if (curQuadrant == 4) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x1, point_y2, point_x1, yCenter + evenYOffset);
              }
            }
            /* case of pie slice overlapping beginning of quadrant
             * not that this is not mutually exclusive with the previous
             * case of a pie slice overlapping the end of the quadrant.
             * For example, the slice which starts at 60 and ends at 10
             * (an "inverse" pie slice) */

	    if ((curQuadrant == endQuadrant) &&
               ((((curQuadrant == 1) || (curQuadrant == 3)) &&
               ((x_point >= end_x) && (y_point <= end_y))) ||
               (((curQuadrant == 2) || (curQuadrant == 4)) &&
               ((x_point <= end_x) && (y_point >= end_y))))) {
              if (curQuadrant == 1) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x1, point_y1, point_x1, yCenter);
              } else if (curQuadrant == 2) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x2, point_y1, point_x2, yCenter - end_y);
              } else if (curQuadrant == 3) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
				  point_x2, point_y2, 
				  point_x2, yCenter + evenYOffset);
              } else if (curQuadrant == 4) {
                draw_clipped_line(sbuf, color, SOLID, clip, 
                    point_x1, point_y2,
				  point_x1, yCenter + evenYOffset + end_y);
              }
            }
          }
        }
      }
      GetNextEllipsePoint(a2, b2, &S, &T, &x_point, &y_point);
    }
}


static void
drawClippedArc(gxj_screen_buffer *sbuf, gxj_pixel_type color,
    int lineStyle, int filled, const jshort *clip,
    int startAngle, int endAngle, int x, int y, int w, int h) {

    int startQuadrant, endQuadrant, startRatio, endRatio, quadrantStatus[4];
    int nPixelsInQuadrant;
    startQuadrant = 0;
    endQuadrant = 0;

    CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);

    GetArcHelperParams(x, y, w, h, startAngle, endAngle, clip,
        &startQuadrant, &startRatio, &endQuadrant, &endRatio,
        quadrantStatus);
            
    if (filled) {
      drawClippedFilledArc(sbuf, color, clip, 
          startQuadrant, startRatio, endQuadrant, endRatio,
          quadrantStatus, x, y, w, h);

    } else {
      nPixelsInQuadrant = (lineStyle == DOTTED) ?
	      calcNPixelsInQuadrant(x, y, w, h) : -1;
      drawClippedOutlineArc(sbuf, color, lineStyle, clip, 
          startQuadrant, startRatio, endQuadrant, endRatio,
          nPixelsInQuadrant, quadrantStatus, x, y, w, h);
    }
}

static int 
SetUpEllipseParameters(int x, int y, int width, int height,
    int *a, int *b, int *evenXOffset, int *evenYOffset,
    int *xCenter, int *yCenter, int *aSquared, int *bSquared,
    int *twoAsquared, int *twoBsquared, int *fourAsquared,
    int *fourBsquared, int *xSlope, int *ySlope,
    int *aSquaredTwo, int *bSquaredTwo) {

  if (width <= 0 || height <= 0) {
    return OUTSIDE;
  }

  if (width == 1 || height == 1) {
    if (width == 1) {
      if (height == 1) {
	return POINT;
      } else {
	return HORIZONTAL | REFLECT;
      }
    } else {
      return HORIZONTAL;
    }
  }

  *evenXOffset = width & 1;	/* 1 if width is even, else 0 */
  *evenYOffset = height & 1;	/* 1 if height is even, else 0 */
  *a = width >> 1;
  *b = height >> 1;
  *xCenter = x + *a + *evenXOffset;
  *yCenter = y + *b;
  *aSquared = *a * *a;		/* always a**2 */
  *bSquared = *b * *b;		/* always b**2 */
  *twoAsquared = *aSquared + *aSquared;	/* always 2*(a**2) */
  *twoBsquared = *bSquared + *bSquared;	/* always 2*(b**2) */
  *fourAsquared = *twoAsquared + *twoAsquared;	/* always 4*(a**2) */
  *fourBsquared = *twoBsquared + *twoBsquared;	/* always 4*(b**2) */
  *xSlope = *fourBsquared;	/* always 4*(b**2)*(x+1) */
  *ySlope = *fourAsquared * (*b - 1);	/* always 4*(a**2)*(y-1) */
  *aSquaredTwo = *aSquared >> 1;/* always (a**2)/2 */
  *bSquaredTwo = *bSquared >> 1;/* always (b**2)/2 */
  return INSIDE;
}

#define XMAJOR 0
#define YMAJOR 1

static void 
drawClippedFourWaySymetricHorizontalLines(gxj_screen_buffer *sbuf,
    const jshort *clip, int flag, int xCenter, int yCenter, int x, int y,
    int evenXOffset, int evenYOffset, gxj_pixel_type color) {

  int     x1 = xCenter - x - evenXOffset;
  int     y1 = yCenter - y;
  int     x2 = xCenter + x;
  int     y2 = yCenter + y + evenYOffset;

  if (clip == NULL) {
    if (flag == XMAJOR) {
      if (x1 != x2) {
        primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
        primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
      } else {
        PRIMDRAWPIXEL(sbuf, color, x1, y1);
        PRIMDRAWPIXEL(sbuf, color, x1, y2);
      }
    } else {
      if (y1 != y2)
        primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
      primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
    }
  } else {
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];
    
    if (clipX2 <= x1 || x2 < clipX1 ||
        clipY2 <= y1 || y2 < clipY1)
      return;
    {
    int y1Inside = (clipY1 <= y1 && y1 < clipY2);
    int y2Inside = (clipY1 <= y2 && y2 < clipY2);
    if (x1 < clipX1)
      x1 = clipX1;
    if (x2 >= clipX2)
      x2 = clipX2-1;
    if (flag == XMAJOR) {
      if (y1Inside)
        primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
      if (y2Inside)
        primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
    } else {
      if (y1Inside && (y1 != y2))
        primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
      if (y2Inside)
        primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
    }
    }
  }
}

static void 
drawClippedFourWaySymetricPixels(gxj_screen_buffer *sbuf,
    const jshort *clip, int flag, int xCenter, int yCenter, int x, int y,
    int evenXOffset, int evenYOffset, gxj_pixel_type color) {

  int     x1 = xCenter - x - evenXOffset;
  int     y1 = yCenter - y;
  int     x2 = xCenter + x;
  int     y2 = yCenter + y + evenYOffset;

    (void)flag;

  if (clip == NULL) {
    PRIMDRAWPIXEL(sbuf, color, x1, y1);
    if ((x1 != x2) || (y1 != y2))
      PRIMDRAWPIXEL(sbuf, color, x2, y2);
    if ((x1 != x2) && (y1 != y2)) {
      PRIMDRAWPIXEL(sbuf, color, x1, y2);
      PRIMDRAWPIXEL(sbuf, color, x2, y1);
    }
  } else {
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    int y1Inside = (clipY1 <= y1 && y1 < clipY2);
    int y2Inside = (clipY1 <= y2 && y2 < clipY2);
    int x1Inside = (clipX1 <= x1 && x1 < clipX2);
    int x2Inside = (clipX1 <= x2 && x2 < clipX2);
    if (x1Inside && y1Inside)
      PRIMDRAWPIXEL(sbuf, color, x1, y1);
    if ((x1 != x2) || (y1 != y2))
      if (x2Inside && y2Inside)
        PRIMDRAWPIXEL(sbuf, color, x2, y2);
    if ((x1 != x2) && (y1 != y2)) {
      if (x1Inside && y2Inside)
        PRIMDRAWPIXEL(sbuf, color, x1, y2);
      if (x2Inside && y1Inside)
        PRIMDRAWPIXEL(sbuf, color, x2, y1);
    }
  }
}

static void 
drawClippedPixel(gxj_screen_buffer *sbuf, const jshort *clip,
    gxj_pixel_type color, int x, int y) {

  if (clip == NULL) {
    PRIMDRAWPIXEL(sbuf, color, x, y);
  } else {
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    if (clipX1 <= x && x < clipX2 &&
        clipY1 <= y && y < clipY2)
      PRIMDRAWPIXEL(sbuf, color, x, y);
  }
}

static void 
drawClippedDottedPixels(gxj_screen_buffer *sbuf, const jshort *clip,
    int quadrant, int xCenter, int yCenter, int x, int y,
	int evenXOffset, int evenYOffset, gxj_pixel_type color,
	dotted_draw_state * dds) {

  int     x1, y1;

  x1 = xCenter + ((quadrant == 1 || quadrant == 2) ? x : -(x + evenXOffset));
  y1 = yCenter + ((quadrant == 1 || quadrant == 4) ? -y : (y + evenYOffset));

  if (clip == NULL) {
    drawDottedPixel(sbuf, color, x1, y1, dds);
  } else {
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    if (clipX1 <= x1 && x1 < clipX2 &&
        clipY1 <= y1 && y1 < clipY2)
      drawDottedPixel(sbuf, color, x1, y1, dds);
  }
}

static void 
drawCompleteFilledEllipse(gxj_screen_buffer *sbuf, const jshort *clip,
    gxj_pixel_type color, int x, int y, int width, int height) {

  int     evenXOffset, evenYOffset, a, b;
  int     xCenter, yCenter, aSquared, bSquared;
  int     twoAsquared, twoBsquared;
  int     fourAsquared, fourBsquared;
  int     xSlope, ySlope;
  int     aSquaredTwo, bSquaredTwo;
  int     decision;
  int     ret;

  width -= 1;
  height -= 1;
  ret = SetUpEllipseParameters(x, y, width, height, &a, &b,
             &evenXOffset, &evenYOffset, &xCenter, &yCenter,
             &aSquared, &bSquared, &twoAsquared, &twoBsquared,
             &fourAsquared, &fourBsquared, &xSlope, &ySlope,
             &aSquaredTwo, &bSquaredTwo);

  if (ret == OUTSIDE)
    return;
  if (ret & POINT) {
    drawClippedPixel(sbuf, clip, color, x, y);
    return;
  }
  if (ret & (HORIZONTAL | REFLECT)) {
    drawClippedRect(sbuf, color, SOLID /*lineStyle*/, FILLED, clip,
                    x, y, width+1, height+1);
    return;
  }
  x = 0;
  y = b;
  decision = twoBsquared - aSquared - (ySlope >> 1) - aSquaredTwo;
  /* X axis major region */
  while (decision <= ySlope) {
    if (decision > 0) {
      drawClippedFourWaySymetricHorizontalLines(sbuf, clip, XMAJOR,
              xCenter, yCenter, x, y, evenXOffset, evenYOffset, color);
      decision -= ySlope;
      y -= 1;
      ySlope -= fourAsquared;
    }
    decision += twoBsquared + xSlope;
    x += 1;
    xSlope += fourBsquared;
  }
  /* Y axis major region */
  decision += ((bSquared - aSquared) +
      (aSquaredTwo - bSquaredTwo) - (xSlope + ySlope)) >> 1;
  while (y >= 0) {
    drawClippedFourWaySymetricHorizontalLines(sbuf, clip, YMAJOR,
            xCenter, yCenter, x, y, evenXOffset, evenYOffset, color);
    if (decision <= 0) {
      decision += xSlope;
      x += 1;
      xSlope += fourBsquared;
    }
    decision += twoAsquared - ySlope;
    y -= 1;
    ySlope -= fourAsquared;
  }
}


static void 
drawCompleteDottedEllipse(gxj_screen_buffer *sbuf, const jshort *clip,
    gxj_pixel_type color, int x, int y, int width, int height) {

  int     evenXOffset, evenYOffset, a, b;
  int     xCenter, yCenter, aSquared, bSquared;
  int     twoAsquared, twoBsquared;
  int     fourAsquared, fourBsquared;
  int     xSlope, xSlopeInit, ySlope, ySlopeInit;
  int     aSquaredTwo, bSquaredTwo;
  int     decision;
  int     ret;
  int     quadrant;
  int     nPixelsPerQuadrant = 0;
  int     nPixels = 0;
  dotted_draw_state dds = START_STROKE;

  ret = SetUpEllipseParameters(x, y, width, height, &a, &b,
          &evenXOffset, &evenYOffset, &xCenter, &yCenter,
          &aSquared, &bSquared, &twoAsquared, &twoBsquared,
          &fourAsquared, &fourBsquared, &xSlopeInit, &ySlopeInit,
          &aSquaredTwo, &bSquaredTwo);
  if (ret == OUTSIDE)
    return;
  if (ret & (HORIZONTAL | REFLECT | POINT)) {
    drawClippedRect(sbuf, color, DOTTED /*lineStyle*/, NOTFILLED, clip,
                    x, y, width, height);
    return;
  }
  for (quadrant = 1; quadrant <= 4; quadrant++) {
    /*
       adjust dds at change of quadrant
       Since this algorthim always works by increasing X and Y
       we use reflection for the other three quadrants and
       we move the offset into the pattern to compensate
       for quadrant crossings.  We also take advantage of the
       fact that a simple on/off pattern can be drawn backwards
       by starting at the off segment of the pattern instead
       of the on segment.
    */
    switch (quadrant) {
      /* first quadrant is increasing X and increasing Y
       * run pattern forward 0 to nPixelsPerQuadrant */
      case 1:
        dds.solidcount = 0;
      break;
      /* second quadrant is increasing X and decreasing Y
       * run pattern backwards 2*nPixelsPerQuadrant to nPixelsPerQuadrant */
      case 2:
        nPixelsPerQuadrant = nPixels;
        dds.solidcount = (nPixelsPerQuadrant<<1) + DOTTED_SOLID_SIZE;
        /* adjust for odd (Y) axis offsets */
        dds.solidcount -= (1-evenYOffset);
      break;
      /* third quadrant is decreasing X and decreasing Y
       * run pattern forward 2*nPixelsPerQuadrant to 3*nPixelsPerQuadrant */
      case 3:
        dds.solidcount = (nPixelsPerQuadrant<<1);
        /* adjust for odd (X and Y) axis offsets */
        dds.solidcount -= (1-evenXOffset) + (1-evenYOffset);
      break;
      /* forth quadrant is increasing X and decreasing Y
       * run pattern backwards 4*nPixelsPerQuadrant to 3*nPixelsPerQuadrant */
      case 4:
        dds.solidcount = (nPixelsPerQuadrant<<2) + DOTTED_SOLID_SIZE;
        /* adjust for current and previous odd (X and Y) axis offsets */
        dds.solidcount -= ((1-evenXOffset)<<1) + ((1-evenYOffset)<<1);
      break;
    }
    dds.drawing = TRUE;
    if (dds.solidcount < 0)
      dds.solidcount = -dds.solidcount;
    dds.solidcount %= (DOTTED_SOLID_SIZE+DOTTED_EMPTY_SIZE);
    if (dds.solidcount >= DOTTED_SOLID_SIZE) {
      dds.drawing = FALSE; dds.emptycount = dds.solidcount-DOTTED_SOLID_SIZE; dds.solidcount = 0;
    }
    x = 0;
    y = b;
    xSlope = xSlopeInit;
    ySlope = ySlopeInit;
    decision = twoBsquared - aSquared - (ySlope >> 1) - aSquaredTwo;
    /* X axis major region */
    while (decision <= ySlope) {
      drawClippedDottedPixels(sbuf, clip, quadrant, xCenter, yCenter, x, y,
		    evenXOffset, evenYOffset, color, &dds);
      nPixels += 1;
      if (decision > 0) {
	decision -= ySlope;
	y -= 1;
	ySlope -= fourAsquared;
      }
      decision += twoBsquared + xSlope;
      x += 1;
      xSlope += fourBsquared;
    }
    /* Y axis major region */
    decision += ((bSquared - aSquared) +
        (aSquaredTwo - bSquaredTwo) - (xSlope + ySlope)) >> 1;
    while (y >= 0) {
      drawClippedDottedPixels(sbuf, clip, quadrant, xCenter, yCenter, x, y,
		    evenXOffset, evenYOffset, color, &dds);
      nPixels += 1;
      if (decision <= 0) {
	decision += xSlope;
	x += 1;
	xSlope += fourBsquared;
      }
      decision += twoAsquared - ySlope;
      y -= 1;
      ySlope -= fourAsquared;
    }
  }
}

static void 
drawCompleteOutlineEllipse(gxj_screen_buffer *sbuf, const jshort *clip,
    gxj_pixel_type color, int x, int y, int width, int height) {

  int     evenXOffset, evenYOffset, a, b;
  int     xCenter, yCenter, aSquared, bSquared;
  int     twoAsquared, twoBsquared;
  int     fourAsquared, fourBsquared;
  int     xSlope, ySlope;
  int     aSquaredTwo, bSquaredTwo;
  int     decision;
  int     ret;

  ret = SetUpEllipseParameters(x, y, width, height, &a, &b,
             &evenXOffset, &evenYOffset, &xCenter, &yCenter,
             &aSquared, &bSquared, &twoAsquared, &twoBsquared,
             &fourAsquared, &fourBsquared, &xSlope, &ySlope,
             &aSquaredTwo, &bSquaredTwo);
  if (ret == OUTSIDE)
    return;
  if (ret & (HORIZONTAL | REFLECT | POINT)) {
    drawClippedRect(sbuf, color, SOLID /*lineStyle*/, NOTFILLED, clip,
                    x, y, width, height);
    return;
  }
  x = 0;
  y = b;
  decision = twoBsquared - aSquared - (ySlope >> 1) - aSquaredTwo;
  /* X axis major region */
  while (decision <= ySlope) {
    drawClippedFourWaySymetricPixels(sbuf, clip, XMAJOR, xCenter, yCenter,
                              x, y, evenXOffset, evenYOffset, color);
    if (decision > 0) {
      decision -= ySlope;
      y -= 1;
      ySlope -= fourAsquared;
    }
    decision += twoBsquared + xSlope;
    x += 1;
    xSlope += fourBsquared;
  }
  /* Y axis major region */
  decision += ((bSquared - aSquared) +
      (aSquaredTwo - bSquaredTwo) - (xSlope + ySlope)) >> 1;
  while (y >= 0) {
    drawClippedFourWaySymetricPixels(sbuf, clip, YMAJOR, xCenter, yCenter,
                              x, y, evenXOffset, evenYOffset, color);
    if (decision <= 0) {
      decision += xSlope;
      x += 1;
      xSlope += fourBsquared;
    }
    decision += twoAsquared - ySlope;
    y -= 1;
    ySlope -= fourAsquared;
  }
}

/*
 * Draw an elliptical arc centered in the given rectangle.  If filled is
 * true, fill the contents of the arc; otherwise just stroke it.  The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle>
 * degrees.  arcAngle may not be negative.
 */
void
draw_arc(gxj_pixel_type color, const jshort *clip, gxj_screen_buffer *sbuf, 
	 int lineStyle, 
	 int x, int y, int width, int height,
	 int filled, int startAngle, int arcAngle)
{
  int endAngle;
  
  REPORT_CALL_TRACE(LC_LOWUI, "LCDUIdrawArc()\n");
  
  CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);

  /* first special case arcs so squishes that they are lines */
  if ((width < 2) || (height < 2)) {
    if (width < 2) {
      width = 0;
    } else {
      --width;
    }
    
    if (height < 2) {
      height = 0;
    } else {
      --height;
    }
    
    if (width == 0) {
      drawClippedVertLine(clip, sbuf, color, x, y, x, y + height);
    } else {
      drawClippedHorzLine(clip, sbuf, color, x, y, x + width, y);
    }
  }
  /* here we take care of normal size arcs */
  else if (arcAngle != 0) {
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    /* first calc normalized startAngle and end Angle */
    if ((arcAngle > 360) || (arcAngle < -360)) {
      startAngle = 0;
      endAngle = 0;
    } else {
      endAngle = startAngle + arcAngle;
        
      /* coordinates are ccw, so we make sure start is smaller than end */
      if (endAngle < startAngle) {
        int tmp = endAngle;
        endAngle = startAngle;
        startAngle = tmp;
      }
            
      while (endAngle < 0)   endAngle   += 360;
      while (startAngle < 0) startAngle += 360;
      
      startAngle %= 360;
      endAngle   %= 360;
    }
    
    /* now check for case of complete ellipse,
     * which we will draw optimized */
    if (startAngle == endAngle) {
      if ((x >= clipX1) &&
         ((x + width) < clipX2) &&
         (y >= clipY1) &&
         ((y + height) < clipY2)) {
        if (filled) {
          drawCompleteFilledEllipse(sbuf, NULL, color, x, y, width, height);
        } else if (lineStyle == DOTTED) {
          drawCompleteDottedEllipse(sbuf, NULL, color, x, y, width, height);
        } else {
          drawCompleteOutlineEllipse(sbuf, NULL, color, x, y,
              width, height);
        }
      } else {
        if (filled) {
          drawCompleteFilledEllipse(sbuf, clip, color, x, y, width, height);
        } else if (lineStyle == DOTTED) {
          drawCompleteDottedEllipse(sbuf, clip, color, x, y, width, height);
        } else {
          drawCompleteOutlineEllipse(sbuf, clip, color, x, y,
              width, height);
        }
      }
    } else {
      drawClippedArc(sbuf, color, lineStyle, filled, clip, 
          startAngle, endAngle, x, y, width, height);
    }
  }
}

static int 
SetUpRoundRectParameters(int arcWidth, int arcHeight,
    int *aSquared, int *bSquared, int *twoAsquared, int *twoBsquared,
	int *fourAsquared, int *fourBsquared, int *xSlope, int *ySlope,
	int *aSquaredTwo, int *bSquaredTwo) {

  if (arcWidth <= 0 || arcHeight <= 0)
    return OUTSIDE;
  if (arcWidth == 1 || arcHeight == 1) {
    if (arcWidth == 1) {
      if (arcHeight == 1) {
	return POINT;
      } else {
	return HORIZONTAL | REFLECT;
      }
    } else {
      return HORIZONTAL;
    }
  }

  *aSquared = arcWidth * arcWidth;	/* always a**2 */
  *bSquared = arcHeight * arcHeight;	/* always b**2 */
  *twoAsquared = *aSquared + *aSquared;	/* always 2*(a**2) */
  *twoBsquared = *bSquared + *bSquared;	/* always 2*(b**2) */
  *fourAsquared = *twoAsquared + *twoAsquared;	/* always 4*(a**2) */
  *fourBsquared = *twoBsquared + *twoBsquared;	/* always 4*(b**2) */
  *xSlope = *fourBsquared;	/* always 4*(b**2)*(x+1) */
  *ySlope = *fourAsquared * (arcHeight - 1);	/* always 4*(a**2)*(y-1) */
  *aSquaredTwo = *aSquared >> 1;/* always (a**2)/2 */
  *bSquaredTwo = *bSquared >> 1;/* always (b**2)/2 */
  return INSIDE;
}

static void 
drawClippedFourWayRoundRectPixels(gxj_screen_buffer * sbuf,
    const jshort *clip, int xOrigin, int yOrigin, int x, int y,
    int width, int height, int arcWidth, int arcHeight,
    gxj_pixel_type color) {

  int     x1 = xOrigin + arcWidth - x;
  int     y1 = yOrigin + arcHeight - y;
  int     x2 = xOrigin + width - arcWidth + x;
  int     y2 = yOrigin + height - arcHeight + y;

  if (clip == NULL) {
    PRIMDRAWPIXEL(sbuf, color, x1, y1);
    PRIMDRAWPIXEL(sbuf, color, x2, y1);
    PRIMDRAWPIXEL(sbuf, color, x1, y2);
    PRIMDRAWPIXEL(sbuf, color, x2, y2);
    if (y == arcHeight && x == 0) {
      primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
      primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
    } else if (x == arcWidth && y == 0) {
      primDrawVertLine(sbuf, color, x1, y1, x1, y2);
      primDrawVertLine(sbuf, color, x2, y1, x2, y2);
    }
  } else {
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    if (clipX2 <= x1 || x2 < clipX1 ||
        clipY2 <= y1 || y2 < clipY1)
      return;
    {
    int y1Inside = (clipY1 <= y1 && y1 < clipY2);
    int y2Inside = (clipY1 <= y2 && y2 < clipY2);
    int x1Inside = (clipX1 <= x1 && x1 < clipX2);
    int x2Inside = (clipX1 <= x2 && x2 < clipX2);
    if (y1Inside) {
      if (x1Inside) {
        PRIMDRAWPIXEL(sbuf, color, x1, y1);
      }
      if (x2Inside) {
        PRIMDRAWPIXEL(sbuf, color, x2, y1);
      }
    }
    if (y2Inside) {
      if (x1Inside) {
        PRIMDRAWPIXEL(sbuf, color, x1, y2);
      }
      if (x2Inside) {
        PRIMDRAWPIXEL(sbuf, color, x2, y2);
      }
    }
    if (y == arcHeight && x == 0) {
      if (y1Inside) {
        drawClippedHorzLine(clip, sbuf, color, x1, y1, x2, y1);
      }
      if (y2Inside) {
        drawClippedHorzLine(clip, sbuf, color, x1, y2, x2, y2);
      }
    } else if (x == arcWidth && y == 0) {
      if (x1Inside) {
        drawClippedVertLine(clip, sbuf, color, x1, y1, x1, y2);
      }
      if (x2Inside) {
        drawClippedVertLine(clip, sbuf, color, x2, y1, x2, y2);
      }
    }
    }
  }
}

static void 
drawClippedFourWayRoundRectLines(gxj_screen_buffer * sbuf,
    const jshort *clip, int xOrigin, int yOrigin, int x, int y,
    int width, int height, int arcWidth, int arcHeight,
    gxj_pixel_type color) {

  int     x1 = xOrigin + arcWidth - x;
  int     y1 = yOrigin + arcHeight - y;
  int     x2 = xOrigin + width - arcWidth + x;
  int     y2 = yOrigin + height - arcHeight + y;

  if (clip == NULL) {
    if (x != arcWidth) {
      primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
      primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
    } else {
      primDrawFilledRect(sbuf, color, x1, y1, x2+1, y2+1);
    }
  } else {
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    if (clipX2 <= x1 || x2 < clipX1 ||
        clipY2 <= y1 || y2 < clipY1)
      return;
    if (x1 < clipX1)
      x1 = clipX1;
    if (x2 >= clipX2)
      x2 = clipX2-1;
    if (x != arcWidth) {
      if (clipY1 <= y1 && y1 < clipY2)
        primDrawHorzLine(sbuf, color, x1, y1, x2, y1);
      if (clipY1 <= y2 && y2 < clipY2)
        primDrawHorzLine(sbuf, color, x1, y2, x2, y2);
    } else {
      if (y1 < clipY1)
        y1 = clipY1;
      if (y2 >= clipY2)
        y2 = clipY2-1;
      primDrawFilledRect(sbuf, color, x1, y1, x2+1, y2+1);
    }
  }
}

static void 
drawCompleteOutlineRoundRect(gxj_screen_buffer * sbuf, const jshort *clip,
    gxj_pixel_type color, int x, int y, int width, int height,
    int arcWidth, int arcHeight) {

  int     aSquared, bSquared;
  int     twoAsquared, twoBsquared;
  int     fourAsquared, fourBsquared;
  int     xSlope, ySlope;
  int     aSquaredTwo, bSquaredTwo;
  int     xOrigin = x;
  int     yOrigin = y;
  int     decision;
  int     ret;

  ret = SetUpRoundRectParameters(arcWidth, arcHeight, &aSquared, &bSquared,
		   &twoAsquared, &twoBsquared, &fourAsquared, &fourBsquared,
			      &xSlope, &ySlope, &aSquaredTwo, &bSquaredTwo);
  if (ret == OUTSIDE)
    return;
  if (ret & (HORIZONTAL | REFLECT | POINT)) {
    drawClippedRect(sbuf, color, SOLID /*lineStyle*/, NOTFILLED, clip,
                    x, y, width, height);
    return;
  }
  x = 0;
  y = arcHeight;
  decision = twoBsquared - aSquared - (ySlope >> 1) - aSquaredTwo;
  /* X axis major region */
  while (decision <= ySlope) {
    drawClippedFourWayRoundRectPixels(sbuf, clip, xOrigin, yOrigin,
                           x, y, width, height, arcWidth, arcHeight, color);
    if (decision > 0) {
      decision -= ySlope;
      y -= 1;
      ySlope -= fourAsquared;
    }
    decision += twoBsquared + xSlope;
    x += 1;
    xSlope += fourBsquared;
  }
  /* Y axis major region */
  decision += ((bSquared - aSquared) +
      (aSquaredTwo - bSquaredTwo) - (xSlope + ySlope)) >> 1;
  while (y >= 0) {
    drawClippedFourWayRoundRectPixels(sbuf, clip, xOrigin, yOrigin,
        x, y, width, height, arcWidth, arcHeight, color);
    if (decision <= 0) {
      decision += xSlope;
      x += 1;
      xSlope += fourBsquared;
    }
    decision += twoAsquared - ySlope;
    y -= 1;
    ySlope -= fourAsquared;
  }
}

static void
drawClippedFourWayDottedRoundRectPixels(gxj_screen_buffer *sbuf,
    const jshort *clip, int xOrigin, int yOrigin, int x, int y,
    int width, int height, int arcWidth, int arcHeight,
    gxj_pixel_type color, dotted_draw_state * dds) {

  int     x1 = xOrigin + arcWidth - x;
  int     y1 = yOrigin + arcHeight - y;
  int     x2 = xOrigin + width - arcWidth + x;
  int     y2 = yOrigin + height - arcHeight + y;

  if (clip == NULL) {
    /* Draw rect sides */
    if (y == arcHeight && x == 0) {
      drawDottedHorzLine(sbuf, color, x1, y1, x2, y1, dds[0]);
      drawDottedHorzLine(sbuf, color, x1, y2, x2, y2, dds[2]);
    } else if (x == arcWidth && y == 0) {
      drawDottedVertLine(sbuf, color, x1, y1, x1, y2, dds[0]);
      drawDottedVertLine(sbuf, color, x2, y1, x2, y2, dds[1]);
    }
    /* Draw rect corners */
    drawDottedPixel(sbuf, color, x1, y1, &(dds[0]));
    drawDottedPixel(sbuf, color, x2, y1, &(dds[1]));
    drawDottedPixel(sbuf, color, x1, y2, &(dds[2]));
    drawDottedPixel(sbuf, color, x2, y2, &(dds[3]));

  } else {
    const jshort clipX1 = clip[0];
    const jshort clipY1 = clip[1];
    const jshort clipX2 = clip[2];
    const jshort clipY2 = clip[3];

    if (clipX2 <= x1 || x2 < clipX1 ||
        clipY2 <= y1 || y2 < clipY1)
      return;

    {
      int y1Inside = (clipY1 <= y1 && y1 < clipY2);
      int y2Inside = (clipY1 <= y2 && y2 < clipY2);
      int x1Inside = (clipX1 <= x1 && x1 < clipX2);
      int x2Inside = (clipX1 <= x2 && x2 < clipX2);

      /* Draw clipped rect sides */
      if (y == arcHeight && x == 0) {
        if (y1Inside) {
          drawClippedDottedHorzLine(clip, sbuf, color,
            x1, y1, x2, y1, dds[0]);
        }
        if (y2Inside) {
          drawClippedDottedHorzLine(clip, sbuf, color,
            x1, y2, x2, y2, dds[2]);
        }
      } else if (x == arcWidth && y == 0) {
        if (x1Inside) {
          drawClippedDottedVertLine(clip, sbuf, color,
            x1, y1, x1, y2, dds[0]);
        }
        if (x2Inside) {
          drawClippedDottedVertLine(clip, sbuf, color,
            x2, y1, x2, y2, dds[1]);
        }
      }

      /* Draw rect corners */
      if (y1Inside) {
        if (x1Inside) {
          drawDottedPixel(sbuf, color, x1, y1, &(dds[0]));
        }
        if (x2Inside) {
          drawDottedPixel(sbuf, color, x2, y1, &(dds[1]));
        }
      }
      if (y2Inside) {
        if (x1Inside) {
          drawDottedPixel(sbuf, color, x1, y2, &(dds[2]));
        }
        if (x2Inside) {
          drawDottedPixel(sbuf, color, x2, y2, &(dds[3]));
        }
      }
    }
  }
}

static void
SetUpDottedRoundRectParams(int x, int y, int width, int height,
        int arcWidth, int arcHeight, dotted_draw_state * dds)
{
    int nPixels;
    int horzLinePixels = width - 2*arcWidth;
    int vertLinePixels = height - 2*arcHeight;
    int nPixelsInArc = calcNPixelsInQuadrant(
        x, y-arcWidth, arcWidth, arcHeight);

    /* 0-th quadrant starts drawing the pattern from the half
     * stroke in the left upper corner moving counter-clockwise;
     * also initialize here stroke states for other quadrants */
    dds[0] = dds[1] = dds[2] = dds[3] = HALF_STROKE;

    /* 1-st quadrant starts drawing in
     * the right upper corner moving clockwise */
    nPixels = horzLinePixels;
    SetUpDottedParamsByPixels(nPixels, &(dds[1]));

    /* 2-nd quadrant starts drawing the pattern in
     * the left bottom corner moving clockwise */
    nPixels = vertLinePixels + 2*nPixelsInArc;
    SetUpDottedParamsByPixels(nPixels, &(dds[2]));

    /* 3-rd quadrant starts drawing in
     * the right bottom corner moving counter-clockwise */
    nPixels = vertLinePixels + 2*nPixelsInArc + horzLinePixels;
    SetUpDottedParamsByPixels(nPixels, &(dds[3]));
}

static void
drawCompleteDottedRoundRect(gxj_screen_buffer * sbuf, const jshort *clip,
    gxj_pixel_type color, int x, int y, int width, int height,
    int arcWidth, int arcHeight) {

  dotted_draw_state dds[4];
  int     aSquared, bSquared;
  int     twoAsquared, twoBsquared;
  int     fourAsquared, fourBsquared;
  int     xSlope, ySlope;
  int     aSquaredTwo, bSquaredTwo;
  int     xOrigin = x;
  int     yOrigin = y;
  int     decision;
  int     ret;

  ret = SetUpRoundRectParameters(arcWidth, arcHeight, &aSquared, &bSquared,
	  &twoAsquared, &twoBsquared, &fourAsquared, &fourBsquared,
	  &xSlope, &ySlope, &aSquaredTwo, &bSquaredTwo);
  if (ret == OUTSIDE)
    return;
  if (ret & (HORIZONTAL | REFLECT | POINT)) {
    drawClippedRect(sbuf, color, DOTTED /*lineStyle*/, NOTFILLED,
        clip, x, y, width, height);
    return;
  }

  SetUpDottedRoundRectParams(x, y, width, height,
      arcWidth, arcHeight, dds);

  x = 0;
  y = arcHeight;
  decision = twoBsquared - aSquared - (ySlope >> 1) - aSquaredTwo;
  /* X axis major region */
  while (decision <= ySlope) {
    drawClippedFourWayDottedRoundRectPixels(sbuf, clip, xOrigin, yOrigin,
        x, y, width, height, arcWidth, arcHeight, color, dds);
    if (decision > 0) {
      decision -= ySlope;
      y -= 1;
      ySlope -= fourAsquared;
    }
    decision += twoBsquared + xSlope;
    x += 1;
    xSlope += fourBsquared;
  }
  /* Y axis major region */
  decision += ((bSquared - aSquared) +
      (aSquaredTwo - bSquaredTwo) - (xSlope + ySlope)) >> 1;
  while (y >= 0) {
    drawClippedFourWayDottedRoundRectPixels(sbuf, clip, xOrigin, yOrigin,
        x, y, width, height, arcWidth, arcHeight, color, dds);

    if (decision <= 0) {
      decision += xSlope;
      x += 1;
      xSlope += fourBsquared;
    }
    decision += twoAsquared - ySlope;
    y -= 1;
    ySlope -= fourAsquared;
  }
}

static void
drawCompleteFilledRoundRect(gxj_screen_buffer * sbuf, const jshort *clip,
    gxj_pixel_type color, int x, int y, int width, int height,
    int arcWidth, int arcHeight) {

  int     aSquared, bSquared;
  int     twoAsquared, twoBsquared;
  int     fourAsquared, fourBsquared;
  int     xSlope, ySlope;
  int     aSquaredTwo, bSquaredTwo;
  int     xOrigin = x;
  int     yOrigin = y;
  int     decision;
  int     ret;

  width -= 1;
  height -= 1;
  ret = SetUpRoundRectParameters(arcWidth, arcHeight, &aSquared, &bSquared,
     &twoAsquared, &twoBsquared, &fourAsquared, &fourBsquared,
     &xSlope, &ySlope, &aSquaredTwo, &bSquaredTwo);
  if (ret == OUTSIDE)
      return;
  if (ret & (HORIZONTAL | REFLECT | POINT)) {
      drawClippedRect(sbuf, color, SOLID /*lineStyle*/, FILLED, clip,
                      x, y, width, height);
      return;
  }
  x = 0;
  y = arcHeight;
  decision = twoBsquared - aSquared - (ySlope >> 1) - aSquaredTwo;
  /* X axis major region */
  while (decision <= ySlope) {
    if (decision > 0) {
      drawClippedFourWayRoundRectLines(sbuf, clip, xOrigin, yOrigin, x, y,
          width, height, arcWidth, arcHeight, color);
      decision -= ySlope;
      y -= 1;
      ySlope -= fourAsquared;
    }
    decision += twoBsquared + xSlope;
    x += 1;
    xSlope += fourBsquared;
  }
  /* Y axis major region */
  decision += ((bSquared - aSquared) +
      (aSquaredTwo - bSquaredTwo) - (xSlope + ySlope)) >> 1;
  while (y >= 0) {
    drawClippedFourWayRoundRectLines(sbuf, clip, xOrigin, yOrigin, x, y,
        width, height, arcWidth, arcHeight, color);
    if (decision <= 0) {
      decision += xSlope;
      x += 1;
      xSlope += fourBsquared;
    }
    decision += twoAsquared - ySlope;
    y -= 1;
    ySlope -= fourAsquared;
  }
}

/*
 * Draw a rectangle at (x,y) with the given width and height.  If filled
 * is true, fill the contents; otherwise just stroke it.  arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 * arcWidth is horizontal radius of the arc at the four, arcHeight is
 * vertical radius of the arc at the four
 */
void
draw_roundrect(gxj_pixel_type color, const jshort *clip, 
	       gxj_screen_buffer *sbuf,
	       int lineStyle, 
	       int x, int y, int width, int height,
	       int filled, int arcWidth, int arcHeight)
{
  int x2, y2;
  const jshort clipX1 = clip[0];
  const jshort clipY1 = clip[1];
  const jshort clipX2 = clip[2];
  const jshort clipY2 = clip[3];

  CHECK_SBUF_CLIP_BOUNDS(sbuf, clip);

  if (arcWidth < 0)
    arcWidth = 0;
  else if (arcWidth > (width>>1))
    arcWidth = width>>1;
  if (arcHeight < 0)
    arcHeight = 0;
  else if (arcHeight > (height>>1))
    arcHeight = height>>1;

  x2 = x + width;
  y2 = y + height;

  /* first do non-rounded rectangles */
  if ((arcWidth == 0) || (arcHeight == 0)) {
    /* check for unclipped
     * note that degenerate cases of lines will be inefficiently drawn */

    if ((x >= clipX1) &&
        (x2 < clipX2) &&
        (y >= clipY1) &&
        (y2 < clipY2)) {
      if (filled) {
        primDrawFilledRect(sbuf, color, x, y, x2, y2);
      } else if (lineStyle == DOTTED) {
        primDrawDottedRect(sbuf, color, x, y, x2, y2);
      } else {
        primDrawSolidRect(sbuf, color, x, y, x2, y2);
      }
    }
    /* clipped, non-rounded rectangles */
    else {
      drawClippedRect(sbuf, color, lineStyle, filled, clip,
          x, y, width, height);
    }
  }
  /* rounded rects */
  else {
    if ((x >= clipX1) &&
        (x2 < clipX2) &&
        (y >= clipY1) &&
        (y2 < clipY2)) {
      if (filled) {
        drawCompleteFilledRoundRect(sbuf, NULL, color, x, y,
            width, height, arcWidth, arcHeight);
      } else if (lineStyle == DOTTED) {
        drawCompleteDottedRoundRect(sbuf, NULL, color, x, y,
            width, height, arcWidth, arcHeight);
      } else {
        drawCompleteOutlineRoundRect(sbuf, NULL, color, x, y,
            width, height, arcWidth, arcHeight);
      }
      return;
    }
    if (filled) {
      drawCompleteFilledRoundRect(sbuf, clip, color, x, y,
          width, height, arcWidth, arcHeight);
    } else if (lineStyle == DOTTED) {
      drawCompleteDottedRoundRect(sbuf, clip, color, x, y,
          width, height, arcWidth, arcHeight);
    } else {
      drawCompleteOutlineRoundRect(sbuf, clip, color, x, y,
          width, height, arcWidth, arcHeight);
    }
  }
  return;
}
