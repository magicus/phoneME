/*
 * @(#)QtToolkit.cc	1.37 06/10/25
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

#include <qt.h>
#include <qstrlist.h>

#include <string.h>

#include "jni.h"
#include "sun_awt_qt_QtToolkit.h"
#include "java_awt_Frame.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_MouseEvent.h"
#include "java_awt_event_PaintEvent.h"
#include "java_awt_event_KeyEvent.h"
#include "KeyCodes.h"
#include "awt.h"
#include "QtComponentPeer.h"
#include "QtWindowPeer.h"
#include "QtApplication.h"
#include <qptrdict.h>
#include "QtEvent.h"
#include "ImageRepresentation.h"
#include "QtToolkit.h"
#include "QtDisposer.h"

// We don't know how to programmatically get the taskbar height yet.
#define TASKBAR_HEIGHT_320x240     19
#define TASKBAR_HEIGHT_640x480     (2*TASKBAR_HEIGHT_320x240)

#define CLICK_PROXIMITY 5

JavaVM *JVM;

struct CachedIDs QtCachedIDs;

/* Initialize variables. */
// 6176847
// Changed to pointer types instead of objects.
QPtrDict<bool> *keyEventDict   = new QPtrDict<bool>(31);
QPtrDict<bool> *crossEventDict = new QPtrDict<bool>(31);
QPtrDict<bool> *mouseEventDict = new QPtrDict<bool>(31);
QPtrDict<bool> *focusEventDict = new QPtrDict<bool>(31);
// 6176847

class QtToolkitEventHandler : public QObject
{
    public:
    QtToolkitEventHandler();
    bool eventFilter(QObject *obj, QEvent *evt);

    private:
    static QtComponentPeer *lastMousePressPeer;
    static QPoint* lastMousePressPoint;

    static void postMouseEvent (JNIEnv *env,
                                jobject thisObj,
                                jint id,
                                jlong when,
                                jint modifiers,
                                jint x,
                                jint y,
                                jint clickCount,
                                jboolean popupTrigger,
                                QtEventObject *event);
    static void postKeyEvent (JNIEnv *env,
                              QtEventObject *event,
                              QtComponentPeer *component);
    static void postMouseButtonEvent (JNIEnv *env,
                                      QtEventObject *event,
                                      QtComponentPeer *component);
    static void postMouseCrossingEvent (JNIEnv *env,
                                        QtEventObject *event,
                                        QtComponentPeer *component);
    static void postMouseMovedEvent (JNIEnv *env,
                                     QtEventObject *event,
                                     QtComponentPeer *component);
    static jlong getCurrentTime();
    static jint getModifiers(ButtonState state);
};


QtComponentPeer *QtToolkitEventHandler::lastMousePressPeer = NULL;
QPoint *QtToolkitEventHandler::lastMousePressPoint = NULL;

#ifdef QT_KEYPAD_MODE
static jclass toolkitClass = NULL;
static jmethodID gotoHomeScreenEDT = NULL;
static jmethodID gotoNextAppEDT = NULL;

// If PASS_ARROW_KEYS is defined, QtToolkit should not
// interpret arrow keys as possible focus transfer gestures and
// should let the Java layer handle the keys.
static bool passArrowKeys = false;
#endif /* QT_KEYPAD_MODE */

/*
 * Class:     sun_awt_qt_QtToolkit
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtToolkit_initIDs (JNIEnv *env, jclass cls)
{
#ifdef QT_KEYPAD_MODE
    /* Caches the toolkitClass and two CDCAms lifecycle methods. */
    toolkitClass = (jclass) env->NewGlobalRef(cls);

    gotoHomeScreenEDT = env->GetStaticMethodID(cls,
                                               "gotoHomeScreenEDT",
                                               "()V");
    gotoNextAppEDT = env->GetStaticMethodID(cls,
                                            "gotoNextAppEDT",
                                            "()V");
#endif /* QT_KEYPAD_MODE */

#ifdef QWS
    /* Need to remember QtToolkit class, since we need it at the end of
     * initialization. (Note :- "cls" gets overwritten in this method
     */
    jclass thisClass = cls ;
#endif /* QWS */

    if (env->GetJavaVM(&JVM) != 0)
	return;
    cls = env->FindClass ("java/lang/Thread");

    if (cls == NULL)
        return;

    QtCachedIDs.java_lang_ThreadClass = (jclass) env->NewGlobalRef
        (cls);
    GET_STATIC_METHOD_ID(java_lang_Thread_yieldMID, "yield", "()V");
    GET_STATIC_METHOD_ID(java_lang_Thread_sleepMID, "sleep", "(J)V");

    cls = env->FindClass ("java/awt/Toolkit");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_ToolkitClass = (jclass) env->NewGlobalRef (cls);
    GET_STATIC_METHOD_ID(java_awt_Toolkit_getDefaultToolkitMID, "getDefaultToolkit", "()Ljava/awt/Toolkit;");
    GET_METHOD_ID(java_awt_Toolkit_getColorModelMID, "getColorModel", "()Ljava/awt/image/ColorModel;");

    cls = env->FindClass ("java/lang/NullPointerException");

    if (cls == NULL)
        return;

    QtCachedIDs.NullPointerExceptionClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/lang/OutOfMemoryError");

    if (cls == NULL)
        return;

    QtCachedIDs.OutOfMemoryErrorClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/lang/ArrayIndexOutOfBoundsException");

    if (cls == NULL)
        return;

    QtCachedIDs.ArrayIndexOutOfBoundsExceptionClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/lang/IllegalArgumentException");

    if (cls == NULL)
        return;

    QtCachedIDs.IllegalArgumentExceptionClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/lang/NoSuchMethodError");

    if (cls == NULL )
        return;

    QtCachedIDs.NoSuchMethodErrorClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/awt/Insets");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_InsetsClass = (jclass) env->NewGlobalRef (cls);

    GET_METHOD_ID (java_awt_Insets_constructorMID, "<init>", "(IIII)V");

    cls = env->FindClass ("java/awt/AWTError");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_AWTErrorClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/awt/Color");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_ColorClass = (jclass) env->NewGlobalRef (cls);
    GET_METHOD_ID (java_awt_Color_constructorMID, "<init>", "(IIII)V");
    GET_METHOD_ID (java_awt_Color_getRGBMID, "getRGB", "()I");

    cls = env->FindClass ("java/awt/Component");

    if (cls == NULL)
        return;
    GET_FIELD_ID (java_awt_Component_peerFID, "peer", "Lsun/awt/peer/ComponentPeer;");
    GET_FIELD_ID (java_awt_Component_backgroundFID, "background", "Ljava/awt/Color;");
    GET_FIELD_ID (java_awt_Component_foregroundFID, "foreground", "Ljava/awt/Color;");
    GET_FIELD_ID (java_awt_Component_xFID, "x", "I");
    GET_FIELD_ID (java_awt_Component_yFID, "y", "I");
    GET_FIELD_ID (java_awt_Component_widthFID, "width", "I");
    GET_FIELD_ID (java_awt_Component_heightFID, "height", "I");
    GET_FIELD_ID (java_awt_Component_eventMaskFID, "eventMask", "J");
    GET_FIELD_ID (java_awt_Component_mouseMotionListenerFID, "mouseMotionListener", "Ljava/awt/event/MouseMotionListener;");
    GET_FIELD_ID (java_awt_Component_mouseListenerFID, "mouseListener", "Ljava/awt/event/MouseListener;");
    GET_FIELD_ID (java_awt_Component_cursorFID, "cursor", "Ljava/awt/Cursor;");
    GET_FIELD_ID (java_awt_Component_fontFID, "font", "Ljava/awt/Font;");
    GET_FIELD_ID (java_awt_Component_isPackedFID, "isPacked", "Z");
    GET_METHOD_ID (java_awt_Component_getBackgroundMID, "getBackground", "()Ljava/awt/Color;");

    cls = env->FindClass("java/awt/Label");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_LabelClass = (jclass) env->NewGlobalRef
        (cls);

    GET_METHOD_ID (java_awt_Label_getTextMID, "getText",
                   "()Ljava/lang/String;");

    cls = env->FindClass ("java/awt/TextArea");

    if (cls == NULL)
        return;

    GET_METHOD_ID (java_awt_TextArea_getScrollbarVisibilityMID,
                   "getScrollbarVisibility", "()I");

    cls = env->FindClass ("java/awt/MenuComponent");

    if (cls == NULL)
        return;

    GET_FIELD_ID (java_awt_MenuComponent_peerFID, "peer", "Lsun/awt/peer/MenuComponentPeer;");

    cls = env->FindClass ("java/awt/Point");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_PointClass = (jclass) env->NewGlobalRef (cls);

    GET_METHOD_ID (java_awt_Point_constructorMID, "<init>", "(II)V");

    cls = env->FindClass ("java/awt/Menu");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_MenuClass = (jclass) env->NewGlobalRef (cls);

    GET_METHOD_ID (java_awt_Menu_isTearOffMID, "isTearOff", "()Z");

    cls = env->FindClass ("java/awt/image/DirectColorModel");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_image_DirectColorModelClass = (jclass) env->NewGlobalRef (cls);
    GET_METHOD_ID (java_awt_image_DirectColorModel_constructorMID, "<init>", "(IIII)V");
    GET_FIELD_ID (java_awt_image_DirectColorModel_red_maskFID, "red_mask", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_red_offsetFID, "red_offset", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_red_scaleFID, "red_scale", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_green_maskFID, "green_mask", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_green_offsetFID, "green_offset", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_green_scaleFID, "green_scale", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_blue_maskFID, "blue_mask", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_blue_offsetFID, "blue_offset", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_blue_scaleFID, "blue_scale", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_maskFID, "alpha_mask", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_offsetFID, "alpha_offset", "I");
    GET_FIELD_ID (java_awt_image_DirectColorModel_alpha_scaleFID, "alpha_scale", "I");

    cls = env->FindClass ("java/awt/image/IndexColorModel");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_image_IndexColorModelClass = (jclass) env->NewGlobalRef (cls);
    GET_METHOD_ID (java_awt_image_IndexColorModel_constructorMID, "<init>", "(II[B[B[B)V");
    GET_FIELD_ID (java_awt_image_IndexColorModel_rgbFID, "rgb", "[I");

    cls = env->FindClass ("java/awt/image/ImageConsumer");

    if (cls == NULL)
        return;

    GET_METHOD_ID (java_awt_image_ImageConsumer_setColorModelMID, "setColorModel", "(Ljava/awt/image/ColorModel;)V");
    GET_METHOD_ID (java_awt_image_ImageConsumer_setHintsMID, "setHints", "(I)V");
    GET_METHOD_ID (java_awt_image_ImageConsumer_setPixelsMID, "setPixels", "(IIIILjava/awt/image/ColorModel;[BII)V");
    GET_METHOD_ID (java_awt_image_ImageConsumer_setPixels2MID, "setPixels", "(IIIILjava/awt/image/ColorModel;[III)V");

    cls = env->FindClass ("sun/awt/image/Image");

    if (cls == NULL)
        return;

    GET_METHOD_ID (sun_awt_image_Image_getImageRepMID, "getImageRep",
                   "()Lsun/awt/image/ImageRepresentation;")

        cls = env->FindClass ("sun/awt/image/OffScreenImageSource");

    if (cls == NULL)
        return;

    GET_FIELD_ID (sun_awt_image_OffScreenImageSource_baseIRFID, "baseIR", "Lsun/awt/image/ImageRepresentation;");
    GET_FIELD_ID (sun_awt_image_OffScreenImageSource_theConsumerFID, "theConsumer", "Ljava/awt/image/ImageConsumer;");

    cls = env->FindClass ("java/awt/Dimension");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_DimensionClass = (jclass) env->NewGlobalRef (cls);
    GET_METHOD_ID (java_awt_Dimension_constructorMID, "<init>", "(II)V");

    cls = env->FindClass ("java/awt/Rectangle");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_RectangleClass = (jclass) env->NewGlobalRef (cls);
    GET_FIELD_ID (java_awt_Rectangle_xFID, "x", "I");
    GET_FIELD_ID (java_awt_Rectangle_yFID, "y", "I");
    GET_FIELD_ID (java_awt_Rectangle_widthFID, "width", "I");
    GET_FIELD_ID (java_awt_Rectangle_heightFID, "height", "I");
    GET_METHOD_ID (java_awt_Rectangle_constructorMID, "<init>", "(IIII)V");

    cls = env->FindClass ("java/awt/Font");

    if (cls == NULL)
        return;

    GET_FIELD_ID (java_awt_Font_peerFID, "peer", "Lsun/awt/peer/FontPeer;");

    cls = env->FindClass ("sun/awt/qt/QtFontPeer");

    if (cls == NULL)
        return;

    QtCachedIDs.QtFontPeerClass = (jclass) env->NewGlobalRef (cls);
    GET_FIELD_ID (QtFontPeer_ascentFID, "ascent", "I");
    GET_FIELD_ID (QtFontPeer_descentFID, "descent", "I");
    GET_FIELD_ID (QtFontPeer_dataFID, "data", "I");
    GET_STATIC_METHOD_ID (QtFontPeer_getFontMID, "getFont",
                          "(I)Ljava/awt/Font;");

    cls = env->FindClass ("java/awt/image/BufferedImage");

    if (cls == NULL)
        return;

    QtCachedIDs.java_awt_image_BufferedImageClass = (jclass) env->NewGlobalRef (cls);
    GET_FIELD_ID (java_awt_image_BufferedImage_peerFID, "peer", "Lsun/awt/image/BufferedImagePeer;");
    GET_METHOD_ID (java_awt_image_BufferedImage_constructorMID, "<init>", "(Lsun/awt/image/BufferedImagePeer;)V");

    cls = env->FindClass( "java/awt/Dialog" );
    if (cls == NULL) {
        return;
    }

    GET_FIELD_ID (java_awt_Dialog_modalFID, "modal", "Z");
    GET_METHOD_ID (java_awt_Dialog_hideMID, "hide", "()V" );

#ifdef QWS
    jmethodID mid = env->GetStaticMethodID (thisClass,
                                            "installImageDecoderFactory",
                                            "([Ljava/lang/String;)V") ;
    if ( mid != NULL ) {
        /* get the list of image formats available on the platform */
        QStrList imgFormatList = QImage::inputFormats() ;
        int numFormats = imgFormatList.count() ;
        jclass string_class = env->FindClass("java/lang/String") ;

        if ( numFormats > 0 && string_class != NULL ) {
            /* JAVA: String[] formats = new String[numFormats] */
            jobjectArray formats = (jobjectArray)env->NewObjectArray(
                                                 numFormats,
                                                 string_class,
                                                 NULL) ;
            if ( formats != NULL ) {
                int i = 0 ;
                /* formats[i] = new String(format) */
                for ( i = 0 ; i < numFormats ; i++ ) {
                    env->SetObjectArrayElement(formats,
                         i,
                         env->NewStringUTF(imgFormatList.at(i))) ;
                }

                /* QtToolkit.installImageDecoderFactory(formats) */
                env->CallStaticVoidMethod(thisClass,mid,formats) ;
            }
        }
    }
#endif /* QWS */
    //6440627.  Initialize the pmutex here.
    QtDisposer::init();
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtToolkit_synthesizeFocusIn (JNIEnv *env, jobject thisObj, jobject windowPeer)
{
    QtDisposer::Trigger guard(env);

#ifdef QWS
#ifndef QT_KEYPAD_MODE

    QtWindowPeer *window = (QtWindowPeer *)QtComponentPeer::getPeerFromJNI (env, windowPeer);
	
    if (window == NULL || window->getWidget() == NULL) {
        return;
    }
	
    window->handleFocusIn(env);

#endif //QT_KEYPAD_MODE
#endif //QWS
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtToolkit_getScreenWidth (JNIEnv *env, jobject thisObj)
{
    jint width;

    AWT_QT_LOCK;
    QWidget *d = QApplication::desktop();
    width = d->width();
    AWT_QT_UNLOCK;

    return width;
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtToolkit_getScreenHeight (JNIEnv *env, jobject thisObj)
{
    jint height;

    AWT_QT_LOCK;
    QWidget *d = QApplication::desktop();
    height = d->height();

#ifdef QWS
    int taskbar_height = 0 ;
    int width = d->width() ;
    if ( height > width ) {
        /* potrait mode */
        switch ( height ) {
        case 320 :
            taskbar_height = TASKBAR_HEIGHT_320x240 ;
            break ;
        case 640 :
            taskbar_height = TASKBAR_HEIGHT_640x480 ;
            break ;
        default :
            break ;
        }
    }
    else {
        /* landscape mode */
        switch ( height ) {
        case 240 :
            taskbar_height = TASKBAR_HEIGHT_320x240 ;
            break ;
        case 480 :
            taskbar_height = TASKBAR_HEIGHT_640x480 ;
            break ;
        default :
            break ;
        }
    }
    height = height - taskbar_height;
#endif

    AWT_QT_UNLOCK;

    return height;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtToolkit_beep (JNIEnv *env, jobject thisObj)
{
    AWT_QT_LOCK;
    QApplication::beep();
    AWT_QT_UNLOCK;
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtToolkit_sync (JNIEnv *env, jobject thisObj)
{
    AWT_QT_LOCK;
    QApplication::flushX();
    AWT_QT_UNLOCK;
}

JNIEXPORT jboolean JNICALL
Java_sun_awt_qt_QtToolkit_isFrameStateSupported (JNIEnv *env, jobject thisObj, jint state)
{
    if (state == java_awt_Frame_NORMAL) 
       return JNI_TRUE;

// We don't support iconification for zaurus now.
#ifndef QWS
    if (state == java_awt_Frame_ICONIFIED) 
       return JNI_TRUE;
#endif

    return JNI_FALSE;
}


JNIEXPORT jobject JNICALL
Java_sun_awt_qt_QtToolkit_createBufferedImage (JNIEnv * env, jclass cls, jobject peer)
{
    return env->NewObject (QtCachedIDs.java_awt_image_BufferedImageClass,
                           QtCachedIDs.java_awt_image_BufferedImage_constructorMID,
                           peer);
}

JNIEXPORT jobject JNICALL
Java_sun_awt_qt_QtToolkit_getBufferedImagePeer (JNIEnv * env, jclass cls, jobject bufferedImage)
{
    return env->GetObjectField (bufferedImage,
                                QtCachedIDs.java_awt_image_BufferedImage_peerFID);
}

/*
 * Class:     sun_awt_qt_QtToolkit
 * Method:    createNativeApp
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtToolkit_createNativeApp (JNIEnv *env, jobject thisObj)
{
    /* We expect this function to be called only once per invocation
       of the VM. */

    // Prepare QApplication constructor arguments.

    static int argc = 1;

    /*
     * We need to create an array of size 3 because the QApplication assigns a
     * NULL pointer to the argv[argc] position after it processes and removes
     * all the Qt command line options from the argv vector.  This is a Qt bug
     * to write past the end of the argv vector.
     */
    static char *argv[3] = { "cvm", "-qws", NULL };

#ifdef QWS
#ifndef QTOPIA
    // If environment varible QWS_CLIENT is not defined,
    // assume we are running as the server (in the qt-embedded sense).
    if (getenv("QWS_CLIENT") == NULL) {
        argc++;
    }
#endif /* QTOPIA */
#endif /* QWS */

#ifdef QT_KEYPAD_MODE
    if (getenv("PASS_ARROW_KEYS") != NULL && strcmp("false", getenv("PASS_ARROW_KEYS"))) {
        // The environment variable must be defined and not equal to "false".
        passArrowKeys = true;
    } else {
        passArrowKeys = false;
    }
#endif /* QT_KEYPAD_MODE */

    /*
     * Retrieve the appName system property, if any.
     * Also keep the QString around because QApplication provides
     * argc() and argv() instance methods for the API clients.
     */
    jclass cls = env->GetObjectClass(thisObj);
    jmethodID mid = env->GetStaticMethodID(cls, "getAppName", "()Ljava/lang/String;");
    jstring appName = (jstring) env->CallStaticObjectMethod(cls, mid);
    QString *str = awt_convertToQString(env, appName);

    // set the argv to the appName if it's provided
    if (str != 0) {
        argv[0] = (char *)str->latin1();
    }

    // Create QApplication object.

    QtApplication *app = new QtApplication(argc, argv);

    app->setGlobalMouseTracking(true);
    //app->dumpObjectTree();
    //app->dumpObjectInfo();

    if (!app) {
        env->ThrowNew(QtCachedIDs.OutOfMemoryErrorClass, NULL);
        return;
    }

#ifdef QT_THREAD_SUPPORT
#if (QT_VERSION < 0x030000)
    /* In Qt2.3.2 expects the same thread that creates the QApplication
     * to call QApplication.exec() and the QApplication constructor locks
     * the qt-library lock. In the peer implementation exec() is called on
     * a different thread, so we cheat by unlocking it here and locking
     * before calling exec() in runNative(). 
     */
    qtApp->unlock(FALSE);
#endif /* QT_VERSION */
#endif /* QT_THREAD_SUPPORT */
}

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtToolkit_finalize (JNIEnv *env, jobject thisObj)
{
}

/*
 * Class:     sun_awt_qt_QtToolkit
 * Method:    runNative
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtToolkit_runNative (JNIEnv *env, jobject thisObj)
{
    /* Override event handler so that we can post low level events (eg mouse and keyboard) to
       the Java event queue for processing by the event dispatch thread. */
    QtToolkitEventHandler evtHandler;    

    //6440627. Qt peer can be accessed before toolkit thread reaches here
    //         Initialize QtDisposer at static initializer instead.
    //QtDisposer::init();

    /* Run the qt event dispatching loop. */
#ifdef QT_THREAD_SUPPORT
#if (QT_VERSION < 0x030000)
    /* See runNative() where we unlock() the library lock and the reasons
     * why we need to lock before calling exec()
     */
    qtApp->lock();
#endif /* QT_VERSION */
#endif /* QT_THREAD_SUPPORT */

    // use the QtApplication's overriden exec() instead of the base class.
    qtApp->exec(&evtHandler);
}

// 6176847
JNIEXPORT void JNICALL
Java_sun_awt_qt_QtToolkit_shutdownEventLoop (JNIEnv *env, jobject thisObj)
{
    qtApp->shutdown();
}
// 6176847

JNIEXPORT void JNICALL
Java_sun_awt_qt_QtToolkit_setMainWidgetNative (JNIEnv *env, jobject
                                               thisObj, jobject
                                               mainWidgetPeer)
{
    AWT_QT_LOCK;

    if (qtApp->mainWidget()) {
        AWT_QT_UNLOCK;
        return;
    }

    if (!mainWidgetPeer) {
        AWT_QT_UNLOCK;
        env->ThrowNew ( QtCachedIDs.NullPointerExceptionClass, NULL );
        return;
    }

    QtDisposer::Trigger guard(env);

    QtComponentPeer *nativePeer = QtComponentPeer::getPeerFromJNI(env,
                                                                  mainWidgetPeer);
    QpWidget *mainWidget = nativePeer->getWidget();
    //    mainWidget->setMouseTracking(true);
    qtApp->setMainWidget( mainWidget );
    qtApp->setFont( mainWidget->font() );

    AWT_QT_UNLOCK;
}

JNIEXPORT jobject JNICALL
Java_sun_awt_qt_QtToolkit_getColorModelNative (JNIEnv *env,
                                               jobject thisObj)
{
    return ImageRepresentation::defaultColorModel(env);
}

JNIEXPORT jint JNICALL
Java_sun_awt_qt_QtToolkit_getScreenResolutionNative (JNIEnv *env,
                                                     jobject thisObj)
{
    jint width ,widthMM, calcDpi;

    AWT_QT_LOCK;
    QWidget *d = QApplication::desktop();
    width = d->width();
    QPaintDeviceMetrics pdm( d );
    widthMM = pdm.widthMM();
    /* 1 inch = 2.54 cm */
    calcDpi = (jint) (width * 25.4) / widthMM;
    AWT_QT_UNLOCK;
    return calcDpi;
}

QtToolkitEventHandler::QtToolkitEventHandler()
{
}


// The following Clone routines clone and return a new QEvent instance
// wrapped within a "QtEventObject". They return NULL if memory cannot 
// be allocated
//
// They are called from inside QtToolkitEventHandler::eventFilter() whenever
// we are about to post an event to the Java layer.  When the Java layer is
// finished with the event, it can decide whether to add the qt event back to
// the qt event queue.  See QtComponentPeer.cc/.java for details.
//
// These events are deleted in the following routines:
//
// o Java_sun_awt_qt_QtComponentPeer_postMouseEventToQt()
// o Java_sun_awt_qt_QtComponentPeer_postKeyEventToQt()
// o Java_sun_awt_qt_QtComponentPeer_eventConsumed()
//
// defined within QtComponentPeer.cc.

static
QtEventObject *CloneMouseEvent(QObject *source, QMouseEvent *me)
{
    QMouseEvent *cme = new QMouseEvent(me->type(),
				       me->pos(),
				       me->globalPos(),
				       me->button(),
				       me->state());
    QT_RETURN_NEW_EVENT_OBJECT(source, cme);
}

static
QtEventObject *CloneEvent(QObject *source,QEvent *e)
{
    QEvent *ce = new QEvent(e->type());
    QT_RETURN_NEW_EVENT_OBJECT(source, ce);
}

static
QtEventObject *CloneKeyEvent(QObject *source, QKeyEvent *e)
{
#ifdef QT_KEYPAD_MODE
    // OMAP Keypad modification:
    // Workaround KEYPAD driver where only key presses are generated
    // and ascii is always 0.
    jint javaCode;
    jint unicode = 0;
    jint modifiers = 0;

    QChar qc;
    QString qs;
    bool transform = false;
    if (Qt::Key_0 <= e->key() && e->key() <= Qt::Key_9) {
        javaCode = awt_qt_getJavaKeyCode (e->key(), FALSE);
        unicode = awt_qt_getUnicodeChar (javaCode, modifiers);
        qc = QChar(unicode);
        qs = QString(qc);
        transform = true;
    }
    /*
    if (e->key() == Qt::Key_Asterisk) {
        qs = QString("*");
        qc = qs.at(0);
        transform = true;
    }
    if (e->key() == Qt::Key_NumberSign) {
        qs = QString("#");
        qc = qs.at(0);
        transform = true;
    }
    */

    QKeyEvent *cke = new QKeyEvent(e->type(),
                                   e->key(),
                                   (transform ? qc.latin1() : e->ascii()),
                                   //(transform ? qc.unicode() : e->ascii()),
                                   e->state(),
                                   (transform ? qs : e->text()),
                                   e->isAutoRepeat(),
                                   e->count());

#else
    QKeyEvent *cke = new QKeyEvent(e->type(), 
                                   e->key(), 
                                   e->ascii(),
                                   e->state(), 
                                   e->text(),
                                   e->isAutoRepeat(), 
                                   e->count());
#endif /* QT_KEYPAD_MODE */

    QT_RETURN_NEW_EVENT_OBJECT(source, cke);
}

/*
void DebugQEvent(QObject* obj, QEvent* event)
{
    QEvent::Type type = event->type();

    // Ignore timer event.
    if (type == QEvent::Timer) return;

    printf("DebugQEvent:\n");
    printf("======================================================================\n");
    printf("event source (object className=%s)->\n", obj->className());
    obj->dumpObjectTree();

    printf("event type (%d)->\n", type);
    QKeyEvent* keyEvent;
    //QMouseEvent* mouseEvent;
    switch (type) {
    case QEvent::KeyPress:
        keyEvent = (QKeyEvent*) event;
        printf("QEvent::KeyPress, key=0x%x, ascii=%d, text=\"%s\", count=%d (%s)\n",
               keyEvent->key(), keyEvent->ascii(), (const char*)keyEvent->text(), keyEvent->count(),
               (keyEvent->isAutoRepeat() ? "autorepeat" : "non-autorepeat"));
        break;
    case QEvent::KeyRelease:
        keyEvent = (QKeyEvent*) event;
        printf("QEvent::KeyRelease, key=0x%x, ascii=%d, text=\"%s\", count=%d (%s)\n",
               keyEvent->key(), keyEvent->ascii(), (const char*)keyEvent->text(), keyEvent->count(),
               (keyEvent->isAutoRepeat() ? "autorepeat" : "non-autorepeat"));
        break;
    default:
        break;
    }
    printf("======================================================================\n");
}
*/

#ifdef QT_KEYPAD_MODE
static QEvent*
NEW_TAB_EVENT(bool shiftOn)
{
    return
        new QKeyEvent(QEvent::KeyPress,
                      shiftOn ? Qt::Key_Backtab : Qt::Key_Tab,
                      '\t',
                      shiftOn ? Qt::ShiftButton : Qt::NoButton);
}

static bool
AMS_EVENT_FILTER(QObject* obj, QEvent* evt, JNIEnv* env)
{
    if (!obj->inherits("QWidget")) return false;

    QKeyEvent* ke;
    bool focusPrev = false;
    bool focusNext = false;
    bool returnVal = false;
    if (evt && evt->type() == QEvent::KeyPress) {
        ke = (QKeyEvent*) evt;
        // Possible AMS events.
        switch (ke->key()) {
        case Qt::Key_Home:
            // Key_Home is pressed.
            if (gotoHomeScreenEDT && toolkitClass) {
                env->CallStaticVoidMethod(toolkitClass,
                                          gotoHomeScreenEDT);
            }
            returnVal = true;
            break;
        case Qt::Key_NumberSign:
            // Key_NumberSign is pressed.
            // Emulates the gotoNextApp button press.
            if (gotoNextAppEDT && toolkitClass) {
                env->CallStaticVoidMethod(toolkitClass,
                                          gotoNextAppEDT);
            }
            returnVal = true;
            break;
        case Qt::Key_Context1:
            // Key_Context1 is pressed.
            if (obj->inherits("QMultiLineEdit")) {
                // while in the text area....
                focusPrev = true;
                returnVal = true;
            }
            break;
        case Qt::Key_Context2:
            // Key_Context2 is pressed.
            if (obj->inherits("QMultiLineEdit")) {
                // while in the text area....
                focusNext = true;
                returnVal = true;
            }
            // Commented out the calling of gotoNextAppEDT.
            // For the time being and without modifying the personal spec,
            // we are mapping Key_Context1/2 to VK_F1/F2 for
            // the application to manage.
            /*
            if (gotoNextAppEDT && toolkitClass) {
                env->CallStaticVoidMethod(toolkitClass,
                                          gotoNextAppEDT);
            }
            returnVal = true;
            */
            break;
        case Qt::Key_Left:
            // Key_Left is pressed.
            if (passArrowKeys) {
                break;
            }

            if (obj->inherits("QListBox")
                /* || obj->inherits("QComboBox") */
                || (obj->inherits("QScrollBar")
                    && ((QScrollBar*)obj)->orientation() == QScrollBar::Vertical)
                )
            {
                // Transforming to Shift+Tab....
                focusPrev = true;
                returnVal = true;
            }
            break;
        case Qt::Key_Right:
            // Key_Right is pressed.
            if (passArrowKeys) {
                break;
            }

            if (obj->inherits("QListBox")
                /* || obj->inherits("QComboBox") */
                || (obj->inherits("QScrollBar")
                    && ((QScrollBar*)obj)->orientation() == QScrollBar::Vertical)
                )
            {
                // Transforming to Tab....
                focusNext = true;
                returnVal = true;
            }
            break;
        case Qt::Key_Up:
            // Key_Up is pressed.
            if (passArrowKeys) {
                break;
            }

            if (obj->inherits("QListBox")
                /* || obj->inherits("QComboBox") */ // For java.awt.Choice, Up and Down initiates focus traversal.
                || obj->inherits("QMultiLineEdit")
                )
            {
                // QListBox or QMultiLineEdit, pass the event through....
                break;
            }
            if (!obj->inherits("QScrollBar") || ((QScrollBar*)obj)->orientation() == QScrollBar::Horizontal) {
                // Transforming to Shift+Tab....
                focusPrev = true;
                returnVal = true;
            }
            break;
        case Qt::Key_Down:
            // Key_Down is pressed.
            if (passArrowKeys) {
                break;
            }

            if (obj->inherits("QListBox")
                /* || obj->inherits("QComboBox") */ // For java.awt.Choice, Up and Down initiates focus traversal.
                || obj->inherits("QMultiLineEdit")
                )
            {
                // QListBox or QComboBox or QMultiLineEdit, pass the event through....
                break;
            }
            if (!obj->inherits("QScrollBar") || ((QScrollBar*)obj)->orientation() == QScrollBar::Horizontal) {
                // Transforming to Tab....
                focusNext = true;
                returnVal = true;
            }
            break;
        default:
            // Nothing to do with AMS.
            break;
        }
        if (focusPrev || focusNext) {
            QApplication::postEvent(obj, NEW_TAB_EVENT(focusPrev));
        }
    }
    return returnVal;
}
#endif /* QT_KEYPAD_MODE */

bool
QtToolkitEventHandler::eventFilter(QObject *obj, QEvent *event)
{
    //DebugQEvent(obj, event);

    JNIEnv *env;
    bool returnVal = FALSE; // allow Qt to process the event.

    if (JVM->AttachCurrentThread((void **) &env, NULL) != 0) {
        return FALSE;
    }

#ifdef QT_KEYPAD_MODE
    if (AMS_EVENT_FILTER(obj, event, env)) return TRUE;
#endif /* QT_KEYPAD_MODE */

    QtDisposer::Trigger guard(env);

    /* Determine the widget from the event. */
    QtComponentPeer *component = QtComponentPeer::getPeerForWidget
        ((QWidget *)obj);

    QKeyEvent *keyEvent;
    QMouseEvent *mouseEvent;

    if (component != NULL) {
        switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
            // check to see if we've already processed this event in java
            //changed the state here from 0x8000 to 0x0800 - docs are
            // wrong; 0x8000 is used by Qt to indicate soft keyboard
            //if (keyEvent->state() & 0x0800) {
            keyEvent = (QKeyEvent *)event;
            //see if this event is in our dictionary; if so, it goes to qt
            if ((*keyEventDict)[keyEvent]) {
                // java is done with this event - send it up to qt
                keyEventDict->remove(keyEvent);
            }
            else {
                // spontaneous refers to events that are delivered from
                // the window system and not application created
                // (for example, using postEvent())
                //
                // We want to post only window system events to java, If we
                // dont do that we could end up in an infinite loop to this
                // method.
                //
                // The default implementation of keyReleaseEvent is to
                // ignore() it (See QWidget::QWidget::keyReleaseEvent). So
                // Qt would try to use the parent widget to dispatch, since
                // eventFilters are called first, this method would be called
                // and without this check, we would post to the java's queue
                // a synthetic event and we get to an infinite loop.
                //
                // Also, a spontaneous event automatically becomes 
                // non-spontaneous if it is not handled by the widget event
                // handler or eventFilters.
                
                if ( IS_SPONTANEOUS(event) ) {
                    // java hasn't seen this event yet - don't give it to Qt
                    postKeyEvent (env, 
                                  CloneKeyEvent(obj, (QKeyEvent *)event), 
                                  component);
                    returnVal = TRUE; // tell Qt not to proceed further with
                                      // the event dispatch. After java
                                      // handles it we will create a cloned
                                      // synthetic event for Qt to process.
                }
            }
            break;

        case QtEvent::Key:
          keyEvent = (QKeyEvent *)((QtEvent *)event)->data() ;
          // treat the key event as spontaneous
          postKeyEvent (env, 
                        CloneKeyEvent(obj,keyEvent), 
                        component);
          returnVal = TRUE; 
          break ;

        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
	    // The Qt-3.3's event propagation mechanism is different from
	    // Qt-2.3.3-emb.  3.3 now propagates the mouse event up its
	    // widget parent chain.  This propagation of the mouse events
	    // conflicts with our mechanism in postMouseMovedEvent() which
	    // determines whether to post a MOUSE_CLICKED event to the Java
	    // layer.
	    //
	    // If we blindly call postMouseButtonEvent() on any button press
	    // event, we can get a button press event for each widget while
	    // Qt-3.3 traverses its parent chain:
	    //
	    // -------------------  Say, there's a mouse press event from Qt
	    // |A      	       	 |  for widget C and we call the function
	    // |   ----------  	 |  which then records that the last mouse
	    // |   |B  	    |  	 |  press happens in widget C.  If Qt's widget
	    // |   |  ----- |	 |  chain does not handle the event, we may
	    // |   |  |C  | |	 |  end up with Qt generating mouse press events
	    // |   |  |---| |	 |  for widget B, and then C, and the last mouse
	    // |   ----------	 |  press will happen in widget A.  When the
	    // |	     	 |  user releases the mouse button at the same
	    // -------------------  position in C, we will get release events
	    //                      for C, B, and then A.  As a result, the
	    //                      mouse clicked event is generated for widget
	    //                      A instead of C, clearly not what we wanted!
	    //
	    // Fortunately, Qt-3.3 changes the event while traversing the parent
	    // chain from a spontaneous event to a non-spontaneous event.  Once
	    // we see a non-spontaneous event, we don't bother calling our post
	    // mouse button event function at all.
	    if ((*mouseEventDict)[event]) {
            mouseEventDict->remove(event);
	    } 
        else 
		if (IS_SPONTANEOUS(event)) {
		    postMouseButtonEvent (env, 
                                  CloneMouseEvent(obj,(QMouseEvent *)event), 
                                  component);
		    returnVal = TRUE;
	    }
        break;
        // 6253974 - now the robot will use a QtEvent to send either a 
        // MouseMove or a MouseButton event - so there are two QtEvent types
        // to check for which should both be treated as spontaneous events
        case QtEvent::MouseMove :
          mouseEvent = (QMouseEvent *)((QtEvent *)event)->data() ;
          // treat the mouse event as spontaneous
          postMouseMovedEvent (env, CloneMouseEvent(obj,mouseEvent), component);
          returnVal = TRUE;
          break ;
        case QtEvent::MouseButton:
          mouseEvent = (QMouseEvent *)((QtEvent *)event)->data() ;
          // treat the mouse event as spontaneous
          postMouseButtonEvent (env, CloneMouseEvent(obj,mouseEvent), 
                                component);
          returnVal = TRUE;
          break ;

        case QEvent::MouseMove:
	    if ((*mouseEventDict)[event]) {
            mouseEventDict->remove(event);
	    } else 
		if (IS_SPONTANEOUS(event)) {
		    postMouseMovedEvent (env, 
                                 CloneMouseEvent(obj,(QMouseEvent *)event), 
                                 component);
		    returnVal = TRUE;
	    }
        break;
            
        case QEvent::Enter:
        case QEvent::Leave:
	    // Qt-3.3 sends Enter/Leave as non-spontaneous events.
	    if ((*crossEventDict)[event]) {
            crossEventDict->remove(event);
	    } else {
            postMouseCrossingEvent (env, 
                                    CloneEvent(obj,event), 
                                    component);
            returnVal = TRUE;
	    }
        break;

        case QEvent::Move:
            // 6238562 - don't ignore spontaneous move events for the 
            // top leve windows and dialogs - those won't get generated in java
            if (!IS_SPONTANEOUS(event) || !((QWidget*)obj)->isTopLevel())
  	        returnVal = TRUE ; // indicate that we have processed the event
            //6238562
/*
	    printf("QtToolkit::eventFilter(): move event (x, y)=(%d, %d) (%s)\n",
		   ((QMoveEvent*)event)->pos().x(), ((QMoveEvent*)event)->pos().y(),
		   (IS_SPONTANEOUS(event) ? "spontaneous" : "not spontaneous"));
*/
            break ;
        default:
            break;
        }
    }

    return returnVal;
}

/* Get the current time, referenced from January 1, 1970 */
jlong
QtToolkitEventHandler::getCurrentTime(void)
{
    static QDate refDate(1970, 1, 1);
    static QTime refTime(0, 0);
    static QDateTime refDateTime(refDate, refTime);

    QDateTime currentTime = QDateTime::currentDateTime();
    jlong secs = - ((jlong) currentTime.secsTo(refDateTime));

    return secs;
}

/* Gets the java.awt.event.Input event modifier mask for the specified state from a QEvent. */

jint
QtToolkitEventHandler::getModifiers (ButtonState state)
{
    jint modifiers = 0;

    if (state & Qt::ShiftButton)
        modifiers |= java_awt_event_InputEvent_SHIFT_MASK;

    if (state & Qt::ControlButton)
        modifiers |= java_awt_event_InputEvent_CTRL_MASK;

    if (state & Qt::AltButton)
        modifiers |= java_awt_event_InputEvent_ALT_MASK;

    if (state & Qt::LeftButton)
        modifiers |= java_awt_event_InputEvent_BUTTON1_MASK;

    if (state & Qt::MidButton)
        modifiers |= java_awt_event_InputEvent_BUTTON2_MASK;

    if (state & Qt::RightButton)
        modifiers |= java_awt_event_InputEvent_BUTTON3_MASK;

    return modifiers;
}

/* Utility function to post mouse events to the AWT event dispatch thread. */

void
QtToolkitEventHandler::postMouseEvent (JNIEnv *env,
                                       jobject thisObj,
                                       jint id,
                                       jlong when,
                                       jint modifiers,
                                       jint x,
                                       jint y,
                                       jint clickCount,
                                       jboolean popupTrigger,
                                       QtEventObject *eventObj)
{
    if ( eventObj == NULL ) {
        return;
    }
    env->CallVoidMethod (thisObj,
                         QtCachedIDs.QtComponentPeer_postMouseEventMID,
                         id,
                         when,
                         modifiers,
                         x ,
                         y ,
                         clickCount,
                         popupTrigger,
                         (jint) eventObj);

    if (env->ExceptionCheck ())
        env->ExceptionDescribe ();
}

/* Qt Callback function that posts mouse events to the target component when the mouse is moved. */

void
QtToolkitEventHandler::postMouseMovedEvent (JNIEnv *env,
                                            QtEventObject *eventObj,
                                            QtComponentPeer *component)
{
    if ( eventObj == NULL ) {
        return;
    }

    jobject thisObj = component->getPeerGlobalRef();
    QMouseEvent *event = (QMouseEvent *)eventObj->event;
    postMouseEvent (env,
                    thisObj,
                    (event->state() & Qt::LeftButton) ?
                    java_awt_event_MouseEvent_MOUSE_DRAGGED :
                    java_awt_event_MouseEvent_MOUSE_MOVED,
                    QtToolkitEventHandler::getCurrentTime(),
                    getModifiers (event->state()),
                    event->x(),
                    event->y(),
                    0,
                    JNI_FALSE,
                    eventObj);
}

/* Qt Callback function that posts mouse events to the target component when the mouse has enetered the component. */

void
QtToolkitEventHandler::postMouseCrossingEvent (JNIEnv *env,
                                               QtEventObject *eventObj,
                                               QtComponentPeer *component)
{
    if ( eventObj == NULL ) {
        return;
    }

    jobject thisObj = component->getPeerGlobalRef();
    QEvent *event = eventObj->event;
    QPoint cursorPos = QpObject::staticCursorPos();
//    int posX = QCursor::pos().x();
//    int posY = QCursor::pos().y();
    int posX = cursorPos.x();
    int posY = cursorPos.y();

    postMouseEvent (env,
                    thisObj,
                    (event->type() == QEvent::Enter) ?
                    java_awt_event_MouseEvent_MOUSE_ENTERED :
                    java_awt_event_MouseEvent_MOUSE_EXITED,
                    QtToolkitEventHandler::getCurrentTime(),
                    0,
                    posX - component->getWidget()->x(),
                    posY - component->getWidget()->y(),
                    0,
                    JNI_FALSE,
                    eventObj);
}

/* Qt Callback function that posts mouse events to the target component when a mouse button
   has been released on the component. */

void
QtToolkitEventHandler::postMouseButtonEvent (JNIEnv *env,
                                             QtEventObject *eventObj,
                                             QtComponentPeer *component)
{
    if ( eventObj == NULL ) {
        return;
    }

    jobject thisObj = component->getPeerGlobalRef();
    QMouseEvent *event = (QMouseEvent *)eventObj->event;
    jint modifiers = getModifiers (event->state());
    
    jint id;
    jboolean popupTrigger;

    switch (event->button()) {
    case Qt::LeftButton:
        modifiers |= java_awt_event_InputEvent_BUTTON1_MASK;
        break;

    case Qt::MidButton:
        modifiers |= java_awt_event_InputEvent_BUTTON2_MASK;
        break;

    case Qt::RightButton:
        modifiers |= java_awt_event_InputEvent_BUTTON3_MASK;
        break;

    default:
        break;
    }

    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonDblClick) {
        component->resetClickCount();

        if (event->type() == QEvent::MouseButtonDblClick) {
            component->incrementClickCount();
        }

        id = java_awt_event_MouseEvent_MOUSE_PRESSED;

        if ((event->button() & Qt::RightButton) && component->getClickCount() == 1) {
            popupTrigger = JNI_TRUE;
         }
#ifdef QWS
        // simulates right mouse click on the zaurus with shift and left-mouse-click
        else if ((event->button() == Qt::LeftButton) &&
                 (event->state() & Qt::ShiftButton) &&
                 (component->getClickCount() == 1)) {
            popupTrigger = JNI_TRUE;
        }
#endif
        else {
            popupTrigger = JNI_FALSE;
        }

        lastMousePressPeer = component;

        if (lastMousePressPoint != NULL)
            delete lastMousePressPoint;

        lastMousePressPoint = new QPoint(event->x(), event->y());
    } else {
        /* If we're here, it's a MouseButtonRelease event. */
        bool isSame = FALSE;
        QtComponentPeer *notifyComponent = component;

        if (lastMousePressPeer != NULL &&
            lastMousePressPeer != component) {
            notifyComponent = lastMousePressPeer;
        } else if (lastMousePressPeer == component) {
            lastMousePressPeer = NULL;
            isSame = TRUE;
        }

        id = java_awt_event_MouseEvent_MOUSE_RELEASED;

        if(notifyComponent->getPeerGlobalRef())
        {
            postMouseEvent (env,
                            notifyComponent->getPeerGlobalRef(),
                            java_awt_event_MouseEvent_MOUSE_RELEASED,
                            QtToolkitEventHandler::getCurrentTime(),
                            modifiers,
                            event->x(),
                            event->y(),
                            component->getClickCount(),
                            JNI_FALSE,
                            eventObj);
        }

        if (lastMousePressPoint != NULL) {
            int lastX = lastMousePressPoint->x();
            int lastY = lastMousePressPoint->y();
            int x = event->x();
            int y = event->y();

            if (isSame &&
                ((x >= (lastX - CLICK_PROXIMITY)) &&
                 (x <= (lastX + CLICK_PROXIMITY))) &&
                ((y >= (lastY - CLICK_PROXIMITY)) &&
                 (y <= (lastY + CLICK_PROXIMITY)))) {
                // send click
                id = java_awt_event_MouseEvent_MOUSE_CLICKED;
                popupTrigger = JNI_FALSE;
            } else return;
        } else return;
    }

    postMouseEvent (env,
                    thisObj,
                    id,
                    QtToolkitEventHandler::getCurrentTime(),
                    modifiers,
                    event->x(),
                    event->y(),
                    component->getClickCount(),
                    popupTrigger,
                    eventObj);
}

void
QtToolkitEventHandler::postKeyEvent (JNIEnv *env, 
                                     QtEventObject *eventObj, 
                                     QtComponentPeer *component)
{
    if ( eventObj == NULL ) {
        return;
    }

    jobject thisObj = component->getPeerGlobalRef();
    QKeyEvent *event = (QKeyEvent *)eventObj->event;

    jint javaCode;
    jint unicode;

    jint modifiers = getModifiers (event->state());

    if (event->key() != 0) {
        javaCode = awt_qt_getJavaKeyCode (event->key(), FALSE);
        unicode = awt_qt_getUnicodeChar (javaCode, modifiers);
    } else {
        // we need to emulate key press/release event here, getting
        // the information from the text
        unicode = (event->text()).at(0).unicode();
        javaCode = awt_qt_getJavaKeyCode(unicode, TRUE);
    }

    // 6228825: KEY_TYPED events are not properly generated for Buttons on zaurus.
    jboolean keyPressed = (event->type() == QEvent::KeyPress);

#ifdef QT_KEYPAD_MODE
    // OMAP Keypad modification:
    // Synthetically generating a key release event because
    // the OMAP Keypad driver currently only generates key press
    // events.
    QKeyEvent* ske = NULL;
    QObject* ssrc = NULL;
    if (keyPressed) {
        // Synthesizing a QEvent::KeyRelease QEvent....
        ske = new QKeyEvent(QEvent::KeyRelease,
                            event->key(),
                            event->ascii(),
                            event->state(),
                            event->text(),
                            event->isAutoRepeat(),
                            event->count());
        ssrc = eventObj->source;
    }
    // OMAP Keypad modification.
#endif /* QT_KEYPAD_MODE */

    env->CallVoidMethod (thisObj, QtCachedIDs.QtComponentPeer_postKeyEventMID,
                         (keyPressed
                          ? java_awt_event_KeyEvent_KEY_PRESSED
                          : java_awt_event_KeyEvent_KEY_RELEASED),
                         QtToolkitEventHandler::getCurrentTime(),
                         modifiers,
                         javaCode,
                         unicode,
                         (jint) eventObj);

    if (env->ExceptionCheck ())
        env->ExceptionDescribe ();

    if (keyPressed &&
        unicode != java_awt_event_KeyEvent_CHAR_UNDEFINED) {
        env->CallVoidMethod (thisObj, 
                             QtCachedIDs.QtComponentPeer_postKeyEventMID,
                             java_awt_event_KeyEvent_KEY_TYPED,
                             QtToolkitEventHandler::getCurrentTime(),
                             modifiers,
                             java_awt_event_KeyEvent_VK_UNDEFINED,
                             unicode,
                             (jint) NULL);
    }

#ifdef QT_KEYPAD_MODE
    // OMAP Keypad modification:
    // Synthetically generating a key release event because
    // the OMAP Keypad driver currently only generates key press
    // events.
    if (keyPressed && ske) {

        QtEventObject *synEvent = new QtEventObject();
        if ( synEvent != NULL ) {
            synEvent->event  = ske;
            synEvent->source = ssrc;
        }
        else {
            return;
        }

        // Posting synthetic key release event to the Java layer....
        env->CallVoidMethod (thisObj, QtCachedIDs.QtComponentPeer_postKeyEventMID,
                             java_awt_event_KeyEvent_KEY_RELEASED,
                             QtToolkitEventHandler::getCurrentTime(),
                             modifiers,
                             javaCode,
                             unicode,
                             (jint) synEvent);
    }
    // OMAP Keypad modification.
#endif /* QT_KEYPAD_MODE */

}
