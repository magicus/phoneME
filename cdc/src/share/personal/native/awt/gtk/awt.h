/*
 * @(#)awt.h	1.31 06/10/10
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

#ifndef _AWT_H_
#define _AWT_H_

#include "jni.h"
#include <stdlib.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include "ThreadLocking.h"

/* The JVM that loaded the GToolkit class. */

extern JavaVM *JVM;

/* JNI IDs of classes, methods and fields used by the native code. */

struct CachedIDs
{
	jclass java_awt_AWTErrorClass;
	jclass java_awt_ColorClass;
	jclass java_awt_DimensionClass;
	jclass java_awt_FontClass;
	jclass java_awt_image_BufferedImageClass;
	jclass java_awt_image_DirectColorModelClass;
	jclass java_awt_image_IndexColorModelClass;
	jclass java_awt_InsetsClass;
	jclass java_awt_PointClass;
	jclass java_awt_RectangleClass;
	jclass sun_awt_NullGraphicsClass;
	jclass GdkGraphicsClass;
	jclass GFontPeerClass;
	jclass NullPointerExceptionClass;
	jclass OutOfMemoryErrorClass;
	jclass ArrayIndexOutOfBoundsExceptionClass;
	jclass IllegalArgumentExceptionClass;
	jfieldID GComponentPeer_dataFID;
	jfieldID GComponentPeer_targetFID;
	jfieldID GComponentPeer_cursorFID;
/*	jfieldID GFontPeer_dataFID; */
	jfieldID GFramePeer_menuBarHeightFID;
	jfieldID GdkGraphics_dataFID;
	jfieldID GdkGraphics_peerFID;
	jfieldID GdkGraphics_originXFID;
	jfieldID GdkGraphics_originYFID;
	jfieldID GMenuBarPeer_dataFID;
	jfieldID GMenuItemPeer_dataFID;
	jfieldID GMenuItemPeer_targetFID;
	jfieldID GMenuItemPeer_menuBarFID;
	jfieldID GPlatformFont_ascentFID;
	jfieldID GPlatformFont_descentFID;
	jfieldID GScrollPanePeer_dataFID;
	jfieldID GWindowPeer_topBorderFID;
	jfieldID GWindowPeer_leftBorderFID;
	jfieldID GWindowPeer_bottomBorderFID;
	jfieldID GWindowPeer_rightBorderFID;
	jfieldID java_awt_event_InputEvent_modifiersFID;
	jfieldID java_awt_event_KeyEvent_modifiedFieldsFID;
	jfieldID java_awt_event_KeyEvent_keyCodeFID;
	jfieldID java_awt_event_KeyEvent_keyCharFID;
	jfieldID java_awt_event_MouseEvent_clickCountFID;
	jfieldID java_awt_AWTEvent_dataFID;
	jfieldID java_awt_Choice_selectedIndexFID;
	jfieldID java_awt_CheckboxMenuItem_stateFID;
	jfieldID java_awt_Color_valueFID;
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
	jfieldID java_awt_Component_fontFID;
	jfieldID java_awt_Cursor_typeFID;
	jfieldID java_awt_FileDialog_dirFID;
	jfieldID java_awt_FileDialog_fileFID;
	jfieldID java_awt_Font_peerFID;
	jfieldID java_awt_FontMetrics_fontFID;
        jfieldID java_awt_Menu_tearOffFID;
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
	jmethodID GButtonPeer_postActionEventMID;
	jmethodID GChoicePeer_postItemEventMID;
	jmethodID GCheckboxMenuItemPeer_postItemEventMID;
	jmethodID GCheckboxPeer_postItemEventMID;
	jmethodID GComponentPeer_canHavePixmapBackgroundMID;
	jmethodID GComponentPeer_getUTF8BytesMID;
	jmethodID GComponentPeer_postKeyEventMID;
	jmethodID GComponentPeer_postMouseEventMID;
	jmethodID GComponentPeer_postPaintEventMID;
	jmethodID GComponentPeer_drawMCStringMID;
	jmethodID GFontPeer_getFontMID;
	jmethodID GFramePeer_postWindowIconifiedMID;
	jmethodID GFramePeer_postWindowDeiconifiedMID;
	jmethodID GdkGraphics_constructWithPeerMID;
        jmethodID GtkClipboard_getStringContentsMID;
        jmethodID GtkClipboard_lostOwnershipMID;
        jmethodID GtkClipboard_updateContentsMID;
	jmethodID GListPeer_postItemEventMID;
	jmethodID GListPeer_postActionEventMID;  // added for Bug #4685270
	jmethodID GMenuItemPeer_postActionEventMID;
	jmethodID GScrollbarPeer_postAdjustmentEvent;
        jmethodID GScrollPanePeer_postAdjustableEventMID;
	jmethodID GTextComponentPeer_postTextEventMID;
	jmethodID GTextFieldPeer_postActionEventMID;
	jmethodID GWindowPeer_focusChangedMID;
	jmethodID GWindowPeer_postMovedEventMID;
	jmethodID GWindowPeer_postResizedEventMID;
	jmethodID GWindowPeer_postWindowEventMID;
	jmethodID GWindowPeer_setDefaultBorders;
	jmethodID GWindowPeer_updateInsetsMID;
	jmethodID java_awt_Color_constructorMID;
	jmethodID java_awt_Component_hideMID;
	jmethodID java_awt_Component_getBackgroundMID;
	jmethodID java_awt_Dimension_constructorMID;
	jmethodID java_awt_Font_contructorWithPeerMID;
	jmethodID java_awt_image_BufferedImage_constructorMID;
	jmethodID java_awt_image_ColorModel_getRGBMID;
	jmethodID java_awt_image_DirectColorModel_constructorMID;	/* (IIIII) */
	jmethodID java_awt_image_ImageConsumer_setColorModelMID;
	jmethodID java_awt_image_ImageConsumer_setHintsMID;
	jmethodID java_awt_image_ImageConsumer_setPixels2MID;
	jmethodID java_awt_image_ImageConsumer_setPixelsMID;
	jmethodID java_awt_image_IndexColorModel_constructorMID;	/* (II[B[B[B) */
	jmethodID java_awt_Insets_constructorMID;
	jmethodID java_awt_Point_constructorMID;
	jmethodID java_awt_Rectangle_constructorMID;
	jmethodID sun_awt_NullGraphics_constructorMID;
};

/* The global variable that hold all the cached IDs. */

extern struct CachedIDs GCachedIDs;

extern jobject awt_gtk_getColorModel (JNIEnv * env);

/* These macros provide convenience for initializing IDs in the GCachedIDs structure.
   I don't really like using the preprocessor for this kind of thing but I think it makes
   the init IDs methods much clearer and more consise. It also ensures that we always check
   for NULL in case the field/method doesn't exist and return. */

#if CVM_DEBUG

#define FIND_CLASS(name) if ((cls = (*env)->FindClass(env, name)) == NULL) return;
#define GET_CLASS(id,name) if ((cls = GCachedIDs.id = (*env)->NewGlobalRef(env, (*env)->FindClass(env, name))) == NULL) return;
#define GET_FIELD_ID(id,name,type) if ((GCachedIDs.id = (*env)->GetFieldID(env, cls, name, type)) == NULL) return;
#define GET_METHOD_ID(id,name,type) if ((GCachedIDs.id = (*env)->GetMethodID(env, cls, name, type)) == NULL) return;
#define GET_STATIC_METHOD_ID(id, name, type) if ((GCachedIDs.id = (*env)->GetStaticMethodID (env, cls, name, type)) == NULL) return;

#else

#define FIND_CLASS(name) cls = (*env)->FindClass(env, name);
#define GET_CLASS(id,name) cls = GCachedIDs.id = (*env)->NewGlobalRef(env, (*env)->FindClass(env, name));
#define GET_FIELD_ID(id,name,type) GCachedIDs.id = (*env)->GetFieldID(env, cls, name, type);
#define GET_METHOD_ID(id,name,type) GCachedIDs.id = (*env)->GetMethodID(env, cls, name, type);
#define GET_STATIC_METHOD_ID(id, name, type) GCachedIDs.id = (*env)->GetStaticMethodID (env, cls, name, type);

#endif

#endif
