/*
 * @(#)jniaccess.h	1.15 06/10/10
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
#define JNIACCESS(obj) struct _JNI##obj JNI##obj

#define MUSTLOAD(obj) (JNI##obj.class == 0)

extern int oops(const char *msg, const char *file, int line);

#define CHECKCLASS(obj)                                                \
do {                                                                   \
    if (JNI##obj.class == NULL) {                                      \
	fprintf(stderr, "file %s, line %d: Class " #obj " is null\n",  \
		__FILE__, __LINE__);                                   \
	fflush(stderr);                                                \
    }                                                                  \
} while (0)

#define CHECKFIELD(fid)                                              \
do {                                                             \
    if (JNI##fid == NULL) {                                      \
	fprintf(stderr, "file %s, line %d: " #fid " is null\n",  \
		__FILE__, __LINE__);                             \
	fflush(stderr);                                          \
    }                                                            \
} while (0)

#define CHECKMETHOD(mid) CHECKFIELD(mid)

#define DEFINEFIELD(obj, name, typ)                                         \
do {                                                                        \
    CHECKCLASS(obj);                                                        \
    JNI##obj.##name = (*env)->GetFieldID(env, JNI##obj.class, #name, typ);  \
    if (JNI##obj.##name == 0) {                                             \
	fprintf(stderr, "file %s, line %d: Could not find field " #name,    \
		__FILE__, __LINE__);                                        \
	fprintf(stderr, "\n"); fflush(stderr);                              \
    }                                                                       \
} while (0)

#define DEFINEMETHODALT(obj, name, nstr, typ)                                       \
do {                                                                           \
    CHECKCLASS(obj);                                                        \
    JNI##obj.##name = (*env)->GetMethodID(env, JNI##obj.class, nstr, typ);    \
    if (JNI##obj.##name == 0) {                                                \
	fprintf(stderr, "file %s, line %d: Could not find method " nstr typ, \
		__FILE__, __LINE__);                                           \
        fprintf(stderr, "\n"); fflush(stderr);                                 \
    }                                                                          \
} while (0)

#define DEFINEMETHOD(obj, name, typ) DEFINEMETHODALT(obj, name, #name, typ)

#define DEFINECONSTRUCTOR(obj, sig, typ)                                         \
do {                                                                             \
    CHECKCLASS(obj);                                                        \
    JNI##obj.new##sig = (*env)->GetMethodID(env, JNI##obj.class, "<init>", typ); \
    if (JNI##obj.new##sig == 0) {                                                \
	fprintf(stderr, "file %s, line %d: Could not find <init>" #sig,          \
		__FILE__, __LINE__);                                             \
        fprintf(stderr, "\n"); fflush(stderr);                                 \
    }                                                                            \
} while  (0)

#define DEFINECLASS(obj, cl) \
    JNI##obj.class = (*env)->NewGlobalRef(env, cl)

#define DEFINECONSTANT(obj, id, typ, t)                                       \
do {                                                                      \
    jfieldID f;                                                           \
    CHECKCLASS(obj);                                                      \
    f = (*env)->GetStaticFieldID(env, JNI##obj.class, #id, typ);          \
    if (f == 0) {                                                         \
	fprintf(stderr, "file %s, line %d: Can't define constant " #id,   \
		__FILE__, __LINE__);                                      \
	fprintf(stderr, "\n"); fflush(stderr);                            \
    }                                                                     \
    JNI##obj.##id = (*env)->GetStatic##t##Field(env, JNI##obj.class, f);  \
} while (0)

#define PDATA(obj, typ) GETFIELD(Int, obj, ##typ, pData)

#define FINDCLASS(obj, path)                                             \
do {                                                                     \
    jclass cl = (*env)->FindClass(env, path);                            \
    if (cl == 0) {                                                       \
	fprintf(stderr, "file %s, line %d: Could not find class " #obj,  \
		__FILE__, __LINE__);                                     \
        fprintf(stderr, "\n"); fflush(stderr);                           \
    }                                                                    \
    JNI##obj.class = (*env)->NewGlobalRef(env, cl);                      \
} while (0)

#define INSTANCEOF(instance, obj) \
    (((JNI##obj.class == 0) ? oops("JNI" #obj ": class is null", __FILE__, __LINE__) : 0), \
    (*env)->IsInstanceOf(env, instance, JNI##obj.class))

#define SETFIELD(typ, obj, cls, fid, value)                          \
do {                                                                 \
    CHECKFIELD(cls.fid);                                             \
    if (!(*env)->IsInstanceOf(env, obj, JNI##cls.class)) {           \
	oops(#obj " is not a " #cls "!", __FILE__, __LINE__);        \
    }                                                                \
    (*env)->Set##typ##Field(env, obj, JNI##cls.fid, value);          \
} while (0)

#define GETFIELD(typ, obj, cls, fid) \
    (((JNI##cls.fid == 0) ? oops("JNI" #fid ": field ID is null", __FILE__, __LINE__) : 0), \
    ((*env)->IsInstanceOf(env, obj, JNI##cls.class) ? 0 : oops(#obj " is not a " #cls "!", \
					  __FILE__, __LINE__)), \
    (*env)->Get##typ##Field(env, obj, JNI##cls.fid))

#define CALLMETHOD9(typ, obj, cls, mid, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
    (((JNI##cls.mid == 0) ? oops("JNI" #mid ": method ID is null", __FILE__, __LINE__) : 0), \
    ((*env)->IsInstanceOf(env, obj, JNI##cls.class) ? 0 : oops(#obj " is not a " #cls "!", \
					  __FILE__, __LINE__)), \
    (*env)->Call##typ##Method(env, obj, JNI##cls.mid, \
			      a1, a2, a3, a4, a5, a6, a7, a8, a9))

#define CALLMETHOD8(typ, obj, cls, mid, a1, a2, a3, a4, a5, a6, a7, a8) \
CALLMETHOD9(typ, obj, cls, mid, a1, a2, a3, a4, a5, a6, a7, a8, 0)

#define CALLMETHOD7(typ, obj, cls, mid, a1, a2, a3, a4, a5, a6, a7) \
CALLMETHOD9(typ, obj, cls, mid, a1, a2, a3, a4, a5, a6, a7, 0, 0)

#define CALLMETHOD6(typ, obj, cls, mid, a1, a2, a3, a4, a5, a6) \
CALLMETHOD9(typ, obj, cls, mid, a1, a2, a3, a4, a5, a6, 0, 0, 0)

#define CALLMETHOD5(typ, obj, cls, mid, a1, a2, a3, a4, a5) \
CALLMETHOD9(typ, obj, cls, mid, a1, a2, a3, a4, a5, 0, 0, 0, 0)

#define CALLMETHOD4(typ, obj, cls, mid, a1, a2, a3, a4) \
CALLMETHOD9(typ, obj, cls, mid, a1, a2, a3, a4, 0, 0, 0, 0, 0)

#define CALLMETHOD3(typ, obj, cls, mid, a1, a2, a3) \
CALLMETHOD9(typ, obj, cls, mid, a1, a2, a3, 0, 0, 0, 0, 0, 0)

#define CALLMETHOD2(typ, obj, cls, mid, a1, a2) \
CALLMETHOD9(typ, obj, cls, mid, a1, a2, 0, 0, 0, 0, 0, 0, 0)

#define CALLMETHOD1(typ, obj, cls, mid, a1) \
CALLMETHOD9(typ, obj, cls, mid, a1, 0, 0, 0, 0, 0, 0, 0, 0)

#define CALLMETHOD(typ, obj, cls, mid) \
CALLMETHOD9(typ, obj, cls, mid, 0, 0, 0, 0, 0, 0, 0, 0, 0)



/* package: sun.awt.gfX.  This is the default (current) package, so we use no prefix. */

extern struct _JNIcursorImage {
    jclass class;

    jfieldID hotX;	      /* I */
    jfieldID hotY;	      /* I */
    jfieldID pData;	      /* I */
    jfieldID image;	      /* Lsun/awt/image/Image */
} JNIcursorImage;

extern struct _JNIdrawableImageImpl {
    jclass class;

    jfieldID pData;	      /* I */
} JNIdrawableImageImpl;

extern struct _JNIfontImpl {
    jclass class;

    jfieldID componentFonts;  /* [Lsun/awt/FontDescriptor */
    jfieldID props;	      /* Ljava/util/Properties */
    jmethodID makeConvertedString; /* ([CII)[Ljava/lang/Object; */
} JNIfontImpl;

extern struct _JNIfontMetricsImpl {
    jclass class;

    jfieldID font;	      /* Ljava/awt/Font; */
    jfieldID ascent;	      /* I */
    jfieldID descent;	      /* I */
    jfieldID height;	      /* I */
    jfieldID leading;	      /* I */
    jfieldID maxAscent;	      /* I */
    jfieldID maxDescent;      /* I */
    jfieldID maxHeight;	      /* I */
    jfieldID maxAdvance;      /* I */
    jfieldID widths;	      /* [I */
} JNIfontMetricsImpl;

extern struct _JNIregionImpl {
    jclass class;

    jfieldID pData;	      /* I */
} JNIregionImpl;

extern struct _JNIgraphicsImpl {
    jclass class;

    jfieldID pData;	      /* I */
    jfieldID foreground;      /* Ljava/awt/Color */
    jfieldID xorColor;	      /* Ljava/awt/Color */
    jfieldID geometry;	      /* Lsun/porting/graphicssystem/GeometryProvider */
    jfieldID drawable;	      /* Lsun/porting/graphicssystem/Drawable */
    jmethodID setupClip;      /* (Ljava/awt/Rectangle)Lsun/porting/graphicssystem/Region */
} JNIgraphicsImpl;

extern struct _JNIgraphicsSystem {
    jclass class;

    jfieldID screenWidth;     /* I */
    jfieldID screenHeight;    /* I */
    jfieldID defaultScreen;   /* Lsun/porting/graphicssystem/Drawable */
    jfieldID colorModel;      /* Lsun/awt/image/ColorModel */
    jfieldID inputHandler;    /* Lsun/porting/windowsystem/EventHandler */
    jfieldID cursorX;	      /* I */
    jfieldID cursorY;	      /* I */
    jfieldID cursorHidden;    /* I */

    jmethodID handleKeyInput;   /* (III)V */
    jmethodID handleMouseInput;	/* (IIII)V */
    jmethodID handleExposeEvent; /* (IIIII)V */

    /* constants */
    jint NONE, PRESSED, RELEASED, ACTION, TYPED;
} JNIgraphicsSystem;

extern struct _JNIscreenImpl {
    jclass class;
} JNIscreenImpl;


/* package: sun.porting.utils */
extern struct _JNIsun_porting_utils_RegionImpl {
    jclass class;

    jfieldID bandList;	      /* Lsun/porting/utils/YXBand */
    jfieldID boundingBox;     /* Lsun/porting/utils/Rectangle */
} JNIsun_porting_utils_RegionImpl;

extern struct _JNIsun_porting_utils_YXBand {
    jclass class;

    jfieldID y;		      /* I */
    jfieldID h;		      /* I */
    jfieldID next;	      /* Lsun/porting/utils/YXband */
    jfieldID prev;	      /* Lsun/porting/utils/YXband */
    jfieldID children;	      /* Lsun/porting/utils/XSpan */
} JNIsun_porting_utils_YXBand;

extern struct _JNIsun_porting_utils_XSpan {
    jclass class;

    jfieldID x;		      /* I */
    jfieldID w;		      /* I */
    jfieldID next;	      /* sun/porting/utils/XSpan */
    jfieldID prev;	      /* sun/porting/utils/XSpan */
} JNIsun_porting_utils_XSpan;


/* package: sun.awt */
extern struct _JNIsun_awt_FontDescriptor {
    jclass class;

    jfieldID nativeName;      /* Ljava/lang/String */
    jfieldID fontCharset;     /* Lsun/io/CharToByteConverter */
} JNIsun_awt_FontDescriptor;

extern struct _JNIsun_awt_DrawingSurfaceInfo {
    jclass class;

    jmethodID getClip;        /* ()Ljava/awt/Shape */
    jmethodID getBounds;      /* ()Ljava/awt/Rectangle */
    jmethodID lock;	      /* ()I */
    jmethodID unlock;         /* ()V */

} JNIsun_awt_DrawingSurfaceInfo;
/* package: sun.awt.image */
extern struct _JNIsun_awt_image_Image {
    jclass class;

    jmethodID getImageRep;    /* ()Lsun/awt/image/ImageRepresentation */
} JNIsun_awt_image_Image;

extern struct _JNIjava_awt_image_ImageConsumer {
    jclass class;

    jmethodID setColorModel;  /* (Ljava/awt/image/ColorModel)V */
    jmethodID setHints;	      /* (I)V */
    jmethodID setIntPixels;   /* (IIIILJava/awt/image/ColorModel;[III)V */
    jmethodID setBytePixels;  /* (IIIILJava/awt/image/ColorModel;[BII)V */

    jint TOPDOWNLEFTRIGHT, COMPLETESCANLINES, SINGLEPASS;
} JNIjava_awt_image_ImageConsumer;

extern struct _JNIsun_awt_image_ImageRepresentation {
    jclass class;

    jmethodID reconstruct;    /* (I)V */
    jfieldID  pData;	      /* I */
    jfieldID  srcW;	      /* I */
    jfieldID  srcH;	      /* I */
    jfieldID  width;	      /* I */
    jfieldID  height;	      /* I */
    jfieldID  hints;	      /* I */
    jfieldID  availinfo;      /* I */
    jfieldID  offscreen;      /* Z */
    jfieldID  newbits;	      /* Ljava/awt/Rectangle */

    jint WIDTH, HEIGHT, SOMEBITS, FRAMEBITS, ALLBITS;
} JNIsun_awt_image_ImageRepresentation;

extern struct _JNIsun_awt_image_OffscreenImageSource {
    jclass class;

    jfieldID baseIR;	      /* Lsun/awt/image/ImageRepresentation */
    jfieldID theConsumer;     /* Ljava/awt/image/ImageConsumer */
} JNIsun_awt_image_OffscreenImageSource;

/* package: sun.io */
extern struct _JNIsun_io_CharToByteConverter {
    jclass class;

    jmethodID toString;
} JNIsun_io_CharToByteConverter;


/* package: java.awt */
extern struct _JNIjava_awt_Dimension {
    jclass class;

    jfieldID width;	      /* I */
    jfieldID height;	      /* I */
} JNIjava_awt_Dimension;

extern struct _JNIjava_awt_Font {
    jclass class;

    jfieldID pData;	      /* I */
    jfieldID size;	      /* I */
    jfieldID style;	      /* I */
    jfieldID peer;	      /* Lsun/awt/peer/FontPeer */
    jfieldID family;	      /* Ljava/lang/String */
} JNIjava_awt_Font;

extern struct _JNIjava_awt_Point {
    jclass class;

    jfieldID x;		      /* I */
    jfieldID y;		      /* I */
} JNIjava_awt_Point;

extern struct _JNIjava_awt_Rectangle {
    jclass class;

    jfieldID x;		      /* I */
    jfieldID y;		      /* I */
    jfieldID width;	      /* I */
    jfieldID height;	      /* I */
} JNIjava_awt_Rectangle;

/* package: java.awt.image */
extern struct _JNIjava_awt_image_ColorModel {
    jclass class;

    jfieldID pData;		/* I */
    jfieldID pixel_bits;	/* I */
    jmethodID getRGB;		/* (I)I */
} JNIjava_awt_image_ColorModel;

extern struct _JNIjava_awt_image_DirectColorModel {
    jclass class;

    jfieldID red_mask;		/* I */
    jfieldID green_mask;	/* I */
    jfieldID blue_mask;		/* I */
    jfieldID alpha_mask;	/* I */
    jfieldID red_offset;	/* I */
    jfieldID green_offset;	/* I */
    jfieldID blue_offset;	/* I */
    jfieldID alpha_offset;	/* I */
    jfieldID red_scale;		/* I */
    jfieldID green_scale;	/* I */
    jfieldID blue_scale;	/* I */
    jfieldID alpha_scale;	/* I */

    jmethodID newIIIII;
} JNIjava_awt_image_DirectColorModel;

extern struct _JNIjava_awt_image_ImageObserver {
    jclass class;

    jint WIDTH, HEIGHT, SOMEBITS, FRAMEBITS, ALLBITS;
} JNIjava_awt_image_ImageObserver;

extern struct _JNIjava_awt_image_IndexColorModel {
    jclass class;

    jfieldID rgb;		/* [I */
    jfieldID map_size;		/* I */
    jfieldID opaque;		/* Z */
    jfieldID transparent_index;	/* I */
    jmethodID newIIaBaBaB;
} JNIjava_awt_image_IndexColorModel;

extern struct _JNIjava_awt_Color {
    jclass class;

    jmethodID getRGB;	      /* ()I */
} JNIjava_awt_Color;

