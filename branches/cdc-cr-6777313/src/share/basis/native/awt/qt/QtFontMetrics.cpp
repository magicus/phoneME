/*
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
 */

#include <stdlib.h>

#include "awt.h"
#include "java_awt_QtFontMetrics.h"
#include "java_awt_Font.h"
#include "QtApplication.h"

JNIEXPORT void JNICALL
Java_java_awt_QtFontMetrics_initIDs (JNIEnv * env, jclass cls)
{
	GET_FIELD_ID (QtFontMetrics_ascentFID, "ascent", "I");
	GET_FIELD_ID (QtFontMetrics_descentFID, "descent", "I");
	// 6286830
        /*GET_FIELD_ID (QtFontMetrics_heightFID, "height", "I");*/
	GET_FIELD_ID (QtFontMetrics_leadingFID, "leading", "I");
	GET_FIELD_ID (QtFontMetrics_maxAdvanceFID, "maxAdvance", "I");
	GET_FIELD_ID (QtFontMetrics_maxAscentFID, "maxAscent", "I");
	GET_FIELD_ID (QtFontMetrics_maxDescentFID, "maxDescent", "I");
	// Code cleanup while fixing 6286830
	/*GET_FIELD_ID (QtFontMetrics_maxHeightFID, "maxHeight", "I");*/
}


JNIEXPORT jint JNICALL
Java_java_awt_QtFontMetrics_pLoadFont (JNIEnv * env, jobject self, jstring fontName, jint height, jint style, jboolean st, jboolean ul)
{
	const char *fname;
	jboolean isCopy;
	int maxAdvance;
	int maxAscent = 0, maxDescent = 0;
	int ascent, descent, leading; // 6286830 qheight not needed.
	int c;

	fname = env->GetStringUTFChars (fontName, &isCopy);
	c =(int) env->GetStringUTFLength (fontName);
	
    AWT_QT_LOCK;
	QFont font(QString::fromUtf8(fname, c), height);

	env->ReleaseStringUTFChars (fontName, fname);
	
	switch(style) {
		case java_awt_Font_BOLD :   font.setBold(TRUE); break;
		case java_awt_Font_ITALIC : font.setItalic(TRUE); break;
		case java_awt_Font_BOLD +
			java_awt_Font_ITALIC :
						font.setBold(TRUE);
						font.setItalic(TRUE);
					break;
	}
	
	if(st == JNI_TRUE) font.setStrikeOut(TRUE);
	if(ul == JNI_TRUE) font.setUnderline(TRUE);

	QFontMetrics qfm(font);
	
	maxAscent = qfm.ascent();
	maxDescent = qfm.descent();
	leading = qfm.leading();
        // 6286830
        // See comments below.
	/*qheight = qfm.height();*/

    QFont *ret = new QFont(font);
    AWT_QT_UNLOCK;
	
    // 6286830
    // Use the default java.awt.FontMetrics.getHeight() implementation
    // instead of using the following if-block which is wrong.
    // Note that trolltech's doc for QFontMetrics::height() says that
    // it is always equal to ascent() + decent() + 1, which is different
    // from AWT's.

	/* Java expects ascent+descent+leading == height
		Can't do much if the numbers are greater ... */
        /*
	if(maxAscent+maxDescent+leading < qheight)
		leading = qheight - (maxAscent+maxDescent);
        */

	maxAdvance = -1;

	ascent = maxAscent;
	descent = maxDescent;

	env->SetIntField (self, QtCachedIDs.QtFontMetrics_ascentFID, (jint) ascent);
	env->SetIntField (self, QtCachedIDs.QtFontMetrics_descentFID, (jint) descent);
	env->SetIntField (self, QtCachedIDs.QtFontMetrics_leadingFID, (jint) leading);

        // 6286830
	/*env->SetIntField (self, QtCachedIDs.QtFontMetrics_heightFID, (jint) qheight);*/
	env->SetIntField (self, QtCachedIDs.QtFontMetrics_maxAscentFID, (jint) maxAscent);
	env->SetIntField (self, QtCachedIDs.QtFontMetrics_maxDescentFID, (jint) maxDescent);
	/*env->SetIntField (self, QtCachedIDs.QtFontMetrics_maxHeightFID, (jint) maxHeight);*/
	env->SetIntField (self, QtCachedIDs.QtFontMetrics_maxAdvanceFID, (jint) maxAdvance);

	return (jint)ret ;
}

JNIEXPORT void JNICALL
Java_java_awt_QtFontMetrics_pDestroyFont(JNIEnv *env, jclass cls, jint font)
{
    AWT_QT_LOCK;
	delete (QFont *)font;
    AWT_QT_UNLOCK;
}

JNIEXPORT jint JNICALL
Java_java_awt_QtFontMetrics_pStringWidth (JNIEnv * env, jclass cls, jint font, jstring string)
{
	const char *chars;
	jboolean isCopy;
	jsize length;
	jint width = 0;

	chars = env->GetStringUTFChars(string, &isCopy);

	if (chars == NULL)
		return 0;
		

    AWT_QT_LOCK;
	if((length = env->GetStringUTFLength(string)) > 0) {
	
		QFontMetrics qfm(*(QFont *)font);
		QString qs(QString::fromUtf8(chars, length));		
		
		// 5106120: TextAttribute.FONT leaves some extra space
		// in the begining and at the end of the string to which
		// it is applied.
		//
		// Use QFontMetrics::width() instead of size(), the latter
		// returns the bounding rectangle of the string, which is
		// not what we want.

		width = qfm.width(qs);

		/*
		QSize qsz = qfm.size(Qt::SingleLine, qs);
		
		width = qsz.width();
		*/
	}
    AWT_QT_UNLOCK;
		
	env->ReleaseStringUTFChars(string, chars);

	return width;
}
