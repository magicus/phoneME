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

#ifndef _GXAPI_GRAPHICS_H_
#define _GXAPI_GRAPHICS_H_

#include <commonKNIMacros.h>
#include <ROMStructs.h>

/**
 * @defgroup lowui Low Level UI
 * @ingroup subsystems
 */

/**
 * @defgroup lowui_gxapi Graphics External Interface
 * @ingroup lowui
 */

/**
 * @file
 * @ingroup lowui_gxapi
 *
 * @brief Porting api for graphics_api library
 */

/**
 * Structure representing the <tt>Graphics</tt> class.
 */
typedef struct Java_javax_microedition_lcdui_Graphics   java_graphics;

/**
 * Get a C structure representing the given <tt>Graphics</tt> class.
 */
#define GXAPI_GET_GRAPHICS_PTR(handle)       (unhand(java_graphics,(handle)))



#endif /* _GXAPI_GRAPHICS_H_ */
