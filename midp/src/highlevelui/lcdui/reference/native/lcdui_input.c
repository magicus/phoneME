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

/**
 * @file
 *
 * Key definitions and methods to handle key presses and their codes.
 */

#include <commonKNIMacros.h>
#include <keymap_input.h>
#include <midpEventUtil.h>

/**
 * IMPL_NOTE:(Doxy - Change this format) 
 *
 * FUNCTION:      getKeyCode(I)I
 * CLASS:         javax.microedition.lcdui.KeyConverter
 * TYPE:          virtual native function
 * OVERVIEW:      Get the system-specific key code corresponding to
 *                 the given gameAction.
 * INTERFACE (operand stack manipulation):
 *   parameters:  gameAction   A game action
 *   returns:     The keyCode associated with that action
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_KeyConverter_getKeyCode) {
    int gameAction = KNI_GetParameterAsInt(1);

    KNI_ReturnInt(keymap_get_key_code(gameAction));
}

/**
 * IMPL_NOTE:(Doxy - Change this format) 
 * FUNCTION:      getSystemKey(I)I
 * CLASS:         javax.microedition.lcdui.KeyConverter
 * TYPE:          virtual native function
 * OVERVIEW:      Get the abstract system key that corresponds to keyCode.
 * INTERFACE (operand stack manipulation):
 *   parameters:  keyCode   A system-specific keyCode
 *   returns:     The SYSTEM_KEY_ constant for this keyCode, or 0 if none
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_KeyConverter_getSystemKey) {
    int gameAction = KNI_GetParameterAsInt(1);

    KNI_ReturnInt(keymap_get_system_key(gameAction));
}

/**
 * IMPL_NOTE:(Doxy - Change this format) 
 * FUNCTION:      getKeyName(I)Ljava/lang/String;
 * CLASS:         javax.microedition.lcdui.KeyConverter
 * TYPE:          virtual native function
 * OVERVIEW:      Get the informative key string corresponding to
 *                 the given keyCode.
 * INTERFACE (operand stack manipulation):
 *   parameters:  keyCode   A system-specific keyCode
 *   returns:     A string name for the key, or null if no name is
 *                 available
 */
KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(javax_microedition_lcdui_KeyConverter_getKeyName) {
    int   keyCode = KNI_GetParameterAsInt(1);
    char *keyName = keymap_get_key_name(keyCode);

    KNI_StartHandles(1);
    KNI_DeclareHandle(str);

    if (keyName != NULL) {
        KNI_NewStringUTF(keyName, str);
    } else {
        if (keymap_is_invalid_key_code(keyCode)) {
            KNI_ReleaseHandle(str); /* Set 'str' to null String object */
        } else {
            jchar charCode = keyCode;
            KNI_NewString(&charCode, 1, str);
        }
    }
    KNI_EndHandlesAndReturnObject(str);
}


/**
 * FUNCTION:      getGameAction(I)I
 * CLASS:         javax.microedition.lcdui.KeyConverter
 * TYPE:          virtual native function
 * OVERVIEW:      Get the abstract gameAction corresponding to the
 *                 given keyCode
 * INTERFACE (operand stack manipulation):
 *   parameters:  keyCode   A system-specific keyCode
 *   returns:     The abstract game action associated with the keyCode
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_KeyConverter_getGameAction) {
    int keyCode = KNI_GetParameterAsInt(1);

    KNI_ReturnInt(keymap_get_game_action(keyCode));
}
