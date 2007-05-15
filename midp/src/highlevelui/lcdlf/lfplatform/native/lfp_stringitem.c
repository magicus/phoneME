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
 * Cross platform functions for StringItem.
 */


#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>

#include <lfpport_font.h>
#include <lfpport_stringitem.h>
#include "lfp_intern_registry.h"

#include <midpUtilKni.h>

/* cached field ids for Font class */
static jfieldID _f_face_cache  = NULL;
static jfieldID _f_style_cache = NULL;
static jfieldID _f_size_cache  = NULL;

#define _CACHE_FIELDID(HANDLE, name, type, cache) \
     ((cache)==NULL?((cache)=KNI_GetFieldID((HANDLE), (name), (type))):(cache))

/**
 * KNI function that creates native resource for the current StringItem.
 *
 * Class: javax.microedition.lcdui.StringItemLFImpl
 *
 * Java prototype:
 * private native int createNativeResource0(int ownerId,
 *	    		String label, int layout, String text, int maxSize,
 *			int constraints, String initialInputMode)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  ownerId            id of the owner's native resource
 *                label              StringItem's label
 *                layout             StringItem's layout
 *                text               StringItem's text
 *                appearanceMode     the appearanceMode of StringItem
 *                font               font to paint text
 *   returns:     id of the created platform widget
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_StringItemLFImpl_createNativeResource0() {
  MidpError err = KNI_OK;
  MidpDisplayable  *ownerPtr;
  MidpItem *itemPtr = NULL;
  pcsl_string label, text;
  pcsl_string_status rc1 = PCSL_STRING_OK, rc2 = PCSL_STRING_OK;
  PlatformFontPtr fontPtr = NULL;
  int appearanceMode, layout;

  KNI_StartHandles(4);
  
  KNI_DeclareHandle(labelJString);
  KNI_DeclareHandle(textJString);
  KNI_DeclareHandle(fontJFont);
  KNI_DeclareHandle(fontHandle);

  ownerPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
  KNI_GetParameterAsObject(2, labelJString);
  layout = KNI_GetParameterAsInt(3);
  KNI_GetParameterAsObject(4, textJString);
  appearanceMode = KNI_GetParameterAsInt(5);
  KNI_GetParameterAsObject(6, fontJFont);

  if (KNI_IsNullHandle(fontJFont) != KNI_TRUE) {
    int face, style, size;

    KNI_FindClass("javax/microedition/lcdui/Font", fontHandle);

    face  = KNI_GetIntField(fontJFont, _CACHE_FIELDID(fontHandle, "face", 
						      "I", _f_face_cache));
    style = KNI_GetIntField(fontJFont, _CACHE_FIELDID(fontHandle, "style", 
						      "I", _f_style_cache));
    size  = KNI_GetIntField(fontJFont, _CACHE_FIELDID(fontHandle, "size", 
						      "I", _f_size_cache));
	  
    err = lfpport_get_font(&fontPtr, face, style, size);
  }

  if (err == KNI_OK) {
    rc1 = midp_jstring_to_pcsl_string(labelJString, &label);
    rc2 = midp_jstring_to_pcsl_string(textJString, &text);
  }

  KNI_EndHandles();

  if (err != KNI_OK || PCSL_STRING_OK != rc1 || PCSL_STRING_OK != rc2) {
    err = KNI_ENOMEM;
    goto cleanup;
  }
  

  itemPtr = MidpNewItem(ownerPtr, MIDP_PLAIN_STRING_ITEM_TYPE+appearanceMode);
  if (itemPtr == NULL) {
    err = KNI_ENOMEM;
    goto cleanup;
  }

  err = lfpport_stringitem_create(itemPtr, ownerPtr, &label, layout,
				  &text, fontPtr, appearanceMode);

 cleanup:  
    pcsl_string_free(&text);
    pcsl_string_free(&label);

    if (err != KNI_OK) {
      MidpDeleteItem(itemPtr);
      KNI_ThrowNew(midpOutOfMemoryError, NULL);
    }

    KNI_ReturnInt(itemPtr);
}

/**
 * KNI function that sets new content on the native resource corresponding to
 * the current StringItem.
 *
 * Class: javax.microedition.lcdui.StringLFImpl
 * Java prototype:
 * private native int setContent0(int nativeId, Image image, altText, 
 *                                appearanceMode)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId pointer to the native resource of the StringItem
 *                text     the new string set in the StringItem
 *                appearanceMode the appearance mode of the passed in text
 *   returns:     <nothing>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_StringItemLFImpl_setContent0() {
  MidpError err = KNI_OK;
  MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int appearanceMode = KNI_GetParameterAsInt(3);
  pcsl_string text;
  pcsl_string_status rc;

  KNI_StartHandles(1);
  KNI_DeclareHandle(textJString);
  
  KNI_GetParameterAsObject(2, textJString);

  rc = midp_jstring_to_pcsl_string(textJString, &text);

  KNI_EndHandles();

  if (PCSL_STRING_OK != rc) {
    err = KNI_ENOMEM;
  } else {
    err = lfpport_stringitem_set_content(itemPtr, &text, appearanceMode);
  }
  
  pcsl_string_free(&text);
  
  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnVoid();
}

/**
 * KNI function that sets new font on the native resource corresponding to
 * the current StringItem.
 *
 * Class: javax.microedition.lcdui.StringLFImpl
 * Java prototype:
 * private native int setFont0(int nativeId, int face, int style, int size)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  id      pointer to the native resource
 *                face     face of the new font set in the StringItem
 *                style    style of the new font set in the StringItem
 *                size     size of the new font set in the StringItem
 *   returns:     <nothing>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_StringItemLFImpl_setFont0() {
  MidpError err = KNI_OK;
  PlatformFontPtr fontPtr;
  MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int face  = KNI_GetParameterAsInt(2);
  int style = KNI_GetParameterAsInt(3);
  int size  = KNI_GetParameterAsInt(4);
  
  err = lfpport_get_font(&fontPtr, face, style, size);

  if (err == KNI_OK) {
    err = lfpport_stringitem_set_font(itemPtr, fontPtr);
  }

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }

  KNI_ReturnVoid();
}
