/*
 * 
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
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


#ifndef PISCES_TRANSFORM_H
#define PISCES_TRANSFORM_H

#include <PiscesDefs.h>

typedef struct _Transform4 {
  jint m00;
  jint m01;
  jint m10;
  jint m11;

} Transform4;

typedef struct _Transform6 {
  jint m00;
  jint m01;
  jint m10;
  jint m11;
  jint m02;
  jint m12;

} Transform6;

void pisces_transform_assign(Transform6* transformD, const Transform6* transformS);
void pisces_transform_invert(Transform6* transform);
void pisces_transform_multiply(Transform6* transformD, const Transform6* transformS);

#endif
