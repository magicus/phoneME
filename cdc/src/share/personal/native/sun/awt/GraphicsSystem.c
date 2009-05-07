/*
 * @(#)GraphicsSystem.c	1.41 06/10/10
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

#define MAX_SUPPORTED_DEPTH 8

#include <jni.h>

#include "sun_awt_gfX_GraphicsSystem.h"
#include "Xheaders.h"
#include "common.h"

/* used to lock display for exclusive access */
#ifndef NDEBUG
JavaVM *VM;

int check_locking_thread(jobject lock) {
    JNIEnv *env;
    int rval;

    (*VM)->AttachCurrentThread(VM, (void **)&env, NULL);
    (*env)->MonitorEnter(env, lock);
    rval = nlc > 0;
    (*env)->MonitorExit(env, lock);

    return rval;
}
#endif

int       nlc = 0;
jobject   native_lock = 0;
jobject   lock_class;


jmethodID lock_waitMID;
jmethodID lock_notifyMID;
jmethodID lock_notifyAllMID;


Display     *display;
Window      root_drawable;
GC          root_gc;
Visual      *root_visual;
XVisualInfo *root_visInfo;
Colormap    root_colormap;
int         screen_depth;

int         numColors;
int         grayscale;

pixelToRGBConverter     pixel2RGB  = 0;
rgbToPixelConverter     RGB2Pixel  = 0;
javaRGBToPixelConverter jRGB2Pixel = 0;
int                     numColors  = 0;

struct GSCachedIDs      GSCachedIDs;
struct IRCachedIDs      IRCachedIDs;

static Cursor   currentCursor = None;
static jboolean cursorVisible = JNI_TRUE;

static XVisualInfo *GetMatchingVisual(int depth, int useColor);
static Colormap SetupColormap(XVisualInfo *visInfo, int desired_depth, int useColor);

/* event codes */

static void createNativeLock(JNIEnv *env, jobject lockObj) {
    jclass tmp;

    if (lockObj == 0) {
	fprintf(stderr, "WHOA! Lock object is null!?\n");
	fflush(stderr);
    }

    tmp = (*env)->GetObjectClass(env, lockObj);

    if (tmp == 0) {
	fprintf(stderr, "WHOA! No class for object?\n");
	fflush(stderr);
    }

    native_lock = (*env)->NewGlobalRef(env, lockObj);
    lock_class  = (*env)->NewGlobalRef(env, tmp);

    if (lock_class == 0) {
	fprintf(stderr, "Failed to create global ref for lock object class\n");
	fflush(stderr);
    }

    lock_waitMID = (*env)->GetMethodID(env, lock_class, "wait", "(J)V");
    lock_notifyMID = (*env)->GetMethodID(env, lock_class, "notify", "()V");
    lock_notifyAllMID = (*env)->GetMethodID(env, lock_class, "notifyAll", "()V");

#ifndef NDEBUG
    (*env)->GetJavaVM(env, &VM);
#endif

}

/*
JNIACCESS(graphicsSystem);
JNIACCESS(java_awt_Dimension);
JNIACCESS(java_awt_Point);
JNIACCESS(java_awt_Rectangle);
*/

JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsSystem_initJNI(JNIEnv *env, jclass cls)
{
    jfieldID    tmpfldid;

/*
    DEFINECLASS(graphicsSystem, cls);

    DEFINEFIELD(graphicsSystem, screenWidth, "I");
    DEFINEFIELD(graphicsSystem, screenHeight, "I");
    DEFINEFIELD(graphicsSystem, defaultScreen, 
		"Lsun/porting/graphicssystem/Drawable;");

    DEFINEFIELD(graphicsSystem, colorModel,
		"Ljava/awt/image/ColorModel;");

    DEFINEFIELD(graphicsSystem, inputHandler,
		"Lsun/porting/windowsystem/EventHandler;");

    DEFINEFIELD(graphicsSystem, cursorX, "I");
    DEFINEFIELD(graphicsSystem, cursorY, "I");
    DEFINEFIELD(graphicsSystem, cursorHidden, "I");
	
    DEFINEMETHOD(graphicsSystem, handleKeyInput, "(III)V");
    DEFINEMETHOD(graphicsSystem, handleMouseInput, "(IIII)V");
    DEFINEMETHOD(graphicsSystem, handleExposeEvent,"(IIIII)V");

    DEFINECONSTANT(graphicsSystem, NONE,     "I", Int);
    DEFINECONSTANT(graphicsSystem, PRESSED,  "I", Int);
    DEFINECONSTANT(graphicsSystem, RELEASED, "I", Int);
    DEFINECONSTANT(graphicsSystem, ACTION,   "I", Int);
    DEFINECONSTANT(graphicsSystem, TYPED,    "I", Int);

    FINDCLASS(java_awt_Dimension, "java/awt/Dimension");
    DEFINEFIELD(java_awt_Dimension, width, "I");
    DEFINEFIELD(java_awt_Dimension, height, "I");

    FINDCLASS(java_awt_Point, "java/awt/Point");
    DEFINEFIELD(java_awt_Point, x, "I");
    DEFINEFIELD(java_awt_Point, y, "I");

    FINDCLASS(java_awt_Rectangle, "java/awt/Rectangle");
    DEFINEFIELD(java_awt_Rectangle, x, "I");
    DEFINEFIELD(java_awt_Rectangle, y, "I");
    DEFINEFIELD(java_awt_Rectangle, width, "I");
    DEFINEFIELD(java_awt_Rectangle, height, "I");
*/


    GSCachedIDs.screenWidthFID = (*env)->GetFieldID(env, cls, "screenWidth", "I");
    GSCachedIDs.screenHeightFID = (*env)->GetFieldID(env, cls, "screenHeight", "I");
    GSCachedIDs.defaultScreenFID = (*env)->GetFieldID(env, cls, "defaultScreen", "Lsun/porting/graphicssystem/Drawable;");
    GSCachedIDs.colorModelFID = (*env)->GetFieldID(env, cls, "colorModel", "Ljava/awt/image/ColorModel;");
    GSCachedIDs.inputHandlerFID = (*env)->GetFieldID(env, cls, "inputHandler", "Lsun/porting/windowsystem/EventHandler;");
    GSCachedIDs.cursorXFID = (*env)->GetFieldID(env, cls, "cursorX", "I");
    GSCachedIDs.cursorYFID = (*env)->GetFieldID(env, cls, "cursorY", "I");
    GSCachedIDs.cursorHiddenFID = (*env)->GetFieldID(env, cls, "cursorHidden", "I");

    GSCachedIDs.handleKeyInputMID = (*env)->GetMethodID(env, cls, "handleKeyInput", "(III)V");
    GSCachedIDs.handleMouseInputMID = (*env)->GetMethodID(env, cls, "handleMouseInput", "(IIII)V");
    GSCachedIDs.handleExposeEventMID = (*env)->GetMethodID(env, cls, "handleExposeEvent", "(IIIII)V");



    if((tmpfldid = (*env)->GetStaticFieldID(env, cls, "NONE", "I"))==NULL) return;
    GSCachedIDs.NONE = (*env)->GetStaticIntField(env, cls, tmpfldid);

    if((tmpfldid = (*env)->GetStaticFieldID(env, cls, "PRESSED", "I"))==NULL) return;
    GSCachedIDs.PRESSED = (*env)->GetStaticIntField(env, cls, tmpfldid);

    if((tmpfldid = (*env)->GetStaticFieldID(env, cls, "RELEASED", "I"))==NULL) return;
    GSCachedIDs.RELEASED = (*env)->GetStaticIntField(env, cls, tmpfldid);

    if((tmpfldid = (*env)->GetStaticFieldID(env, cls, "ACTION", "I"))==NULL) return;
    GSCachedIDs.ACTION = (*env)->GetStaticIntField(env, cls, tmpfldid);

    if((tmpfldid = (*env)->GetStaticFieldID(env, cls, "TYPED", "I"))==NULL) return;
    GSCachedIDs.TYPED = (*env)->GetStaticIntField(env, cls, tmpfldid);

    GSCachedIDs.java_awt_Dimension = (*env)->FindClass(env, "java/awt/Dimension");
    GSCachedIDs.java_awt_Dimension_widthFID = (*env)->GetFieldID(env, GSCachedIDs.java_awt_Dimension, "width", "I");
    GSCachedIDs.java_awt_Dimension_heightFID = (*env)->GetFieldID(env, GSCachedIDs.java_awt_Dimension, "height", "I");

    GSCachedIDs.java_awt_Point = (*env)->FindClass(env, "java/awt/Point");
    GSCachedIDs.java_awt_Point_xFID = (*env)->GetFieldID(env, GSCachedIDs.java_awt_Point, "x", "I");
    GSCachedIDs.java_awt_Point_yFID = (*env)->GetFieldID(env, GSCachedIDs.java_awt_Point, "y", "I");


    GSCachedIDs.java_awt_Rectangle = (*env)->FindClass(env, "java/awt/Rectangle");
    GSCachedIDs.java_awt_Rectangle_xFID = (*env)->GetFieldID(env, GSCachedIDs.java_awt_Rectangle, "x", "I");
    GSCachedIDs.java_awt_Rectangle_yFID = (*env)->GetFieldID(env, GSCachedIDs.java_awt_Rectangle, "y", "I");
    GSCachedIDs.java_awt_Rectangle_widthFID = (*env)->GetFieldID(env, GSCachedIDs.java_awt_Rectangle, "width", "I");
    GSCachedIDs.java_awt_Rectangle_heightFID = (*env)->GetFieldID(env, GSCachedIDs.java_awt_Rectangle, "height", "I");

}

/*
 * Class:     sun_awt_gfX_GraphicsSystem
 * Method:    init
 * Signature: (I)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsSystem_init(JNIEnv *env, jobject obj,
				     jobject lockObj,
				     jint depth,  jboolean useColor)
{
    unsigned int width, height;
    unsigned int mask = 0;

    XSetWindowAttributes attr;
    XSizeHints *h;

    createNativeLock(env, lockObj);

    width = (*env)->GetIntField(env, obj, GSCachedIDs.screenWidthFID);
    height = (*env)->GetIntField(env, obj, GSCachedIDs.screenHeightFID);

    NATIVE_LOCK(env);

    display = XOpenDisplay(NULL);
    if (display == NULL) {
	fprintf(stderr, "XOpenDisplay: unable to connect to X server %s\n\n",
		XDisplayName(NULL));
	exit(-1);
    }

    root_visInfo = GetMatchingVisual(depth, useColor ? 1 : 0);
    if (root_visInfo == NULL) {
	fprintf(stderr, "Could not find a suitable Visual in which to run.\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "The emulator wants %d-bit %s\n", depth,
		useColor ? ((depth <= 8) ? "PseudoColor" : "TrueColor") 
		         : "GrayScale");
	if (depth < MAX_SUPPORTED_DEPTH) {
	    fprintf(stderr, "and can go up to a maximum bit depth of %d.\n",
		    MAX_SUPPORTED_DEPTH);
	}
	fprintf(stderr, "The X server may not be able to provide a visual\n");
	fprintf(stderr, "which has the right depth and colormap size.\n");
	exit(-2);
    }

    root_visual  = root_visInfo->visual;
    screen_depth = root_visInfo->depth;

    root_colormap = SetupColormap(root_visInfo, depth, useColor ? 1 : 0);

    attr.background_pixel = 0;                mask |= CWBackPixel;
    attr.border_pixel     = 0;                mask |= CWBorderPixel;
    attr.bit_gravity      = NorthWestGravity; mask |= CWBitGravity;
    attr.event_mask       = ExposureMask;     mask |= CWEventMask;
    attr.colormap         = root_colormap;    mask |= CWColormap;

    root_drawable = XCreateWindow(display, DefaultRootWindow(display), 
				  0, 0, width, height, 0, screen_depth, 
				  InputOutput, root_visual, mask, &attr);

    /* request that the window be of fixed size */
    h = XAllocSizeHints();
    if (h != NULL) {
	h->flags = PMinSize | PMaxSize;
	h->min_width = h->max_width = width;
	h->min_height = h->max_height = height;
	XSetWMNormalHints(display, root_drawable, h);
	XFree(h);
    }

    setupImageDecode();

    NATIVE_UNLOCK(env);

    (*env)->SetObjectField(env, obj, GSCachedIDs.colorModelFID, getColorModel(env));

    /* root window will be mapped etc. by the event processor */
}


static Cursor getBlankCursor()
{
    static Cursor blankCursor = 0;

    if (blankCursor == 0) {
	Pixmap pm;
	XColor fg, bg;
	GC g;

	assert(NATIVE_LOCK_HELD());
	fg.pixel = 0;
	bg.pixel = 0xffffffff;
	pm = XCreatePixmap(display, root_drawable, 1, 1, 1);
	g = XCreateGC(display, pm, 0, 0);
	XSetForeground(display, g, 0);
	XDrawPoint(display, pm, g, 0, 0);
	blankCursor = XCreatePixmapCursor(display, pm, pm, &fg, &bg, 0, 0);
	XFreePixmap(display, pm);
    }

    return blankCursor;
}

static void setCursor(JNIEnv *env, Cursor cursor)
{
    NATIVE_LOCK(env);
    if (cursor == None) {
	cursor = getBlankCursor();
    }
    XDefineCursor(display, root_drawable, cursor);
    XFlush(display);
    NATIVE_UNLOCK(env);
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsSystem_sync(JNIEnv *env, jobject obj)
{
    NATIVE_LOCK(env);
    XSync(display, False);
    NATIVE_UNLOCK(env);
}

JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsSystem_beep(JNIEnv *env, jobject obj)
{
    NATIVE_LOCK(env);
    XBell(display, 0);
    NATIVE_UNLOCK(env);
}

/*
 * Class:     sun_awt_gfX_GraphicsSystem
 * Method:    getScreenResolution
 * Signature: ()I
 */
JNIEXPORT jint JNICALL
Java_sun_awt_gfX_GraphicsSystem_getScreenResolution(JNIEnv *env, jobject obj)
{
    int xRes, yRes;

    NATIVE_LOCK(env);

    xRes = (int) (  DisplayWidth(display, DefaultScreen(display)) * 25.4)
	          / DisplayWidthMM(display, DefaultScreen(display));

    yRes = (int) (  DisplayHeight(display, DefaultScreen(display)) * 25.4)
	          / DisplayHeightMM(display, DefaultScreen(display));

    NATIVE_UNLOCK(env);

    return (xRes < yRes) ? xRes : yRes;
}


/*
 * Class:     sun_awt_gfX_GraphicsSystem
 * Method:    setCursorVisible
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsSystem_setCursorVisible(JNIEnv *env,
						 jobject obj,
						 jboolean visible)
{
    cursorVisible = visible;
    setCursor(env, (visible) ? currentCursor : None);
}

/* extern Cursor cursorImage_getCursor(JNIEnv *env, jobject image); */

/*
 * Class:     sun_awt_gfX_GraphicsSystem
 * Method:    setCursor
 * Signature: (Lsun/porting/graphicssystem/CursorImage;)V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsSystem_setCursor(JNIEnv *env, jobject obj, jobject image)
{
    /* if no image supplied, default to blank cursor */

    NATIVE_LOCK(env);
    if (image == NULL) {
        currentCursor = getBlankCursor();
    } else {
      /*        currentCursor = cursorImage_getCursor(env, image); */
    }
    setCursor(env, (cursorVisible) ? currentCursor : None);
    NATIVE_UNLOCK(env);
}

static void
PostKeyEvent(JNIEnv *env, jobject obj, int ID, int keycode, int keychar)
{
    (*env)->CallVoidMethod(env, obj, GSCachedIDs.handleKeyInputMID, (jint)ID, (jint)keycode, (jint)keychar);

    /* keychar is needed for backwards compatibility with JDK 1.0 */

    if ((keychar > 0) && (ID == GSCachedIDs.PRESSED)) {
        (*env)->CallVoidMethod(env, obj, GSCachedIDs.handleKeyInputMID, GSCachedIDs.TYPED, (jint)0, (jint)keychar);
    }
}

static int mouseX = -1;
static int mouseY = -1;

static void
PostMouseEvent(JNIEnv *env, jobject obj, int ID, int x, int y, unsigned int button)
{
    mouseX = x;
    mouseY = y;

    (*env)->CallVoidMethod(env, obj, GSCachedIDs.handleMouseInputMID, (jint)ID, (jint)mouseX, (jint)mouseY, (jint)button);
}

extern int LookupKeycode(KeySym sym);

/* event/input stream handling taken from solaris JDK */
static int inputpending = 0;
static int flushpending = 0;
static jlong flushtime;

#define FLUSH_TIMEOUT       100     /* milliseconds */

static jlong 
TimeMillis()
{
    struct timeval t;

    gettimeofday(&t, 0);

    return (((jlong)t.tv_sec) * 1000) + (((jlong)t.tv_usec) / 1000);
}

void 
gfx_output_flush(JNIEnv *env)
{
    if (!flushpending) {
        flushpending = 1;
        flushtime = TimeMillis() + FLUSH_TIMEOUT;
        NATIVE_NOTIFY_ALL(env);
    }
}

/*
 * Class:     sun_awt_gfX_GraphicsSystem
 * Method:    runInputThread
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_gfX_GraphicsSystem_runInputThread(JNIEnv *env, jobject this)
{
    jlong tm = 250;		/* kick the thread now and then while pending */
				/* because notify can come at the wrong time? */
    struct timeval poll;
    int res;

    /**
     * This thread's only job is to inform the event loop thread that
     * events are available.
     */
    poll.tv_sec = 0;
    poll.tv_usec = 250000;
    NATIVE_LOCK(env);
    while (1) {
        fd_set rdset;

        if (XPending(display) == 0) {
            FD_ZERO(&rdset);
            FD_SET(ConnectionNumber(display), &rdset);
            NATIVE_UNLOCK(env);
            res = select(ConnectionNumber(display) + 1, &rdset, 0, 0, &poll);
            NATIVE_LOCK(env);
	    if (res == 0) continue;
        }
        inputpending = 1;
        while (inputpending) {
	    NATIVE_NOTIFY_ALL(env);
            NATIVE_WAIT(env, tm);
        }
    }
}

/*
 * Class:     sun_awt_gfX_GraphicsSystem
 * Method:    runEventLoop
 * Signature: ()V
 */
JNIEXPORT void JNICALL 
Java_sun_awt_gfX_GraphicsSystem_runEventLoop(JNIEnv *env, jobject obj)
{
    int pointer_outside_window = 1;

    NATIVE_LOCK(env);
    XSelectInput(display, root_drawable, 
		   ButtonPressMask     | ButtonReleaseMask
		 | KeyPressMask        | KeyReleaseMask
		 | EnterWindowMask     | LeaveWindowMask
		 | PointerMotionMask 
		 | ExposureMask        | FocusChangeMask);

    XMapWindow(display, root_drawable);
    XFlush(display);

    flushpending = 0;
    assert((NATIVE_LOCK_HELD() == 1));
    NATIVE_UNLOCK(env);

    while (1) {
	while (!inputpending) {
	    jlong tm;

	    if (flushpending) {
		jlong now = TimeMillis();

		tm = flushtime - now;
		if (tm <= 0) {
		    break;
		}
	    } else {
		tm = 0;
	    }
	    NATIVE_LOCK(env);
	    NATIVE_WAIT(env, tm);
	    NATIVE_UNLOCK(env);
	}

	NATIVE_LOCK(env);

	if (flushpending) {
	    XFlush(display);
	    flushpending = 0;
	}

/* this macro declares "e" as a pointer to the given type, */
/* and sets it to point to the data field of the event.    */
#define REDECLARE(_typ_) _typ_ *e = (_typ_ *) &(ev.xany)
        while (XPending(display) > 0) {
	    XEvent ev;

	    flushpending = 0;	/* XPending flushes events */
	    XNextEvent(display, &ev);
	    NATIVE_UNLOCK(env);

	    /*
	     * fprintf(stderr, "Got event of type %d\n", ev.type); 
	     * fflush(stderr);
	     */

	    switch(ev.type) {
	    case KeyPress: {
		REDECLARE(XKeyPressedEvent);
		KeySym sym;
		char buf[16];
		int n = XLookupString(e, buf, sizeof(buf), &sym, NULL);
		NATIVE_LOCK(env);
		sym = XKeycodeToKeysym(e->display, e->keycode, 0);
		NATIVE_UNLOCK(env);
		PostKeyEvent(env, obj, GSCachedIDs.PRESSED, LookupKeycode(sym), (n == 1) ? buf[0] : 0);
		break;
	    }

	    case KeyRelease: {
		REDECLARE(XKeyReleasedEvent);
		KeySym sym;
		char buf[16];
		int n = XLookupString(e, buf, sizeof(buf), &sym, NULL);
		NATIVE_LOCK(env);
		sym = XKeycodeToKeysym(e->display, e->keycode, 0);
		NATIVE_UNLOCK(env);
		PostKeyEvent(env, obj, GSCachedIDs.RELEASED, LookupKeycode(sym), (n == 1) ? buf[0] : 0);
		break;
	    }

	    case ButtonPress: {
		REDECLARE(XButtonPressedEvent);
		PostMouseEvent(env, obj, GSCachedIDs.PRESSED, e->x, e->y, e->button);
		break;
	    }

	    case ButtonRelease: {
		REDECLARE(XButtonReleasedEvent);
		PostMouseEvent(env, obj, GSCachedIDs.RELEASED, e->x, e->y, e->button);
		break;
	    }

	    case MotionNotify: {
		REDECLARE(XPointerMovedEvent);
		if (! pointer_outside_window) {
		    PostMouseEvent(env, obj, GSCachedIDs.NONE, e->x, e->y, 0);
		}
		break;
	    }

	    case EnterNotify: {
		REDECLARE(XEnterWindowEvent);
		pointer_outside_window = 0;
		/* this will look as though the pointer jumped */
		PostMouseEvent(env, obj, GSCachedIDs.NONE, e->x, e->y, 0);
		break;
	    }

	    case LeaveNotify: {
		REDECLARE(XLeaveWindowEvent);
		PostMouseEvent(env, obj, GSCachedIDs.NONE, e->x, e->y, 0);
		pointer_outside_window = 1;
		break;
	    }

	    case FocusIn: {
		/* REDECLARE(XFocusInEvent); */
		/* Maybe we don't need this. But what if wmgr is click-to-type? */
		break;
	    }

	    case FocusOut:  {
		/* REDECLARE(XFocusOutEvent); */
		/* Maybe we don't need this. But what if wmgr is click-to-type? */
		break;
	    }

	    case Expose:
	    case GraphicsExpose: {
		REDECLARE(XExposeEvent);

                (*env)->CallVoidMethod(env, obj, GSCachedIDs.handleExposeEventMID, e->x, e->y, e->width, e->height, e->count);
		break;
	    }

	    case MappingNotify: {
		/* REDECLARE(XMappingEvent); */
		/* not clear that we need to do anything with this */
		/* but it is an event that is always delivered     */
		break;
	    }

	    case SelectionClear: {
		/* REDECLARE(XSetSelectClearEvent); */
		/* not clear that we need to do anything with this */
		/* but it is an event that is always delivered     */
		break;
	    }

	    case SelectionRequest: {
		/* REDECLARE(XSelectionRequestEvent); */
		/* not clear that we need to do anything with this */
		/* but it is an event that is always delivered     */
		break;
	    }

	    case SelectionNotify: {
		/* REDECLARE(XSelectionEvent); */
		/* not clear that we need to do anything with this */
		/* but it is an event that is always delivered     */
		break;
	    }

	    case ClientMessage: {
		/* REDECLARE(XClientMessageEvent); */
		/* not clear that we need to do anything with this */
		/* but it is an event that is always delivered     */
		break;
	    }

	    case NoExpose:
		break;

	    /* these were not selected for and should therefore not occur */
	    case PropertyNotify:
	    case ColormapNotify:
	    case UnmapNotify:
	    case MapNotify:
	    case KeymapNotify:
	    case VisibilityNotify:
	    case CreateNotify:
	    case DestroyNotify:
	    case MapRequest:
	    case ReparentNotify:
	    case ConfigureNotify:
	    case ConfigureRequest:
	    case GravityNotify:
	    case ResizeRequest:
	    case CirculateNotify:
	    case CirculateRequest:
		fprintf(stderr, "Got unexpected event of type %d\n", ev.type);
		break;

	    default:
		fprintf(stderr, "Got event with unrecognized type: %d\n", ev.type);
		break;
	    }

	    NATIVE_LOCK(env);
	}
	inputpending = 0;
        NATIVE_NOTIFY_ALL(env);
	NATIVE_UNLOCK(env);
    }
}


#ifdef MAYBE
static int allColorsAreGray(jint *colors, int length) {
    while (--length >= 0) {
	int n = colors[length];
	int b = n & 0xff;
	if ((n ^ (b | (b<<8) | (b<<16))) != 0) {
	    return 0;	/* not gray */
	}
    }

    return 1;
}
#endif

#ifdef DUMP_VISUAL
static void dumpVisual(XVisualInfo *visualList)
{
    fprintf(stderr, "visual on screen #%d:\n", visualList->screen);
    fprintf(stderr, "    visual id: 0x%02x\n", visualList->visualid);
    switch (visualList->class) {
    case StaticGray:
	fprintf(stderr, "    class:     StaticGray\n"); break;
    case GrayScale:
	fprintf(stderr, "    class:     GrayScale\n"); break;
    case StaticColor:
	fprintf(stderr, "    class:     StaticColor\n"); break;
    case PseudoColor:
	fprintf(stderr, "    class:     PseudoColor\n"); break;
    case TrueColor:
	fprintf(stderr, "    class:     TrueColor\n"); break;
    case DirectColor:
	fprintf(stderr, "    class:     DirectColor\n"); break;
    }

    fprintf(stderr, "    depth:     %d planes\n", visualList->depth);
    fprintf(stderr, "    cmap size: %d\n", visualList->colormap_size);

    fprintf(stderr, "    red, green, blue masks: 0x%x, 0x%x, 0x%x\n",
	    visualList->red_mask,
	    visualList->green_mask,
	    visualList->blue_mask);

    fprintf(stderr, "    significant bits in color specification: %d bits\n",
	    visualList->bits_per_rgb);
}
#endif /* DUMP_VISUAL */

static XVisualInfo *GetMatchingVisual(int depth, int useColor) {

    int visualsMatched;
    XVisualInfo vTemplate, *visualList;
    int mask = VisualScreenMask | VisualDepthMask | VisualClassMask;

    assert(NATIVE_LOCK_HELD());

    vTemplate.screen = DefaultScreen(display);
    vTemplate.depth  = depth;

    if (!useColor) {
	vTemplate.class  = GrayScale;

	do {
	    visualList = XGetVisualInfo(display, mask,
					&vTemplate, &visualsMatched);
	    if (visualsMatched != 0) {
#ifdef DUMP_VISUAL
		dumpVisual(visualList);
#endif /* DUMP_VISUAL */
		return visualList;
	    }

	    vTemplate.depth <<= 1;
	    if ((vTemplate.depth > 8) && vTemplate.class == GrayScale) {
		vTemplate.class = StaticGray;
		vTemplate.depth = depth;
	    }
	} while (vTemplate.depth <= 8);
    }

    if (depth <= 8) {
	vTemplate.colormap_size = (1 << depth);
	mask |= VisualColormapSizeMask;
    }

    {
	static char depth_table[] = {4, 6, 8, 12, 15, 16, 18, 24, 32};
	int ix = 0;
	int cutoff = (depth == 4) ? 4 : 8;

	if (depth > 32) {
	    depth = 8;
	    ix = 2;
	}

	while (depth_table[ix] < depth) {
	    ++ix;
	}

	vTemplate.class  = PseudoColor;
	do {
	    vTemplate.depth  = depth_table[ix++];
	    if (vTemplate.depth > cutoff) {
		mask = VisualScreenMask | VisualDepthMask;
	    }

	    visualList = XGetVisualInfo(display, mask,
					&vTemplate, &visualsMatched);

	    if (visualsMatched != 0) {
#ifdef DUMP_VISUAL
		dumpVisual(visualList);
#endif /* DUMP_VISUAL */
		return visualList;
	    }
	} while (vTemplate.depth < MAX_SUPPORTED_DEPTH);

	return NULL;
    }
}

unsigned long mapped_pixels[256];

extern void setupGray2(Colormap cmap, 
		       unsigned long *mapped_pixels, 
		       XVisualInfo *visInfo);

extern void setupGray4(Colormap cmap, 
		       unsigned long *mapped_pixels, 
		       XVisualInfo *visInfo);

extern void setupColor4(Colormap cmap, 
			unsigned long *mapped_pixels, 
			XVisualInfo *visInfo);

extern void setupColor8(Colormap cmap, 
			unsigned long *mapped_pixels, 
			XVisualInfo *visInfo);

static Colormap SetupColormap(XVisualInfo *visInfo,
			      int desired_depth, int useColor) {
    Colormap cmap;
    unsigned int cmapLength = 1 << desired_depth;

    assert(NATIVE_LOCK_HELD());

    cmap = XCreateColormap(display, DefaultRootWindow(display),
			   visInfo->visual, AllocNone);

    if ((visInfo->class == GrayScale || visInfo->class == PseudoColor)
	&& !XAllocColorCells(display, cmap, False, 0, 0, 
			     mapped_pixels, cmapLength)) {
	XFreeColormap(display, cmap);
	cmap = 0;
	fprintf(stderr, "XAllocColorCells: failed to allocate %d cells\n",
		cmapLength);
	return cmap;
    }

    if (useColor) {
	switch (desired_depth) {
	case 8:
	    setupColor8(cmap, mapped_pixels, visInfo);
	break;

	case 4:
	    setupColor4(cmap, mapped_pixels, visInfo);
	break;

	default:
	    fprintf(stderr, "Unsupported color depth %d\n", desired_depth);
	    fprintf(stderr, "Using 8 instead\n");
	    setupColor8(cmap, mapped_pixels, visInfo);
	    break;
	}
    } else {
	switch (desired_depth) {
	case 4:
	    setupGray4(cmap, mapped_pixels, visInfo);
	    break;

	case 2:
	    setupGray2(cmap, mapped_pixels, visInfo);
	    break;

	default:
	    fprintf(stderr, "Unsupported grayscale depth %d\n", desired_depth);
	    fprintf(stderr, "Using 4 instead\n");
	    setupGray4(cmap, mapped_pixels, visInfo);
	    break;
	}
    }

    return cmap;
}

/*
 * Class:     sun_awt_gfX_GraphicsSystem
 * Method:    adjustCursorSize
 * Signature: (Ljava/awt/Dimension;)V
 */
JNIEXPORT void JNICALL 
Java_sun_awt_gfX_GraphicsSystem_adjustCursorSize(JNIEnv * env, 
						 jobject obj, 
						 jobject cursorSize)
{
    jint width  = (*env)->GetIntField(env, cursorSize, GSCachedIDs.java_awt_Dimension_widthFID); 
    jint height = (*env)->GetIntField(env, cursorSize, GSCachedIDs.java_awt_Dimension_heightFID); 

    unsigned new_width, new_height;
    int status;

    NATIVE_LOCK(env);
    status = XQueryBestCursor(display, root_drawable, 
			      (unsigned)width, (unsigned)height,
			      &new_width, &new_height);
    NATIVE_UNLOCK(env);

    if (status == True) {

        (*env)->SetIntField(env, cursorSize, GSCachedIDs.java_awt_Dimension_widthFID, (jint)new_width);
        (*env)->SetIntField(env, cursorSize, GSCachedIDs.java_awt_Dimension_heightFID, (jint)new_height);
    } else {
	/* 16x16 is always legal. */
        (*env)->SetIntField(env, cursorSize, GSCachedIDs.java_awt_Dimension_widthFID, (jint)16);
        (*env)->SetIntField(env, cursorSize, GSCachedIDs.java_awt_Dimension_heightFID, (jint)16);
    }
}

jobject getColorModel(JNIEnv *env) {
    Visual    *v;
    int       depth;
    jobject   colorModel;

    v     = root_visual;
    depth = screen_depth;

    if (v->class == TrueColor) {
        /* make a DirectColorModel */
/*
	if (MUSTLOAD(java_awt_image_DirectColorModel)) {
	    initDirectColorModel(env);
	}
*/

/*
	colorModel = 
	    (*env)->NewObject(env, 
			      JNIjava_awt_image_DirectColorModel.class,
			      JNIjava_awt_image_DirectColorModel.newIIIII,
			      depth, v->red_mask, v->green_mask, v->blue_mask, 
			      0);
*/

        colorModel = 
            (*env)->NewObject(env, IRCachedIDs.DirectColorModel,
                                   IRCachedIDs.DirectColorModel_constructor,
                                   depth, v->red_mask, v->green_mask, v->blue_mask, 0 );



    } else {
        /* Make an IndexColorModel */
        jbyteArray red = (*env)->NewByteArray(env, numColors);
	jbyteArray grn = (*env)->NewByteArray(env, numColors);
	jbyteArray blu = (*env)->NewByteArray(env, numColors);
	jbyte *rBase, *gBase, *bBase;
	unsigned int i;

/*
	if (MUSTLOAD(java_awt_image_IndexColorModel)) {
	    initIndexColorModel(env);
	}
*/

	rBase = (*env)->GetByteArrayElements(env, red, 0);
	gBase = (*env)->GetByteArrayElements(env, grn, 0);
	bBase = (*env)->GetByteArrayElements(env, blu, 0);

	for (i = 0; i < numColors; ++i) {
	    int r, g, b;
	    pixel2RGB(i, &r, &g, &b);
	    rBase[i] = (jbyte)r;
	    gBase[i] = (jbyte)g;
	    bBase[i] = (jbyte)b;
	}

	(*env)->ReleaseByteArrayElements(env, red, rBase, JNI_COMMIT);
	(*env)->ReleaseByteArrayElements(env, grn, gBase, JNI_COMMIT);
	(*env)->ReleaseByteArrayElements(env, blu, bBase, JNI_COMMIT);

/*
	colorModel= 
	    (*env)->NewObject(env, 
			      JNIjava_awt_image_IndexColorModel.class,
			      JNIjava_awt_image_IndexColorModel.newIIaBaBaB,
			      depth, numColors, red, grn, blu);
*/

        colorModel = 
            (*env)->NewObject(env, IRCachedIDs.IndexColorModel,
                                   IRCachedIDs.IndexColorModel_constructor,
                                   depth, numColors, red, grn, blu );

    }

    return colorModel;
}


int oops(const char *msg, const char *file, int line) {
    fprintf(stderr, "file %s, line %d: %s\n", file, line, msg);
    fflush(stderr);
    return 0;
}

