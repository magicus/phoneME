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
#include <commonKNIMacros.h>
#include <midpError.h>

#include <gx_font.h>
#include <gxapi_constants.h>
#include "gxapi_intern_graphics.h"
#include <font_constants.h>

/**
 * @file
 * 
 * Implementation of Java native methods for the <tt>Font</tt> class.
 */

/**
 * Initializes the native peer of this <tt>Font</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     init(III)V
 * </pre>
 *
 * @param face The face of the font to initialize
 * @param style The style of the font to initialize
 * @param size The point size of the font to initialize
 */
KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(javax_microedition_lcdui_Font_init) {
    jboolean free_size = KNI_GetParameterAsBoolean(4);
    int size  = (int)KNI_GetParameterAsInt(3);
    int style = (int)KNI_GetParameterAsInt(2);
    int face  = (int)KNI_GetParameterAsInt(1);
    int ascent, descent, leading;

    KNI_StartHandles(1);
    KNI_DeclareHandle(thisObject);
    
    if (free_size == KNI_FALSE) {
        /* size is one of the SIZE_XXX constants */
        size = OEM_FONT_SIZE(size);
    }

    KNI_GetParameterAsObject(0, thisObject);

    gx_get_fontinfo(face, style, size, &ascent, &descent, &leading);

    SNI_BEGIN_RAW_POINTERS;

    GET_FONT_PTR(thisObject)->baseline = (jint)ascent;
    GET_FONT_PTR(thisObject)->height = (jint)(ascent + descent + leading);
    GET_FONT_PTR(thisObject)->size = (jint)size;
    GET_FONT_PTR(thisObject)->sizeRounded = (jint) LCDUI_FONT_SIZE(size);

    SNI_END_RAW_POINTERS;
    
    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Gets the advance width of the specified character using this
 * <tt>Font</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     charWidth(C)I
 * </pre>
 *
 * @param ch the character to be measured
 *
 * @return the total advance width in pixels (a non-negative value)
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_Font_charWidth) {
    jchar c = (jchar)KNI_GetParameterAsChar(1);
    int face, style, size;

    KNI_StartHandles(1);

    KNI_DeclareHandle(thisObject);
    KNI_GetParameterAsObject(0, thisObject);

    DECLARE_FONT_PARAMS(thisObject);

    KNI_EndHandles();
    KNI_ReturnInt(gx_get_charswidth(face, style, size, &c, 1));
}

/**
 * Gets the combined advance width of multiple characters, starting at
 * the specified offset and for the specified number of characters using
 * this <tt>Font</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     charsWidth([CII)I
 * </pre>
 *
 * @param ch the array of characters to be measured
 * @param offset the index of the first character to measure
 * @param length the number of characters to measure
 *
 * @return the total width of the character range in pixels
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_Font_charsWidth) {
    int length = (int)KNI_GetParameterAsInt(3);
    int offset = (int)KNI_GetParameterAsInt(2);
    int chLen;
    jint result = 0;

    KNI_StartHandles(2);

    KNI_DeclareHandle(ch);
    KNI_DeclareHandle(thisObject);

    KNI_GetParameterAsObject(1, ch);
    KNI_GetParameterAsObject(0, thisObject);

    if ((chLen = KNI_GetArrayLength(ch)) == -1) {
        KNI_ThrowNew(midpNullPointerException, NULL);
    } else if (   (offset < 0) 
               || (offset > chLen) 
               || (length < 0)
               || (length > chLen)
               || ((offset + length) < 0)
               || ((offset + length) > chLen)) {
        KNI_ThrowNew(midpArrayIndexOutOfBoundsException, NULL);
    } else if (length != 0) {
        int      face, style, size;

        DECLARE_FONT_PARAMS(thisObject);

        SNI_BEGIN_RAW_POINTERS;

        result = gx_get_charswidth(face, style, size, 
				   &(JavaCharArray(ch)[offset]),
				   length);

        SNI_END_RAW_POINTERS;
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/**
 * Gets the total advance width of the given <tt>String</tt> in this
 * <tt>Font</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     stringWidth(Ljava/lang/String;)I
 * </pre>
 *
 * @param str the <tt>String</tt> to be measured
 *
 * @return the total advance width of the <tt>String</tt> in pixels
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_Font_stringWidth) {
    int strLen;
    jint result = 0;

    KNI_StartHandles(2);

    KNI_DeclareHandle(str);
    KNI_DeclareHandle(thisObject);

    KNI_GetParameterAsObject(1, str);
    KNI_GetParameterAsObject(0, thisObject);

    if ((strLen = KNI_GetStringLength(str)) == -1) {
        KNI_ThrowNew(midpNullPointerException, NULL);
    } else {
        int      face, style, size;
        _JavaString *jstr;

        DECLARE_FONT_PARAMS(thisObject);

        SNI_BEGIN_RAW_POINTERS;
     
        jstr = GET_STRING_PTR(str);

        result = gx_get_charswidth(face, style, size, 
				   jstr->value->elements + jstr->offset,
				   strLen);

        SNI_END_RAW_POINTERS;
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}

/**
 * Gets the total advance width of a portion of the given <tt>String<tt>
 * in this <tt>Font</tt>.
 * <p>
 * Java declaration:
 * <pre>
 *     substringWidth(Ljava/lang/String;II)I
 * </pre>
 *
 * @param str a <tt>String</tt> to be measured
 * @param offset the index of the first character of the substring to measure
 * @param length the number of characters in the substring to measure
 *
 * @return the total advance width of the substring in pixels
 */
KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(javax_microedition_lcdui_Font_substringWidth) {
    int length = KNI_GetParameterAsInt(3);
    int offset = KNI_GetParameterAsInt(2);
    int strLen;
    jint result = 0;

    KNI_StartHandles(2);

    KNI_DeclareHandle(str);
    KNI_DeclareHandle(thisObject);

    KNI_GetParameterAsObject(1, str);
    KNI_GetParameterAsObject(0, thisObject);

    if ((strLen = KNI_GetStringLength(str)) == -1) {
        KNI_ThrowNew(midpNullPointerException, NULL);
    } else if ( (offset < 0) 
		|| (offset > strLen) 
		|| (length < 0)
		|| (length > strLen)
		|| ((offset + length) < 0)
		|| ((offset + length) > strLen)) {
	KNI_ThrowNew(midpStringIndexOutOfBoundsException, NULL);
    } else if (length != 0) {
        int      face, style, size;
        _JavaString *jstr;

        DECLARE_FONT_PARAMS(thisObject);

        SNI_BEGIN_RAW_POINTERS;

        jstr = GET_STRING_PTR(str);

        result = gx_get_charswidth(face, style, size, 
				   jstr->value->elements + 
				   (jstr->offset + offset),
				   length);

        SNI_END_RAW_POINTERS;
    }

    KNI_EndHandles();
    KNI_ReturnInt(result);
}
