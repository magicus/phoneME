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
 * Cross platform functions for CustomItem.
 */
#include <kni.h>

#include <midpMalloc.h>
#include <midpError.h>
#include <lfpport_customitem.h>
#include "lfp_intern_registry.h"
#include <midpUtilKni.h>

#include <gxp_image.h>

/**
 * KNI function that creates new native resource for the current CustomItem.
 *
 * The native widget created by this function must take into account
 * label and body locations, using Item methods.
 * Also it must be able to grab key/pointer events directed to this 
 * item in order to pass them to java through the event queue.
 * The implementation should create a native offline buffer to be used
 * as the target for draw operation os the CustomItem, and as a source for
 * screen refresh in case a repaint is requested from the operating system.
 *
 * Class: javax.microedition.lcdui.CustomItemLFImpl
 * Java prototype:
 * private native int createNativeResource0(int ownerId, String label, 
 *                                          int layout)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  ownerId            pointer to the owner's native resource
 *                label              CustomItem's label
 *                layout             CustomItem's layout
 *   return pointer to the created native resource
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_CustomItemLFImpl_createNativeResource0() {
    MidpError err = KNI_OK;
    MidpItem *ciPtr = NULL;
    pcsl_string label;
    pcsl_string_status rc;
    MidpDisplayable *ownerPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
    int layout = KNI_GetParameterAsInt(3);

    KNI_StartHandles(1);
    KNI_DeclareHandle(labelHandle);

    KNI_GetParameterAsObject(2, labelHandle);

    rc = midp_jstring_to_pcsl_string(labelHandle, &label);

    KNI_EndHandles();

    if (PCSL_STRING_OK != rc) {
        err = KNI_ENOMEM;
        goto cleanup;
    }

    ciPtr = MidpNewItem(ownerPtr, MIDP_CUSTOM_ITEM_TYPE);
    if (ciPtr == NULL) {
        err = KNI_ENOMEM;
        goto cleanup;
    }

    err = lfpport_customitem_create(ciPtr, ownerPtr, &label, layout);

cleanup:
    pcsl_string_free(&label);

    if (err != KNI_OK) {
        MidpDeleteItem(ciPtr);
        KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(ciPtr);
}


/**
 * KNI function that causes a bitblt on the customItem
 *
 * As the draw operations on the CustomItem are routed to the offline buffer,
 * this call will cause bitblt from the offline buffer to the screen.
 * Note that if the CustomItem is not visible, for example - a scroll caused
 * the Item to be completely above or below the viewport, than the 
 * implementation can safely return without blitting.
 *
 * Class: javax.microedition.lcdui.CustomItemLFImpl
 * Java prototype:
 * private native int refresh0 ( int nativeId ) 
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId pointer to the native resource of the StringItem
 *                x,y     coordinates relative to widget
 *                width,height     invalid dimensions, or < 0 for "all"
 *   returns:     <nothing>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_CustomItemLFImpl_refresh0() {

    MidpItem *ciPtr = (MidpItem *)KNI_GetParameterAsInt(1);

    jint x = KNI_GetParameterAsInt(2);
    jint y = KNI_GetParameterAsInt(3);
    jint width = KNI_GetParameterAsInt(4);
    jint height = KNI_GetParameterAsInt(5);

    MidpError err = lfpport_customitem_refresh(ciPtr,
					       x, y,
					       width, height);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}



/**
 * This function returns label height in native widget.
 * The value is needed for the java layer to calculate refresh regions.
 *
 * Native implementation of Java function:
 * private static native int getLabelHeight0 ( int nativeId , int width ) ;
 * @param nativeId native resource id for this Item
 * @param width tentative width used to calculate the height
 * @return label height in native widget
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_CustomItemLFImpl_getLabelHeight0() {

    MidpItem *ciPtr = (MidpItem *)KNI_GetParameterAsInt(1);

    int width = KNI_GetParameterAsInt(2);

    int heightRet = 0;

    MidpError err = lfpport_customitem_get_label_height(width,
							&heightRet,
							ciPtr);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }
    
    KNI_ReturnInt(heightRet);
}

/**
 * Native implementation of Java function:
 * private static native int getLabelWidth0 ( int nativeId , 
 * 					      int contentWidth ) ;
 * <b>currently unimplemented</b>
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_CustomItemLFImpl_getLabelWidth0() {

    MidpItem *ciPtr = (MidpItem *)KNI_GetParameterAsInt(1);

    int contentWidth = KNI_GetParameterAsInt(2);

    int widthRet = 0;

    MidpError err = lfpport_customitem_get_label_width(&widthRet,
						       contentWidth,
						       ciPtr);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }
    
    KNI_ReturnInt(widthRet);
}

/**
 * Returns the value of the native constant ITEM_PAD.
 * The value is needed for the java layer to calculate refresh regions.
 * 
 * Native implementation of Java function:
 * private static native int getItemPad0 ( int nativeId ) ;
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_CustomItemLFImpl_getItemPad0() {

    MidpItem *ciPtr = (MidpItem *)KNI_GetParameterAsInt(1);

    int pad = 0;

    MidpError err = lfpport_customitem_get_item_pad(&pad, ciPtr);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(pad);
}


/**
 * Sets the content buffer. All paints are done to that buffer.
 * When paint is processed snapshot of the buffer is flushed to
 * the native resource content area.
 * Native implementation of Java function:
 * private static native int setContentBuffer0 ( int nativeId, Image img );
 * @param nativeId native resource is for this CustomItem
 * @param img mutable image that serves as an offscreen buffer
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_CustomItemLFImpl_setContentBuffer0() {

  MidpError err = KNI_OK;
  unsigned char* imgPtr = NULL;
  MidpItem *ciPtr = (MidpItem *)KNI_GetParameterAsInt(1);

  KNI_StartHandles(1);
  KNI_DeclareHandle(image);

  KNI_GetParameterAsObject(2, image);
  
  if (KNI_IsNullHandle(image) != KNI_TRUE) {
    imgPtr = gxp_get_imagedata(image);
  }

  KNI_EndHandles();

  err = lfpport_customitem_set_content_buffer(ciPtr, imgPtr);
  
  if (err != KNI_OK) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnVoid();
}
