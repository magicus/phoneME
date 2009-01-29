/*
 * @(#)FontImpl.c	1.16 06/10/10
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
#include "java_awt_Font.h"
#include "Xheaders.h"
#include "common.h"
#include <string.h>

typedef char Boolean;

JNIACCESS(java_awt_Font);
JNIACCESS(sun_awt_FontDescriptor);
JNIACCESS(sun_io_CharToByteConverter);
JNIACCESS(fontImpl);

JNIEXPORT void JNICALL 
Java_sun_awt_gfX_FontImpl_initJNI(JNIEnv *env, jclass cl)
{
    DEFINECLASS(fontImpl, cl);
    DEFINEFIELD(fontImpl, componentFonts, "[Lsun/awt/FontDescriptor;");
    DEFINEFIELD(fontImpl, props, "Ljava/util/Properties;");
    DEFINEMETHOD(fontImpl, makeConvertedString, "([CII)[Ljava/lang/Object;");

    FINDCLASS(java_awt_Font, "java/awt/Font");
    DEFINEFIELD(java_awt_Font, pData, "I");
    DEFINEFIELD(java_awt_Font, size,  "I");
    DEFINEFIELD(java_awt_Font, style,  "I");
    DEFINEFIELD(java_awt_Font, peer,   "Lsun/awt/peer/FontPeer;");
    DEFINEFIELD(java_awt_Font, family, "Ljava/lang/String;");

    FINDCLASS(sun_awt_FontDescriptor, "sun/awt/FontDescriptor");
    DEFINEFIELD(sun_awt_FontDescriptor, nativeName, "Ljava/lang/String;");
    DEFINEFIELD(sun_awt_FontDescriptor, fontCharset,
		"Lsun/io/CharToByteConverter;");

    FINDCLASS(sun_io_CharToByteConverter, "sun/io/CharToByteConverter");
    DEFINEMETHOD(sun_io_CharToByteConverter, toString, 
		 "()Ljava/lang/String;");
}

#define ZALLOC(T)	((struct T *)sysCalloc(1, sizeof(struct T)))

#define defaultXLFD "-*-helvetica-*-*-*-*-*-*-12-*-*-*-iso8859-1"

static XFontStruct *
loadFont(Display * display, char *name, int pointSize)
{

    XFontStruct *f;

    assert(NATIVE_LOCK_HELD());
    /*
     * try the exact xlfd name in for.properties<.lang>
     */
    f = XLoadQueryFont(display, name);
    if (f != NULL) {
        return f;
    }
    /*
     * try nearly font
     *
     *  1. specify FAMILY_NAME, WEIGHT_NAME, SLANT, POINT_SIZE, 
     *     CHARSET_REGISTRY and CHARSET_ENCODING.
     *  2. change POINT_SIZE to PIXEL_SIZE
     *  3. change FAMILY_NAME to *
     *  4. specify only PIXEL_SIZE and CHARSET_REGISTRY/ENCODING
     *  5. change PIXEL_SIZE +1/-1/+2/-2...+4/-4 
     *  6. default font pattern
     */
    {
        /*
         * This code assumes the name contains exactly 14 '-' delimiter.
         * If not use default pattern.
         */
        int i, pixelSize;
        Boolean useDefault = FALSE;

        char buffer[BUFSIZ], buffer2[BUFSIZ], *family, *style, *slant, *encoding, *start, *end;

        if (strlen(name) > BUFSIZ - 1) {
            useDefault = TRUE;
        } else {
            strcpy(buffer, name);
        }

#define NEXT_HYPHEN\
        start = end + 1;\
        end = strchr(start, '-');\
        if (end == NULL) {\
                              useDefault = TRUE;\
        break;\
        }\
        *end = '\0'

             do {
                 end = buffer;

                 /* skip FOUNDRY */
                 NEXT_HYPHEN;

                 /* set FAMILY_NAME */
                 NEXT_HYPHEN;
                 family = start;

                 /* set STYLE_NAME */
                 NEXT_HYPHEN;
                 style = start;

                 /* set SLANT */
                 NEXT_HYPHEN;
                 slant = start;

                 /* skip SETWIDTH_NAME, ADD_STYLE_NAME, PIXEL_SIZE
                    POINT_SIZE, RESOLUTION_X, RESOLUTION_Y, SPACING
                    and AVERAGE_WIDTH */
                 NEXT_HYPHEN;
                 NEXT_HYPHEN;
                 NEXT_HYPHEN;
                 NEXT_HYPHEN;
                 NEXT_HYPHEN;
                 NEXT_HYPHEN;
                 NEXT_HYPHEN;
                 NEXT_HYPHEN;

                 /* set CHARSET_REGISTRY and CHARSET_ENCODING */
                 encoding = end + 1;
             }
             while (0);

#define TRY_LOAD\
        f = XLoadQueryFont(display, buffer2);\
        if (f != NULL) {\
                            strcpy(name, buffer2);\
        return f;\
        }

        if (useDefault == FALSE) {
            /* try 1. */
            jio_snprintf(buffer2, sizeof(buffer2),
                         "-*-%s-%s-%s-*-*-*-%d-*-*-*-*-%s",
                         family, style, slant, pointSize, encoding);
            TRY_LOAD;

            /* search bitmap font */
            pixelSize = pointSize / 10;

            /* try 2. */
            jio_snprintf(buffer2, sizeof(buffer2),
                         "-*-%s-%s-%s-*-*-%d-*-*-*-*-*-%s",
                         family, style, slant, pixelSize, encoding);
            TRY_LOAD;

            /* try 3 */
            jio_snprintf(buffer2, sizeof(buffer2),
                         "-*-*-%s-%s-*-*-%d-*-*-*-*-*-%s",
                         style, slant, pixelSize, encoding);

            /* try 4. */
            jio_snprintf(buffer2, sizeof(buffer2),
                         "-*-*-*-*-*-*-%d-*-*-*-*-*-%s",
                         pixelSize, encoding);
            TRY_LOAD;

            /* try 5. */
            for (i = 1; i < 4; i++) {
                if (pixelSize < i)
                    break;
                jio_snprintf(buffer2, sizeof(buffer2),
                             "-*-%s-%s-%s-*-*-%d-*-*-*-*-*-%s",
                             family, style, slant, pixelSize + i, encoding);
                TRY_LOAD;

                jio_snprintf(buffer2, sizeof(buffer2),
                             "-*-%s-%s-%s-*-*-%d-*-*-*-*-*-%s",
                             family, style, slant, pixelSize - i, encoding);
                TRY_LOAD;

                jio_snprintf(buffer2, sizeof(buffer2),
                             "-*-*-*-*-*-*-%d-*-*-*-*-*-%s",
                             pixelSize + i, encoding);
                TRY_LOAD;

                jio_snprintf(buffer2, sizeof(buffer2),
                             "-*-*-*-*-*-*-%d-*-*-*-*-*-%s",
                             pixelSize - i, encoding);
                TRY_LOAD;
            }
        }
    }

    strcpy(name, defaultXLFD);
    return XLoadQueryFont(display, defaultXLFD);
}

/*
 * Hardwired list of mappings for generic font names "Helvetica",
 * "TimesRoman", "Courier", "Dialog", and "DialogInput".
 */
static const char *defaultfontname = "fixed";
static const char *defaultfoundry = "misc";
static const char *anyfoundry = "*";
static const char *anystyle = "*-*";
static const char *isolatin1 = "iso8859-1";

static const char *
Style(int s)
{
    switch (s) {
        case java_awt_Font_ITALIC:
            return "medium-i";
        case java_awt_Font_BOLD:
            return "bold-r";
        case java_awt_Font_BOLD + java_awt_Font_ITALIC:
            return "bold-i";
        case java_awt_Font_PLAIN:
        default:
            return "medium-r";
    }
}


static int 
FontName(JNIEnv * env, jstring name, 
	 const char **foundry, const char **facename, const char **encoding)
{
    const char *cname;

    if (JNU_IsNull(env, name)) {
        return 0;
    }
    cname = (const char *) (*env)->GetStringUTFChars(env, name, 0);

    /* additional default font names */
    if (!strcmp(cname, "Serif")) {
        *foundry = "adobe";
        *facename = "times";
        *encoding = isolatin1;
    } else if (!strcmp(cname, "SansSerif")) {
        *foundry = "adobe";
        *facename = "helvetica";
        *encoding = isolatin1;
    } else if (!strcmp(cname, "Monospaced")) {
        *foundry = "adobe";
        *facename = "courier";
        *encoding = isolatin1;
    } else if (strcmp(cname, "Helvetica") == 0) {
        *foundry = "adobe";
        *facename = "helvetica";
        *encoding = isolatin1;
    } else if (strcmp(cname, "TimesRoman") == 0) {
        *foundry = "adobe";
        *facename = "times";
        *encoding = isolatin1;
    } else if (strcmp(cname, "Courier") == 0) {
        *foundry = "adobe";
        *facename = "courier";
        *encoding = isolatin1;
    } else if (strcmp(cname, "Dialog") == 0) {
        *foundry = "b&h";
        *facename = "lucida";
        *encoding = isolatin1;
    } else if (strcmp(cname, "DialogInput") == 0) {
        *foundry = "b&h";
        *facename = "lucidatypewriter";
        *encoding = isolatin1;
    } else if (strcmp(cname, "ZapfDingbats") == 0) {
        *foundry = "itc";
        *facename = "zapfdingbats";
        *encoding = "*-*";
    } else {
#ifdef DEBUG
        jio_fprintf(stderr, "Unknown font: %s\n", cname);
#endif
        *foundry = defaultfoundry;
        *facename = defaultfontname;
        *encoding = isolatin1;
    }

    if (cname)
	(*env)->ReleaseStringUTFChars(env, name, cname);

    return 1;
}

static jboolean 
IsMultiFont(JNIEnv * env, jobject this)
{
    jobject peer;
    jobject props;

    if (this == 0) {
        return JNI_FALSE;
    }

    peer = GETFIELD(Object, this, java_awt_Font, peer);
    if (peer == 0) {
        return JNI_FALSE;
    }

    props = GETFIELD(Object, peer, fontImpl, props);
    if (props == 0) {
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

struct FontData *
GetFontData(JNIEnv * env, jobject font, const char **errmsg)
{

    if (!JNU_IsNull(env, font) && IsMultiFont(env, font)) {
        struct FontData *fdata;
        int i, size;
        const char *nativename;
	const char *ctmp;
        jobjectArray componentFonts;
        jobject peer;
        jobject fontDescriptor;
        jstring fontDescriptorName;
        jobject charToByteConverter;
        jstring charToByteConverterName;

	fdata = (struct FontData *) PDATA(font, java_awt_Font);
        if (fdata != NULL && fdata->flist != NULL) {
            return fdata;
        }

        size = GETFIELD(Int, font, java_awt_Font, size);
        fdata = (struct FontData *) ZALLOC(FontData);
	if (fdata == NULL) {
	    if (errmsg) {
		*errmsg = "java/lang" "OutOfMemoryError";
	    }
	    return NULL;
	}
	
	peer = GETFIELD(Object, font, java_awt_Font, peer);

        componentFonts = GETFIELD(Object, peer, fontImpl, componentFonts);

        fdata->charset_num = (*env)->GetArrayLength(env, componentFonts);

        fdata->flist = (awtFontList *) sysCalloc((unsigned)fdata->charset_num,
						 sizeof(awtFontList));
	if (fdata->flist == NULL) {
	    if (errmsg) {
		*errmsg = "java/lang" "OutOfMemoryError";
	    }
	    return NULL;
	}

        fdata->xfont = NULL;
	NATIVE_LOCK(env);
        for (i = 0; i < fdata->charset_num; i++) {
            /*
             * set xlfd name
             */

            fontDescriptor = (*env)->GetObjectArrayElement(env, componentFonts, i);
            fontDescriptorName = GETFIELD(Object, fontDescriptor,
					  sun_awt_FontDescriptor, nativeName);

            if (!JNU_IsNull(env, fontDescriptorName)) {
                nativename = (const char *) 
		    (*env)->GetStringUTFChars(env, fontDescriptorName, 0);
            } else {
                nativename = (const char *)"";
            }

            fdata->flist[i].xlfd = sysMalloc(strlen(nativename)
					     + strlen(defaultXLFD));
	    if (fdata->flist[i].xlfd == NULL) {
		if (errmsg) {
		    *errmsg = "java/lang" "OutOfMemoryError";
		}
		NATIVE_UNLOCK(env);
		return NULL;
	    }

            jio_snprintf(fdata->flist[i].xlfd, 
			 strlen(nativename) + strlen(defaultXLFD),
                         nativename, size * 10);

            if (nativename && (nativename != ""))
                (*env)->ReleaseStringUTFChars(env, fontDescriptorName, 
					      (const char *) nativename);

            /*
             * set charset_name
             */

            charToByteConverter = GETFIELD(Object, fontDescriptor,
					   sun_awt_FontDescriptor, fontCharset);
	    
            charToByteConverterName = 
		CALLMETHOD(Object, charToByteConverter,
			   sun_io_CharToByteConverter, toString);

	    ctmp = (*env)->GetStringUTFChars(env, charToByteConverterName, 0);
            fdata->flist[i].charset_name = (char *) malloc(strlen(ctmp) + 1);
	    if (fdata->flist[i].charset_name == NULL) {
		if (errmsg) {
		    *errmsg = "java/lang" "OutOfMemoryError";
		}
		NATIVE_UNLOCK(env);
		return NULL;
	    }

	    strcpy(fdata->flist[i].charset_name, ctmp);
	    (*env)->ReleaseStringUTFChars(env, charToByteConverterName, ctmp);

            /*
             * set load & XFontStruct 
             */
            fdata->flist[i].load = 0;

            if (fdata->xfont == NULL &&
                strstr(fdata->flist[i].charset_name, "8859_1")) {
                if ((fdata->flist[i].xfont =
		     loadFont(display,
                             fdata->flist[i].xlfd, size * 10))) {
                    fdata->flist[i].load = 1;

                    fdata->xfont = fdata->flist[i].xfont;
                    fdata->flist[i].index_length = 1;
                } else {
                    if (errmsg) {
                        *errmsg = "java/lang" "NullPointerException";
                    }
		    NATIVE_UNLOCK(env);
                    return NULL;
                }
            }
        }
        /*
         * XFontSet will create if the peer of TextField/TextArea
         * are used.
         */
        fdata->xfs = NULL;

	SETFIELD(Int, font, java_awt_Font, pData, (jint)fdata);

	NATIVE_UNLOCK(env);
        return fdata;
    } else {
        struct FontData *fdata;
        char fontSpec[1024];
        long height;
        long oheight;
        int above = 0;              /* tries above height */
        int below = 0;              /* tries below height */
        const char *foundry;
        const char *name;
        const char *encoding;
        const char *style;
        XFontStruct *xfont;
        jstring family;

        if (JNU_IsNull(env, font)) {
            if (errmsg) {
                *errmsg = "java/lang" "NullPointerException";
            }
            return (struct FontData *) NULL;
        }

	fdata = (struct FontData *) PDATA(font, java_awt_Font);
        if (fdata != NULL && fdata->xfont != NULL) {
            return fdata;
        }

	family = GETFIELD(Object, font, java_awt_Font, family);
        if (!FontName(env, family, 
		      (const char **)&foundry, 
		      (const char **)&name, 
		      (const char **)&encoding)) {
            if (errmsg) {
                *errmsg = "java/lang" "NullPointerException";
            }
            return (struct FontData *) NULL;
        }

	style = Style(GETFIELD(Int, font, java_awt_Font, style));
        oheight = height = GETFIELD(Int, font, java_awt_Font, size);

	NATIVE_LOCK(env);
        while (1) {
            jio_snprintf(fontSpec, sizeof(fontSpec), "-%s-%s-%s-*-*-%d-*-*-*-*-*-%s",
                         foundry,
                         name,
                         style,
                         height,
                         encoding);

            /*fprintf(stderr,"LoadFont: %s\n", fontSpec); */
            xfont = XLoadQueryFont(display, fontSpec);
            /* NOTE: sometimes XLoadQueryFont returns a bogus font structure */
            /* with negative ascent. */
            if (xfont == (Font) NULL || xfont->ascent < 0) {
                if (xfont) {
                    XFreeFont(display, xfont);
                }
                if (foundry != anyfoundry) {
                    /* Try any other foundry before messing with the sizes */
                    foundry = anyfoundry;
                    continue;
                }
                /* We couldn't find the font. We'll try to find an */
                /* alternate by searching for heights above and below our */
                /* preferred height. We try for 4 heights above and below. */
                /* If we still can't find a font we repeat the algorithm */
                /* using misc-fixed as the font. If we then fail, then we */
                /* give up and signal an error. */
                if (above == below) {
                    above++;
                    height = oheight + above;
                } else {
                    below++;
                    if (below > 4) {
                        if (name != defaultfontname || style != anystyle) {
                            name = defaultfontname;
                            foundry = defaultfoundry;
                            height = oheight;
                            style = anystyle;
                            encoding = isolatin1;
                            above = below = 0;
                            continue;
                        } else {
                            if (errmsg) {
                                *errmsg = "java/io/" "IOException";
                            }
			    NATIVE_UNLOCK(env);
                            return (struct FontData *) NULL;
                        }
                    }
                    height = oheight - below;
                }
                continue;
            } else {
                fdata = ZALLOC(FontData);

                if (fdata == NULL) {
                    if (errmsg) {
                        *errmsg = "java/lang" "OutOfMemoryError";
                    }
                } else {
                    fdata->xfont = xfont;
		    SETFIELD(Int, font, java_awt_Font, pData, (jint)fdata);
                }
		NATIVE_UNLOCK(env);
                return fdata;
            }
        }
	/* not reached */
    }
}


/*
 * Class:     java_awt_Font
 * Method:    pDispose
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_java_awt_Font_pDispose(JNIEnv *env, jobject this)
{
    struct FontData *fdata;
    int i;

    fdata = (struct FontData *) PDATA(this, java_awt_Font);
    if (fdata == NULL) {
        return;
    }

    if (IsMultiFont(env, this)) {
        for (i = 0; i < fdata->charset_num; i++) {
            sysFree((void *)fdata->flist[i].xlfd);
	    sysFree((void *)fdata->flist[i].charset_name);
        }
        sysFree((void *)fdata->flist);
    }

    NATIVE_LOCK(env);
    XFreeFont(display, fdata->xfont);
    NATIVE_UNLOCK(env);

    /* fdata = (struct FontData *) malloc(sizeof(struct FontData)) */;
    sysFree((void *)fdata);
    SETFIELD(Int, this, java_awt_Font, pData, 0);
    
    return;
}


static int 
GetFontDescriptorNumber(JNIEnv * env,
                               jobject font,
                               jobject fd)
{
    int i, num;
    /* initialize to 0 so that DeleteLocalRef will work. */
    jobjectArray componentFonts = 0;
    jobject peer = 0;
    jobject temp;
    jboolean validRet = JNI_FALSE;

    peer = GETFIELD(Object, font, java_awt_Font, peer);
    if (!peer)
        goto done;

    componentFonts = (jobjectArray) 
	GETFIELD(Object, peer, fontImpl, componentFonts);

    if (!componentFonts)
        goto done;

    num = (*env)->GetArrayLength(env, componentFonts);

    for (i = 0; i < num; i++) {
        temp = (*env)->GetObjectArrayElement(env, componentFonts, i);

        if ((*env)->IsSameObject(env, fd, temp)) {
            validRet = JNI_TRUE;
            break;
        }
    }

 done:

    if (validRet)
        return i;

    return 0;
}


void 
DrawMFString(JNIEnv *env, jcharArray s, GC gc, Drawable drawable,
	     jobject font, int x, int y, int offset, int sLength)
{
    jobjectArray dataArray;
    char *err;
    jbyte *stringData;
    char *offsetStringData;
    int stringCount,i;
    int size;
    struct FontData *fdata = GetFontData(env, font, (const char **)&err);
    jobject fontDescriptor;
    jbyteArray data;
    int j;
    int X = x;
    int Y = y;
    int length;
    XFontStruct *xf;

    if (!JNU_IsNull(env, s) && !JNU_IsNull(env, font)) {
        jobject peer;

	peer = GETFIELD(Object, font, java_awt_Font, peer);
        
        dataArray = CALLMETHOD3(Object, peer,
				fontImpl, makeConvertedString,
				s, offset, sLength);

        if ((*env)->ExceptionOccurred(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }

        if (dataArray == 0) {
            return;
        }
    } else {
        return;
    }

    stringCount = (*env)->GetArrayLength(env, dataArray);
    size = GETFIELD(Int, font, java_awt_Font, size);

    assert(NATIVE_LOCK_HELD());
    for (i = 0; i < stringCount; i+=2)
    {
        fontDescriptor = (*env)->GetObjectArrayElement(env, dataArray, i);
        data = (*env)->GetObjectArrayElement(env, dataArray, i + 1);

        /* Bail if we've finished */
        if (fontDescriptor == 0 || data == 0) {
            return;
	}

        /*
         * Most multi font strings aren't. So we delay adding
         * the offset until we know that there are additional
         * strings to be drawn
         */
        if(i > 0)
        {
            if (fdata->flist[j].index_length == 2) {
            } else {
            }
        }
        /*
         * Is it more expensive to copy the whole array, or realloc
         * it to the correct size during conversion? Another choice
         * might be to count the data converted and insert the length
         * at the start of the string.
         */

        j = GetFontDescriptorNumber(env, font, fontDescriptor);
	
	/* 
	 * The strange-looking code that follows this comment is
	 * the result of upstream optimizations. In the array of
	 * alternating font descriptor and buffers, the buffers
	 * contain their length in the first four bytes, a la
	 * Pascal arrays. There is another occurence of these below.
	 */
        stringData = (*env)->GetByteArrayElements(env, data, 0);
        length = (stringData[0] << 24) | (stringData[1] << 16) |
	    (stringData[2] << 8) | stringData[3];
        offsetStringData = (char *)(stringData + (4 * sizeof(char)));

        if (fdata->flist[j].load == 0) {
            xf = loadFont(display,
                          fdata->flist[j].xlfd, size * 10);
            if (xf == NULL) {
	        (*env)->ReleaseByteArrayElements(env, data, stringData, 
						      JNI_ABORT);
                continue;
            }
            fdata->flist[j].load = 1;
            fdata->flist[j].xfont = xf;
            if (xf->min_byte1 == 0 && xf->max_byte1 == 0) {
                fdata->flist[j].index_length = 1;
            } else {
                fdata->flist[j].index_length = 2;
	    }
        }
        xf = fdata->flist[j].xfont;
        XSetFont(display, gc, xf->fid);
        if (fdata->flist[j].index_length == 2) {
            XDrawString16(display, drawable, gc, X, Y,
                          (XChar2b *)offsetStringData, length/2);

            X += XTextWidth16(xf, (XChar2b *)offsetStringData, length/2);

        } else {
            XDrawString(display, drawable, gc, X, Y,
                        offsetStringData, length);

            X += XTextWidth(xf, offsetStringData, length);
        }

        (*env)->ReleaseByteArrayElements(env, data, stringData, JNI_ABORT);
    }
}

/*
 * get multi font string width with multiple X11 font
 */
long 
GetMFStringWidth(JNIEnv * env, jcharArray s,
		 int offset, int sLength, jobject font)
{
    char *err;
    jbyte *stringData;
    char *offsetStringData;
    int stringCount,i;
    int size;
    struct FontData *fdata;
    jobject fontDescriptor;
    jbyteArray data;
    int j;
    int width = 0;
    int length;
    XFontStruct *xf;
    jobjectArray dataArray;

    assert(NATIVE_LOCK_HELD());

    if (!JNU_IsNull(env, s) && !JNU_IsNull(env, font))
    {
        jobject peer;

	peer = GETFIELD(Object, font, java_awt_Font, peer);

        dataArray = CALLMETHOD3(Object, peer,
				fontImpl, makeConvertedString,
				s, offset, sLength);

        if ((*env)->ExceptionOccurred(env)) {
            (*env)->ExceptionDescribe(env);
            (*env)->ExceptionClear(env);
        }

        if (dataArray == 0) {
            return 0;
        }
    } else {
        return 0;
    }

    fdata = GetFontData(env, font, (const char **)&err);

    stringCount = (*env)->GetArrayLength(env, dataArray);

    size = GETFIELD(Int, font, java_awt_Font, size);

    for (i = 0; i < stringCount; i+=2)
    {
        fontDescriptor = (*env)->GetObjectArrayElement(env, dataArray, i);
        data = (*env)->GetObjectArrayElement(env, dataArray, i + 1);

        /* Bail if we've finished */
        if (fontDescriptor == 0 || data == 0) {
            break;
	}
        
        j = GetFontDescriptorNumber(env, font, fontDescriptor);

        if (fdata->flist[j].load == 0) {
            xf = loadFont(display,
                          fdata->flist[j].xlfd, size * 10);
            if (xf == NULL) {
                continue;
            }
            fdata->flist[j].load = 1;
            fdata->flist[j].xfont = xf;
            if (xf->min_byte1 == 0 && xf->max_byte1 == 0) {
                fdata->flist[j].index_length = 1;
            } else {
                fdata->flist[j].index_length = 2;
	    }
        }
        xf = fdata->flist[j].xfont;

        stringData = (*env)->GetByteArrayElements(env, data,NULL);
        length = (stringData[0] << 24) | (stringData[1] << 16) |
            (stringData[2] << 8) | stringData[3];
        offsetStringData = (char *)(stringData + (4 * sizeof(char)));

        if (fdata->flist[j].index_length == 2) {
            width += XTextWidth16(xf, (XChar2b *)offsetStringData, length/2);
        } else {
            width += XTextWidth(xf, offsetStringData, length);
        }

        (*env)->ReleaseByteArrayElements(env, data, stringData, JNI_ABORT);
    }

    return width;
}
