/*
 * @(#)X11FontMetrics.c	1.14 06/10/10
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
#include "java_awt_X11FontMetrics.h"


JNIEXPORT void JNICALL
Java_java_awt_X11FontMetrics_initIDs(JNIEnv * env, jclass cls)
{
        GET_FIELD_ID(X11FontMetrics_ascentFID, "ascent", "I");
        GET_FIELD_ID(X11FontMetrics_descentFID, "descent", "I");
        GET_FIELD_ID(X11FontMetrics_heightFID, "height", "I");
        GET_FIELD_ID(X11FontMetrics_leadingFID, "leading", "I");
        GET_FIELD_ID(X11FontMetrics_maxAdvanceFID, "maxAdvance", "I");
        GET_FIELD_ID(X11FontMetrics_maxAscentFID, "maxAscent", "I");
        GET_FIELD_ID(X11FontMetrics_maxDescentFID, "maxDescent", "I");
        GET_FIELD_ID(X11FontMetrics_maxHeightFID, "maxHeight", "I");
}


#define max(a,b) ((a>b)?a:b)

JNIEXPORT int JNICALL
Java_java_awt_X11FontMetrics_pLoadFont(JNIEnv *env, jobject this, jbyteArray fontName)
{
    char *fname;
    int fnameLen;
    XFontSet  xfontset;

    int mchars;
    char **mcharset;
    char *mstrdrawn;


    XFontStruct **xfonts;
    char **xfontnames;
    int i, numFonts;

    int             ascent = 0, descent = 0;
    int             maxAscent = 0, maxDescent = 0;
    int             maxHeight = 0, maxAdvance = 0;

    
    fnameLen = (*env)->GetArrayLength(env, fontName);

    fname = (char *)calloc(fnameLen+1, sizeof(char));

    (*env)->GetByteArrayRegion(env, fontName, 0, fnameLen, (jbyte *)fname);

    xfontset = XCreateFontSet(xawt_display, fname, (char ***)&mcharset, &mchars, (char **)&mstrdrawn);

    if(mchars) XFreeStringList(mcharset);

    free(fname);
    
    if (xfontset == NULL)
    {
    	xfontset = XCreateFontSet(xawt_display, "-adobe-courier-medium-r-*--*-120-*", (char ***)&mcharset, &mchars, (char **)&mstrdrawn);
    	
    	if(mchars) XFreeStringList(mcharset);
    }

    if(xfontset!=NULL) {

      numFonts = XFontsOfFontSet(xfontset, (XFontStruct ***)&xfonts, (char ***)&xfontnames);

      for (i = 0; i < numFonts; i++) {
	ascent = max(ascent, xfonts[i]->ascent);
	descent = max(descent, xfonts[i]->descent);
	maxAscent = max(maxAscent, xfonts[i]->max_bounds.ascent);
	maxDescent = max(maxDescent, xfonts[i]->max_bounds.descent);

	maxHeight = max(maxHeight, maxAscent + maxDescent);
	maxAdvance = max(maxAdvance, xfonts[i]->max_bounds.rbearing - xfonts[i]->min_bounds.lbearing);

      }

      (*env)->SetIntField(env, this, XCachedIDs.X11FontMetrics_ascentFID, (jint) ascent);
      (*env)->SetIntField(env, this, XCachedIDs.X11FontMetrics_descentFID, (jint) descent);
      (*env)->SetIntField(env, this, XCachedIDs.X11FontMetrics_leadingFID, (jint) 1);

      (*env)->SetIntField(env, this, XCachedIDs.X11FontMetrics_heightFID, (jint) (ascent + descent + 1));
      (*env)->SetIntField(env, this, XCachedIDs.X11FontMetrics_maxAscentFID, (jint) maxAscent);
      (*env)->SetIntField(env, this, XCachedIDs.X11FontMetrics_maxDescentFID, (jint) maxDescent);
      (*env)->SetIntField(env, this, XCachedIDs.X11FontMetrics_maxHeightFID, (jint) maxHeight);
      (*env)->SetIntField(env, this, XCachedIDs.X11FontMetrics_maxAdvanceFID, (jint) maxAdvance);
    }
 
 	else
 	{
 		(*env)->ThrowNew (env, XCachedIDs.AWTError, "Could not load font set");
 	}

    return (jint)xfontset;

}



JNIEXPORT int JNICALL
Java_java_awt_X11FontMetrics_pBytesWidth(JNIEnv *env, jobject this, jint fontSet, jbyteArray data, jint offset, jint length)
{
    char str[64], *strA;
    XRectangle extents;
    int reqAlloc = (length+1>sizeof(str));


    if(reqAlloc)
      strA = (char *)calloc(length+1, sizeof(char));


    /* test for null and error conditions */

      (*env)->GetByteArrayRegion(env, data, offset, length, (jbyte *)(reqAlloc?strA:str));

      XmbTextExtents((XFontSet)fontSet, str, length, NULL, &extents);

      if(reqAlloc) free(strA);

      return (jint)extents.width;
}


