/*
 * @(#)GPlatformFont.c	1.5 06/10/10
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

#include <gdk/gdk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "java_awt_Font.h"
#include "sun_awt_gtk_GPlatformFont.h"
#include "awt.h"

#define DEFAULT_FONT "-adobe-courier-medium-r-normal--*-120-*"


JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GPlatformFont_init (JNIEnv *env, jobject this, jstring font, jint size)
{
	gchar *gfont;
	GdkFont *gdkfont;
    char sizedFont[400];
	
	gfont = (gchar *)(*env)->GetStringUTFChars (env, font, NULL);
	
	if (gfont == NULL)
		return 0;
	
	/* Replace %d in fontset with size of font. */
	
	snprintf(sizedFont, sizeof(sizedFont), gfont, (int)(size * 10));
	
	(*env)->ReleaseStringUTFChars (env, font, gfont);
	
	awt_gtk_threadsEnter();
	gdkfont = gdk_font_load (sizedFont);
	
	if (gdkfont == NULL)
		gdkfont = gdk_font_load (DEFAULT_FONT);
	
	awt_gtk_threadsLeave();
	
/*	(*env)->SetIntField (env, this, GCachedIDs.GFontPeer_dataFID, (jint)gdkfont); */
	
	if (gdkfont == NULL)
	{
		char message[550];
		
		snprintf (message, sizeof(message), "Could not load font set \"%s\" and default font \"" DEFAULT_FONT "\" not present", sizedFont);
		(*env)->ThrowNew(env, GCachedIDs.java_awt_AWTErrorClass, message);
	}
	
	else
	{
		/* FIXME
		(*env)->SetIntField (env, this, GCachedIDs.GFontPeer_ascentFID, (jint)gdkfont->ascent);
		(*env)->SetIntField (env, this, GCachedIDs.GFontPeer_descentFID, (jint)gdkfont->descent);
		*/
	}
	
	return (jint)gdkfont;
	
}

