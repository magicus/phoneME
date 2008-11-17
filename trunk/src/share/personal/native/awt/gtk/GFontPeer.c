/*
 * @(#)GFontPeer.c	1.16 06/10/10
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
#include "sun_awt_gtk_GFontPeer.h"
#include "awt.h"

#define DEFAULT_FONT "-adobe-courier-medium-r-normal--*-120-*"

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFontPeer_initIDs (JNIEnv *env, jclass cls)
{
	cls = (*env)->FindClass (env, "java/awt/FontMetrics");
	
	if (cls == NULL)
		return;
		
	GET_FIELD_ID (java_awt_FontMetrics_fontFID, "font", "Ljava/awt/Font;");
	
	cls = (*env)->FindClass (env, "java/awt/Font");
	
	if (cls == NULL)
		return;
		
	GCachedIDs.java_awt_FontClass = (*env)->NewGlobalRef (env, cls);
	GET_METHOD_ID (java_awt_Font_contructorWithPeerMID, "<init>", "(Ljava/lang/String;IILsun/awt/peer/FontPeer;)V");
}

#if 0

JNIEXPORT void JNICALL
Java_sun_awt_gtk_GFontPeer_init (JNIEnv *env, jobject this, jobject font, jstring fontset, jint size)
{
	gchar *gfontset;
	char  sizedFontset[500], temp[500], *token;
	GdkFont *gdkfont;
	
	gfontset = (gchar *)(*env)->GetStringUTFChars (env, fontset, NULL);
	
	if (gfontset == NULL)
		return;
		
	/* Replace %d in fontset with size of font. */
	
	strcpy(temp, gfontset);
	memset(sizedFontset, '\0', sizeof(sizedFontset));

	token = strtok(temp, ",");
	
	while (token != NULL)
	{
		sprintf(sizedFontset + strlen(sizedFontset), token, (int)(size * 10));
		strcat(sizedFontset, ",");
		token = strtok(NULL, ",");
	}
	
	(*env)->ReleaseStringUTFChars (env, fontset, gfontset);
	
	awt_gtk_threadsEnter();
	gdkfont = gdk_fontset_load (sizedFontset);
	
	if (gdkfont == NULL)
		gdkfont = gdk_fontset_load (DEFAULT_FONT);
		
	awt_gtk_threadsLeave();
	
	(*env)->SetIntField (env, this, GCachedIDs.GFontPeer_dataFID, (jint)gdkfont);
	
	if (gdkfont == NULL)
	{
		char message[550];
		
		sprintf (message, "Could not load font set \"%s\" and default font \"" DEFAULT_FONT "\" not present", sizedFontset);
		(*env)->ThrowNew(env, GCachedIDs.java_awt_AWTErrorClass, message);
	}
	
	else
	{
		(*env)->SetIntField (env, this, GCachedIDs.GPlatformFont_ascentFID, (jint)gdkfont->ascent);
		(*env)->SetIntField (env, this, GCachedIDs.GPlatformFont_descentFID, (jint)gdkfont->descent);
	}
}

#endif

JNIEXPORT jboolean JNICALL
Java_sun_awt_gtk_GFontPeer_areFontsTheSame (JNIEnv *env, jclass cls, jint font1, jint font2)
{
	jboolean result;

	awt_gtk_threadsEnter();
	result = gdk_font_equal ((GdkFont *)font1, (GdkFont *)font2);
	awt_gtk_threadsLeave();
	
	return result;
}

JNIEXPORT jobject JNICALL
Java_sun_awt_gtk_GFontPeer_createFont (JNIEnv *env, jclass cls, jstring name, jobject peer, jint gfont)
{
	jobject font;
	jint style = java_awt_Font_PLAIN, size = 14;	/* TODO: Set these appropriately. */
/*	GdkFont *gdkfont = (GdkFont *)(*env)->GetIntField (env, peer, GCachedIDs.GFontPeer_dataFID); */
	GdkFont *gdkfont = (GdkFont *)gfont;
	
	if (gdkfont == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return NULL;
	}
	
	font = (*env)->NewObject (env, GCachedIDs.java_awt_FontClass,
							  GCachedIDs.java_awt_Font_contructorWithPeerMID,
							  name,
							  style,
							  size,
							  peer);
							  
	if (font == NULL)
		return NULL;

	/*
	(*env)->SetIntField (env, peer, GCachedIDs.GFontPeer_ascentFID, (jint)gdkfont->ascent);
	(*env)->SetIntField (env, peer, GCachedIDs.GFontPeer_descentFID, (jint)gdkfont->descent);
	 */
	(*env)->SetObjectField (env, peer, GCachedIDs.java_awt_FontMetrics_fontFID, font);
	
	return font;
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GFontPeer_asciiCharWidth (JNIEnv *env, jobject this, jchar c)
{
	jobject font;
	GdkFont *gdkfont;
	jint width;
	
	font = (*env)->GetObjectField (env, this, GCachedIDs.java_awt_FontMetrics_fontFID);
	
	if (font == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return 0;
	}
/*	
	gdkfont = (GdkFont *)(*env)->GetIntField (env, this, GCachedIDs.GFontPeer_dataFID);
	
	if (gdkfont == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return 0;
	}
*/	
	awt_gtk_threadsEnter();
	width = gdk_char_width (gdkfont, (gchar)c);
	awt_gtk_threadsLeave();
	
	return width;
}

JNIEXPORT jint JNICALL
Java_sun_awt_gtk_GFontPeer_stringWidthNative (JNIEnv *env, jobject this, jbyteArray string, jint gstringLen, jint gfont)
{
	const gchar *gstring;
/*	gint gstringLen;
	jobject font; */
	GdkFont *gdkfont;
	jint width; 

/*
	font = (*env)->GetObjectField (env, this, GCachedIDs.java_awt_FontMetrics_fontFID);
	
	if (font == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return 0;
	}
	
	gdkfont = (GdkFont *)(*env)->GetIntField (env, this, GCachedIDs.GFontPeer_dataFID);
*/
	
	gdkfont = (GdkFont *)gfont;
	if (gdkfont == NULL)
	{
		(*env)->ThrowNew (env, GCachedIDs.NullPointerExceptionClass, NULL);
		return 0;
	}

	
	gstring = (const gchar *)(*env)->GetByteArrayElements (env, string, NULL);
	
	if (gstring == NULL)
		return 0;

/*
	gstringLen = (*env)->GetArrayLength(env, string);
*/
	awt_gtk_threadsEnter();
	width = (jint)gdk_text_width (gdkfont, gstring, gstringLen);
	awt_gtk_threadsLeave();
	
	(*env)->ReleaseByteArrayElements (env, string, (jbyte *)gstring, JNI_ABORT);
	
	return width;
}
