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

#include <stdlib.h>
#include <sni.h>
#include <midpError.h>

#include <imgapi_image.h>
#include <img_image.h>
#include <img_errorcodes.h>


/**
 * Gets an ARGB integer array from this <tt>ImmutableImage</tt>. The
 * array consists of values in the form of 0xAARRGGBB.
 * <p>
 * Java declaration:
 * <pre>
 *     getRGB([IIIIIII)V
 * </pre>
 *
 * @param rgbData The target integer array for the ARGB data
 * @param offset Zero-based index of first ARGB pixel to be saved
 * @param scanlen Number of intervening pixels between pixels in
 *                the same column but in adjacent rows
 * @param x The x coordinate of the upper left corner of the
 *          selected region
 * @param y The y coordinate of the upper left corner of the
 *          selected region
 * @param width The width of the selected region
 * @param height The height of the selected region
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Image_getRGB) {
    int height = KNI_GetParameterAsInt(7);
    int width = KNI_GetParameterAsInt(6);
    int y = KNI_GetParameterAsInt(5);
    int x = KNI_GetParameterAsInt(4);
    int scanlength = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int buflen;
    int *rgbBuffer;
    int img_width;
    int img_height;
    jboolean iae = KNI_FALSE;
    java_imagedata * srcImageDataPtr = NULL;

    KNI_StartHandles(2);
    KNI_DeclareHandle(rgbData);
    KNI_DeclareHandle(thisObject);

    KNI_GetParameterAsObject(1, rgbData);
    KNI_GetThisPointer(thisObject);

    srcImageDataPtr = IMGAPI_GET_IMAGE_PTR(thisObject)->imageData;

    img_width  = srcImageDataPtr->width;
    img_height = srcImageDataPtr->height;

    /* see if absolute value of scanlength is greater than or equal to width */
    if (scanlength >= 0 && scanlength < width) {
        iae = KNI_TRUE;
    } else if (scanlength < 0 && (0 - scanlength) < width) {
        iae = KNI_TRUE;
    }
    if (KNI_IsNullHandle(rgbData)) {
        KNI_ThrowNew(midpNullPointerException, NULL);
    } else if((y < 0) || (x < 0) || (x + width > img_width) ||
              (y + height > img_height) || iae == KNI_TRUE) {
        KNI_ThrowNew(midpIllegalArgumentException, NULL);
    } else if (height < 0 || width < 0 ) {
        /* spec says noop in this case */
    } else {
        buflen = KNI_GetArrayLength(rgbData);
        if (offset < 0
            || offset + ((height - 1) * scanlength) + width > buflen
            || offset + ((height - 1) * scanlength) < 0) {
            KNI_ThrowNew(midpArrayIndexOutOfBoundsException, NULL);
        } else {
            img_native_error_codes error = IMG_NATIVE_IMAGE_NO_ERROR;

            SNI_BEGIN_RAW_POINTERS;

            rgbBuffer = JavaIntArray(rgbData);
            img_get_argb(srcImageDataPtr, rgbBuffer,
                         offset, scanlength,
                         x, y, width, height, &error);

            SNI_END_RAW_POINTERS;

            if (error != IMG_NATIVE_IMAGE_NO_ERROR) {
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
            }
        }
    }

    KNI_EndHandles();
    KNI_ReturnVoid();
}

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
