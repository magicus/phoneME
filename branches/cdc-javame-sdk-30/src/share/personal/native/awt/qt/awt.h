/*
 * @(#)awt.h	1.23 06/10/25
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

#ifndef _AWT_H_
#define _AWT_H_

#include "jni.h"
#include <stdlib.h>

#include <qstring.h>

// Qt seems to be defining Q_WS_X11 correctly if we are natively building on Qt/SuSE.
// But just in case.
// If we support Qt/WIN32 in the future, this needs to be removed.
#ifndef QWS
#  ifndef Q_WS_X11
#    define Q_WS_X11
#  endif 
#endif

/* Utility function to return a unicode representation of the jstring */
QString *
awt_convertToQString(JNIEnv *env, jstring str);

jstring
awt_convertToJstring(JNIEnv *env, QString &str);

/* The JVM that loaded the QtToolkit class. */

extern JavaVM *JVM;

/* JNI IDs of classes, methods and fields used by the native code. */

struct CachedIDs
{
    jclass java_lang_ThreadClass;
    jclass java_awt_AWTErrorClass;
    jclass java_awt_ColorClass;
    jclass java_awt_DimensionClass;
    jclass java_awt_FontClass;
    jclass java_awt_image_BufferedImageClass;
    jclass java_awt_image_DirectColorModelClass;
    jclass java_awt_image_IndexColorModelClass;
    jclass java_awt_InsetsClass;
    jclass java_awt_LabelClass;
    jclass java_awt_KeyboardFocusManagerClass;
    jclass java_awt_MenuClass;
    jclass java_awt_PointClass;
    jclass java_awt_RectangleClass;
    jclass java_awt_ToolkitClass;
    jclass QtGraphicsClass;
    jclass QtImageRepresentationClass;
    jclass QtFontPeerClass;
    jclass QtWindowPeerClass;
    jclass QtComponentPeerClass;
    jclass QtImageDecoderClass;
    jclass NullPointerExceptionClass;
    jclass OutOfMemoryErrorClass;
    jclass ArrayIndexOutOfBoundsExceptionClass;
    jclass IllegalArgumentExceptionClass;
    jclass WindowEventClass;
    jclass FocusEventClass;
    jclass NoSuchMethodErrorClass;
    jfieldID QtComponentPeer_dataFID;
    jfieldID QtComponentPeer_targetFID;
    jfieldID QtComponentPeer_cursorFID;
    jfieldID QtFontPeer_ascentFID;
    jfieldID QtFontPeer_descentFID;
    jfieldID QtFontPeer_leadingFID;
    jfieldID QtFontPeer_dataFID;
    jfieldID QtFontPeer_strikethroughFID;
    jfieldID QtFontPeer_underlineFID;
    jfieldID QtGraphics_dataFID;
    jfieldID QtGraphics_imageFID;
    jfieldID QtGraphics_peerFID;
    jfieldID QtGraphics_originXFID;
    jfieldID QtGraphics_originYFID;
    jfieldID QtImageRepresentation_finishCalledFID;
    jfieldID QtImageRepresentation_drawSucceededFID;
    jfieldID QtMenuBarPeer_dataFID;
    jfieldID QtMenuItemPeer_dataFID;
    jfieldID QtMenuItemPeer_targetFID;
    jfieldID QtMenuItemPeer_menuBarFID;
    jfieldID QtWindowPeer_topBorderFID;
    jfieldID QtWindowPeer_leftBorderFID;
    jfieldID QtWindowPeer_bottomBorderFID;
    jfieldID QtWindowPeer_rightBorderFID;
    jfieldID QtWindowPeer_qwsInitFID;
    jfieldID QtWindowPeer_qwsXFID;
    jfieldID QtWindowPeer_qwsYFID;
    jfieldID java_awt_event_InputEvent_modifiersFID;
    jfieldID java_awt_event_KeyEvent_modifiedFieldsFID;
    jfieldID java_awt_event_KeyEvent_keyCodeFID;
    jfieldID java_awt_event_KeyEvent_keyCharFID;
    jfieldID java_awt_event_MouseEvent_clickCountFID;
    jfieldID java_awt_AWTEvent_dataFID;
    jfieldID java_awt_Adjustable_horizontalFID;
    jfieldID java_awt_Adjustable_verticalFID;
    jfieldID java_awt_Choice_selectedIndexFID;
    jfieldID java_awt_CheckboxMenuItem_stateFID;
    jfieldID java_awt_Component_eventMaskFID;
    jfieldID java_awt_Component_heightFID;
    jfieldID java_awt_Component_mouseListenerFID;
    jfieldID java_awt_Component_mouseMotionListenerFID;
    jfieldID java_awt_Component_peerFID;
    jfieldID java_awt_Component_widthFID;
    jfieldID java_awt_Component_xFID;
    jfieldID java_awt_Component_yFID;
    jfieldID java_awt_Component_backgroundFID;
    jfieldID java_awt_Component_foregroundFID;
    jfieldID java_awt_Component_cursorFID;
    jfieldID java_awt_Dialog_modalFID;
    jfieldID java_awt_Component_fontFID;
    jfieldID java_awt_Component_isPackedFID;
    jfieldID java_awt_Cursor_typeFID;
    jfieldID java_awt_FileDialog_dirFID;
    jfieldID java_awt_FileDialog_fileFID;
    jfieldID java_awt_Font_peerFID;
    jfieldID java_awt_FontMetrics_fontFID;
    jfieldID java_awt_MenuComponent_peerFID;
    jfieldID java_awt_MenuItem_shortcutFID;
    jfieldID java_awt_MenuShortcut_keyFID;
    jfieldID java_awt_MenuShortcut_usesShiftFID;
    jfieldID java_awt_Rectangle_heightFID;
    jfieldID java_awt_Rectangle_widthFID;
    jfieldID java_awt_Rectangle_xFID;
    jfieldID java_awt_Rectangle_yFID;
    jfieldID java_awt_Scrollbar_valueFID;
    jfieldID java_awt_ScrollPane_scrollbarDisplayPolicyFID;
    jfieldID java_awt_Window_warningStringFID;   //6233632
    jfieldID java_awt_image_BufferedImage_peerFID;    
    jfieldID java_awt_image_DirectColorModel_red_maskFID;
    jfieldID java_awt_image_DirectColorModel_red_offsetFID;
    jfieldID java_awt_image_DirectColorModel_red_scaleFID;
    jfieldID java_awt_image_DirectColorModel_green_maskFID;
    jfieldID java_awt_image_DirectColorModel_green_offsetFID;
    jfieldID java_awt_image_DirectColorModel_green_scaleFID;
    jfieldID java_awt_image_DirectColorModel_blue_maskFID;
    jfieldID java_awt_image_DirectColorModel_blue_offsetFID;
    jfieldID java_awt_image_DirectColorModel_blue_scaleFID;
    jfieldID java_awt_image_DirectColorModel_alpha_maskFID;
    jfieldID java_awt_image_DirectColorModel_alpha_offsetFID;
    jfieldID java_awt_image_DirectColorModel_alpha_scaleFID;
    jfieldID sun_awt_image_ImageRepresentation_availinfoFID;
    jfieldID sun_awt_image_ImageRepresentation_heightFID;
    jfieldID sun_awt_image_ImageRepresentation_hintsFID;
    jfieldID sun_awt_image_ImageRepresentation_newbitsFID;
    jfieldID sun_awt_image_ImageRepresentation_offscreenFID;
    jfieldID sun_awt_image_ImageRepresentation_pDataFID;
    jfieldID sun_awt_image_ImageRepresentation_widthFID;
    jfieldID java_awt_image_IndexColorModel_rgbFID;
    jfieldID sun_awt_image_OffScreenImageSource_baseIRFID;
    jfieldID sun_awt_image_OffScreenImageSource_theConsumerFID;
    jfieldID QtImageDecoder_widthFID;
    jfieldID QtImageDecoder_heightFID;
    jfieldID QtImageDecoder_colorModelFID;
    jfieldID QtImageDecoder_RGB24DCMFID;
    jfieldID QtImageDecoder_RGB32DCMFID;
    jmethodID QtButtonPeer_postActionEventMID;
    jmethodID QtChoicePeer_postItemEventMID;
    jmethodID QtCheckboxMenuItemPeer_postItemEventMID;
    jmethodID QtCheckboxPeer_postItemEventMID;
    jmethodID QtComponentPeer_canHavePixmapBackgroundMID;
    jmethodID QtComponentPeer_getUTF8BytesMID;
    jmethodID QtComponentPeer_postEventMID;
    jmethodID QtComponentPeer_postFocusEventMID;
    jmethodID QtComponentPeer_postKeyEventMID;
    jmethodID QtComponentPeer_postMouseEventMID;
    jmethodID QtComponentPeer_postPaintEventMID;
    jmethodID QtFontPeer_getFontMID;
    jmethodID QtFramePeer_updateMenuBarHeightMID;
    jmethodID QtGraphics_constructWithPeerMID;
    jmethodID QtClipboard_lostOwnershipMID;
    jmethodID QtClipboard_updateContentsMID;
    jmethodID QtListPeer_postItemEventMID;
    jmethodID QtMenuItemPeer_postActionEventMID;
    jmethodID QtScrollbarPeer_postAdjustmentEvent;
    jmethodID QtScrollPanePeer_postAdjustmentEventMID;
    jmethodID QtTextComponentPeer_postTextEventMID;
    jmethodID QtTextFieldPeer_postActionEventMID;
    jmethodID QtWindowPeer_postFocusChangeEventsMID;
    jmethodID QtWindowPeer_postMovedEventMID;
    jmethodID QtWindowPeer_postResizedEventMID;
    jmethodID QtWindowPeer_postShownEventMID;
    jmethodID QtWindowPeer_postWindowEventMID;
    jmethodID QtWindowPeer_setInsetsMID;
    jmethodID QtWindowPeer_updateInsetsMID;
    jmethodID QtWindowPeer_setQWSCoordsMID;
    jmethodID QtWindowPeer_hideLaterMID;
    jmethodID QtWindowPeer_restoreBoundsMID;
    jmethodID QtWindowPeer_notifyShownMID;
    jmethodID java_awt_event_WindowEvent_constructorMID;
    jmethodID java_lang_Thread_yieldMID;
    jmethodID java_lang_Thread_sleepMID;
    jmethodID java_awt_Color_constructorMID;
    jmethodID java_awt_Color_getRGBMID;
    jmethodID java_awt_Component_getBackgroundMID;
    jmethodID java_awt_Dialog_hideMID;
    jmethodID java_awt_Dimension_constructorMID;
    jmethodID java_awt_KeyboardFocusManager_shouldNativelyFocusHeavyweightMID;
    jmethodID java_awt_KeyboardFocusManager_markClearGlobalFocusOwnerMID;
    jmethodID java_awt_event_FocusEvent_constructorMID;
    jmethodID java_awt_Font_contructorWithPeerMID;
    jmethodID java_awt_Toolkit_getDefaultToolkitMID;
    jmethodID java_awt_Toolkit_getColorModelMID;
    jmethodID java_awt_image_BufferedImage_constructorMID;
    jmethodID java_awt_image_ColorModel_getRGBMID;
    jmethodID java_awt_image_DirectColorModel_constructorMID; /* (IIIII) */
    jmethodID java_awt_image_ImageConsumer_setColorModelMID;
    jmethodID java_awt_image_ImageConsumer_setHintsMID;
    jmethodID java_awt_image_ImageConsumer_setPixels2MID;
    jmethodID java_awt_image_ImageConsumer_setPixelsMID;
    jmethodID java_awt_image_IndexColorModel_constructorMID; /* (II[B[B[B) */
    jmethodID java_awt_image_IndexColorModel_constructor2MID; /* (II[BIZ) */
    jmethodID java_awt_Insets_constructorMID;
    jmethodID java_awt_Label_getTextMID;
    jmethodID java_awt_Menu_isTearOffMID;
    jmethodID java_awt_Point_constructorMID;
    jmethodID java_awt_Rectangle_constructorMID; 
    jmethodID java_awt_TextArea_getScrollbarVisibilityMID;
    jmethodID sun_awt_image_Image_getImageRepMID;
};

/* The global variable that hold all the cached IDs. */

extern struct CachedIDs QtCachedIDs;

extern jobject awt_qt_getColorModel (JNIEnv *env);

/* These macros provide convenience for initializing IDs in the QtCachedIDs structure.
   I don't really like using the preprocessor for this kind of thing but I think it makes
   the init IDs methods much clearer and more consise. It also ensures that we always check
   for NULL in case the field/method doesn't exist and return. */

#ifndef __cplusplus

#define GET_FIELD_ID(id, name, type) if ((QtCachedIDs.id = (*env)->GetFieldID (env, cls, name, type)) == NULL) return;
#define GET_STATIC_FIELD_ID(id, name, type) if ((QtCachedIDs.id = (*env)->GetStaticFieldID(env, cls, name, type)) == NULL) return;
#define GET_METHOD_ID(id, name, type) if ((QtCachedIDs.id = (*env)->GetMethodID (env, cls, name, type)) == NULL) return;
#define GET_STATIC_METHOD_ID(id, name, type) if ((QtCachedIDs.id = (*env)->GetStaticMethodID (env, cls, name, type)) == NULL) return;

#else

#define GET_FIELD_ID(id, name, type) if ((QtCachedIDs.id = env->GetFieldID (cls, name, type)) == NULL) return;
#define GET_STATIC_FIELD_ID(id, name, type) if ((QtCachedIDs.id = env->GetStaticFieldID(cls, name, type)) == NULL) return;
#define GET_METHOD_ID(id, name, type) if ((QtCachedIDs.id = env->GetMethodID (cls, name, type)) == NULL) return;
#define GET_STATIC_METHOD_ID(id, name, type) if ((QtCachedIDs.id = env->GetStaticMethodID (cls, name, type)) == NULL) return;

#endif  

#include <sys/time.h>
#include <time.h>
extern struct timeval  awt_stime , awt_etime ;

/**
 * Start the stop watch
 */
#define SWATCH_START  (gettimeofday(&awt_stime,NULL))

/**
 * Stop the stop watch.
 *
 * @param _mesg Any string that should be displayed before the total time 
 *        taken
 */
#define SWATCH_END(_mesg)   {\
    gettimeofday(&awt_etime,NULL); \
    printf(_mesg " :%ld ms\n", \
           (((awt_etime.tv_sec * 1000000) + awt_etime.tv_usec)- \
            ((awt_stime.tv_sec * 1000000) + awt_stime.tv_usec))/1000) ; \
}

#endif
