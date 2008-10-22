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
 * This file contains kni calls related to the SoftButtonLayer
 * this is needed once the platfrm uses its own native SoftButtonLayer
 */

#include <commonKNIMacros.h>
#include <lcdlf_export.h>
#include <midpError.h>
#include <midpMalloc.h>

/**
 * KNI function to verify is native soft buttons are supported
 * Function: static public native boolean isNativeSoftButtonLayerSupported0();
 *
 * Class: com.sun.midp.chameleon.layers.SoftButtonLayer
 */
 
KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_chameleon_layers_SoftButtonLayer_isNativeSoftButtonLayerSupported0) {
    KNI_ReturnBoolean(lcdlf_is_native_softbutton_layer_supported());
}

/**
 * KNI function to set the SoftButton's label on the native layer.
 * Function: public native void setNativeCommand0(String label, int softButtonIndex)
 *
 * Class: com.sun.midp.chameleon.layers.SoftButtonLayer
 */
 
KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_layers_SoftButtonLayer_setNativeSoftButtonLabel0) {
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

