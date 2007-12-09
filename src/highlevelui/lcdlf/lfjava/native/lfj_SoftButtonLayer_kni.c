/*
 * @(#)lfj_cskin.c	1.14 06/06/17
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */

// This file contains kni calls related to the SoftButtonLayer
// this is needed once the platfrm uses its own native SoftButtonLayer

#include <commonKNIMacros.h>
#include <lcdlf_export.h>
#include <midpError.h>
#include <midpMalloc.h>

/**
 * KNI function to verify is native soft buttons are supported
 * Function: public native void setNativeCommand0(String label, int softButtonIndex)
 *
 * Class: com.sun.midp.chameleon.layers.SoftButtonLayer
 */
 
KNI_RETURNTYPE_BOOLEAN
Java_com_sun_midp_chameleon_layers_SoftButtonLayer_isNativeSoftButtonLabel0() {
    KNI_ReturnBoolean(lcdlf_is_softbutton_label_on_native_layer());
}

/**
 * KNI function to set the SoftButton's label on the native layer.
 * Function: public native void setNativeCommand0(String label, int softButtonIndex)
 *
 * Class: com.sun.midp.chameleon.layers.SoftButtonLayer
 */
 
KNI_RETURNTYPE_VOID
Java_com_sun_midp_chameleon_layers_SoftButtonLayer_setNativeSoftButtonLabel0() {
	int strLen    = 0;
	int sfbIndex  = 0; 
	jchar *buffer = NULL;

	KNI_StartHandles(1);
	KNI_DeclareHandle(stringHandle);
	KNI_GetParameterAsObject(1, stringHandle);

	sfbIndex = KNI_GetParameterAsInt(2);
	strLen = KNI_GetStringLength(stringHandle); 

	if (strLen>0) {
		buffer = (jchar *) midpMalloc(strLen * sizeof(jchar));
		if (buffer == NULL) {
			KNI_ThrowNew(midpOutOfMemoryError, NULL);
		} else {
			KNI_GetStringRegion(stringHandle, 0, strLen, buffer);
			lcdlf_set_softbutton_label_on_native_layer(buffer, strLen, sfbIndex );
			midpFree(buffer);
		}
	} else { //the label is a null or emprty string 
		lcdlf_set_softbutton_label_on_native_layer((jchar*)"\x0\x0", 0, sfbIndex );
	} 
	KNI_EndHandles();
	KNI_ReturnVoid();
}

