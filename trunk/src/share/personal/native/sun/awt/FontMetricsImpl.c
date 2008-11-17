/*
 * @(#)FontMetricsImpl.c	1.12 06/10/10
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 *
 */
#include "common.h"
#include "sun_awt_gfX_FontMetricsImpl.h"
extern long GetMFStringWidth(JNIEnv * env, jcharArray s,
			     int offset, int sLength, jobject font);


/*
 * Class:     sun_awt_motif_X11FontMetrics
 * Method:    getMFCharsWidth
 * Signature: ([CIILjava/awt/Font;)I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_gfX_FontMetricsImpl_getMFCharsWidth(JNIEnv *env,
						 jobject this,
						 jcharArray data,
						 jint offset,
						 jint length,
						 jobject font) 
{
    jint retVal;

    NATIVE_LOCK(env);
    retVal = GetMFStringWidth(env, data, offset, length, font);
    NATIVE_UNLOCK(env);

    return retVal;
}

JNIACCESS(fontMetricsImpl);
JNIEXPORT void JNICALL
Java_sun_awt_gfX_FontMetricsImpl_initJNI(JNIEnv *env, jclass cl)
{
    DEFINECLASS(fontMetricsImpl, cl);
    DEFINEFIELD(fontMetricsImpl, font,       "Ljava/awt/Font;");
    DEFINEFIELD(fontMetricsImpl, ascent,     "I");
    DEFINEFIELD(fontMetricsImpl, descent,    "I");
    DEFINEFIELD(fontMetricsImpl, height,     "I");
    DEFINEFIELD(fontMetricsImpl, leading,    "I");
    DEFINEFIELD(fontMetricsImpl, maxAscent,  "I");
    DEFINEFIELD(fontMetricsImpl, maxDescent, "I");
    DEFINEFIELD(fontMetricsImpl, maxHeight,  "I");
    DEFINEFIELD(fontMetricsImpl, maxAdvance, "I");
    DEFINEFIELD(fontMetricsImpl, widths,     "[I");
}

/*
 * Class:     sun_awt_motif_X11FontMetrics
 * Method:    bytesWidth
 * Signature: ([BII)I
 */
JNIEXPORT jint JNICALL 
Java_sun_awt_gfX_FontMetricsImpl_bytesWidth(JNIEnv *env,
					    jobject this,
					    jbyteArray str,
					    jint off,
					    jint len) 
{
    unsigned char *s, *tmpPointer;
    jint w = 0;
    int ch;
    int cnt;
    jobject widths;
    jint *tempWidths;
    jint maxAdvance;
    int widlen;

    if (JNU_IsNull(env, str)) {
        NullPointerException(env, "NullPointerException");

        return 0;
    }
    cnt = (*env)->GetArrayLength(env, str);

    if (cnt == 0) {
        return 0;
    }

    widths = GETFIELD(Object, this, fontMetricsImpl, widths);
    maxAdvance = GETFIELD(Int, this, fontMetricsImpl, maxAdvance);

    if (!JNU_IsNull(env, widths)) {
        w = 0;
        widlen = (*env)->GetArrayLength(env, widths);
	tempWidths = (*env)->GetIntArrayElements(env, widths, NULL);
        
        s = tmpPointer = (unsigned char *) 
	    (*env)->GetByteArrayElements(env, str, NULL);

        if(s == NULL)
            return 0;

        while (--cnt >= 0) {
            ch = *tmpPointer++;
            if (ch < widlen) {
                w += tempWidths[ch];
            } else {
                w += maxAdvance;
            }
        }

	(*env)->ReleaseIntArrayElements(env, widths, tempWidths, JNI_ABORT);
        (*env)->ReleaseByteArrayElements(env, str, (jbyte *) s, JNI_ABORT);    
    } else {
        w = maxAdvance * cnt;
    }
    
    return w;
}

/*
 * Class:     sun_awt_motif_X11FontMetrics
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_sun_awt_gfX_FontMetricsImpl_init(JNIEnv *env, jobject this)
{
    jobject font;
    struct FontData *fdata;
    jint tempWidths[256];
    jintArray widths;
    int ccount;
    int i;
    int tempWidthsIndex;
    char *err;

    if (JNU_IsNull(env, this)) {
        NullPointerException(env, "NullPointerException");
        return;
    }

    NATIVE_LOCK(env);
    font = GETFIELD(Object, this, fontMetricsImpl, font);
    if (JNU_IsNull(env, this)) {
        NullPointerException(env, "NullPointerException");
        NATIVE_UNLOCK(env);
        return;
    }
    fdata = GetFontData(env, font, (const char **)&err);
    if (fdata == NULL) {
        InternalError2(env, err);
        NATIVE_UNLOCK(env);
        return;
    }

    SETFIELD(Int, this, fontMetricsImpl, ascent,  (jint) fdata->xfont->ascent);
    SETFIELD(Int, this, fontMetricsImpl, descent, (jint) fdata->xfont->descent);
    SETFIELD(Int, this, fontMetricsImpl, leading, (jint) 1); /* ??? */
    SETFIELD(Int, this, fontMetricsImpl, height, 
	     (jint) fdata->xfont->ascent + fdata->xfont->descent + 1);
    SETFIELD(Int, this, fontMetricsImpl, maxAscent,
	     (jint) fdata->xfont->max_bounds.ascent);
    SETFIELD(Int, this, fontMetricsImpl, maxDescent,
	     (jint) fdata->xfont->max_bounds.descent);
    SETFIELD(Int, this, fontMetricsImpl, maxHeight,
	     (jint) fdata->xfont->max_bounds.ascent
	          + fdata->xfont->max_bounds.descent + 1);
    SETFIELD(Int, this, fontMetricsImpl, maxAdvance,
	     (jint) fdata->xfont->max_bounds.width);

    widths = (*env)->NewIntArray(env, 256);
    SETFIELD(Object, this, fontMetricsImpl, widths, widths);
    if (JNU_IsNull(env, widths)) {
        OutOfMemoryError(env, "OutOfMemoryError");
        NATIVE_UNLOCK(env);
        return;
    }
    /*
     * We could pin the array and then release it, but I believe this method
     * is faster and perturbs the VM less
     *
     */
    memset(tempWidths, 0, sizeof(tempWidths));

    tempWidthsIndex = fdata->xfont->min_char_or_byte2;

    ccount = fdata->xfont->max_char_or_byte2 - fdata->xfont->min_char_or_byte2;

    if (fdata->xfont->per_char) {
        for (i = 0; i <= ccount; i++) {
            tempWidths[tempWidthsIndex++] = (jint) fdata->xfont->per_char[i].width;
        }
    } else {
        for (i = 0; i <= ccount; i++) {
            tempWidths[tempWidthsIndex++] = (jint) fdata->xfont->max_bounds.width;
        }
    }

    (*env)->SetIntArrayRegion(env, widths, 0, 256, (jint *) tempWidths);

    NATIVE_UNLOCK(env);
}
