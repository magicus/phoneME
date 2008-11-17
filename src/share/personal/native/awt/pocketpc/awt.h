/*
 * /* @(#)awt.h	1.74 03/02/19
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
#include "ThreadLocking.h"

/* The JVM that loaded the PPCToolkit class. */

extern JavaVM *JVM;

/* JNI IDs of classes, methods and fields used by the native code. */

struct CachedIDs
{
    jclass      InternalErrorClass;
    jclass	NullPointerExceptionClass;
    jclass      ArrayIndexOutOfBoundsExceptionClass;
    jclass	OutOfMemoryErrorClass;
    jclass      PPCObjectPeerClass;
    jclass	PPCFontPeerClass;
    jclass      PPCFontMetricsClass;
    jclass      java_awt_ToolkitClass;
    jclass      java_awt_AWTExceptionClass;
    jclass      java_awt_ChoiceClass;
    jclass      java_awt_ColorClass;
    jclass	java_awt_DialogClass;
    jclass      java_awt_FrameClass;
    jclass      java_awt_ListClass;
    jclass	java_awt_PointClass;
    jclass      java_awt_TextAreaClass;
    jclass      java_awt_RectangleClass;
    jclass      java_lang_SystemClass;
    jclass      java_lang_ThreadClass;
    jclass      java_awt_event_ComponentEventClass;
    jclass      java_awt_event_FocusEventClass;
    jclass      java_awt_event_KeyEventClass;
    jclass      java_awt_event_MouseEventClass;
    jclass      java_awt_event_WindowEventClass;
    jclass      java_awt_image_ColorModelClass;
    jclass	java_awt_image_DirectColorModelClass;
    jclass	java_awt_image_IndexColorModelClass;
    jclass      java_awt_image_BufferedImageClass;
    jfieldID    PPCFileDialogPeer_parentFID;
    jfieldID    PPCFontMetrics_ascentFID;
    jfieldID    PPCFontMetrics_descentFID;
    jfieldID    PPCFontMetrics_leadingFID;
    jfieldID    PPCFontMetrics_heightFID;
    jfieldID    PPCFontMetrics_maxAscentFID;
    jfieldID    PPCFontMetrics_maxDescentFID;
    jfieldID    PPCFontMetrics_maxHeightFID;
    jfieldID    PPCFontMetrics_maxAdvanceFID;
    jfieldID    PPCFontMetrics_widthsFID;
    jfieldID    PPCToolkit_dbgTraceFID;
    jfieldID	PPCToolkit_dbgVerifyFID;
    jfieldID	PPCToolkit_dbgBreakFID;
    jfieldID	PPCToolkit_dbgHeapCheckFID;
    jfieldID	PPCToolkit_frameWidthFID;
    jfieldID	PPCToolkit_frameHeightFID;
    jfieldID	PPCToolkit_vsbMinHeightFID;
    jfieldID	PPCToolkit_vsbWidthFID;
    jfieldID	PPCToolkit_hsbMinWidthFID;
    jfieldID	PPCToolkit_hsbHeightFID;
    jfieldID    PPCGraphics_foregroundFID;
    jfieldID    PPCGraphics_imageFID;
    jfieldID	PPCGraphics_originXFID;
    jfieldID	PPCGraphics_originYFID;
    jfieldID    PPCGraphics_pDataFID;
    jfieldID    PPCObjectPeer_pDataFID;
    jfieldID    PPCObjectPeer_targetFID;
    jfieldID    PPCPanelPeer_insets_FID;
    jfieldID	PPCMenuItemPeer_menuBarFID;
    jfieldID    PPCMenuItemPeer_isCheckboxFID;
    jfieldID    PPCMenuItemPeer_shortcutLabelFID;
    jfieldID	PPCMenuBarPeer_dataFID;
    jfieldID    java_awt_AWTEvent_dataFID;
    jfieldID    java_awt_Component_widthFID;
    jfieldID    java_awt_Component_heightFID;
    jfieldID    java_awt_Component_xFID;
    jfieldID    java_awt_Component_yFID;
    jfieldID    java_awt_Dimension_widthFID;
    jfieldID    java_awt_Dimension_heightFID;
    jfieldID	java_awt_Insets_topFID;
    jfieldID	java_awt_Insets_bottomFID;
    jfieldID	java_awt_Insets_leftFID;
    jfieldID	java_awt_Insets_rightFID;
    jfieldID	java_awt_Font_pDataFID;
    jfieldID    java_awt_Font_peerFID;
    jfieldID	java_awt_FontMetrics_fontFID;
    jfieldID	java_awt_ScrollPane_scrollbarDisplayPolicyFID;
    jfieldID    java_awt_Event_xFID;
    jfieldID    java_awt_Event_yFID;
    jfieldID    java_awt_Event_targetFID;
    jfieldID	java_awt_Rectangle_heightFID;
    jfieldID	java_awt_Rectangle_widthFID;
    jfieldID	java_awt_Rectangle_xFID;
    jfieldID	java_awt_Rectangle_yFID;
    jfieldID    java_awt_image_ColorModel_pDataFID;
    jfieldID	java_awt_image_IndexColorModel_rgbFID;
    jfieldID    java_awt_datatransfer_StringSelection_dataFID;
    jfieldID    java_lang_String_offsetFID;
    jfieldID    java_awt_image_DirectColorModel_red_maskFID;
    jfieldID    java_awt_image_DirectColorModel_red_offsetFID;
    jfieldID    java_awt_image_DirectColorModel_red_scaleFID;
    jfieldID    java_awt_image_DirectColorModel_green_maskFID;
    jfieldID    java_awt_image_DirectColorModel_green_offsetFID;
    jfieldID    java_awt_image_DirectColorModel_green_scaleFID;
    jfieldID    java_awt_image_DirectColorModel_blue_maskFID;
    jfieldID    java_awt_image_DirectColorModel_blue_offsetFID;
    jfieldID    java_awt_image_DirectColorModel_blue_scaleFID;
    jfieldID    java_awt_image_DirectColorModel_alpha_maskFID;
    jfieldID    java_awt_image_DirectColorModel_alpha_offsetFID;
    jfieldID    java_awt_image_DirectColorModel_alpha_scaleFID;
    jfieldID    java_awt_image_BufferedImage_peerFID;    
    jfieldID    sun_awt_CharsetString_fontDescriptorFID;
    jfieldID    sun_awt_CharsetString_charsetCharsFID;
    jfieldID    sun_awt_CharsetString_lengthFID;
    jfieldID    sun_awt_CharsetString_offsetFID;
    jfieldID    sun_awt_FontDescriptor_nativeNameFID;
    jfieldID    sun_awt_FontDescriptor_fontCharsetFID;
    jfieldID    sun_awt_PlatformFont_componentFontsFID;
    jfieldID    sun_awt_PlatformFont_propsFID;
    jfieldID	sun_awt_image_ImageRepresentation_pDataFID;
    jfieldID	sun_awt_image_ImageRepresentation_availinfoFID;
    jfieldID	sun_awt_image_ImageRepresentation_heightFID;
    jfieldID	sun_awt_image_ImageRepresentation_widthFID;
    jfieldID	sun_awt_image_ImageRepresentation_hintsFID;
    jfieldID	sun_awt_image_ImageRepresentation_newbitsFID;
    jfieldID	sun_awt_image_OffScreenImageSource_baseIRFID;
    jfieldID	sun_awt_image_OffScreenImageSource_theConsumerFID;
    jmethodID	PPCButtonPeer_handleActionMID;
    jmethodID   PPCClipboard_lostSelectionOwnershipMID;
    jmethodID	PPCCheckboxPeer_handleActionMID;
    jmethodID   PPCCheckboxMenuItemPeer_handleActionMID;
    jmethodID   PPCChoicePeer_handleActionMID;
    jmethodID   PPCComponentPeer_handlePaintMID;
    jmethodID   PPCComponentPeer_postEventMID;
    jmethodID	PPCComponentPeer_setBackgroundMID;
    jmethodID	PPCComponentPeer_handleExposeMID;
    jmethodID   PPCDialogPeer_setDefaultColorMID;
    jmethodID   PPCFileDialogPeer_handleSelectedMID;
    jmethodID   PPCFileDialogPeer_handleCancelMID;
    jmethodID   PPCFontMetrics_getHeightMID;
    jmethodID   PPCFontMetrics_getFontMetricsMID;
    jmethodID	PPCFontMetrics_getWidthsMID;
    jmethodID   PPCListPeer_handleActionMID;
    jmethodID   PPCListPeer_handleListChangedMID;
    jmethodID   PPCListPeer_preferredSizeMID;
    jmethodID	PPCMenuItemPeer_postActionEventMID;
    jmethodID   PPCMenuItemPeer_getDefaultFontMID;
    jmethodID   PPCMenuItemPeer_handleActionMID;
    jmethodID   PPCObjectPeer_getPeerForTargetMID;
    jmethodID   PPCScrollPanePeer_scrolledHorizontalMID;
    jmethodID   PPCScrollPanePeer_scrolledVerticalMID;
    jmethodID   PPCScrollPanePeer_getScrollChildMID;
    jmethodID   PPCTextAreaPeer_replaceText;
    jmethodID   PPCTextFieldPeer_handleActionMID;
    jmethodID   PPCTextComponentPeer_valueChangedMID;
    jmethodID   PPCWindowPeer_postFocusOnActivateMID;
    jmethodID   java_awt_AWTEvent_getIDMID;
    jmethodID   java_awt_AWTEvent_isConsumedMID;
    jmethodID   java_awt_Adjustable_getBlockIncrementMID;
    jmethodID   java_awt_Adjustable_setBlockIncrementMID;
    jmethodID   java_awt_Adjustable_getUnitIncrementMID;
    jmethodID	java_awt_Button_getLabelMID;
    jmethodID	java_awt_Checkbox_getLabelMID;
    jmethodID	java_awt_Checkbox_getStateMID;
    jmethodID	java_awt_Checkbox_getCheckboxGroupMID;
    jmethodID   java_awt_CheckboxMenuItem_getStateMID;
    jmethodID   java_awt_Color_constructorMID;
    jmethodID	java_awt_Color_getRGBMID;
    jmethodID	java_awt_image_ColorModel_getRGBMID;
    jmethodID	java_awt_Component_getHeightMID;
    jmethodID	java_awt_Component_getWidthMID;
    jmethodID	java_awt_Component_getXMID;
    jmethodID	java_awt_Component_getYMID;
    jmethodID	java_awt_Component_getBackgroundMID;
    jmethodID	java_awt_Component_getForegroundMID;
    jmethodID	java_awt_Component_getFontMID;
    jmethodID   java_awt_Component_getToolkitMID;
    jmethodID	java_awt_Component_isEnabledMID;
    jmethodID	java_awt_Component_isVisibleMID;
    jmethodID   java_awt_Component_getFontMetricsMID;
    jmethodID   java_awt_Component_getPreferredSizeMID;
    jmethodID   java_awt_Component_setSizeMID;
    jmethodID   java_awt_Container_getComponentCountMID;
    jmethodID   java_awt_Container_getComponentsMID;
    jmethodID   java_awt_Choice_getItemMID;
#ifdef DEBUG
    jmethodID   java_awt_Choice_getItemCountMID;
    jmethodID   java_awt_Choice_getSelectedIndexMID;
#endif
    jmethodID   java_awt_Dialog_getTitleMID;
    jmethodID   java_awt_FileDialog_getDirectoryMID;
    jmethodID   java_awt_FileDialog_getFileMID;
    jmethodID   java_awt_FileDialog_getModeMID;
    jmethodID   java_awt_Font_getStyleMID;
    jmethodID   java_awt_Font_getSizeMID;
    jmethodID   java_awt_Font_getNameMID;
    jmethodID   java_awt_FontMetrics_getHeightMID;
    jmethodID   java_awt_Label_getAlignmentMID;
    jmethodID   java_awt_Label_getTextMID;
    jmethodID   java_awt_List_getItemMID;
    jmethodID   java_awt_MenuItem_isEnabledMID;
    jmethodID   java_awt_MenuComponent_getFontMID;
    jmethodID   java_awt_MenuBar_countMenusMID;
    jmethodID   java_awt_MenuBar_getMenuMID;
    jmethodID   java_awt_MenuItem_getLabelMID;
    jmethodID   java_awt_Menu_countItemsMID;
    jmethodID   java_awt_Menu_getItemMID;
    jmethodID	java_awt_Point_constructorMID;
    jmethodID   java_awt_Rectangle_constructorMID;
    jmethodID   java_awt_Scrollbar_getOrientationMID;
    jmethodID   java_awt_Scrollbar_getLineIncrementMID;
    jmethodID   java_awt_Scrollbar_getPageIncrementMID;
    jmethodID   java_awt_ScrollPane_gethAdjustableMID;
    jmethodID   java_awt_ScrollPane_getvAdjustableMID;
    jmethodID	java_awt_Window_getWarningStringMID;
    jmethodID   java_awt_event_ComponentEvent_constructorMID;
    jmethodID   java_awt_event_FocusEvent_constructorMID;
    jmethodID	java_awt_event_InputEvent_getModifiersMID;
    jmethodID   java_awt_event_KeyEvent_constructorMID;
    jmethodID	java_awt_event_KeyEvent_getKeyCodeMID;
    jmethodID	java_awt_event_KeyEvent_getKeyCharMID; 
    jmethodID   java_awt_event_MouseEvent_constructorMID;
    jmethodID   java_awt_event_MouseEvent_getXMID;
    jmethodID   java_awt_event_MouseEvent_getYMID;
    jmethodID   java_awt_event_WindowEvent_constructorMID;
    jmethodID   java_awt_TextArea_getScrollbarVisibilityMID;
    jmethodID   java_awt_Toolkit_getFontMetricsMID;
    jmethodID   java_awt_Toolkit_getColorModelMID;
    jmethodID   java_awt_Toolkit_getDefaultToolkitMID;
    jmethodID	java_awt_image_DirectColorModel_constructorMID;
    jmethodID	java_awt_image_IndexColorModel_constructorMID;
    jmethodID	java_awt_image_ImageConsumer_setColorModelMID;
    jmethodID	java_awt_image_ImageConsumer_setHintsMID;
    jmethodID	java_awt_image_ImageConsumer_setPixelsMID;
    jmethodID   java_awt_image_BufferedImage_constructorMID;
    jmethodID   java_lang_System_gcMID;
    jmethodID   java_lang_System_runFinalizationMID;
    jmethodID   java_lang_Thread_dumpStackMID;
    jmethodID   sun_awt_EmbeddedFrame_isCursorAllowedMID;
    jmethodID   sun_awt_image_Image_getBackgroundMID; 
    jmethodID   sun_awt_PlatformFont_makeMultiCharsetStringMID;   
   /*
    jclass	java_awt_AWTErrorClass;
    jclass 	java_awt_ColorClass;
    jclass	java_awt_DimensionClass;
    jclass	java_awt_FontClass;
    jclass	java_awt_image_DirectColorModelClass;
    jclass	java_awt_image_IndexColorModelClass;
    jclass	java_awt_InsetsClass;
    jclass	java_awt_RectangleClass;
    jclass	PPCGraphicsClass;
    jclass	PPCFontPeerClass;
    jclass	ArrayIndexOutOfBoundsExceptionClass;
    jclass	IllegalArgumentExceptionClass;
    jfieldID	PPCFontPeer_ascentFID;
    jfieldID	PPCFontPeer_descentFID;
    jfieldID	PPCFontPeer_dataFID;
    jfieldID	PPCFramePeer_menuBarHeightFID;
    jfieldID	PPCGraphics_dataFID;
    jfieldID	PPCGraphics_peerFID;
    jfieldID	PPCMenuBarPeer_dataFID;
    jfieldID	PPCMenuItemPeer_menuBarFID;
    jfieldID	PPCWindowPeer_topBorderFID;
    jfieldID	PPCWindowPeer_leftBorderFID;
    jfieldID	PPCWindowPeer_bottomBorderFID;
    jfieldID	PPCWindowPeer_rightBorderFID;
    jfieldID	java_awt_Choice_selectedIndexFID;
    jfieldID	java_awt_CheckboxMenuItem_stateFID;
    jfieldID	java_awt_Component_eventMaskFID;
    jfieldID	java_awt_Component_mouseListenerFID;
    jfieldID	java_awt_Component_mouseMotionListenerFID;
    jfieldID	java_awt_Component_peerFID;
    jfieldID	java_awt_Component_backgroundFID;
    jfieldID	java_awt_Component_foregroundFID;
    jfieldID	java_awt_Component_cursorFID;
    jfieldID	java_awt_Component_fontFID;
    jfieldID	java_awt_Cursor_typeFID;
    jfieldID	java_awt_FileDialog_dirFID;
    jfieldID	java_awt_FileDialog_fileFID;
    jfieldID	java_awt_FontMetrics_fontFID;
    jfieldID	java_awt_MenuComponent_peerFID;
    jfieldID	java_awt_MenuItem_shortcutFID;
    jfieldID	java_awt_MenuShortcut_keyFID;
    jfieldID	java_awt_MenuShortcut_usesShiftFID;
    jfieldID	java_awt_Rectangle_heightFID;
    jfieldID	java_awt_Rectangle_widthFID;
    jfieldID	java_awt_Rectangle_xFID;
    jfieldID	java_awt_Rectangle_yFID;
    jfieldID	java_awt_Scrollbar_valueFID;
    jfieldID	java_awt_image_DirectColorModel_red_maskFID;
    jfieldID	java_awt_image_DirectColorModel_red_offsetFID;
    jfieldID	java_awt_image_DirectColorModel_red_scaleFID;
    jfieldID	java_awt_image_DirectColorModel_green_maskFID;
    jfieldID	java_awt_image_DirectColorModel_green_offsetFID;
    jfieldID	java_awt_image_DirectColorModel_green_scaleFID;
    jfieldID	java_awt_image_DirectColorModel_blue_maskFID;
    jfieldID	java_awt_image_DirectColorModel_blue_offsetFID;
    jfieldID	java_awt_image_DirectColorModel_blue_scaleFID;
    jfieldID	java_awt_image_DirectColorModel_alpha_maskFID;
    jfieldID	java_awt_image_DirectColorModel_alpha_offsetFID;
    jfieldID	java_awt_image_DirectColorModel_alpha_scaleFID;
    jfieldID	sun_awt_image_ImageRepresentation_availinfoFID;
    jfieldID	sun_awt_image_ImageRepresentation_heightFID;
    jfieldID	sun_awt_image_ImageRepresentation_hintsFID;
    jfieldID	sun_awt_image_ImageRepresentation_newbitsFID;
    jfieldID	sun_awt_image_ImageRepresentation_offscreenFID;
    jfieldID	sun_awt_image_ImageRepresentation_pDataFID;
    jfieldID	sun_awt_image_ImageRepresentation_widthFID;
    jfieldID	java_awt_image_IndexColorModel_rgbFID;
    jfieldID	sun_awt_image_OffScreenImageSource_baseIRFID;
    jfieldID	sun_awt_image_OffScreenImageSource_theConsumerFID;
    jmethodID	PPCChoicePeer_postItemEventMID;
    jmethodID	PPCCheckboxMenuItemPeer_postItemEventMID;
    jmethodID	PPCCheckboxPeer_postItemEventMID;
    jmethodID	PPCComponentPeer_canHavePixmapBackgroundMID;
    jmethodID	PPCComponentPeer_postKeyEventMID;
    jmethodID	PPCComponentPeer_postMouseEventMID;
    jmethodID	PPCComponentPeer_postPaintEventMID;
    jmethodID	PPCDialogPeer_wakeUpModalThreadMID;
    jmethodID	PPCFontPeer_getFontMID;
    jmethodID	PPCGraphics_constructWithPeerMID;
    jmethodID	PPCListPeer_postItemEventMID;
    jmethodID   PPCMenuBarPeer_countMenusMID;
    jmethodID   PPCMenuBarPeer_getMenuMID;
    jmethodID	PPCMenuItemPeer_postActionEventMID;
    jmethodID   PPCMenuItemPeer_countItemsMID;
    jmethodID   PPCMenuItemPeer_getMenuMID;
    jmethodID	PPCScrollbarPeer_postAdjustmentEvent;
    jmethodID	PPCTextComponentPeer_postTextEventMID;
    jmethodID	PPCTextFieldPeer_postActionEventMID;
    jmethodID	PPCWindowPeer_postFocusChangeEventsMID;
    jmethodID 	PPCWindowPeer_postMovedEventMID;
    jmethodID 	PPCWindowPeer_postResizedEventMID;
    jmethodID	PPCWindowPeer_postWindowEventMID;
    jmethodID	PPCWindowPeer_setDefaultBorders;
    jmethodID	PPCWindowPeer_updateInsetsMID;
    jmethodID	java_awt_Color_constructorMID;
    jmethodID	java_awt_Component_getFontMID;
    jmethodID   java_awt_Cursor_getTypeMID;
    jmethodID	java_awt_Dimension_constructorMID;
    jmethodID	java_awt_Font_contructorWithPeerMID;
    jmethodID   java_awt_MenuItem_getLabelMID;
    jmethodID	java_awt_image_ColorModel_getRGBMID;
    jmethodID	java_awt_image_DirectColorModel_constructorMID;
    jmethodID	java_awt_image_ImageConsumer_setColorModelMID;
    jmethodID	java_awt_image_ImageConsumer_setHintsMID;
    jmethodID	java_awt_image_ImageConsumer_setPixels2MID;
    jmethodID	java_awt_image_ImageConsumer_setPixelsMID;
    jmethodID	java_awt_image_IndexColorModel_constructorMID;
    jmethodID	java_awt_Insets_constructorMID;
    jmethodID	java_awt_Rectangle_constructorMID;
    */
};

/* The global variable that hold all the cached IDs. */

extern struct CachedIDs WCachedIDs;

extern jobject awt_pocketpc_getColorModel (JNIEnv *env);

/* These macros provide convenience for initializing IDs in the CachedIDs 
   structure. I don't really like using the preprocessor for this kind of 
   thing but I think it makes the init IDs methods much clearer and more 
   consise. It also ensures that we always check for NULL in case the 
   field/method doesn't exist and return. */

#ifndef __cplusplus

#define GET_FIELD_ID(id, name, type) if ((WCachedIDs.id = (*env)->GetFieldID (env, cls, name, type)) == NULL) return;
#define GET_STATIC_FIELD_ID(id, name, type) if ((WCachedIDs.id = (*env)->GetStaticFieldID(env, cls, name, type)) == NULL) return;
#define GET_METHOD_ID(id, name, type) if ((WCachedIDs.id = (*env)->GetMethodID (env, cls, name, type)) == NULL) return;
#define GET_STATIC_METHOD_ID(id, name, type) if ((WCachedIDs.id = (*env)->GetStaticMethodID (env, cls, name, type)) == NULL) return;

#else

#define GET_FIELD_ID(id, name, type) if ((WCachedIDs.id = env->GetFieldID (cls, name, type)) == NULL) return;
#define GET_STATIC_FIELD_ID(id, name, type) if ((WCachedIDs.id = env->GetStaticFieldID(cls, name, type)) == NULL) return;
#define GET_METHOD_ID(id, name, type) if ((WCachedIDs.id = env->GetMethodID (cls, name, type)) == NULL) return;
#define GET_STATIC_METHOD_ID(id, name, type) if ((WCachedIDs.id = env->GetStaticMethodID (cls, name, type)) == NULL) return;

#endif  

#include "stdhdrs.h"

#undef ASSERT
#undef VERIFY
#undef UNIMPLEMENTED
#undef INLINE

#if defined(DEBUG)

extern void CallDebugger(char* expr, char* file, unsigned line);
extern void DumpJavaStack(JNIEnv *env);

#define ASSERT(exp) \
    (void)( (exp) || (CallDebugger(#exp, __FILE__, __LINE__), 0) )
#define VERIFY(exp)     ASSERT(exp)
#define UNIMPLEMENTED() ASSERT(FALSE)

// Disable inlining.
#define INLINE

#else

#define ASSERT(exp) ((void)0)
#define UNIMPLEMENTED() \
    env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "unimplemented");

// VERIFY macro -- assertion where expression is always evaluated 
// (normally used for BOOL functions).
#define VERIFY(exp) ((void)(exp))

// Enable inlining.
#define INLINE inline

#endif  /* !DEBUG */

extern COLORREF DesktopColor2RGB(int colorIndex);

// AWT_TRACE definition
// FIXME: use printf for now because jio_fprint and CVMconsolePrint drags in
// 	other definitions from their *.h include files that conflict with
//	PPC
#if defined(DEBUG)
#  define AWT_TRACE(__args) if (AwtToolkit::GetInstance().Verbose()) NKDbgPrintfW __args
#else
#  define AWT_TRACE(__args)
#endif

// AWT critical section tracing (very verbose). Goes to stderr to sync with thread dumps
//#if defined(CSDEBUG)
//# define AWT_CS_TRACE(__args) /*if (AwtToolkit::GetInstance().Verbose())*/ jio_fprintf __args
//#else   
# define AWT_CS_TRACE(__args) 
//#endif

// Miscellaneous definitions
#define PDATA(T, x) ((class T *)(env->GetIntField(x, WCachedIDs.PPCObjectPeer_pDataFID)))
#define SET_PDATA(x, d) (env->SetIntField(x, WCachedIDs.PPCObjectPeer_pDataFID, d))

#define CHECK_NULL(p, m) {\
	if (p == NULL) {\
		env->ThrowNew(WCachedIDs.NullPointerExceptionClass, m);\
		return;\
	}\
}
#define CHECK_NULL_RETURN(p, m) {\
	if (p == NULL) {\
		env->ThrowNew(WCachedIDs.NullPointerExceptionClass, m);\
		return 0;\
	}\
}
#define CHECK_PEER(p) {\
	if (p == NULL || env->GetIntField(p, WCachedIDs.PPCObjectPeer_pDataFID) == 0) {\
		env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "invalid peer");\
		return;\
	}\
}
#define CHECK_PEER_RETURN(p) {\
	if (p == NULL || env->GetIntField(p, WCachedIDs.PPCObjectPeer_pDataFID) == 0) {\
		env->ThrowNew(WCachedIDs.NullPointerExceptionClass, "invalid peer");\
		return 0;\
	}\
}
#define CHECK_OBJ(o)  ASSERT(o != NULL)



/*
 *   On Win95 AWT throws a NullPointerException when it can't allocate any
 *   more resources. To be compliant to our spec it should throw an
 *   OutOfMemoryError.
 *   I added the macro CHECK_PEER_CREATION() to awt.h and changed CHECK_PEER()
 *   to CHECK_PEER_CREATION() in all of the native _create methods of all
 *   AWT components.
 */
#define CHECK_PEER_CREATION(p) {\
	if (p == NULL || env->GetIntField(p, WCachedIDs.PPCObjectPeer_pDataFID) == 0) {\
		env->ThrowNew(WCachedIDs.OutOfMemoryErrorClass, "invalid peer");\
		return;\
	}\
}



// (from Generic.c) Make it easier to determine appropriate code paths:
#if defined (WIN32)
	#define IS_WIN32 TRUE
#else
	#define IS_WIN32 FALSE
#endif

#ifndef WINCE
#define IS_NT      (IS_WIN32 && !(::GetVersion() & 0x80000000))
#define IS_WIN32S  (IS_WIN32 && !IS_NT && LOBYTE(LOWORD(::GetVersion())) < 4)
#define IS_WIN95   (IS_WIN32 && !IS_NT && LOBYTE(LOWORD(::GetVersion())) >= 4)
#define IS_WIN4X   (IS_WIN32 && LOBYTE(::GetVersion()) >= 4)
#define IS_WINVER_ATLEAST(maj, min) \
                   ((maj) < LOBYTE(LOWORD(::GetVersion())) || \
                      (maj) == LOBYTE(LOWORD(::GetVersion())) && \
                      (min) <= HIBYTE(LOWORD(::GetVersion())))
#else
#define IS_NT      FALSE
#define IS_WIN32S  TRUE
#define IS_WIN95   FALSE
#define IS_WIN4X   TRUE
#define IS_WINCE   TRUE
#define ISWINVER_ATLEAST(maj, min) TRUE
#endif

// macros to crack a LPARAM into two ints -- used for signed coordinates,
// such as with mouse messages.
#define LO_INT(l)           ((int)(short)(l))
#define HI_INT(l)           ((int)(short)(((DWORD)(l) >> 16) & 0xFFFF))

#ifdef WINCE
#define calloc(count,size) memset(sysMalloc((count)*(size)), 0, (count)*(size))
#define PALETTEINDEX(i) ((COLORREF) (0x01000000 | (DWORD) (WORD) (i)))
#endif

#endif
