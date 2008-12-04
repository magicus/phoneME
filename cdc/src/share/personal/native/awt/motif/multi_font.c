/*
 * @(#)multi_font.c	1.44 06/10/10
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

/*
 * These routines are used for display string with multi font.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "awt_p.h"
#include "awt_Font.h"
#include "multi_font.h"


XmFontList 
getFontList(JNIEnv * env, jobject font)
{
	jobject         peer, xfsname;
	XmFontList      fontlist;
	XmFontListEntry fontlistentry;
	char           *xfsStr;
	char            newstr[500], tmpstr[500], *tok;
	int             fontsize;

	static int      tagcnt;
	char            tagstr[20];
	jstring         fonttag;

	if((peer = (*env)->GetObjectField(env, font, MCachedIDs.java_awt_Font_peerFID))==NULL)
	  return NULL;

	fontlist = (XmFontList) (*env)->GetIntField(env, peer, MCachedIDs.MFontPeer_pDataFID);

	if (fontlist == NULL) {
		fontsize = (*env)->GetIntField(env, font, MCachedIDs.java_awt_Font_sizeFID) * 10;
		if ((xfsname = (*env)->GetObjectField(env, peer, MCachedIDs.MFontPeer_xfsnameFID)) == NULL)
			return NULL;

		xfsStr = (char *) (*env)->GetStringUTFChars(env, xfsname, NULL);

		strcpy(tmpstr, xfsStr);
		memset(newstr, '\0', sizeof(newstr));

		tok = strtok(tmpstr, ";,");
		while (tok != NULL) {
			sprintf(newstr + strlen(newstr), tok, fontsize);
			strcat(newstr, ",");
			tok = strtok(NULL, ";,");
		}

		sprintf(tagstr, "M-%d", tagcnt++);

		fontlistentry = XmFontListEntryLoad(awt_display, newstr, XmFONT_IS_FONTSET, tagstr);
		fontlist = XmFontListAppendEntry(NULL, fontlistentry);
		XmFontListEntryFree(&fontlistentry);

		XmRegisterSegmentEncoding(tagstr, XmFONTLIST_DEFAULT_TAG);

		(*env)->ReleaseStringUTFChars(env, xfsname, xfsStr);

		(*env)->SetIntField(env, peer, MCachedIDs.MFontPeer_pDataFID, (jint) fontlist);
		fonttag = (*env)->NewStringUTF(env, tagstr);
		(*env)->SetObjectField(env, peer, MCachedIDs.MFontPeer_fonttagFID, fonttag);
	}
	return fontlist;
}

#if 0

void 
getFontTag(JNIEnv * env, jobject font, char *tag)
{
	jobject         peer;
	jstring         fonttag;
	jbyteArray      jtag;
	int             blen;

	peer = (*env)->GetObjectField(env, font, MCachedIDs.java_awt_Font_peerFID);
	fonttag = (jstring) (*env)->GetObjectField(env, peer, MCachedIDs.MFontPeer_fonttagFID);

	jtag = (*env)->CallObjectMethod(env, fonttag, MCachedIDs.java_lang_String_getBytesMID);

	blen = (*env)->GetArrayLength(env, jtag);
	(*env)->GetByteArrayRegion(env, jtag, 0, blen, (jbyte *) tag);

	tag[blen] = '\0';
}

#endif
