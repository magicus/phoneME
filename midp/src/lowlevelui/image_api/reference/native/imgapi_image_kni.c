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

#include <sni.h>
#include <midpError.h>


#ifndef UNDER_CE
/*
 * Dummy functions for native soft buttons and menus
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_layers_SoftButtonLayer_setNativeSoftButton) {
    KNI_ReturnVoid();
}


KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_layers_SoftButtonLayer_setNativePopupMenu) {
    KNI_ReturnVoid();
}

/*
 * These are dummy functions for testing native text editor. You can
 * change src/configuration/linux_fb/skin.xml to have TEXTFIELD_NATIVE_EDITOR=1
 * and then look at the stdout for the messages sent by TextFieldLFImpl.java
 */

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_enableNativeEditor) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_setNativeEditorContent) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_disableNativeEditor) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_getNativeEditorContent) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(temp);
    KNI_ReleaseHandle(temp);
    KNI_EndHandlesAndReturnObject(temp);
}

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_getNativeEditorCursorIndex) {    
    KNI_ReturnInt(0);
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_MIDPWindow_disableAndSyncNativeEditor) {
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_OBJECT
KNIDECL(javax_microedition_lcdui_TextFieldLFImpl_mallocToJavaChars) {
    KNI_StartHandles(1);
    KNI_DeclareHandle(temp);
    KNI_ReleaseHandle(temp);
    KNI_EndHandlesAndReturnObject(temp);
}
#endif
