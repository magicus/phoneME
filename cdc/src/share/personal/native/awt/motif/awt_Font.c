/*
 * @(#)awt_Font.c	1.70 06/10/10
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
#include "awt_p.h"
#include <string.h>
#include "java_awt_Component.h"
#include "java_awt_Font.h"
#include "java_awt_FontMetrics.h"
#include "sun_awt_motif_MToolkit.h"
#include "sun_awt_motif_X11FontMetrics.h"
#include "sun_awt_motif_MFontPeer.h"	/* bug 4068618 */

#include "java_awt_Dimension.h"
#include "awt_Font.h"
#include "multi_font.h"

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFontPeer_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.MFontPeer_pDataFID = (*env)->GetFieldID(env, cls, "pData", "I");
	MCachedIDs.MFontPeer_xfsnameFID = (*env)->GetFieldID(env, cls, "xfsname", "Ljava/lang/String;");
	MCachedIDs.MFontPeer_fonttagFID = (*env)->GetFieldID(env, cls, "fonttag", "Ljava/lang/String;");
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11FontMetrics_initIDs(JNIEnv * env, jclass cls)
{
	MCachedIDs.X11FontMetrics_ascentFID = (*env)->GetFieldID(env, cls, "ascent", "I");
	MCachedIDs.X11FontMetrics_descentFID = (*env)->GetFieldID(env, cls, "descent", "I");
	MCachedIDs.X11FontMetrics_heightFID = (*env)->GetFieldID(env, cls, "height", "I");
	MCachedIDs.X11FontMetrics_leadingFID = (*env)->GetFieldID(env, cls, "leading", "I");
	MCachedIDs.X11FontMetrics_maxAdvanceFID = (*env)->GetFieldID(env, cls, "maxAdvance", "I");
	MCachedIDs.X11FontMetrics_maxAscentFID = (*env)->GetFieldID(env, cls, "maxAscent", "I");
	MCachedIDs.X11FontMetrics_maxDescentFID = (*env)->GetFieldID(env, cls, "maxDescent", "I");
	MCachedIDs.X11FontMetrics_maxHeightFID = (*env)->GetFieldID(env, cls, "maxHeight", "I");
	MCachedIDs.X11FontMetrics_widthsFID = (*env)->GetFieldID(env, cls, "widths", "[I");
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_X11FontMetrics_init(JNIEnv * env, jobject this)
{
	jobject         font;
	XmFontList      fontlist;
	XmFontContext   context;
	XmFontType      ret_type;
	XmFontListEntry flentry;
	XtPointer       xfontentry;
	XFontStruct   **xfont;

	char          **xfontname;

	Boolean         ret;

	int             i, num_font = 0;

	int             ascent = 0, descent = 0;
	int             maxAscent = 0, maxDescent = 0;
	int             maxHeight = 0, maxAdvance = 0;

	/* TODO: check the need of these variables
          jintArray       widths; 
          jint            dummyInt[256]; */

	if (this == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	font = (*env)->GetObjectField(env, this, MCachedIDs.java_awt_FontMetrics_fontFID);

	if (font == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	fontlist = getFontList(env, font);

	if (fontlist == NULL) {
		AWT_UNLOCK();
		return;
	}
	ret = XmFontListInitFontContext(&context, fontlist);

	while ((flentry = XmFontListNextEntry(context)) != NULL) {
		xfontentry = XmFontListEntryGetFont(flentry, &ret_type);

		if (ret_type == XmFONT_IS_FONTSET)
			num_font = (int) XFontsOfFontSet((XFontSet) xfontentry, &xfont, &xfontname);
		else if (ret_type == XmFONT_IS_FONT) {
			num_font = (xfontentry==NULL)?0:1;
			xfont = (XFontStruct **) & xfontentry;
		} else
			continue;

		for (i = 0; i < num_font; i++) {
			ascent = max(ascent, xfont[i]->ascent);
			descent = max(descent, xfont[i]->descent);
			maxAscent = max(maxAscent, xfont[i]->max_bounds.ascent);
			maxDescent = max(maxDescent, xfont[i]->max_bounds.descent);

			maxHeight = max(maxHeight, maxAscent + maxDescent);
			maxAdvance = max(maxAdvance, xfont[i]->max_bounds.rbearing - xfont[i]->min_bounds.lbearing);
		}
	}

	XmFontListFreeFontContext(context);

	(*env)->SetIntField(env, this, MCachedIDs.X11FontMetrics_ascentFID, (jint) ascent);
	(*env)->SetIntField(env, this, MCachedIDs.X11FontMetrics_descentFID, (jint) descent);
	(*env)->SetIntField(env, this, MCachedIDs.X11FontMetrics_leadingFID, (jint) 1);

	(*env)->SetIntField(env, this, MCachedIDs.X11FontMetrics_heightFID, (jint) (ascent + descent + 1));
	(*env)->SetIntField(env, this, MCachedIDs.X11FontMetrics_maxAscentFID, (jint) maxAscent);
	(*env)->SetIntField(env, this, MCachedIDs.X11FontMetrics_maxDescentFID, (jint) maxDescent);
	(*env)->SetIntField(env, this, MCachedIDs.X11FontMetrics_maxHeightFID, (jint) maxHeight);
	(*env)->SetIntField(env, this, MCachedIDs.X11FontMetrics_maxAdvanceFID, (jint) maxAdvance);

	/*
	widths = (*env)->NewIntArray(env, 256);

	if (widths == NULL) {
		AWT_UNLOCK();
		return;
	}

	(*env)->SetObjectField(env, this, MCachedIDs.X11FontMetrics_widthsFID, widths);

	for (i = 0; i < 256; i++)
		dummyInt[i] = (jint) maxAdvance;

	(*env)->SetIntArrayRegion(env, widths, 0, 256, dummyInt);

	if ((*env)->ExceptionCheck(env)) {
		AWT_UNLOCK();
		return;
	}
	*/
}


JNIEXPORT void  JNICALL
Java_java_awt_Font_dispose(JNIEnv * env, jobject this)
{
	/* this code eliminated to fix bug 4068618 */
}

/* bug 4068618 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MFontPeer_dispose(JNIEnv * env, jobject this)
{
	XmFontList     *fontData;

	if (this == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return;
	}
	AWT_LOCK();

	fontData = (XmFontList *) (*env)->GetIntField(env, this, MCachedIDs.MFontPeer_pDataFID);

	if(fontData!=NULL) XtFree((char *)fontData);

	(*env)->SetIntField(env, this, MCachedIDs.MFontPeer_pDataFID, (jint) NULL);
	AWT_UNLOCK();
}



JNIEXPORT jint  JNICALL
Java_sun_awt_motif_X11FontMetrics_bytesWidth(JNIEnv * env, jobject this, jbyteArray data, jint off, jint len)
{
        jobject         font;
	jbyte           *bytereg;
	int             blen, strwidth;
	XmString        xim;
	XmFontList      fontlist;

	if (data == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return 0;
	}
	AWT_LOCK();

	strwidth = 0;

	font = (*env)->GetObjectField(env, this, MCachedIDs.java_awt_FontMetrics_fontFID);

	if (font == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_NullPointerExceptionClass, NULL);
		return strwidth;
	}
	fontlist = getFontList(env, font);

	if (fontlist == NULL) {
		AWT_UNLOCK();
		return strwidth;
	}

	blen = len+1;

	bytereg = (jbyte *)XtCalloc(blen, sizeof(jbyte));

	if(bytereg!=NULL) {
	  (*env)->GetByteArrayRegion(env, data, off, len, bytereg);
	  bytereg[len] = '\0';
	  if((xim = XmStringCreateLocalized((char *) bytereg))!=NULL) {

	    strwidth = XmStringWidth(fontlist, xim);

	    XmStringFree(xim);
	  }

	  XtFree((char *)bytereg);
	}

	return strwidth;
	/* AWT_FLUSH_UNLOCK(); */

}
