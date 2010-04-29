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
 * This file contains Java native function for playing alert sound.
 */

#include <anc_audio.h>
#include <midpEventUtil.h>

/**
 * Calls platform specific function to play a sound.
 * <p>
 * Java declaration:
 * <pre>
 *     playAlertSound(II)Z
 * </pre>
 * Java parameters:
 * <pre>
 *   displayId The display ID associated with this Display
 *   alertType  Type of alert
 *   returns true if sound was played.
 * </pre> 
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(javax_microedition_lcdui_Display_playAlertSound0) {
    int alertType = KNI_GetParameterAsInt(2);
    int displayId = KNI_GetParameterAsInt(1);

    if (midpHasForeground(displayId)) {
      /* Alert type happens to be the same as sound type */
      KNI_ReturnBoolean(anc_play_sound(alertType));
    }
    
    KNI_ReturnBoolean(KNI_FALSE);
}
