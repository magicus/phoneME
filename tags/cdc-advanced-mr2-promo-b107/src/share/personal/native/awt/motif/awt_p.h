/*
 * @(#)awt_p.h	1.107 06/10/10
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
 * Motif-specific data structures for AWT Java objects.
 *
 */
#ifndef _AWT_P_H_
#define _AWT_P_H_

/* turn on to do event filtering */
#define NEW_EVENT_MODEL
/* turn on to only filter keyboard events */
#define KEYBOARD_ONLY_EVENTS

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>
#include <X11/Shell.h>
#include <X11/StringDefs.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <Xm/CascadeB.h>
#include <Xm/DrawingA.h>
#include <Xm/FileSB.h>
#include <Xm/BulletinB.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/ScrollBar.h>
#include <Xm/ScrolledW.h>
#include <Xm/SelectioB.h>
#include <Xm/SeparatoG.h>
#include <Xm/ToggleB.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/List.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>

#include "jni.h"

#include "awt.h"

#include "awt_util.h"
#include "common.h"


extern Pixel    awt_pixel_by_name(Display * dpy, char *color, char *defaultColor);

struct WidgetInfo {
	Widget          widget;
	Widget          origin;
	void           *peer;
	long            event_mask;
	struct WidgetInfo *next;
};

#define RepaintPending_NONE 	0
#define RepaintPending_REPAINT  (1 << 0)
#define RepaintPending_EXPOSE   (1 << 1)

#define max(x, y) (((int)(x) > (int)(y)) ? (x) : (y))
#define min(x, y) (((int)(x) < (int)(y)) ? (x) : (y))

typedef struct _DamageRect {
	int             x1;
	int             y1;
	int             x2;
	int             y2;
}               DamageRect;

struct ComponentData {
	Widget          widget;

	/*
	 * Global reference to peer. This is used by call back functions. It
	 * is stored here so it can be deleted in the MComponentPeer.pCleanup
	 * method.
	 */

	jobject         peerGlobalRef;

	int             repaintPending;
	DamageRect      repaintRect;
	DamageRect      exposeRect;
	Cursor          cursor;
	Widget          activePopup;
};

struct MessageDialogData {
	struct ComponentData comp;
	long            isModal;
};

struct CanvasData {
	struct ComponentData comp;
	Widget          shell;
	int             flags;
};

struct GraphicsData {
	Drawable        drawable;
	GC              gc;
	XRectangle      cliprect;
	Pixel           fgpixel;
	Pixel           xorpixel;
	char            clipset;
	char            xormode;
};

struct MenuItemData {
	struct ComponentData comp;
	int             index;
};

struct MenuData {
	struct ComponentData comp;
	struct MenuItemData itemData;
};


#define W_GRAVITY_INITIALIZED 1
#define W_IS_EMBEDDED 2

struct ModalDialogHolder {
	Widget          widget;
};

/* Window Manager (WM). See runningWM() in awt_util.c, awt_Frame.c	 */
#define WM_YET_TO_BE_DETERMINED 0
#define CDE_WM                  1
#define MOTIF_WM                2
#define OPENLOOK_WM             3
#define NO_WM                   98
#define OTHER_WM                99

struct FrameData {
	struct CanvasData winData;
	long            isModal;
	long            mappedOnce;
	Widget          mainWindow;
	Widget          contentWindow;
	Widget          menuBar;
	Widget          warningWindow;
	long            top;	/* these four are the insets...	 */
	long            bottom;
	long            left;
	long            right;
	long            mbHeight;	/* height of the menubar window	 */
	long            wwHeight;	/* height of the warning window	 */
	unsigned        fixInsets;	/* Literal hardcoded insets set	* in
					 * MFramePeer must be fixed	* for
					 * window manager / display	 */
	Boolean         shellResized;	/* frame shell has been resized	 */
	Boolean         canvasResized;	/* frame inner canvas resized	 */
	Boolean         menuBarReset;	/* frame menu bar added/removed	 */
	Boolean         isResizable;	/* is this window resizable ?	 */
	Boolean         isIconic;	/* is this window iconified ?   */
	Boolean         isFixedSizeSet;	/* is fixed size already set ?	 */
	Boolean         isShowing;	/* is this window now showing ?	 */
	Boolean         initialReshape;	/* is this first reshape call ? */
	Boolean         fixTargetSize;	/* must correct the target size */
	struct ModalDialogHolder *modalDialogHolder;
	long            hasIMStatusWindow;
};

struct ListData {
	struct ComponentData comp;
	Widget          list;
};

struct TextAreaData {
	struct ComponentData comp;
	Widget          txt;
};

struct TextFieldData {
	struct ComponentData comp;
	int             echoContextID;
	Boolean         echoContextIDInit;
};

struct FileDialogData {
	struct ComponentData comp;
	char           *file;
};


struct ChoiceData {
	struct ComponentData comp;
	Widget          menu;
	Widget         *items;
	int             maxitems;
	int             n_items;
};

struct ImageData {
	Pixmap          xpixmap;
	Pixmap          xmask;
};

/*
 * Cache of IDs used for quicker JNI access. IDs without pacakage qualifier
 * are for classes in the sun.awt.motif package.
 */

struct CachedIDs {
	jclass          java_awt_AWTErrorClass;
	jclass          java_awt_InsetsClass;
	jclass          java_awt_DimensionClass;
	jclass          java_io_IOExceptionClass;
	jclass          java_lang_ArrayIndexOutOfBoundsExceptionClass;
	jclass          java_lang_IllegalArgumentExceptionClass;
	jclass          java_lang_InternalErrorClass;
	jclass          java_lang_NullPointerExceptionClass;
	jclass          java_lang_OutOfMemoryErrorClass;
	jclass          java_awt_event_FocusEventClass;
	jclass          java_awt_event_KeyEventClass;
	jclass          java_awt_event_MouseEventClass;
	jclass          java_awt_image_DirectColorModelClass;
	jclass          java_awt_image_IndexColorModelClass;
	jclass          java_awt_PointClass;
	jclass          java_awt_RectangleClass;
	jclass          sun_awt_EmbeddedFrameClass;
	jclass          MEmbeddedFrameClass;
	jclass          X11GraphicsClass;

	jfieldID        java_awt_AWTEvent_consumedFID;
	jfieldID        java_awt_AWTEvent_dataFID;
	jfieldID        java_awt_AWTEvent_idFID;
	jfieldID        java_awt_Label_labelFID;
	jfieldID        java_awt_Button_labelFID;
	jfieldID        java_awt_Checkbox_labelFID;
	jfieldID        java_awt_Color_pDataFID;
	jfieldID        java_awt_Component_heightFID;
	jfieldID        java_awt_Component_widthFID;
	jfieldID        java_awt_Component_xFID;
	jfieldID        java_awt_Component_yFID;
	jfieldID        java_awt_Component_peerFID;
	jfieldID        java_awt_Cursor_typeFID;
	jfieldID        java_awt_datatransfer_DataFlavor_atomFID;
	jfieldID        java_awt_Dialog_modalFID;
	jfieldID        java_awt_Dialog_resizableFID;
	jfieldID        java_awt_Event_dataFID;
	jfieldID        java_awt_FileDialog_fileFID;
	jfieldID        java_awt_FileDialog_modeFID;
	jfieldID        java_awt_Font_familyFID;
	jfieldID        java_awt_FontMetrics_fontFID;
	jfieldID        java_awt_Font_pDataFID;
	jfieldID        java_awt_Font_peerFID;
	jfieldID        java_awt_Font_sizeFID;
	jfieldID        java_awt_Font_styleFID;
	jfieldID        java_awt_Frame_mbManagementFID;
	jfieldID        java_awt_Frame_resizableFID;
	jfieldID        java_awt_event_InputEvent_modifiersFID;
	jfieldID        java_awt_event_KeyEvent_keyCodeFID;
	jfieldID        java_awt_event_KeyEvent_keyCharFID;
	jfieldID        java_awt_Insets_topFID;
	jfieldID        java_awt_Insets_leftFID;
	jfieldID        java_awt_Insets_bottomFID;
	jfieldID        java_awt_Insets_rightFID;
	jfieldID        java_awt_MenuItem_labelFID;
	jfieldID        java_awt_MenuItem_enabledFID;
	jfieldID        java_awt_MenuItem_shortcutFID;
	jfieldID        java_awt_Menu_isHelpMenuFID;
	jfieldID        java_awt_Menu_tearOffFID;
	jfieldID        java_awt_Rectangle_xFID;
	jfieldID        java_awt_Rectangle_yFID;
	jfieldID        java_awt_Rectangle_widthFID;
	jfieldID        java_awt_Rectangle_heightFID;
	jfieldID        java_awt_Scrollbar_maximumFID;
	jfieldID        java_awt_Scrollbar_minimumFID;
	jfieldID        java_awt_Scrollbar_orientationFID;
	jfieldID        java_awt_Scrollbar_pageIncrementFID;
	jfieldID        java_awt_Scrollbar_valueFID;
	jfieldID        java_awt_Scrollbar_visibleAmountFID;
	jfieldID        java_awt_ScrollPane_scrollbarDisplayPolicyFID;
	jfieldID        java_awt_TextArea_scrollbarVisibilityFID;
	jfieldID        java_awt_TextField_echoCharFID;
	jfieldID        java_awt_Window_warningStringFID;
	jfieldID        MComponentPeer_cursorSetFID;
	jfieldID        MComponentPeer_handleFID;
	jfieldID        MComponentPeer_mbManagementFID;
	jfieldID        MComponentPeer_pDataFID;
	jfieldID        MComponentPeer_targetFID;
	jfieldID        MDialogPeer_insetsFID;
	jfieldID        MDrawingSurfaceInfo_hFID;
	jfieldID        MDrawingSurfaceInfo_imgrepFID;
	jfieldID        MDrawingSurfaceInfo_peerFID;
	jfieldID        MDrawingSurfaceInfo_stateFID;
	jfieldID        MDrawingSurfaceInfo_wFID;
	jfieldID        MEmbeddedFramePeer_handleFID;
	jfieldID        MFramePeer_insetsFID;
	jfieldID        MFileDialogPeer_fileFID;
        jfieldID        MFontPeer_fonttagFID;
	jfieldID        MFontPeer_pDataFID;
	jfieldID        MFontPeer_xfsnameFID;
	jfieldID        MMenuBarPeer_pDataFID;
	jfieldID        MMenuItemPeer_isCheckboxFID;
	jfieldID        MMenuItemPeer_pDataFID;
	jfieldID        MMenuItemPeer_targetFID;
	jfieldID        MScrollPanePeer_dragInProgressFID;
	jfieldID        MWindowPeer_insetsFID;
	jfieldID        X11Graphics_pDataFID;
	jfieldID        X11Graphics_peerFID;
	jfieldID        X11Graphics_originXFID;
	jfieldID        X11Graphics_originYFID;
	jfieldID        sun_awt_CharsetString_charsetCharsFID;
	jfieldID        sun_awt_CharsetString_fontDescriptorFID;
	jfieldID        sun_awt_CharsetString_lengthFID;
	jfieldID        sun_awt_CharsetString_offsetFID;
	jfieldID        sun_awt_FontDescriptor_fontCharsetFID;
	jfieldID        sun_awt_FontDescriptor_nativeNameFID;
	jfieldID        sun_awt_PlatformFont_componentFontsFID;
	jfieldID        sun_awt_PlatformFont_propsFID;
	jfieldID        sun_awt_image_ImageRepresentation_pDataFID;
	jfieldID        sun_awt_image_ImageRepresentation_availinfoFID;
	jfieldID        sun_awt_image_ImageRepresentation_widthFID;
	jfieldID        sun_awt_image_ImageRepresentation_heightFID;
	jfieldID        sun_awt_image_ImageRepresentation_hintsFID;
	jfieldID        sun_awt_image_ImageRepresentation_newbitsFID;
	jfieldID        sun_awt_image_ImageRepresentation_offscreenFID;
	jfieldID        sun_awt_image_OffScreenImageSource_baseIRFID;
	jfieldID        sun_awt_image_OffScreenImageSource_theConsumerFID;
	jfieldID        X11FontMetrics_ascentFID;
	jfieldID        X11FontMetrics_descentFID;
	jfieldID        X11FontMetrics_heightFID;
	jfieldID        X11FontMetrics_leadingFID;
	jfieldID        X11FontMetrics_maxAdvanceFID;
	jfieldID        X11FontMetrics_maxAscentFID;
	jfieldID        X11FontMetrics_maxDescentFID;
	jfieldID        X11FontMetrics_maxHeightFID;
	jfieldID        X11FontMetrics_widthsFID;
	jfieldID        X11Graphics_imageFID;
	jfieldID        X11Selection_atomFID;
	jfieldID        X11Selection_contentsFID;
	jfieldID        X11Selection_dataFID;
	jfieldID        X11Selection_holderFID;
	jfieldID        X11Selection_targetArrayFID;

	jmethodID       java_awt_Component_getFontMID;
	jmethodID       java_awt_Dimension_constructorMID;
	jmethodID       java_awt_Color_getRGBMID;
	jmethodID       java_awt_MenuComponent_getFontMID;
	jmethodID       java_awt_event_FocusEvent_constructorMID;	/* (Ljava/awt/Component;I
									 * Z) */
	jmethodID       java_awt_event_KeyEvent_constructorMID;	/* (Ljava/awt/Component;I
								 * JIIC) */
	jmethodID       java_awt_event_MouseEvent_constructorMID;
	jmethodID       java_awt_Point_constructorMID;
	jmethodID       java_awt_Rectangle_constructorMID;
	jmethodID       java_awt_image_DirectColorModel_constructorMID;	/* (IIIII) */
	jmethodID       java_awt_image_ImageConsumer_setColorModelMID;
	jmethodID       java_awt_image_ImageConsumer_setHintsMID;
	jmethodID       java_awt_image_ImageConsumer_setPixelsMID;
	jmethodID       java_awt_image_ImageConsumer_setPixels2MID;
	jmethodID       java_awt_image_IndexColorModel_constructorMID;	/* (II[B[B[B) */
	jmethodID       java_awt_Insets_constructorMID;
	jmethodID       java_awt_datatransfer_Transferable_getTransferDataFlavorsMID;
	jmethodID       java_lang_Object_notifyMID;
	jmethodID       java_lang_Object_notifyAllMID;
	jmethodID       java_lang_Object_toStringMID;
	jmethodID       java_lang_Object_waitMID;	/* (J)V */
	jmethodID       java_lang_String_getBytesMID;
	jmethodID       MButtonPeer_actionMID;
	jmethodID       MCheckboxPeer_actionMID;
	jmethodID       MCheckboxMenuItemPeer_actionMID;
	jmethodID       MChoicePeer_actionMID;
	jmethodID       MComponentPeer_getZOrderPositionMID;
	jmethodID       MComponentPeer_handleExposeMID;
	jmethodID       MComponentPeer_handleRepaintMID;
	jmethodID       MComponentPeer_postEventMID;
	jmethodID       MDialogPeer_handleActivateMID;
	jmethodID       MDialogPeer_handleDeactivateMID;
	jmethodID       MDialogPeer_handleMovedMID;
	jmethodID       MDialogPeer_handleQuitMID;
	jmethodID       MDialogPeer_handleResizeMID;
	jmethodID       MEmbeddedFrame_constructorMID;
	jmethodID       MFileDialogPeer_handleCancelMID;
	jmethodID       MFileDialogPeer_handleQuitMID;
	jmethodID       MFileDialogPeer_handleSelectedMID;
	jmethodID       MFramePeer_handleActivateMID;
	jmethodID       MFramePeer_handleDeactivateMID;
	jmethodID       MFramePeer_handleDeiconifyMID;
	jmethodID       MFramePeer_handleIconifyMID;
	jmethodID       MFramePeer_handleMovedMID;
	jmethodID       MFramePeer_handleQuitMID;
	jmethodID       MFramePeer_handleResizeMID;
	jmethodID       MListPeer_actionMID;
	jmethodID       MListPeer_handleListChangedMID;
	jmethodID       MMenuItemPeer_actionMID;	/* (JI)V */
	jmethodID       MPanelPeer_makeCursorsVisibleMID;
	jmethodID       MPopupMenuPeer_destroyNativeWidgetAfterGettingTreeLockMID;
	jmethodID       MScrollbarPeer_dragAbsoluteMID;
	jmethodID       MScrollbarPeer_lineDownMID;
	jmethodID       MScrollbarPeer_lineUpMID;
	jmethodID       MScrollbarPeer_pageDownMID;
	jmethodID       MScrollbarPeer_pageUpMID;
	jmethodID       MScrollPanePeer_scrolledHorizontalMID;
	jmethodID       MScrollPanePeer_scrolledVerticalMID;
	jmethodID       MTextPeer_pasteFromClipboardMID;
	jmethodID       MTextPeer_valueChangedMID;
	jmethodID       MTextPeer_selectMapNotifyMID;
	jmethodID       MTextFieldPeer_actionMID;
	jmethodID       MWindowPeer_handleDeiconifyMID;
	jmethodID       MWindowPeer_handleIconifyMID;
	jmethodID       X11InputMethod_dispatchCommittedTextMID;
	jmethodID       sun_awt_image_Image_getBackgroundMID;
	jmethodID       sun_awt_PlatformFont_makeMultiCharsetStringMID;
	jmethodID       sun_io_CharToByteConverter_convertMID;
	jmethodID       X11Selection_getStringDataFromOwnerMID;
	jmethodID       X11Selection_lostSelectionOwnershipMID;
};

extern struct CachedIDs MCachedIDs;

extern XtAppContext awt_appContext;
extern Pixel    awt_defaultBg;
extern Pixel    awt_defaultFg;

/* allocated and initialize a structure */
#define XDISPLAY awt_display;
#define XVISUAL  awt_visInfo;
#define XFOREGROUND awt_defaultFg;
#define XBACKGROUND awt_defaultBg;
#define XCOLORMAP   awt_cmap;


extern void     awt_put_back_event(JNIEnv * env, XEvent * event);
extern void     awt_MToolkit_modalWait(JNIEnv * env, int (*terminateFn) (void *data), void *data, int *modalNum);
extern void     awt_output_flush(JNIEnv * env);

extern jboolean awt_addWidget(JNIEnv * env, Widget w, Widget origin, void *peer, long event_mask);
extern void     awt_delWidget(Widget w);
extern void     awt_enableWidgetEvents(Widget w, long event_mask);
extern void     awt_disableWidgetEvents(Widget w, long event_mask);
extern int      xerror_handler(Display * disp, XErrorEvent * err);

/* TODO: Motif internals. Need to fix 4090493. */
#define MOTIF_XmINVALID_DIMENSION	((Dimension) 0xFFFF)
#define MOTIF_XmDEFAULT_INDICATOR_DIM	((Dimension) 9)

extern Dimension awt_computeIndicatorSize(struct FontData * fdata);
extern Dimension awt_adjustIndicatorSizeForMenu(Dimension indSize);

#endif				/* _AWT_P_H_ */
