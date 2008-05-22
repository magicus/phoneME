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
 * Shared cross platform functions for all Items.
 */

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>
#include <midpEvents.h>

#include <lfpport_item.h>
#include "lfp_intern_registry.h"
#include <midpUtilKni.h>


/*=========================================================================
 * FUNCTION:      show0(I)V
 * CLASS:         javax.microedition.lcdui.StringItemLFImpl
 * TYPE:          virtual native function
 * OVERVIEW:      Show function pointer. This function could be called 
 *                on a Item that is already shown.
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId
 *   returns:     <none>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ItemLFImpl_show0() {
  MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);

  itemPtr->show(itemPtr);

  KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      hide0(I)V
 * CLASS:         javax.microedition.lcdui.ItemLFImpl
 * TYPE:          virtual native function
 * OVERVIEW:      
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId
 *   returns:     <none>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ItemLFImpl_hide0() {
  MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);

  itemPtr->hide(itemPtr);

  KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      delete0(I)V
 * CLASS:         javax.microedition.lcdui.ItemLFImpl
 * TYPE:          virtual native function
 * OVERVIEW:      
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId
 *   returns:     <none>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ItemLFImpl_delete0() {
    MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);

    MidpDeleteItem(itemPtr);

    KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      setLocation0(III)V
 * CLASS:         javax.microedition.lcdui.ItemLFImpl
 * TYPE:          virtual native function
 * OVERVIEW:      sets bounds on the native resource
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId
 *   		  x
 *                y
 *   returns:     <none>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ItemLFImpl_setLocation0() {
  MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int x = KNI_GetParameterAsInt(2);
  int y = KNI_GetParameterAsInt(3);
 
  itemPtr->relocate(itemPtr, x, y);

  KNI_ReturnVoid();
}

/*=========================================================================
 * FUNCTION:      setSize0(III)V
 * CLASS:         javax.microedition.lcdui.ItemLFImpl
 * TYPE:          virtual native function
 * OVERVIEW:      sets bounds on the native resource
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId
 *   		  w
 *                h
 *   returns:     <none>
 *=======================================================================*/
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ItemLFImpl_setSize0() {
  MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int w = KNI_GetParameterAsInt(2);
  int h = KNI_GetParameterAsInt(3);
 
  itemPtr->resize(itemPtr, w, h);

  KNI_ReturnVoid();
}

/**
 * KNI function that sets label on the native widget corresponding to the
 * current Item.
 *
 * Class: javax.microedition.lcdui.ItemLFImpl
 *
 * Java prototype:
 * native void setLabel0(int nativeId, String label)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId pointer to the native resource for this Item
 *                label the new label set in this Item
 *   returns:     <none>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ItemLFImpl_setLabel0() {
    MidpError err = KNI_OK;
    pcsl_string label;
    pcsl_string_status rc;
    MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  
    KNI_StartHandles(1);

    KNI_DeclareHandle(labelJString);

    KNI_GetParameterAsObject(2, labelJString);

    rc = midp_jstring_to_pcsl_string(labelJString, &label);

    KNI_EndHandles();

    if (PCSL_STRING_OK != rc) {
      err = KNI_ENOMEM;
    } else {
      err = itemPtr->setLabel(itemPtr, &label);
    }
   
    pcsl_string_free(&label);

    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnVoid();
}

/**
 * KNI function that gets minimum width of the native widget for
 * current Item.
 *
 * Class: javax.microedition.lcdui.ItemLFImpl
 *
 * Java prototype:
 * native int getMinimumWidth0(int nativeId)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId pointer to the native resource for this Item
 *   returns:        the minimum width
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_ItemLFImpl_getMinimumWidth0() {
    MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
    int width;

    MidpError err = itemPtr->getMinimumWidth(&width, itemPtr);
    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(width);
}

/**
 * KNI function that gets minimum height of the native widget for
 * current Item.
 *
 * Class: javax.microedition.lcdui.ItemLFImpl
 *
 * Java prototype:
 * native int getMinimumHeight0(int nativeId)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId pointer to the native resource for this Item
 *   returns:        the minimum height
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_ItemLFImpl_getMinimumHeight0() {
    MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
    int height;

    MidpError err = itemPtr->getMinimumHeight(&height, itemPtr);
    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(height);
}


/**
 * KNI function that gets preferred width of the native widget for
 * current Item.
 *
 * Class: javax.microedition.lcdui.ItemLFImpl
 *
 * Java prototype:
 * native int getPreferredWidth0(int nativeId, int h)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId pointer to the native resource for this Item
 *                h  the tentative content height in pixels, 
 *                   or -1 if a tentative height has not been computed
 *   returns:        the preferred width
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_ItemLFImpl_getPreferredWidth0() {
    MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
    int height = KNI_GetParameterAsInt(2);
    int width;

    MidpError err = itemPtr->getPreferredWidth(&width, itemPtr, height);
    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(width);
}

/**
 * KNI function that gets preferred height of the native widget for
 * current Item.
 *
 * Class: javax.microedition.lcdui.ItemLFImpl
 *
 * Java prototype:
 * native int getPreferredWidth0(int nativeId, int h)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId pointer to the native resource for this Item
 *                w       the tentative content width in pixels, 
 *                        or -1 if a tentative width has not been computed
 *   returns:        the preferred height
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_ItemLFImpl_getPreferredHeight0() {
    MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
    int width = KNI_GetParameterAsInt(2);
    int height;

    MidpError err = itemPtr->getPreferredHeight(&height, itemPtr, width);
    if (err != KNI_OK) {
	KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(height);
}
