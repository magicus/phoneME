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

#ifndef _GXUTL_GRAPHICS_H_
#define _GXUTL_GRAPHICS_H_

#include <kni.h>


/**
 * @defgroup lowui_gxutl Graphics Utilities External Interface
 * @ingroup lowui
 */

/**
 * @file
 * @ingroup lowui_gxutl
 *
 * @brief Porting api for graphics_util library
 */

/**
 * @name Reference point locations, from Graphics.java
 * @{
 */
#define HCENTER   1
#define VCENTER   2
#define LEFT      4
#define RIGHT     8
#define TOP      16
#define BOTTOM   32
#define BASELINE 64
/** @} */

/**
 * @name Flags for line types
 * @{
 */
#define SOLID 0
#define DOTTED 1
/** @} */

/**
 * @name Various transformations possible for sprites and images
 * The transform functions as defined
 * in the javax.microedition.lcdui.game.Sprite class
 * @{
 */
#define TRANS_NONE           0
#define TRANS_MIRROR_ROT180  1
#define TRANS_MIRROR         2
#define TRANS_ROT180         3
#define TRANS_MIRROR_ROT270  4
#define TRANS_ROT90          5
#define TRANS_ROT270         6
#define TRANS_MIRROR_ROT90   7
/** @} */

/**
 * defines the bit denoting that the x- and y-
 * axes are interchanged for this transformation
 */
#define TRANSFORM_INVERTED_AXES 0x4

/**
 * Defines the bit denoting that the direction
 * of the x-axis is inverted for this transformation
 */
#define TRANSFORM_X_FLIP 0x2

/**
 * Defines the bit denoting that the direction
 * of the y-axis is inverted for this transformation
 */
#define TRANSFORM_Y_FLIP 0x1

/**
 * @name Flags for LCDUIgetDisplayParams
 * @{
 */
#define SUPPORTS_COLOR         1
#define SUPPORTS_POINTER       2
#define SUPPORTS_MOTION        4
#define SUPPORTS_KEYREPEAT     8
#define SUPPORTS_DOUBLEBUFFER 16
/** @} */

/**
 * @name Flags for font descriptors
 * @{
 */
#define STYLE_PLAIN         0
#define STYLE_BOLD          1
#define STYLE_ITALIC        2
#define STYLE_UNDERLINED    4

#define SIZE_SMALL          8
#define SIZE_MEDIUM         0
#define SIZE_LARGE         16

#define FACE_SYSTEM         0
#define FACE_MONOSPACE     32
#define FACE_PROPORTIONAL  64
/** @} */

#endif /* _GXUTL_GRAPHICS_H_ */
