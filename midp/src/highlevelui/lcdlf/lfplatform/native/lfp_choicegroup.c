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
 * Cross platform ChoiceGroup's native functions.
 */

#include <kni.h>

#include <midpError.h>
#include <midpMalloc.h>

#include <lfpport_choicegroup.h>
#include "lfp_intern_registry.h"
#include <midpUtilKni.h>

#include <gxp_image.h>

/* cached field ids for CGElement class */
static jfieldID _cgEl_stringEl_cache    = NULL;
static jfieldID _cgEl_imageDataEl_cache = NULL;
static jfieldID _cgEl_selected_cache    = NULL;
static jfieldID _cgEl_font_cache        = NULL;

/* cached field ids for Font class */
static jfieldID _f_face_cache  = NULL;
static jfieldID _f_style_cache = NULL;
static jfieldID _f_size_cache  = NULL;

#define _CACHE_FIELDID(HANDLE, name, type, cache) \
     ((cache)==NULL?((cache)=KNI_GetFieldID((HANDLE), (name), (type))):(cache))
/**
 * KNI function that creates native resource for the current StringItem.
 * <p>
 * Java declaration:
 * <pre>
 *     createNativeResource0(ISIII[OII)I
 * </pre>
 *
 * @param ownerId Owner screen's native resource id (MidpDisplayable *)
 * @param label - label to be used for this ChoiceGroup
 * @param layout layout directive associated with this ChoiceGroup
 * @param choiceType - should be EXCLUSIVE, MULTIPLE, IMPLICIT, POPUP
 * @param fitPolicy  - to be used to display created ChoiceGroup
 * @param cgElements - elements array with string, image, font, selected state
 *                     information per element
 * @param numChoices - number of elements in the ChoiceGroup
 * @param selectedIndex - currently selected index (for EXCLUSIVE, IMPLICIT, and
 *                        POPUP)
 * @return native resource id (MidpItem *) of this StringItem
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_createNativeResource0() {
  MidpError err = KNI_OK;
  MidpDisplayable  *ownerPtr = NULL;
  MidpItem *cgPtr = NULL;
  pcsl_string label_str;
  MidpChoiceGroupElement *cgChoices = NULL;
  int choiceType, layout;
  int fitPolicy;
  int numChoices = 0;
  int selectedIndex;
  int i = 0;
  pcsl_string_status perr;

  ownerPtr = (MidpDisplayable *)KNI_GetParameterAsInt(1);
  layout = KNI_GetParameterAsInt(3);
  choiceType = KNI_GetParameterAsInt(4);
  fitPolicy  = KNI_GetParameterAsInt(5);
  numChoices = KNI_GetParameterAsInt(7);
  selectedIndex  = KNI_GetParameterAsInt(8);

  KNI_StartHandles(8);
  
  KNI_DeclareHandle(labelJString);
  KNI_DeclareHandle(cgElementsJObject);
  KNI_DeclareHandle(cgElement);
  KNI_DeclareHandle(strJString);
  KNI_DeclareHandle(imgJImage);
  KNI_DeclareHandle(fontJFont);
  KNI_DeclareHandle(cgElementHandle);
  KNI_DeclareHandle(fontHandle);

  KNI_GetParameterAsObject(2, labelJString);
  KNI_GetParameterAsObject(6, cgElementsJObject);
 
  if (numChoices > 0) {
    jobjectArray cgElementsArray;
    KNI_FindClass("javax/microedition/lcdui/ChoiceGroup$CGElement", 
		  cgElementHandle);

    KNI_FindClass("javax/microedition/lcdui/Font", fontHandle);
	  
    cgElementsArray = (jobjectArray)cgElementsJObject;

    cgChoices = (MidpChoiceGroupElement *)
		midpMalloc(sizeof(MidpChoiceGroupElement) * numChoices);
    if (cgChoices == NULL) {
      err = KNI_ENOMEM;
    }

    for (i = 0; err == KNI_OK && i < numChoices; i++) {

      KNI_GetObjectArrayElement(cgElementsArray, i, cgElement);

      KNI_GetObjectField(cgElement, 
			 _CACHE_FIELDID(cgElementHandle, "stringEl", 
					"Ljava/lang/String;", 
					_cgEl_stringEl_cache), strJString);

      perr = midp_jstring_to_pcsl_string(strJString, &cgChoices[i].string);
      if (PCSL_STRING_OK != perr) {
        err = KNI_ENOMEM;
      } else {

	KNI_GetObjectField(cgElement, 
			   _CACHE_FIELDID(cgElementHandle, "imageDataEl", 
					  "Ljavax/microedition/lcdui/ImageData;", 
					  _cgEl_imageDataEl_cache), imgJImage);

	if (KNI_IsNullHandle(imgJImage) == KNI_TRUE) {
	  cgChoices[i].image = NULL;
	} else {
	  cgChoices[i].image = gxp_get_imagedata(imgJImage);
	}

	cgChoices[i].selected = 
	  KNI_GetBooleanField(cgElement, _CACHE_FIELDID(cgElementHandle, 
							"selected", "Z",
							_cgEl_selected_cache));
	
	KNI_GetObjectField(cgElement,
			   _CACHE_FIELDID(cgElementHandle, "fontEl",
					  "Ljavax/microedition/lcdui/Font;", 
					  _cgEl_font_cache), fontJFont); 
	
	if (KNI_IsNullHandle(fontJFont) == KNI_TRUE) {
	  cgChoices[i].font = NULL;
	} else {
	  
	  int face, style, size; /* usually only few fonts are set */

	  face = KNI_GetIntField(fontJFont, _CACHE_FIELDID(fontHandle, "face", 
                             "I", _f_face_cache));
	  style = KNI_GetIntField(fontJFont, _CACHE_FIELDID(fontHandle, 
                             "style",
                             "I", _f_style_cache));
      size = KNI_GetIntField(fontJFont, _CACHE_FIELDID(fontHandle, "size",
                             "I", _f_size_cache));

            if ((err = lfpport_get_font(&(cgChoices[i].font), face, style, size))
                != KNI_OK) {
                  err = KNI_ENOMEM;
                  i++;
                  break;
            }
        }
      }
    }
  }
 
  if (err == KNI_OK) {
    if(PCSL_STRING_OK
        != midp_jstring_to_pcsl_string(labelJString, &label_str)) {
      err = KNI_ENOMEM;
    }
  }

  KNI_EndHandles();

  if (err == KNI_OK) {
    cgPtr = MidpNewItem(ownerPtr, 
			MIDP_EXCLUSIVE_CHOICE_GROUP_TYPE + choiceType - 1);
    
    if (cgPtr == NULL) {
      err = KNI_ENOMEM;
    } else {
      err = lfpport_choicegroup_create(cgPtr, ownerPtr, &label_str, layout,
				    choiceType, cgChoices, numChoices,
				    selectedIndex, fitPolicy);
    }
  }

  // do clean up
  pcsl_string_free(&label_str);
  for (i--; i >= 0; i--) {
    pcsl_string_free(&cgChoices[i].string);
  }
  midpFree(cgChoices);
  
  if (err != KNI_OK) {
    MidpDeleteItem(cgPtr);
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }

  KNI_ReturnInt(cgPtr);
}

/**
 * KNI function that inserts new element with passed in string and image 
 *  into the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     insert0(IISOB)V
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @param elementNum - location at which new element should be inserted
 * @param stringPart - string part of the element to be inserted
 * @param imagePart - image part of the element to be inserted
 * @param selected  - current selection state of the inserted element
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_insert0() {
  MidpError err = KNI_OK;
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  MidpChoiceGroupElement cgElement;
  pcsl_string_status perr;
  int elementNum = KNI_GetParameterAsInt(2);
  

  KNI_StartHandles(2);
  KNI_DeclareHandle(stringPartJString);
  KNI_DeclareHandle(imgPartJImage);
  
  KNI_GetParameterAsObject(3, stringPartJString);
  KNI_GetParameterAsObject(4, imgPartJImage);

  if (KNI_IsNullHandle(imgPartJImage) == KNI_TRUE) {
    cgElement.image = NULL;
  } else {
    cgElement.image = gxp_get_imagedata(imgPartJImage);
  }

  perr = midp_jstring_to_pcsl_string(stringPartJString, &cgElement.string);
  cgElement.selected = KNI_GetParameterAsBoolean(5);
  cgElement.font = NULL;

  KNI_EndHandles();

  if (PCSL_STRING_OK == perr) {
    err = lfpport_choicegroup_insert(cgPtr, elementNum, cgElement);
  }
  
  pcsl_string_free(&cgElement.string);
  
  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnVoid();
}

/**
 * KNI function that deletes an element at the passed in location  
 * from the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     delete0(III)V
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @param elementNum - location at which an element should be deleted
 * @param selectedIndex  - index of an element that should be selected 
 *                        after deletion is done
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_delete0() {
  MidpError err = KNI_OK;
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int elementNum = KNI_GetParameterAsInt(2);
  int selectedIndex = KNI_GetParameterAsInt(3);

  err = lfpport_choicegroup_delete(cgPtr, elementNum, selectedIndex);

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnVoid();  
}

/**
 * KNI function that deletes an element at the passed in location  
 * from the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     deleteAll0(I)V
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_deleteAll0() {
  MidpError err = KNI_OK;
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);

  err = lfpport_choicegroup_delete_all(cgPtr);

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnVoid();  
}

/**
 * KNI function that sets content on an element with passed in index
 * in the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     set0(IISOB)V
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @param elementNum - index of an element which content should be changed
 * @param stringPart - new string part of the element
 * @param imagePart - new image part of the element
 * @param selected  - current selection state of the element
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_set0() {
  MidpError err = KNI_OK;
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  MidpChoiceGroupElement cgElement;
  pcsl_string_status perr;
  int elementNum = KNI_GetParameterAsInt(2);

  KNI_StartHandles(2);
  KNI_DeclareHandle(stringPartJString);
  KNI_DeclareHandle(imgPartJImage);
  
  KNI_GetParameterAsObject(3, stringPartJString);
  KNI_GetParameterAsObject(4, imgPartJImage);

  if (KNI_IsNullHandle(imgPartJImage) == KNI_TRUE) {
    cgElement.image = NULL;
  } else {
    cgElement.image  = gxp_get_imagedata(imgPartJImage);
  }

  perr = midp_jstring_to_pcsl_string(stringPartJString, &cgElement.string);
  cgElement.selected = KNI_GetParameterAsBoolean(5);
  cgElement.font = NULL;

  KNI_EndHandles();

  if (PCSL_STRING_OK == perr) {
    err = lfpport_choicegroup_set(cgPtr, elementNum, cgElement);
  }
  
  pcsl_string_free(&cgElement.string);

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }

  KNI_ReturnVoid();
}

/**
 * KNI function that sets selected state on an element with passed in index
 * in the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     setSelectedIndex0(III)V
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @param elementNum - index of an element which content should be changed
 * @param selected  - current selection state of the element
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_setSelectedIndex0() {
  MidpError err = KNI_OK;
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int elementNum = KNI_GetParameterAsInt(2);
  jboolean selected = KNI_GetParameterAsBoolean(3);

  err = lfpport_choicegroup_set_selected_index(cgPtr, elementNum, selected);

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnVoid();  
}

/**
 * KNI function that gets currently selected element 
 * from the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     getSelectedIndex0(I)I
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @return - currently selected element
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_getSelectedIndex0() {

  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int elementNum;

  MidpError err = lfpport_choicegroup_get_selected_index(&elementNum, cgPtr);

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnInt(elementNum);  
}

/**
 * KNI function that sets selected state on element 
 * in the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     setSelectedFlags0(I[BI)V
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @param selectedArray - array with selected state value for the elements
 * @param numSelectedArray - number of elements in the selectedArray
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_setSelectedFlags0() {
  MidpError err = KNI_OK;
  jboolean *selectedArray = NULL;
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int i, selectedArrayLength = KNI_GetParameterAsInt(3);


  selectedArray =(jboolean *)midpMalloc(sizeof(jboolean) * 
					selectedArrayLength);

  if (selectedArray == NULL) {
    err = KNI_ENOMEM;
  } else {

    KNI_StartHandles(1);
    
    KNI_DeclareHandle(selectedArrayJObjectArray);
    
    KNI_GetParameterAsObject(2, selectedArrayJObjectArray);
    
    for (i = 0; i < selectedArrayLength; i++) {
      selectedArray[i] = KNI_GetBooleanArrayElement(
				    (jbooleanArray)selectedArrayJObjectArray, 
				    (jsize)i);
    }
    
    KNI_EndHandles();
    
    err = lfpport_choicegroup_set_selected_flags(cgPtr, selectedArray, 
						 selectedArrayLength);

    midpFree(selectedArray);
  }  

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnVoid();
}

/**
 * KNI function that sets selected state on element 
 * in the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     getSelectedFlags0(I[BI)I
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @param selectedArray_return - return array with selected state
 *                               value for the elements
 * @param numSelectedArray - number of elements in the selectedArray_return
 * @param - number of elements selected in selectedArray_return
 */
KNIEXPORT KNI_RETURNTYPE_INT
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_getSelectedFlags0() {
  MidpError err = KNI_OK;
  jboolean *selectedArray = NULL; 
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int i, selectedArrayLength = KNI_GetParameterAsInt(3);
  int numSelected = 0;

  selectedArray = (jboolean *)midpMalloc(sizeof(jboolean) * 
					 selectedArrayLength);

  if (selectedArray == NULL) {
    err = KNI_ENOMEM;
  } else {

    KNI_StartHandles(1);
    
    KNI_DeclareHandle(selectedReturnJObjectArray);
    
    KNI_GetParameterAsObject(2, selectedReturnJObjectArray);

    err = lfpport_choicegroup_get_selected_flags(&numSelected, cgPtr,
						 selectedArray, 
						 selectedArrayLength);
    if (err == KNI_OK) {
      for (i = 0; i < selectedArrayLength; i++) {
	KNI_SetBooleanArrayElement(selectedReturnJObjectArray, 
				   i, selectedArray[i]);
	if (selectedArray[i]) {
	  numSelected++;
	}
      }
    }
    
    KNI_EndHandles();

    midpFree(selectedArray);
  } 

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnInt(numSelected);  
}

/**
 * KNI function that gets selected state of an element 
 * in the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     isSelected0(II)B
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @param element - index of an element which selection state has to determined
 * @param - true if an element is selected, false - otherwise
 */
KNIEXPORT KNI_RETURNTYPE_BOOLEAN
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_isSelected0() {
  MidpError err = KNI_OK;
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int elementNum = KNI_GetParameterAsInt(2);
  jboolean selected;

  err = lfpport_choicegroup_is_selected(&selected, cgPtr, elementNum);

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnBoolean(selected);   
}

/**
 * KNI function that sets fit policy for
 * on the native resource corresponding to the current ChoiceGroup.
 *
 * Java declaration:
 * <pre>
 *     setFitPolicy0(II)V
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @param fitPolicy - TEXT_WRAP_ON, TEXT_WRAP_OFF, TEXT_WRAP_DEFAULT
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_setFitPolicy0() {
  MidpError err = KNI_OK;
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int fitPolicy = KNI_GetParameterAsInt(2);

  err = lfpport_choicegroup_set_fit_policy(cgPtr, fitPolicy);

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }
  
  KNI_ReturnVoid();  
}

/**
 * KNI function that sets new font on the native resource corresponding to
 * the current ChoiceGroup
 *
 * Java declaration:
 * <pre>
 *     setFont0(IIIII)V
 * </pre>
 * @param nativeId - id of the native resource corresponding to the current
 *                   ChoiceGroup
 * @param elementNum - index of an element which font is being changed
 * @param face       - face of the new font set in the ChoiceGroup
 * @param style      - style of the new font set in the ChoiceGroup
 * @param size       - size of the new font set in the ChoiceGroup
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_javax_microedition_lcdui_ChoiceGroupLFImpl_setFont0() {
  MidpError err = KNI_OK;
  PlatformFontPtr fontPtr;
  MidpItem *cgPtr = (MidpItem *)KNI_GetParameterAsInt(1);
  int elementNum  = KNI_GetParameterAsInt(2);
  int face  = KNI_GetParameterAsInt(3);
  int style = KNI_GetParameterAsInt(4);
  int size  = KNI_GetParameterAsInt(5);
  
  err = lfpport_get_font(&fontPtr, face, style, size);

  if (err == KNI_OK) {
    err = lfpport_choicegroup_set_font(cgPtr, elementNum, fontPtr);
  }

  if (err == KNI_ENOMEM) {
    KNI_ThrowNew(midpOutOfMemoryError, NULL);
  }

  KNI_ReturnVoid();
}
