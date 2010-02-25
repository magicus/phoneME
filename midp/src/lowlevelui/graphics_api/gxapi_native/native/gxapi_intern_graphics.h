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

#ifndef _GXAPI_INTERN_GRAPHICS_H_
#define _GXAPI_INTERN_GRAPHICS_H_

#include <commonKNIMacros.h>
#include <ROMStructs.h>

#include <midpEventUtil.h>

#include <gx_image.h>
#include <gx_graphics.h>


/**
 * Structure representing the <tt>Font</tt> class.
 */
typedef struct Java_javax_microedition_lcdui_Font       _MidpFont;


/**
 * Structure representing the <tt>String</tt> class.
 */
typedef struct Java_java_lang_String _JavaString;


/**
 * Get a C structure representing the given <tt>Font</tt> class.
 */
#define GET_FONT_PTR(handle)           (unhand(_MidpFont,(handle)))


/**
 * Get a C structure representing the given <tt>String</tt> class.
 */
#define GET_STRING_PTR(handle) (unhand(_JavaString,(handle)))


/**
 * Gets the line style of the given graphics object.
 *
 * @param G handle to the <tt>Graphics</tt> object
 */
#define GET_LINESTYLE(G)  \
    GXAPI_GET_GRAPHICS_PTR((G))->style

/**
 * Gets the pixel of the given graphics object.
 *
 * @param G handle to the <tt>Graphics</tt> object
 */
#define GET_PIXEL(G)      \
    GXAPI_GET_GRAPHICS_PTR((G))->pixel

/**
 * Gets the <tt>Font</tt> object of the <tt>Graphics</tt> object.
 *
 * @param G handle to the <tt>Graphics</tt> object
 * @param F a declared, but not defined, object handle to store
 *        the <tt>Font</tt> object; this macro sets the value of F
 */
#define GET_FONT(G, F)          \
    (GET_FONT_PTR((F)) = GXAPI_GET_GRAPHICS_PTR((G))->currentFont)

/**
 * Sets the face, style, and size values of the given handle's
 * <tt>Font</tt> object.
 *
 * @note Before using this macro, the variables
 * <tt>face</tt>, <tt>style</tt>, and <tt>size</tt> must be declared.
 *
 * @param F a handle to the <tt>Font</tt> object
 */
#define DECLARE_FONT_PARAMS(F)  \
    face  = GET_FONT_PTR((F))->face; \
    style = GET_FONT_PTR((F))->style; \
    size  = GET_FONT_PTR((F))->size

/**
 * Determines if Graphics operation is allowed on this
 * graphics object.
 * If the target is an offscreen image, operation is always
 * allowed. If target is null, then it is really intended for
 * physical screen, only allowed if the display is in foreground.
 *
 * @param G - Handle to the <tt>Graphics</tt> object.
 */
#if ENABLE_MULTIPLE_ISOLATES
#define GRAPHICS_OP_IS_ALLOWED(G) \
    (GXAPI_GET_GRAPHICS_PTR(G)->img != NULL || \
     midpHasForeground(GXAPI_GET_GRAPHICS_PTR(G)->displayId))
#else
#define GRAPHICS_OP_IS_ALLOWED(G) 1
#endif

/**
 * Checks that the anchor is set correctly.
 *
 * @param anchor The anchor type.
 * @param illegal_vpos The vertical component that 
 *                     is not allowed for this anchor.
 * @return 1 if anchor is valid, 0 if invalid
 *
 */
int check_anchor(int anchor, int illegal_vpos);

/**
 * Normalizes anchor coordinates to top-left coordinates.
 *
 * @return 1 if anchor is valid, 0 if invalid
 */
int normalize_anchor(jint* X, jint* Y, jint width, jint height, jint anchor);

#endif /* _GXAPI_INTERN_GRAPHICS_H_ */
