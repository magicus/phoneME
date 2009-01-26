/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

#ifndef _JSR239_KNI_INTERFACE_H
#define _JSR239_KNI_INTERFACE_H

#include <kni.h>

#define STRATEGY_USE_WINDOW  0
#define STRATEGY_USE_PIXMAP  1
#define STRATEGY_USE_PBUFFER 2

typedef struct {
  int width;
  int height;
  int stride;
  unsigned char rSize;
  unsigned char gSize;
  unsigned char bSize;
  unsigned char aSize;
  unsigned char rOffset;
  unsigned char gOffset;
  unsigned char bOffset;
  unsigned char aOffset;
  void *pixels;        /* 8888 RGBA buffer, used by OpenGL ES */

  void *screen_buffer; /* 565 RGB or 8888 ARGB buffer, used in another implementation */
  int pixelBytes;
  int atab[256], rtab[256], gtab[256], btab[256];
} JSR239_Pixmap;


#ifdef __cplusplus
extern "C" {
#endif

/* Initialization */

void JSR239_initialize();
void JSR239_shutdown();

/* Native window */

jint JSR239_getWindowStrategy(jobject winGraphicsHandle);
jint JSR239_getWindowNativeID(jobject winGraphicsHandle);

/* Pixmap */

JSR239_Pixmap *JSR239_getWindowPixmap(jobject winGraphicsHandle,
                                      jint width, jint height,
                                      jint pixelBytes,
                                      jint redSize, jint greenSize,
                                      jint blueSize, jint alphaSize);
JSR239_Pixmap *JSR239_getImagePixmap(jobject imageGraphicsHandle,
                                     jint width, jint height,
                                     jint pixelBytes,
                                     jint redSize, jint greenSize,
                                     jint blueSize, jint alphaSize);
void JSR239_destroyPixmap(JSR239_Pixmap *pixmap);

/* Window content */

void JSR239_getWindowContents(JSR239_Pixmap *dst,
                              jobject srcGraphicsHandle, 
                              jint srcWidth, jint srcHeight,
                              jint deltaHeight);
void JSR239_putWindowContents(jobject dst, jint delta_height,
                              JSR239_Pixmap *src, 
                              jint clipX, jint clipY, jint clipWidth, jint clipHeight,
                              jint flipY);

/* Screem buffer */

void
copyFromScreenBuffer(JSR239_Pixmap *dst,
                     void *src, int srcWidth, int srcHeight,
                     int deltaHeight);

void
copyToScreenBuffer(JSR239_Pixmap *src, jint deltaHeight, 
                   jint clipX, jint clipY, jint clipWidth, jint clipHeight,
                   jint dstWidth, jint dstHeight,
                   jint flipY);

/* Memory management */

void JSR239_memcpy(void *dst, void *src, int nbytes);
void *JSR239_malloc(int size);
void JSR239_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* #ifdef _JSR239_KNI_INTERFACE_H */
