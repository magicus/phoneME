/*
 * @(#)MWFontMetrics.c	1.13 06/10/10
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

#include <stdlib.h>

#include "awt.h"
#include "java_awt_MWFontMetrics.h"


JNIEXPORT void JNICALL
Java_java_awt_MWFontMetrics_initIDs (JNIEnv * env, jclass cls)
{
	GET_FIELD_ID (MWFontMetrics_ascentFID, "ascent", "I");
	GET_FIELD_ID (MWFontMetrics_descentFID, "descent", "I");
	GET_FIELD_ID (MWFontMetrics_heightFID, "height", "I");
	GET_FIELD_ID (MWFontMetrics_leadingFID, "leading", "I");
	GET_FIELD_ID (MWFontMetrics_maxAdvanceFID, "maxAdvance", "I");
	GET_FIELD_ID (MWFontMetrics_maxAscentFID, "maxAscent", "I");
	GET_FIELD_ID (MWFontMetrics_maxDescentFID, "maxDescent", "I");
	GET_FIELD_ID (MWFontMetrics_maxHeightFID, "maxHeight", "I");
}


JNIEXPORT jint JNICALL
Java_java_awt_MWFontMetrics_pLoadFont (JNIEnv * env, jobject this, jstring fontName, jint height)
{
	const char *fname;
	FontRenderer *fr = mwawt_hardwareInterface->fontRenderer;
	MWFont *font;
	MWFontInfo fontinfo;
	jboolean isCopy;

	fname = (*env)->GetStringUTFChars (env, fontName, &isCopy);
	font = fr->CreateFont (fname, height);
	(*env)->ReleaseStringUTFChars (env, fontName, fname);

	if (font == NULL)
	{
		(*env)->ThrowNew (env, MWCachedIDs.AWTError, "Could not load font");
		return 0;
	}

	font->GetFontInfo (font, &fontinfo);

	(*env)->SetIntField (env, this, MWCachedIDs.MWFontMetrics_ascentFID, (jint) fontinfo.ascent);
	(*env)->SetIntField (env, this, MWCachedIDs.MWFontMetrics_descentFID, (jint) fontinfo.descent);
	(*env)->SetIntField (env, this, MWCachedIDs.MWFontMetrics_leadingFID, (jint) fontinfo.leading);

	(*env)->SetIntField (env, this, MWCachedIDs.MWFontMetrics_heightFID, (jint) (fontinfo.ascent + fontinfo.descent + fontinfo.leading));
	(*env)->SetIntField (env, this, MWCachedIDs.MWFontMetrics_maxAscentFID, (jint) fontinfo.maxAscent);
	(*env)->SetIntField (env, this, MWCachedIDs.MWFontMetrics_maxDescentFID, (jint) fontinfo.maxDescent);
	/*(*env)->SetIntField (env, this, MWCachedIDs.MWFontMetrics_maxHeightFID, (jint) fontinfo.maxHeight);*/
	(*env)->SetIntField (env, this, MWCachedIDs.MWFontMetrics_maxAdvanceFID, (jint) fontinfo.maxAdvance);

	return (jint) font;
}

JNIEXPORT void JNICALL
Java_java_awt_MWFontMetrics_pDestroyFont(JNIEnv *env, jclass cls, jint font)
{
	MWFont *mwfont = (MWFont *)font;

    mwfont->Destroy(mwfont);
}

JNIEXPORT jint JNICALL
Java_java_awt_MWFontMetrics_pCharWidth (JNIEnv * env, jclass cls, jint font, jchar c)
{
	MWFont *mwfont = (MWFont *)font;

	return mwfont->CharWidth(mwfont, (UNICODE_CHAR)c);
}

JNIEXPORT jint JNICALL
Java_java_awt_MWFontMetrics_pCharsWidth (JNIEnv * env, jclass cls, jint font, jcharArray charArray,
										 jint offset, jint length)
{
	jchar *chars;
	jint width;
	MWFont *mwfont = (MWFont *)font;

	chars = malloc (sizeof(jchar) * length);

	if (chars == NULL)
	{
		(*env)->ThrowNew (env, MWCachedIDs.OutOfMemoryError, NULL);
		return 0;
	}

	(*env)->GetCharArrayRegion(env, charArray, offset, length, chars);

	if ((*env)->ExceptionCheck(env))
	{
		free(chars);
		return 0;
	}

	width = (jint)mwfont->StringWidth(mwfont, (UNICODE_CHAR *)chars, length);
	free(chars);

	return width;
}

JNIEXPORT jint JNICALL
Java_java_awt_MWFontMetrics_pStringWidth (JNIEnv * env, jclass cls, jint font, jstring string)
{
	const jchar *chars;
	jboolean isCopy;
	jsize length;
	MWFont *mwfont = (MWFont *)font;
	jint width;

	chars = (*env)->GetStringChars(env, string, &isCopy);

	if (chars == NULL)
		return 0;

	length = (*env)->GetStringLength(env, string);

	width = (jint)mwfont->StringWidth(mwfont, (UNICODE_CHAR *)chars, length);

	(*env)->ReleaseStringChars(env, string, chars);

	return width;
}
