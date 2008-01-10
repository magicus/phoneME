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

#include <commonKNIMacros.h>

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
