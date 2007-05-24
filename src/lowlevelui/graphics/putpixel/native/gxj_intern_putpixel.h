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

#ifndef _GXJ_INTERN_IMAGE_PUTPIXEL_H_
#define _GXJ_INTERN_IMAGE_PUTPIXEL_H_

#include <gxj_putpixel.h>

/*
 * Enable silent screen buffer clipping checks in all prim* functions
 */
#define PRIM_CLIPPING        1

#if ENABLE_BOUNDS_CHECKS
#define CHECK_SBUF_CLIP_BOUNDS(_sbuf, clip) \
  do { \
   (_sbuf)->g = NULL; \
   if ((clip[0] < 0) || (clip[0] > (_sbuf)->width) || \
       (clip[1] < 0) || (clip[1] > (_sbuf)->height) || \
       (clip[2] < 0) || (clip[2] > (_sbuf)->width) || \
       (clip[3] < 0) || (clip[3] > (_sbuf)->height)) \
      reportToLog(LOG_CRITICAL, LC_LOWUI, "Clip outside gxj_screen_buffer at file %s, line %d, clipX1=%d,clipY1=%d,clipX2=%d,clipY2=%d,width=%d,height=%d\n",__FILE__,__LINE__,clip[0],clip[1],clip[2],clip[3],(_sbuf)->width,(_sbuf)->height); \
  } \
  } while (0)
#define CHECK_XY_CLIP(_sbuf,_x,_y) \
  do { \
    if (((_sbuf)->g != NULL) && \
       ((((_sbuf)->g)->clipX1 > (_x)) || (((_sbuf)->g)->clipX2 <= (_x)) || \
       (((_sbuf)->g)->clipY1 > (_y)) || (((_sbuf)->g)->clipY2 <= (_y)))) \
      reportToLog(LOG_CRITICAL, LC_LOWUI, "XY outside Clip at file %s, line %d, clipX1=%d,clipY1=%d,clipX2=%d,clipY2=%d,x=%d,y=%d\n",__FILE__,__LINE__,((_sbuf)->g)->clipX1,((_sbuf)->g)->clipY1,((_sbuf)->g)->clipX2,((_sbuf)->g)->clipY2,(_x),(_y)); \
   if (((_x) < 0) || (((_sbuf)->width) <= (_x)) || \
       ((_y) < 0) || (((_sbuf)->height) <= (_y))) \
      reportToLog(LOG_CRITICAL, LC_LOWUI, "XY outside gxj_screen_buffer at file %s, line %d, clipX1=%d,clipY1=%d,clipX2=%d,clipY2=%d,width=%d,height=%d,x=%d,y=%d\n",__FILE__,__LINE__,((_sbuf)->g)->clipX1,((_sbuf)->g)->clipY1,((_sbuf)->g)->clipX2,((_sbuf)->g)->clipY2,(_sbuf)->width,(_sbuf)->height,(_x),(_y)); \
  } while (0)
#define CHECK_PTR_CLIP(_sbuf,_ptr) \
  do { \
   int _idx = ((gxj_pixel_type*)_ptr)-(_sbuf)->pixelData; \
   int _y = (_idx)/(_sbuf)->width; \
   int _x = (_idx)%(_sbuf)->width; \
   if ((((unsigned int)(_ptr) & (1))) || \
       ((((_sbuf)->g != NULL) && (((_sbuf)->g)->clipX1 > (_x))) || \
       (((_sbuf)->g)->clipX2 <= (_x+0)) || \
       (((_sbuf)->g)->clipY1 > (_y)) || (((_sbuf)->g)->clipY2 <= (_y)))) \
      reportToLog(LOG_CRITICAL, LC_LOWUI, "Pointer outside Clip at file %s, line %d, clipX1=%d,clipY1=%d,clipX2=%d,clipY2=%d,pointer=%p,index=%d,x=%d,y=%d\n",__FILE__,__LINE__,((_sbuf)->g)->clipX1,((_sbuf)->g)->clipY1,((_sbuf)->g)->clipX2,((_sbuf)->g)->clipY2,(_ptr),(_idx),(_x),(_y)); \
   if (((_x) < 0) || (((_sbuf)->width) <= (_x)) || \
       ((_y) < 0) || (((_sbuf)->height) <= (_y))) \
      reportToLog(LOG_CRITICAL, LC_LOWUI, "Pointer outside gxj_screen_buffer at file %s, line %d, clipX1=%d,clipY1=%d,clipX2=%d,clipY2=%d,width=%d,height=%d,x=%d,y=%d\n",__FILE__,__LINE__,((_sbuf)->g)->clipX1,((_sbuf)->g)->clipY1,((_sbuf)->g)->clipX2,((_sbuf)->g)->clipY2,(_sbuf)->width,(_sbuf)->height,(_x),(_y)); \
  } while (0)
#define CHECK_LLPTR_CLIP(_sbuf,_ptr) \
  do { \
   int _idx = ((gxj_pixel_type*)_ptr)-(_sbuf)->pixelData; \
   int _y = (_idx)/(_sbuf)->width; \
   int _x = (_idx)%(_sbuf)->width; \
   if ((((unsigned int)(_ptr) & (3))) || \
       ((((_sbuf)->g != NULL) && (((_sbuf)->g)->clipX1 > (_x))) || \
       (((_sbuf)->g)->clipX2 <= (_x+3)) || \
       (((_sbuf)->g)->clipY1 > (_y)) || (((_sbuf)->g)->clipY2 <= (_y)))) \
      reportToLog(LOG_CRITICAL, LC_LOWUI, "Pointer outside Clip at file %s, line %d, clipX1=%d,clipY1=%d,clipX2=%d,clipY2=%d,pointer=%p,index=%d,x=%d,y=%d\n",__FILE__,__LINE__,((_sbuf)->g)->clipX1,((_sbuf)->g)->clipY1,((_sbuf)->g)->clipX2,((_sbuf)->g)->clipY2,(_ptr),(_idx),(_x),(_y)); \
   if (((_x) < 0) || (((_sbuf)->width) <= (_x)) || \
       ((_y) < 0) || (((_sbuf)->height) <= (_y))) \
      reportToLog(LOG_CRITICAL, LC_LOWUI, "Pointer outside gxj_screen_buffer at file %s, line %d, clipX1=%d,clipY1=%d,clipX2=%d,clipY2=%d,width=%d,height=%d,x=%d,y=%d\n",__FILE__,__LINE__,((_sbuf)->g)->clipX1,((_sbuf)->g)->clipY1,((_sbuf)->g)->clipX2,((_sbuf)->g)->clipY2,(_sbuf)->width,(_sbuf)->height,(_x),(_y)); \
  } while (0)
#else
#define CHECK_SBUF_CLIP_BOUNDS(_sbuf, _graphics)
#define CHECK_XY_CLIP(_sbuf,_x,_y)
#define CHECK_PTR_CLIP(_sbuf,_ptr)
#define CHECK_LLPTR_CLIP(_sbuf,_ptr)
#endif

#ifndef max
#define max(x,y)        x > y ? x :y
#endif

#ifndef min
#define min(x,y)        x > y ? y: x
#endif

#ifndef MAXINT32
#define MAXINT32 ((int)0x7FFFFFFF)
#endif

/* flags for line types */
#define SOLID 0
#define DOTTED 1

#define NOTFILLED 0
#define FILLED    1

#ifndef NULL
#define NULL   0
#endif
 
#ifndef FALSE
#define FALSE   0
#endif

#ifndef TRUE
#define TRUE   1
#endif
 
/**
 * draw pixel at pixelData[y*width+x]
 * partial clipping to sbuf[width,height] and (y*width+x < 0)
 */
#if PRIM_CLIPPING
#define PRIMDRAWPIXEL(_sbuf,_color,_x,_y) \
  do { \
    int _i= (_y) * (_sbuf)->width + (_x); \
    if (((_x) < (_sbuf)->width) && ((_y) < (_sbuf)->height) && (_i>=0)) { \
      CHECK_XY_CLIP((_sbuf),(_x),(_y)); \
      (_sbuf)->pixelData[_i] = (_color); \
    } \
  } while (0)
#else
#define PRIMDRAWPIXEL(_sbuf,_color,_x,_y) \
  do { \
    int _i= (_y) * (_sbuf)->width + (_x); \
    CHECK_XY_CLIP((_sbuf),(_x),(_y)); \
    (_sbuf)->pixelData[_i] = (_color); \
  } while (0)
#endif

void draw_clipped_line(gxj_screen_buffer *sbuf, 
                       gxj_pixel_type color, int lineStyle,
                       const jshort *clip, int x1, int y1, int x2, int y2);

void draw_clipped_rect(gxj_screen_buffer *sbuf, 
                       gxj_pixel_type color, int lineStyle, 
                       int filled, const jshort *clip, 
                       int x, int y, int width, int height);

void fill_triangle(gxj_screen_buffer *sbuf, gxj_pixel_type color, 
                        const jshort *clip, 
                        int x1, int y1, int x2, int y2, int x3, int y3);

void draw_arc(gxj_pixel_type color, const jshort *clip, 
              gxj_screen_buffer *sbuf,
              int lineStyle, int x, int y, int width, int height,
	      int filled, int startAngle, int arcAngle);

void draw_roundrect(gxj_pixel_type color, const jshort *clip, 
                    gxj_screen_buffer *sbuf,
                    int lineStyle, int x, int y,
		    int width, int height,
		    int filled, int arcWidth, int arcHeight);

#endif /* _MIDPINTERN_PUTPIXEL_H_ */
