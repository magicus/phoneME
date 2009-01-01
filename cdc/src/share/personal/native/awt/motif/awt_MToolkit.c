/*
 * @(#)awt_MToolkit.c	1.141 06/10/10
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

#include <assert.h>
#include <locale.h>
#include <time.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <X11/cursorfont.h>

#include "awt_p.h"

#include "java_awt_AWTEvent.h"
#include "java_awt_SystemColor.h"
#include "java_awt_Component.h"

#ifdef PERSONAL
#include "java_awt_Label.h"

#include "sun_awt_motif_MLabelPeer.h"
#endif

#include "sun_awt_motif_MComponentPeer.h"
#include "sun_awt_motif_MToolkit.h"
#include "sun_awt_motif_InputThread.h"
#include "sun_awt_motif_X11InputMethod.h"

#include "color.h"
#include "canvas.h"


extern jobject  currentX11InputMethodInstance;

static struct WidgetInfo *awt_winfo = (struct WidgetInfo *) 0;

#ifdef NETSCAPE
#include <signal.h>
extern int      awt_init_xt;
#endif

Display        *awt_display;
Visual         *awt_visual;
Window          awt_root;
long            awt_screen;
int             awt_depth;
XVisualInfo     awt_visInfo;
GC              awt_maskgc;

Colormap        awt_cmap;
void           *awt_lock;
XtAppContext    awt_appContext;
Pixel           awt_defaultBg;
Pixel           awt_defaultFg;
int             awt_whitepixel;
int             awt_blackpixel;
int             awt_multiclick_time;	/* milliseconds */
int             awt_MetaMask = 0;
int             awt_AltMask = 0;
int             awt_NumLockMask = 0;
Cursor          awt_scrollCursor;

#define MODAL_LOCK_PRINT(_funcn, _lockstr)\
 if (inModalWait) {\
}

/*
 * dialog modality fix
 */
#define WIDGET_ARRAY_SIZE 5

static int      arraySize = 0;
static int      arrayIndx = 0;
static Widget  *dShells = NULL;

struct CachedIDs MCachedIDs;
JavaVM         *JVM;

#ifdef DEBUG_AWT_LOCK
int             awt_locked;
char           *lastF;
int             lastL;
#endif

/* Cache IDs of commonly used classes, methods and fields. */

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MToolkit_initIDs(JNIEnv * env, jclass cls)
{
	/* Cache reference to Java VM. */

	if ((*env)->GetJavaVM(env, &JVM) != 0)
		return;

	/* Cache JNI IDs. */

	cls = (*env)->FindClass(env, "java/lang/Object");

	if (cls == NULL)
		return;

	MCachedIDs.java_lang_Object_notifyMID = (*env)->GetMethodID(env, cls, "notify", "()V");
	MCachedIDs.java_lang_Object_notifyAllMID = (*env)->GetMethodID(env, cls, "notifyAll", "()V");
	MCachedIDs.java_lang_Object_waitMID = (*env)->GetMethodID(env, cls, "wait", "(J)V");
	MCachedIDs.java_lang_Object_toStringMID =
		(*env)->GetMethodID(env, cls, "toString", "()Ljava/lang/String;");


	cls = (*env)->FindClass(env, "java/lang/String");

	if (cls == NULL)
		return;

	MCachedIDs.java_lang_String_getBytesMID = (*env)->GetMethodID(env, cls, "getBytes", "()[B");


	cls = (*env)->FindClass(env, "java/awt/AWTError");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_AWTErrorClass = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "java/awt/Insets");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_InsetsClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.java_awt_Insets_constructorMID = (*env)->GetMethodID(env, cls, "<init>", "(IIII)V");

	cls = (*env)->FindClass(env, "java/awt/Dimension");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_DimensionClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.java_awt_Dimension_constructorMID = (*env)->GetMethodID(env, cls, "<init>", "(II)V");

	cls = (*env)->FindClass(env, "java/io/IOException");

	if (cls == NULL)
		return;

	MCachedIDs.java_io_IOExceptionClass = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "java/lang/ArrayIndexOutOfBoundsException");

	if (cls == NULL)
		return;

	MCachedIDs.java_lang_ArrayIndexOutOfBoundsExceptionClass = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "java/lang/IllegalArgumentException");

	if (cls == NULL)
		return;

	MCachedIDs.java_lang_IllegalArgumentExceptionClass = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "java/lang/InternalError");

	if (cls == NULL)
		return;

	MCachedIDs.java_lang_InternalErrorClass = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "java/lang/NullPointerException");

	if (cls == NULL)
		return;

	MCachedIDs.java_lang_NullPointerExceptionClass = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "java/lang/OutOfMemoryError");

	if (cls == NULL)
		return;

	MCachedIDs.java_lang_OutOfMemoryErrorClass = (*env)->NewGlobalRef(env, cls);

	cls = (*env)->FindClass(env, "java/awt/event/FocusEvent");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_event_FocusEventClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.java_awt_event_FocusEvent_constructorMID =
		(*env)->GetMethodID(env, cls, "<init>", "(Ljava/awt/Component;IZ)V");

	cls = (*env)->FindClass(env, "java/awt/event/KeyEvent");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_event_KeyEventClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.java_awt_event_KeyEvent_keyCodeFID = (*env)->GetFieldID(env, cls, "keyCode", "I");
	MCachedIDs.java_awt_event_KeyEvent_keyCharFID = (*env)->GetFieldID(env, cls, "keyChar", "C");
	MCachedIDs.java_awt_event_KeyEvent_constructorMID =
		(*env)->GetMethodID(env, cls, "<init>", "(Ljava/awt/Component;IJIIC)V");

	cls = (*env)->FindClass(env, "java/awt/event/MouseEvent");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_event_MouseEventClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.java_awt_event_MouseEvent_constructorMID =
		(*env)->GetMethodID(env, cls, "<init>", "(Ljava/awt/Component;IJIIIIZ)V");

	cls = (*env)->FindClass(env, "java/awt/image/DirectColorModel");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_image_DirectColorModelClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.java_awt_image_DirectColorModel_constructorMID =
		(*env)->GetMethodID(env, cls, "<init>", "(IIIII)V");

	cls = (*env)->FindClass(env, "java/awt/image/ImageConsumer");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_image_ImageConsumer_setColorModelMID =
		(*env)->GetMethodID(env, cls, "setColorModel", "(Ljava/awt/image/ColorModel;)V");
	MCachedIDs.java_awt_image_ImageConsumer_setHintsMID = (*env)->GetMethodID(env, cls, "setHints", "(I)V");
	MCachedIDs.java_awt_image_ImageConsumer_setPixelsMID =
		(*env)->GetMethodID(env, cls, "setPixels", "(IIIILjava/awt/image/ColorModel;[BII)V");
	MCachedIDs.java_awt_image_ImageConsumer_setPixels2MID =
		(*env)->GetMethodID(env, cls, "setPixels", "(IIIILjava/awt/image/ColorModel;[III)V");

	cls = (*env)->FindClass(env, "java/awt/image/IndexColorModel");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_image_IndexColorModelClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.java_awt_image_IndexColorModel_constructorMID =
		(*env)->GetMethodID(env, cls, "<init>", "(II[B[B[B)V");

	cls = (*env)->FindClass(env, "java/awt/Point");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_PointClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.java_awt_Point_constructorMID = (*env)->GetMethodID(env, cls, "<init>", "(II)V");

	cls = (*env)->FindClass(env, "java/awt/Rectangle");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_RectangleClass = (*env)->NewGlobalRef(env, cls);
	MCachedIDs.java_awt_Rectangle_xFID = (*env)->GetFieldID(env, cls, "x", "I");
	MCachedIDs.java_awt_Rectangle_yFID = (*env)->GetFieldID(env, cls, "y", "I");
	MCachedIDs.java_awt_Rectangle_widthFID = (*env)->GetFieldID(env, cls, "width", "I");
	MCachedIDs.java_awt_Rectangle_heightFID = (*env)->GetFieldID(env, cls, "height", "I");
	MCachedIDs.java_awt_Rectangle_constructorMID = (*env)->GetMethodID(env, cls, "<init>", "(IIII)V");

	cls = (*env)->FindClass(env, "java/awt/AWTEvent");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_AWTEvent_consumedFID = (*env)->GetFieldID(env, cls, "consumed", "Z");
	MCachedIDs.java_awt_AWTEvent_dataFID = (*env)->GetFieldID(env, cls, "data", "I");
	MCachedIDs.java_awt_AWTEvent_idFID = (*env)->GetFieldID(env, cls, "id", "I");

	cls = (*env)->FindClass(env, "java/awt/Color");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Color_pDataFID = (*env)->GetFieldID(env, cls, "pData", "I");
	MCachedIDs.java_awt_Color_getRGBMID = (*env)->GetMethodID(env, cls, "getRGB", "()I");

	cls = (*env)->FindClass(env, "java/awt/Component");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Component_heightFID = (*env)->GetFieldID(env, cls, "height", "I");
	MCachedIDs.java_awt_Component_widthFID = (*env)->GetFieldID(env, cls, "width", "I");
	MCachedIDs.java_awt_Component_xFID = (*env)->GetFieldID(env, cls, "x", "I");
	MCachedIDs.java_awt_Component_yFID = (*env)->GetFieldID(env, cls, "y", "I");
	MCachedIDs.java_awt_Component_peerFID =
		(*env)->GetFieldID(env, cls, "peer", "Lsun/awt/peer/ComponentPeer;");
	MCachedIDs.java_awt_Component_getFontMID = (*env)->GetMethodID(env, cls, "getFont", "()Ljava/awt/Font;");

	cls = (*env)->FindClass(env, "java/awt/Cursor");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Cursor_typeFID = (*env)->GetFieldID(env, cls, "type", "I");

	cls = (*env)->FindClass(env, "java/awt/Event");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Event_dataFID = (*env)->GetFieldID(env, cls, "data", "I");

	cls = (*env)->FindClass(env, "java/awt/Font");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Font_familyFID = (*env)->GetFieldID(env, cls, "family", "Ljava/lang/String;");
	MCachedIDs.java_awt_Font_pDataFID = (*env)->GetFieldID(env, cls, "pData", "I");
	MCachedIDs.java_awt_Font_peerFID = (*env)->GetFieldID(env, cls, "peer", "Lsun/awt/peer/FontPeer;");
	MCachedIDs.java_awt_Font_sizeFID = (*env)->GetFieldID(env, cls, "size", "I");
	MCachedIDs.java_awt_Font_styleFID = (*env)->GetFieldID(env, cls, "style", "I");

	cls = (*env)->FindClass(env, "java/awt/FontMetrics");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_FontMetrics_fontFID = (*env)->GetFieldID(env, cls, "font", "Ljava/awt/Font;");

	cls = (*env)->FindClass(env, "java/awt/event/InputEvent");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_event_InputEvent_modifiersFID = (*env)->GetFieldID(env, cls, "modifiers", "I");

	cls = (*env)->FindClass(env, "java/awt/Insets");

	if (cls == NULL)
		return;

	MCachedIDs.java_awt_Insets_topFID = (*env)->GetFieldID(env, cls, "top", "I");
	MCachedIDs.java_awt_Insets_leftFID = (*env)->GetFieldID(env, cls, "left", "I");
	MCachedIDs.java_awt_Insets_bottomFID = (*env)->GetFieldID(env, cls, "bottom", "I");
	MCachedIDs.java_awt_Insets_rightFID = (*env)->GetFieldID(env, cls, "right", "I");

	/*
	 * cls = (*env)->FindClass (env, "sun/awt/motif/MEmbeddedFramePeer");
	 *
	 * if (cls == NULL) return;
	 *
	 * MCachedIDs.MEmbeddedFramePeer_handleFID = (*env)->GetFieldID (env,
	 * cls, "handle", "I");
	 *
	 */

	cls = (*env)->FindClass(env, "sun/awt/CharsetString");

	if (cls == NULL)
		return;

	MCachedIDs.sun_awt_CharsetString_charsetCharsFID = (*env)->GetFieldID(env, cls, "charsetChars", "[C");
	MCachedIDs.sun_awt_CharsetString_fontDescriptorFID =
		(*env)->GetFieldID(env, cls, "fontDescriptor", "Lsun/awt/FontDescriptor;");
	MCachedIDs.sun_awt_CharsetString_lengthFID = (*env)->GetFieldID(env, cls, "length", "I");
	MCachedIDs.sun_awt_CharsetString_offsetFID = (*env)->GetFieldID(env, cls, "offset", "I");

	cls = (*env)->FindClass(env, "sun/awt/FontDescriptor");

	if (cls == NULL)
		return;

	MCachedIDs.sun_awt_FontDescriptor_fontCharsetFID =
		(*env)->GetFieldID(env, cls, "fontCharset", "Lsun/io/CharToByteConverter;");
	MCachedIDs.sun_awt_FontDescriptor_nativeNameFID =
		(*env)->GetFieldID(env, cls, "nativeName", "Ljava/lang/String;");

	cls = (*env)->FindClass(env, "sun/awt/PlatformFont");

	if (cls == NULL)
		return;

	MCachedIDs.sun_awt_PlatformFont_componentFontsFID =
		(*env)->GetFieldID(env, cls, "componentFonts", "[Lsun/awt/FontDescriptor;");
	MCachedIDs.sun_awt_PlatformFont_propsFID =
		(*env)->GetFieldID(env, cls, "props", "Ljava/util/Properties;");
	MCachedIDs.sun_awt_PlatformFont_makeMultiCharsetStringMID =
		(*env)->GetMethodID(env, cls, "makeMultiCharsetString",
			    "(Ljava/lang/String;)[Lsun/awt/CharsetString;");

	cls = (*env)->FindClass(env, "sun/awt/image/Image");

	if (cls == NULL)
		return;

	MCachedIDs.sun_awt_image_Image_getBackgroundMID =
		(*env)->GetMethodID(env, cls, "getBackground", "()Ljava/awt/Color;");

	cls = (*env)->FindClass(env, "sun/awt/image/ImageRepresentation");

	if (cls == NULL)
		return;

	MCachedIDs.sun_awt_image_ImageRepresentation_pDataFID = (*env)->GetFieldID(env, cls, "pData", "I");
	MCachedIDs.sun_awt_image_ImageRepresentation_availinfoFID =
		(*env)->GetFieldID(env, cls, "availinfo", "I");
	MCachedIDs.sun_awt_image_ImageRepresentation_widthFID = (*env)->GetFieldID(env, cls, "width", "I");
	MCachedIDs.sun_awt_image_ImageRepresentation_heightFID = (*env)->GetFieldID(env, cls, "height", "I");
	MCachedIDs.sun_awt_image_ImageRepresentation_hintsFID = (*env)->GetFieldID(env, cls, "hints", "I");
	MCachedIDs.sun_awt_image_ImageRepresentation_newbitsFID =
		(*env)->GetFieldID(env, cls, "newbits", "Ljava/awt/Rectangle;");
	MCachedIDs.sun_awt_image_ImageRepresentation_offscreenFID =
		(*env)->GetFieldID(env, cls, "offscreen", "Z");

	cls = (*env)->FindClass(env, "sun/awt/image/OffScreenImageSource");

	if (cls == NULL)
		return;

	MCachedIDs.sun_awt_image_OffScreenImageSource_baseIRFID =
		(*env)->GetFieldID(env, cls, "baseIR", "Lsun/awt/image/ImageRepresentation;");
	MCachedIDs.sun_awt_image_OffScreenImageSource_theConsumerFID =
		(*env)->GetFieldID(env, cls, "theConsumer", "Ljava/awt/image/ImageConsumer;");

	cls = (*env)->FindClass(env, "sun/io/CharToByteConverter");

	if (cls == NULL)
		return;

	MCachedIDs.sun_io_CharToByteConverter_convertMID =
		(*env)->GetMethodID(env, cls, "convert", "([CII[BII)I");
}

void
awt_shellPoppedUp(Widget shell, XtPointer modal, XtPointer call_data)
{
	if (arrayIndx == arraySize) {
		/* if we have not allocate an array, do it first */
		arraySize += WIDGET_ARRAY_SIZE;

		if (arrayIndx == 0) {
			dShells = (Widget *) XtMalloc(sizeof(Widget) * arraySize);
		} else {
			XtRealloc((char *)dShells, sizeof(Widget) * arraySize);
		}
	}
	dShells[arrayIndx] = shell;
	arrayIndx++;
}

void
awt_shellPoppedDown(Widget shell, XtPointer modal, XtPointer call_data)
{
	arrayIndx--;

	if (dShells[arrayIndx] == shell) {
		dShells[arrayIndx] = NULL;
		return;
	} else {
		int             i;

		/* find the position of the shell in the array */
		for (i = arrayIndx; i >= 0; i--) {
			if (dShells[i] == shell) {
				break;
			}
		}

		/* remove the found element */
		while (i <= arrayIndx - 1) {
			dShells[i] = dShells[i + 1];
			i++;
		}
	}
}

Boolean
awt_isWidgetModal(Widget widget)
{
	Widget          w;

	for (w = widget; !XtIsShell(w); w = XtParent(w)) {
	}

	while (w != NULL) {
		if (w == dShells[arrayIndx - 1]) {
			return True;
		}
		w = XtParent(w);
	}
	return False;
}

Boolean
awt_isModal()
{
	return (arrayIndx > 0);
}

/*
 * error handlers
 */

/*
 * This error handler is registered twice to work around a Solaris 2.6 bug.
 * Refer to awt_InputMethod.c for details.
 */
extern int
xerror_handler(Display * disp, XErrorEvent * err)
{
	return 0;
}


static void
set_mod_mask(int modnum, int *mask)
{
	switch (modnum) {
		case 1:
		*mask = Mod1Mask;
		break;
	case 2:
		*mask = Mod2Mask;
		break;
	case 3:
		*mask = Mod3Mask;
		break;
	case 4:
		*mask = Mod4Mask;
		break;
	case 5:
		*mask = Mod5Mask;
		break;
	default:
		*mask = 0;
	}
}

static void
setup_modifier_map(Display * disp)
{
	XModifierKeymap *modmap;
	int             i, k, nkeys;
	KeyCode         keycode, metaL, metaR, altL, altR, numLock;

	metaL = XKeysymToKeycode(disp, XK_Meta_L);
	metaR = XKeysymToKeycode(disp, XK_Meta_R);
	altR = XKeysymToKeycode(disp, XK_Alt_R);
	altL = XKeysymToKeycode(disp, XK_Alt_L);
	numLock = XKeysymToKeycode(disp, XK_Num_Lock);

	modmap = XGetModifierMapping(disp);
	if (modmap == NULL) {
		return;
	}
	nkeys = modmap->max_keypermod;
	for (i = 3 * nkeys;
	     i < (nkeys * 8) && (awt_MetaMask == 0 || awt_AltMask == 0 || awt_NumLockMask == 0); i++) {
		keycode = modmap->modifiermap[i];
		k = (i / nkeys) - 2;
		if (awt_MetaMask == 0 && (keycode == metaL || keycode == metaR)) {
			set_mod_mask(k, &awt_MetaMask);
		} else if (awt_AltMask == 0 && (keycode == altL || keycode == altR)) {
			set_mod_mask(k, &awt_AltMask);
		} else if (awt_NumLockMask == 0 && keycode == numLock) {
			set_mod_mask(k, &awt_NumLockMask);
		}
	}
	XFreeModifiermap(modmap);
}


Boolean         scrollBugWorkAround;

/*
 * fix for bug #4088106 - ugly text boxes and grayed out looking text
 */

XmColorProc     oldColorProc;

void
ColorProc(bg_color, fg_color, sel_color, ts_color, bs_color)
	XColor         *bg_color;
	XColor         *fg_color;
	XColor         *sel_color;
	XColor         *ts_color;
	XColor         *bs_color;
{
	unsigned long   plane_masks[1];
	unsigned long   colors[5];

	/* use the default procedure to calculate colors */
	oldColorProc(bg_color, fg_color, sel_color, ts_color, bs_color);

	/* check if there is enought free color cells */
	if (XAllocColorCells(awt_display, awt_cmap, False, plane_masks, 0, colors, 5)) {
		XFreeColors(awt_display, awt_cmap, colors, 5, 0);
		return;
	}
	/* find the closest matches currently available */
	fg_color->pixel = AwtColorMatch(fg_color->red >> 8, fg_color->green >> 8, fg_color->blue >> 8);
	fg_color->flags = DoRed | DoGreen | DoBlue;
	XQueryColor(awt_display, awt_cmap, fg_color);
	sel_color->pixel = AwtColorMatch(sel_color->red >> 8, sel_color->green >> 8, sel_color->blue >> 8);
	sel_color->flags = DoRed | DoGreen | DoBlue;
	XQueryColor(awt_display, awt_cmap, sel_color);
	ts_color->pixel = AwtColorMatch(ts_color->red >> 8, ts_color->green >> 8, ts_color->blue >> 8);
	ts_color->flags = DoRed | DoGreen | DoBlue;
	XQueryColor(awt_display, awt_cmap, ts_color);
	bs_color->pixel = AwtColorMatch(bs_color->red >> 8, bs_color->green >> 8, bs_color->blue >> 8);
	bs_color->flags = DoRed | DoGreen | DoBlue;
	XQueryColor(awt_display, awt_cmap, bs_color);
}

JNIEXPORT void  JNICALL
Java_sun_awt_motif_MToolkit_init(JNIEnv * env, jobject this)
{
	int             argc = 0;
	char           *argv[1];
	/*	XColor          color; */
	XVisualInfo     viTmp, *pVI;
	int             numvi;
	char           *multiclick_time_query = NULL;

	XInitThreads();

	XtToolkitInitialize();
	XtToolkitThreadInitialize();

	XtSetLanguageProc(NULL, NULL, NULL);

	argv[0] = 0;
	awt_appContext = XtCreateApplicationContext();
	awt_display = XtOpenDisplay(awt_appContext, NULL, "J2MEApp", "Java Motif Bridge", NULL, 0, &argc, argv);


	/* XSynchronize(awt_display, True); */

	if (awt_display == NULL) {
		(*env)->ThrowNew(env, MCachedIDs.java_lang_InternalErrorClass, "Can't connect to X11 window server");
		AWT_UNLOCK();
		return;
	}
	multiclick_time_query = XGetDefault(awt_display, "*", "multiClickTime");
	if (multiclick_time_query) {
		awt_multiclick_time = XtGetMultiClickTime(awt_display);
	}
	scrollBugWorkAround = TRUE;

	/*
	 * Create the cursor for TextArea scrollbars...
	 */
	awt_scrollCursor = XCreateFontCursor(awt_display, XC_left_ptr);

	awt_screen = DefaultScreen(awt_display);
	awt_root = RootWindow(awt_display, awt_screen);

	awt_visual = DefaultVisual(awt_display, awt_screen);
	awt_depth = DefaultDepth(awt_display, awt_screen);
	awt_cmap = DefaultColormap(awt_display, awt_screen);

	viTmp.visualid = XVisualIDFromVisual(awt_visual);
	viTmp.depth = awt_depth;
	pVI = XGetVisualInfo(awt_display, VisualIDMask | VisualDepthMask, &viTmp, &numvi);

	if (!pVI) {
		/* This should never happen. */

		(*env)->ThrowNew(env, MCachedIDs.java_lang_InternalErrorClass,
				 "Can't find default visual information");
		AWT_UNLOCK();
		return;
	}
	awt_visInfo = *pVI;
	XFree(pVI);
	pVI = NULL;


	awt_blackpixel = BlackPixel(awt_display, awt_screen);
	awt_whitepixel = WhitePixel(awt_display, awt_screen);

	if (!awt_allocate_colors()) {
		/*
		 * static char *visnames[] = { "StaticGray", "GrayScale",
		 * "StaticColor", "PseudoColor", "TrueColor", "DirectColor"
		 * };
		 */

		(*env)->ThrowNew(env, MCachedIDs.java_lang_InternalErrorClass, "Display type and depth not supported");
		AWT_UNLOCK();
		return;
	}
	awt_defaultBg = AwtColorMatch(200, 200, 200);
	awt_defaultFg = awt_blackpixel;

	{
		Pixmap          one_bit;
		XGCValues       xgcv;

		xgcv.background = 0;
		xgcv.foreground = 1;
		one_bit = XCreatePixmap(awt_display, awt_root, 1, 1, 1);
		awt_maskgc = XCreateGC(awt_display, one_bit, GCForeground | GCBackground, &xgcv);
		XFreePixmap(awt_display, one_bit);
	}


	setup_modifier_map(awt_display);

	/*
	 * fix for bug #4088106 - ugly text boxes and grayed out looking text
	 */
	oldColorProc = XmGetColorCalculation();
	XmSetColorCalculation(ColorProc);

	AWT_UNLOCK();
}


static struct WidgetInfo *
findWidgetInfo(Widget widget)
{
	struct WidgetInfo *cw;

	for (cw = awt_winfo; cw != NULL; cw = cw->next) {
		if (cw->widget == widget) {
			return cw;
		}
	}
	return NULL;
}


static void    *
findPeer(Widget * pwidget)
{
	struct WidgetInfo *cw;
	Widget          widgetParent;
	void           *peer;

	if ((cw = findWidgetInfo(*pwidget)) != NULL) {
		return cw->peer;
	}
	/*
	 * fix for 4053856, robi.khan@eng couldn't find peer corresponding to
	 * widget but the widget may be child of one with a peer, so recurse
	 * up the hierarchy
	 */
	widgetParent = XtParent(*pwidget);
	if (widgetParent != NULL) {
		peer = findPeer(&widgetParent);
		if (peer != NULL) {
			/*
			 * found peer attached to ancestor of given widget,
			 * so set widget return value as well
			 */
			*pwidget = widgetParent;
			return peer;
		}
	}
	return NULL;
}

jboolean
awt_addWidget(JNIEnv * env, Widget w, Widget parent, void *peer, long event_flags)
{
	if (!XtIsSubclass(w, xmDrawingAreaWidgetClass)) {
		struct WidgetInfo *nw = (struct WidgetInfo *) XtCalloc(1, sizeof(struct WidgetInfo));


		if ((findWidgetInfo(parent) == NULL) && (parent != NULL))
			awt_addWidget(env, parent, XtParent(parent), peer, event_flags);


		if (nw) {
			nw->widget = w;
			nw->origin = parent;
			nw->peer = peer;
			nw->event_mask = event_flags;
			nw->next = awt_winfo;
			awt_winfo = nw;

			if (event_flags & java_awt_AWTEvent_KEY_EVENT_MASK) {
				XtAddEventHandler(w, KeyPressMask | KeyReleaseMask, False, awt_canvas_handleEvent, peer);
			}
			if (event_flags & java_awt_AWTEvent_MOUSE_EVENT_MASK) {
				XtAddEventHandler(w,
				       ButtonPressMask | ButtonReleaseMask |
						  EnterWindowMask | LeaveWindowMask, False, awt_canvas_handleEvent, peer);
			}
			if (event_flags & java_awt_AWTEvent_MOUSE_MOTION_EVENT_MASK) {
				XtAddEventHandler(w, PointerMotionMask, False, awt_canvas_handleEvent, peer);
			}
			if (event_flags & java_awt_AWTEvent_FOCUS_EVENT_MASK) {
				XtAddEventHandler(w, FocusChangeMask, False, awt_canvas_handleEvent, peer);
			}
		} else {
			(*env)->ThrowNew(env, MCachedIDs.java_lang_OutOfMemoryErrorClass, NULL);
			return JNI_FALSE;
		}
	}
	return JNI_TRUE;
}

void
awt_delWidget(Widget w)
{
	struct WidgetInfo *cw;

	if (awt_winfo != NULL) {
		if (awt_winfo->widget == w) {
			cw = awt_winfo;
			awt_winfo = awt_winfo->next;
			XFree((void *) cw);
			cw = NULL;
		} else {
			struct WidgetInfo *pw;

			for (pw = awt_winfo, cw = awt_winfo->next; cw != NULL; pw = cw, cw = cw->next) {
				if (cw->widget == w) {
					pw->next = cw->next;
					XFree((void *) cw);
					cw = NULL;
					break;
				}
			}
		}
	}
}


/*
  Remove all the event handlers that were registered for this widget's
  client_data (peer) and its parents. If this is not done - it could
  potentially cause the app to core dump since the peer (globalRef) is
  cleaned up and the parent widgets would be registered with client
  data that is garbage.. 
 */
void 
awt_removeEventHandlerForWidget(Widget w, jobject peerGlobalRef)
{
    Widget parent = XtParent(w);
    for (; parent != NULL; 
         parent = XtParent(parent)) {
        //printf("Removing Event handler for Widget : 0x%x\n", w);
        XtRemoveEventHandler(parent, 
                             XtAllEvents, 
                             TRUE,
                             awt_canvas_handleEvent,
                             peerGlobalRef);
        
    }

}

void
awt_enableWidgetEvents(Widget widget, long event_flag)
{
	struct WidgetInfo *cw;

	for (cw = awt_winfo; cw != NULL; cw = cw->next) {
		if (cw->widget == widget) {
			cw->event_mask |= event_flag;
		}
	}
}

void
awt_disableWidgetEvents(Widget widget, long event_flag)
{
	struct WidgetInfo *cw;

	for (cw = awt_winfo; cw != NULL; cw = cw->next) {
		if (cw->widget == widget) {
			cw->event_mask &= ~event_flag;
		}
	}
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_InputThread_run(JNIEnv * env, jobject this)
{
	/* XtAppMainLoop(awt_appContext); */

#if 0
	XEvent          xev;

	for (;;) {
		XtAppNextEvent(awt_appContext, &xev);

		printf("Event :%d\n", xev.type);

		XtDispatchEvent(&xev);
	}

#endif
	/* NOT REACHED */
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MToolkit_run(JNIEnv * env, jobject this)
{
	XtAppMainLoop(awt_appContext);
}


JNIEXPORT void  JNICALL
Java_sun_awt_motif_MToolkit_sync(JNIEnv * env, jobject this)
{
	AWT_LOCK();
	XSync(awt_display, False);

	AWT_UNLOCK();
}


JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MToolkit_getScreenWidth(JNIEnv * env, jobject this)
{
  int w =  DisplayWidth(awt_display, awt_screen);

	return w;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MToolkit_getScreenHeight(JNIEnv * env, jobject this)
{
  int h = DisplayHeight(awt_display, awt_screen);

	return h;
}

JNIEXPORT jint  JNICALL
Java_sun_awt_motif_MToolkit_getScreenResolution(JNIEnv * env, jobject this)
{

	return (int) ((DisplayWidth(awt_display, awt_screen) * 25.4) / DisplayWidthMM(awt_display, awt_screen));
}

JNIEXPORT jobject JNICALL
Java_sun_awt_motif_MToolkit_makeColorModel(JNIEnv * env, jclass clazz)
{
	return awt_getColorModel(env);
}

/*
 * Class:     sun_awt_motif_MToolkit Method:    beep Signature: ()V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MToolkit_beep(JNIEnv * env, jobject this)
{
  XLockDisplay(awt_display);
	XBell(awt_display, 0);
	XFlush(awt_display);
	XUnlockDisplay(awt_display);
}

/* TODO: check the need of this function */
#if 0 
static long
colorToRGB(XColor * color)
{
	long            rgb = 0;

	rgb |= ((color->red >> 8) << 16);
	rgb |= ((color->green >> 8) << 8);
	rgb |= ((color->blue >> 8) << 0);

	return rgb;
}
#endif

/*
 * Class:     sun_awt_motif_MToolkit Method:    loadSystemColors Signature:
 * ([I)V
 */
JNIEXPORT void  JNICALL
Java_sun_awt_motif_MToolkit_loadSystemColors(JNIEnv * env, jobject this, jintArray systemColors)
{

#if 0

	Widget          frame, panel, control, menu, text, scrollbar;
	Colormap        cmap;
	Pixel           bg, fg, highlight, shadow;
	Pixel           pixels[java_awt_SystemColor_NUM_COLORS];
	XColor         *colorsPtr;
	int             count = 0;
	int             i, j;
	jint            rgb[java_awt_SystemColor_NUM_COLORS];

	AWT_LOCK();

	/*
	 * initialize array of pixels
	 */
	for (i = 0; i < java_awt_SystemColor_NUM_COLORS; i++) {
		pixels[i] = -1;
	}

	/*
	 * Create phantom widgets in order to determine the default colors;
	 * this is somewhat inelegant, however it is the simplest and most
	 * reliable way to determine the system's default colors for objects.
	 */
	frame = XtAppCreateShell("AWTColors", "XApplication", vendorShellWidgetClass, awt_display, NULL, 0);
	/*
	 * XtSetMappedWhenManaged(frame, False); XtRealizeWidget(frame);
	 */
	panel = XmCreateDrawingArea(frame, "awtPanelColor", NULL, 0);
	control = XmCreatePushButton(panel, "awtControlColor", NULL, 0);
	menu = XmCreatePulldownMenu(control, "awtColorMenu", NULL, 0);
	text = XmCreateText(panel, "awtTextColor", NULL, 0);
	scrollbar = XmCreateScrollBar(panel, "awtScrollbarColor", NULL, 0);

	XtVaGetValues(panel, XmNbackground, &bg, XmNforeground, &fg, XmNcolormap, &cmap, NULL);

	pixels[java_awt_SystemColor_WINDOW] = bg;
	count++;
	pixels[java_awt_SystemColor_INFO] = bg;
	count++;
	pixels[java_awt_SystemColor_WINDOW_TEXT] = fg;
	count++;
	pixels[java_awt_SystemColor_INFO_TEXT] = fg;
	count++;

	XtVaGetValues(menu, XmNbackground, &bg, XmNforeground, &fg, NULL);

	pixels[java_awt_SystemColor_MENU] = bg;
	count++;
	pixels[java_awt_SystemColor_MENU_TEXT] = fg;
	count++;

	XtVaGetValues(text, XmNbackground, &bg, XmNforeground, &fg, NULL);

	pixels[java_awt_SystemColor_TEXT] = bg;
	count++;
	pixels[java_awt_SystemColor_TEXT_TEXT] = fg;
	count++;
	pixels[java_awt_SystemColor_TEXT_HIGHLIGHT] = fg;
	count++;
	pixels[java_awt_SystemColor_TEXT_HIGHLIGHT_TEXT] = bg;
	count++;

	XtVaGetValues(control,
		      XmNbackground, &bg,
		      XmNforeground, &fg, XmNtopShadowColor, &highlight, XmNbottomShadowColor, &shadow, NULL);

	pixels[java_awt_SystemColor_CONTROL] = bg;
	count++;
	pixels[java_awt_SystemColor_CONTROL_TEXT] = fg;
	count++;
	pixels[java_awt_SystemColor_CONTROL_HIGHLIGHT] = highlight;
	count++;
	pixels[java_awt_SystemColor_CONTROL_LT_HIGHLIGHT] = highlight;
	count++;
	pixels[java_awt_SystemColor_CONTROL_SHADOW] = shadow;
	count++;
	pixels[java_awt_SystemColor_CONTROL_DK_SHADOW] = shadow;
	count++;

	XtVaGetValues(scrollbar, XmNbackground, &bg, NULL);
	pixels[java_awt_SystemColor_SCROLLBAR] = bg;
	count++;

	/*
	 * Convert pixel values to RGB
	 */
	colorsPtr = (XColor *) XtMalloc(count * sizeof(XColor));
	j = 0;
	for (i = 0; i < java_awt_SystemColor_NUM_COLORS; i++) {
		if (pixels[i] != -1) {
			colorsPtr[j++].pixel = pixels[i];
		}
	}
	XQueryColors(awt_display, cmap, colorsPtr, count);

	/*
	 * Fill systemColor array with new rgb values
	 */

	memset(rgb, '\0', sizeof(rgb));
	j = 0;
	for (i = 0; i < java_awt_SystemColor_NUM_COLORS; i++) {
		if (pixels[i] != -1)
			rgb[i] = colorToRGB(&colorsPtr[j++]) | 0xFF000000;
	}

	(*env)->SetIntArrayRegion(env, systemColors, 0, i, rgb);

	XtDestroyWidget(frame);
	XFree(colorsPtr);

	AWT_UNLOCK();

#endif
}
