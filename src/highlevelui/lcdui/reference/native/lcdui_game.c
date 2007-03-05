/*
 *   
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

#include <stdlib.h>

#include <commonKNIMacros.h>
#include <midpError.h>
#include <gx_image.h>

/**
 * Structure representing the <tt>GameCanvas</tt> class.
 */
typedef struct Java_javax_microedition_lcdui_game_GameCanvas _MidpGameCanvas;

/**
 * Get a C structure representing the given <tt>GameCanvas</tt> class. */
#define GET_GAMECANVAS_PTR(handle) (unhand(_MidpGameCanvas,(handle)))

/**
 * FUNCTION:      setSuppressKeyEvents(Ljavax/microedition/lcdui/Canvas;Z)V
 * CLASS:         javax.microedition.lcdui.game.GameCanvas
 * TYPE:          virtual native function
 * OVERVIEW:      Sets a private field in a public class defined in a 
 *                 different package.
 * INTERFACE (operand stack manipulation):
 *   parameters:  c                  the object whose field we are to set
 *                suppressKeyEvents  value to set the private field to
 *   returns:     <nothing>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_game_GameCanvas_setSuppressKeyEvents) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(canvas);

    KNI_GetParameterAsObject(1, canvas);

    GET_GAMECANVAS_PTR(canvas)->suppressKeyEvents = 
        KNI_GetParameterAsBoolean(2);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/** Structure representing the <tt>Image</tt> class. */
typedef struct Java_javax_microedition_lcdui_Image java_image;
/** Get a C structure representing the given <tt>Image</tt> class. */
#define GET_IMAGE_PTR(handle) (unhand(java_image,(handle)))

/**
  * FUNCTION:      resizeImage(Ljavax/microedition/lcdui/Image;IIZ)V
  * CLASS:         com.sun.midp.lcdui.GameCanvasLFImpl
  * TYPE:          virtual native function
  * OVERVIEW:      Resize image to new dimension
  * INTERFACE (operand stack manipulation):
  *   parameters:  image Image instance to resize
  *                w new width of the image
  *                h new height of the image
  *                keepContent if true the original image content will
  *                  be preserved, though it will be clipped according to
  *                  the new image geometry
  *   returns:     <nothing>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_lcdui_GameCanvasLFImpl_resizeImage) {
    int w = KNI_GetParameterAsInt(2);
    int h = KNI_GetParameterAsInt(3);
    jboolean keepContent = KNI_GetParameterAsBoolean(4);

    KNI_StartHandles(1);
    KNI_DeclareHandle(image);
    KNI_GetParameterAsObject(1, image);

    /* null checking is handled by the Java layer, but test just in case */
    if (KNI_IsNullHandle(image)) {
        KNI_ThrowNew(midpNullPointerException, NULL);
    } else {
        const java_imagedata *imageDataPtr =
            GET_IMAGE_PTR(image)->imageData;
        gx_resize_image(imageDataPtr, w, h, keepContent);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

#undef GET_IMAGE_PTR
