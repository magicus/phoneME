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
 * Cross platform functions for ImageItem.
 */

#include <stdlib.h>

#include <kni.h>

#include <midpError.h>
#include "lfp_intern_registry.h"
#include <lfpport_imageitem.h>
#include <midpUtilKni.h>

#include <gxp_image.h>


/**
 * KNI function that creates new native resource for the current ImageItem.
 *
 * Class: javax.microedition.lcdui.ImageItemLFImpl
 * Java prototype:
 * private native int createNativeResource0(int ownerId, String label,
 *                                          int layout,
 *                                          Image img, String altText,
 *                                          int appearanceMode)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  ownerId            pointer to the owner's native resource
 *                label              ImageItem's label
 *                layout             ImageItem's layout
 *                img                ImageItem's image
 *                altText            ImageItem's attText
 *                appearanceMode     the appearanceMode of ImageItem
 *   return pointer to the created native resource
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_ImageItemLFImpl_createNativeResource0() {
  MidpError err = KNI_OK;
  MidpDisplayable  *ownerPtr;
  MidpItem *itemPtr = NULL;
  pcsl_string label, altText;
  pcsl_string_status rc1,rc2;
  unsigned char* imgPtr = NULL;
  int appearanceMode, layout;

  KNI_StartHandles(3);
  
  KNI_DeclareHandle(labelJString);
  KNI_DeclareHandle(image);
  KNI_DeclareHandle(altTextJString);

  ownerPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
  KNI_GetParameterAsObject(2, labelJString);
  layout = KNI_GetParameterAsInt(3);
  KNI_GetParameterAsObject(4, image);
  KNI_GetParameterAsObject(5, altTextJString);

  if (KNI_IsNullHandle(image) != KNI_TRUE) {
    imgPtr = gxp_get_imagedata(image);
  }

  appearanceMode = KNI_GetParameterAsInt(6);

  rc1 = midp_jstring_to_pcsl_string(labelJString, &label);
  rc2 = midp_jstring_to_pcsl_string(altTextJString, &altText);

  KNI_EndHandles();

  /* NULL and empty strings are acceptable. */
  if (PCSL_STRING_OK != rc1 || PCSL_STRING_OK != rc2 ) {
    err = KNI_ENOMEM;
    goto cleanup;
  }

  itemPtr = MidpNewItem(ownerPtr, MIDP_PLAIN_IMAGE_ITEM_TYPE+appearanceMode);
  if (itemPtr == NULL) {
    err = KNI_ENOMEM;
    goto cleanup;
  }

  err = lfpport_imageitem_create(itemPtr, ownerPtr, &label, layout,
				 imgPtr, &altText, appearanceMode);

cleanup:
  pcsl_string_free(&altText);
  pcsl_string_free(&label);

  if (err != KNI_OK) {
    MidpDeleteItem(itemPtr);
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }

  KNI_ReturnInt(itemPtr);
}

/**
 * KNI function that sets new content on the native resource corresponding to
 * the current ImageItem.
 *
 * Class: javax.microedition.lcdui.ImageItemLFImpl
 * Java prototype:
 * private native int setContent0(int nativeId, Image image, altText, 
 *                                appearanceMode)
 *
 * INTERFACE (operand stack manipulation):
 *   parameters:  nativeId   pointer to a native resource of this ImageItem
 *                image     the new image set in the ImageItem
 *                altText   the new altText set in the ImageItem
 *                appearanceMode the actual appearance mode to be used
 *   returns:     <nothing>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ImageItemLFImpl_setContent0() {
  MidpError err = KNI_OK;
  unsigned char* imgPtr = NULL;
  pcsl_string altText;
  pcsl_string_status rc;
  MidpItem *itemPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int appearanceMode = KNI_GetParameterAsInt(4);

  KNI_StartHandles(2);
  KNI_DeclareHandle(image);
  KNI_DeclareHandle(altTextJString);

  KNI_GetParameterAsObject(2, image);
  
  if (KNI_IsNullHandle(image) != KNI_TRUE) {
    imgPtr = gxp_get_imagedata(image);
  }
  
  KNI_GetParameterAsObject(3, altTextJString);

  rc = midp_jstring_to_pcsl_string(altTextJString, &altText);

  KNI_EndHandles();

  /* NULL and empty strings are acceptable. */
  if (PCSL_STRING_OK != rc) {
    err = KNI_ENOMEM;
  } else {
    err = lfpport_imageitem_set_content(itemPtr, imgPtr, &altText,
				        appearanceMode);
  }

  pcsl_string_free(&altText);
  
  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }

  KNI_ReturnVoid();
}


