/*
* @(#)PPCToolkit.cpp	1.57 03/02/21
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
#include "jni.h"
#include "sun_awt_pocketpc_PPCToolkit.h"
#include "sun_awt_pocketpc_ShutdownHook.h"
#include "java_awt_event_InputEvent.h"
#include "java_awt_event_MouseEvent.h"
#include "java_awt_event_PaintEvent.h"
#include "java_awt_event_KeyEvent.h"
#include "awt.h"
#include "PPCToolkit.h"
#include "PPCComponentPeer.h"
#include "PPCPopupMenuPeer.h"
#include "PPCListPeer.h"
#include "PPCClipboard.h"
#include "CmdIDList.h"
#include "PPCImage.h"

struct CachedIDs WCachedIDs;

#ifdef WINCE
#include "ppcResource.h"
#endif /* WINCE */

#define IDT_AWT_MOUSECHECK 0x101


AwtToolkit *AwtToolkit::theInstance;
static HANDLE awtDllHandle;

//Netscape : Contains the offset height for the combo box, used for 
//changing the font of a choice control after the control has been created.  
//Varies by specific OS.
int AwtToolkit::sm_nComboHeightOffset;

extern BOOL g_bMenuLoop; // Bug #4039858 (Selecting menu item causes bogus mouse click event)


static const TCHAR* szAwtToolkitClassName = TEXT("SunAwtToolkit");

/************************************************************************
 * Exported functions
 */

extern "C" BOOL APIENTRY DllMain(HANDLE hInstance, DWORD ul_reason_for_call, 
                                 LPVOID)
{
    // Don't use the TRY and CATCH_BAD_ALLOC_RET macros if we're detaching
    // the library. Doing so causes awt.dll to call back into the VM during
    // shutdown. This crashes the HotSpot VM.
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
	AwtToolkit::GetInstance().SetModuleHandle((HMODULE)hInstance);
    }

    return TRUE;
}

/* Called when System.exit is invoked. We need to shut down the main WINCE event
   loop in a nice way by calling Wince_main_quit. */

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_ShutdownHook_winceMainQuit (JNIEnv *env, jobject thisObj)
{
    AwtToolkit::AtExit();
}

/*
 * Class:     sun_awt_pocketpc_PPCToolkit
 * Method:    initIDs
 * Signature: ()V
 */
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCToolkit_initIDs (JNIEnv *env, jclass cls)
{
    GET_FIELD_ID(PPCToolkit_dbgTraceFID, "dbgTrace", "Z");
    GET_FIELD_ID(PPCToolkit_dbgVerifyFID, "dbgVerify", "Z");
    GET_FIELD_ID(PPCToolkit_dbgBreakFID, "dbgBreak", "Z");
    GET_FIELD_ID(PPCToolkit_dbgHeapCheckFID, "dbgHeapCheck", "Z");
    GET_FIELD_ID(PPCToolkit_frameWidthFID, "frameWidth", "I");
    GET_FIELD_ID(PPCToolkit_frameHeightFID, "frameHeight", "I");
    GET_FIELD_ID(PPCToolkit_vsbMinHeightFID, "vsbMinHeight", "I");
    GET_FIELD_ID(PPCToolkit_vsbWidthFID, "vsbWidth", "I");
    GET_FIELD_ID(PPCToolkit_hsbMinWidthFID, "hsbMinWidth", "I");
    GET_FIELD_ID(PPCToolkit_hsbHeightFID, "hsbHeight", "I");

    cls = env->FindClass ("java/lang/ArrayIndexOutOfBoundsException");
    
    if (cls == NULL)
	return;
    
    WCachedIDs.ArrayIndexOutOfBoundsExceptionClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/awt/Toolkit");

    if (cls == NULL)
        return;
   
    WCachedIDs.java_awt_ToolkitClass = (jclass) env->NewGlobalRef (cls);
    GET_STATIC_METHOD_ID(java_awt_Toolkit_getDefaultToolkitMID, "getDefaultToolkit", "()Ljava/awt/Toolkit;");
    GET_METHOD_ID(java_awt_Toolkit_getColorModelMID, "getColorModel", "()Ljava/awt/image/ColorModel;");

    cls = env->FindClass ("java/lang/NullPointerException");
    
    if (cls == NULL)
	return;
    
    WCachedIDs.NullPointerExceptionClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/lang/InternalError");
    
    if (cls == NULL)
	return;
    
    WCachedIDs.InternalErrorClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/lang/OutOfMemoryError");
    
    if (cls == NULL)
	return;
    
    WCachedIDs.OutOfMemoryErrorClass = (jclass) env->NewGlobalRef
    (cls);

    cls = env->FindClass ("java/awt/AWTException");

    if (cls == NULL)
	return;
    
    WCachedIDs.java_awt_AWTExceptionClass = (jclass)
	env->NewGlobalRef(cls);

    cls = env->FindClass ("java/awt/Color");
	
    if (cls == NULL)
	return;
		
    WCachedIDs.java_awt_ColorClass = (jclass) env->NewGlobalRef (cls);
	
    GET_METHOD_ID (java_awt_Color_getRGBMID, "getRGB", "()I");
    GET_METHOD_ID (java_awt_Color_constructorMID, "<init>", "(III)V");

    cls = env->FindClass ("java/awt/Point");
	
    if (cls == NULL)
	return;
		
    WCachedIDs.java_awt_PointClass = (jclass) env->NewGlobalRef (cls);

    GET_METHOD_ID (java_awt_Point_constructorMID, "<init>", "(II)V");
	
    cls = env->FindClass ("java/awt/Rectangle");
	
    if (cls == NULL)
	return;
		
    WCachedIDs.java_awt_RectangleClass = (jclass) env->NewGlobalRef (cls);
    GET_FIELD_ID (java_awt_Rectangle_xFID, "x", "I");
    GET_FIELD_ID (java_awt_Rectangle_yFID, "y", "I");
    GET_FIELD_ID (java_awt_Rectangle_widthFID, "width", "I");
    GET_FIELD_ID (java_awt_Rectangle_heightFID, "height", "I");
    GET_METHOD_ID (java_awt_Rectangle_constructorMID, "<init>", "(IIII)V");
    GET_METHOD_ID (java_awt_Rectangle_constructorMID, "<init>", "(IIII)V");

    cls = env->FindClass( "java/awt/Choice" );
    
    if ( cls == NULL )
      return;

    WCachedIDs.java_awt_ChoiceClass = (jclass) env->NewGlobalRef(cls);
    
    GET_METHOD_ID( java_awt_Choice_getItemMID, "getItem",
    "(I)Ljava/lang/String;" );

#ifdef DEBUG
    // We get the item count for a Choice item only in
    // AwtChoice::VerifyState(), which is a diagnostic routine.

    GET_METHOD_ID( java_awt_Choice_getItemCountMID, "getItemCount",
                   "()I" );
    GET_METHOD_ID( java_awt_Choice_getSelectedIndexMID, "getSelectedIndex",
                   "()I" );
#endif // DEBUG

    cls = env->FindClass ("java/awt/Component");
	
    if (cls == NULL)
	return;

    GET_FIELD_ID (java_awt_Component_xFID, "x", "I");
    GET_FIELD_ID (java_awt_Component_yFID, "y", "I");
    GET_FIELD_ID (java_awt_Component_widthFID, "width", "I");
    GET_FIELD_ID (java_awt_Component_heightFID, "height", "I");

    GET_METHOD_ID (java_awt_Component_getXMID, "getX", "()I");
    GET_METHOD_ID (java_awt_Component_getYMID, "getY", "()I");
    GET_METHOD_ID (java_awt_Component_getWidthMID, "getWidth", "()I");
    GET_METHOD_ID (java_awt_Component_getHeightMID, "getHeight",
		   "()I");

    GET_METHOD_ID (java_awt_Component_getBackgroundMID,
		   "getBackground", "()Ljava/awt/Color;"); 
    GET_METHOD_ID (java_awt_Component_getForegroundMID,
		   "getForeground", "()Ljava/awt/Color;"); 
    GET_METHOD_ID (java_awt_Component_isEnabledMID,
		   "isEnabled", "()Z");
    GET_METHOD_ID (java_awt_Component_isVisibleMID,
		   "isVisible", "()Z");
    GET_METHOD_ID (java_awt_Component_getFontMID,
		   "getFont", "()Ljava/awt/Font;");
    GET_METHOD_ID (java_awt_Component_getPreferredSizeMID,
                   "getPreferredSize", "()Ljava/awt/Dimension;" );
    GET_METHOD_ID (java_awt_Component_getToolkitMID,
		   "getToolkit", "()Ljava/awt/Toolkit;");
    GET_METHOD_ID (java_awt_Component_setSizeMID,
                   "setSize", "(II)V" );
    
    cls = env->FindClass ("java/awt/Container");
    GET_METHOD_ID (java_awt_Container_getComponentCountMID,
                   "getComponentCount", "()I");
    GET_METHOD_ID (java_awt_Container_getComponentsMID,
                   "getComponents", "()[Ljava/awt/Component;");
    /*
    cls = env->FindClass ("java/awt/Cursor");
	
    if (cls == NULL)
	return;
		
    GET_METHOD_ID (java_awt_Cursor_getTypeMID, "getType", "()I");
    */

    cls = env->FindClass ("java/awt/image/BufferedImage");
    
    if (cls == NULL)
        return;
    
    WCachedIDs.java_awt_image_BufferedImageClass = (jclass) env->NewGlobalRef (cls);
    GET_FIELD_ID (java_awt_image_BufferedImage_peerFID, "peer", "Lsun/awt/image/BufferedImagePeer;");
    GET_METHOD_ID (java_awt_image_BufferedImage_constructorMID, "<init>", "(Lsun/awt/image/BufferedImagePeer;)V");


    cls = env->FindClass ("java/awt/Dialog");
	
    if (cls == NULL)
	return;

    WCachedIDs.java_awt_DialogClass = (jclass) env->NewGlobalRef (cls);

    GET_METHOD_ID (java_awt_Dialog_getTitleMID, "getTitle",
                   "()Ljava/lang/String;");

    cls = env->FindClass ("sun/awt/image/OffScreenImageSource");    
    if (cls == NULL)
        return;
    
    GET_FIELD_ID (sun_awt_image_OffScreenImageSource_baseIRFID, "baseIR", "Lsun/awt/image/ImageRepresentation;");
    GET_FIELD_ID (sun_awt_image_OffScreenImageSource_theConsumerFID, "theConsumer", "Ljava/awt/image/ImageConsumer;");

    cls = env->FindClass ("java/awt/Dimension");
	
    if (cls == NULL)
	return;

    GET_FIELD_ID (java_awt_Dimension_heightFID, "height", "I");
    GET_FIELD_ID (java_awt_Dimension_widthFID, "width", "I");
		
    cls = env->FindClass ("java/awt/Event");

    if (cls == NULL)
        return;

    GET_FIELD_ID  (java_awt_Event_xFID, "x", "I");
    GET_FIELD_ID  (java_awt_Event_yFID, "y", "I");
    GET_FIELD_ID  (java_awt_Event_targetFID, "target", "Ljava/lang/Object;");
		
    cls = env->FindClass("java/awt/Font");
    
    if (cls == NULL) 
	return;
    
    GET_FIELD_ID(java_awt_Font_pDataFID, "pData", "I");
    GET_FIELD_ID(java_awt_Font_peerFID, "peer",
		 "Lsun/awt/peer/FontPeer;");
    GET_METHOD_ID(java_awt_Font_getStyleMID, "getStyle", "()I");
    GET_METHOD_ID(java_awt_Font_getSizeMID,  "getSize",  "()I");
    GET_METHOD_ID(java_awt_Font_getNameMID,  "getName",  "()Ljava/lang/String;");

    cls = env->FindClass ("java/awt/FontMetrics");
	
    if (cls == NULL)
	return;

    GET_FIELD_ID(java_awt_FontMetrics_fontFID, "font", "Ljava/awt/Font;");
    GET_METHOD_ID (java_awt_FontMetrics_getHeightMID, "getHeight",
		   "()I");
		
    cls = env->FindClass ("java/awt/Frame");
	
    if (cls == NULL)
	return;
		
    WCachedIDs.java_awt_FrameClass = (jclass) env->NewGlobalRef (cls);

    cls = env->FindClass ("java/awt/Insets");
	
    if (cls == NULL)
	return;

    GET_FIELD_ID (java_awt_Insets_topFID, "top", "I");
    GET_FIELD_ID (java_awt_Insets_bottomFID, "bottom", "I");
    GET_FIELD_ID (java_awt_Insets_leftFID, "left", "I");
    GET_FIELD_ID (java_awt_Insets_rightFID, "right", "I");    

    cls = env->FindClass( "java/awt/List" );
    if ( cls == NULL )
        return;

    WCachedIDs.java_awt_ListClass = (jclass) env->NewGlobalRef(cls);

    GET_METHOD_ID( java_awt_List_getItemMID, "getItem",
    "(I)Ljava/lang/String;" );


    cls = env->FindClass ("java/awt/Toolkit");
    if (cls == NULL)
	  return;

    GET_METHOD_ID (java_awt_Toolkit_getFontMetricsMID,
	             "getFontMetrics", "(Ljava/awt/Font;)Ljava/awt/FontMetrics;");

    cls = env->FindClass ("java/awt/Window");
	
    if (cls == NULL)
	return;

    GET_METHOD_ID (java_awt_Window_getWarningStringMID,
		   "getWarningString", "()Ljava/lang/String;");


    cls = env->FindClass ("java/awt/datatransfer/StringSelection");

    if (cls == NULL)
	return;

    GET_FIELD_ID(java_awt_datatransfer_StringSelection_dataFID,
		 "data", "Ljava/lang/String;");
    
    cls = env->FindClass ("java/awt/event/ComponentEvent");

    if (cls == NULL)
	return;
    
    WCachedIDs.java_awt_event_ComponentEventClass = (jclass)
	env->NewGlobalRef(cls);
    GET_METHOD_ID (java_awt_event_ComponentEvent_constructorMID, "<init>",
		   "(Ljava/awt/Component;I)V");

    cls = env->FindClass ("java/awt/event/FocusEvent");

    if (cls == NULL)
	return;

    WCachedIDs.java_awt_event_FocusEventClass = (jclass)
	env->NewGlobalRef(cls);

    GET_METHOD_ID (java_awt_event_FocusEvent_constructorMID, "<init>",
		   "(Ljava/awt/Component;IZ)V");

    cls = env->FindClass ("java/awt/event/InputEvent");

    if (cls == NULL)
	return;
    
    GET_METHOD_ID (java_awt_event_InputEvent_getModifiersMID, "getModifiers",
		   "()I");

    cls = env->FindClass ("java/awt/event/KeyEvent");

    if (cls == NULL)
	return;

    WCachedIDs.java_awt_event_KeyEventClass = (jclass)
	env->NewGlobalRef(cls);

    GET_METHOD_ID (java_awt_event_KeyEvent_constructorMID, "<init>",
		   "(Ljava/awt/Component;IJIIC)V");
    GET_METHOD_ID (java_awt_event_KeyEvent_getKeyCodeMID, "getKeyCode", "()I");
    GET_METHOD_ID (java_awt_event_KeyEvent_getKeyCharMID, "getKeyChar", "()C");

    cls = env->FindClass ("java/awt/event/MouseEvent");

    if (cls == NULL)
	return;

    WCachedIDs.java_awt_event_MouseEventClass = (jclass)
	env->NewGlobalRef(cls);

    GET_METHOD_ID (java_awt_event_MouseEvent_constructorMID, "<init>",
		   "(Ljava/awt/Component;IJIIIIZ)V");
    GET_METHOD_ID (java_awt_event_MouseEvent_getXMID, "getX", "()I");
    GET_METHOD_ID (java_awt_event_MouseEvent_getYMID, "getY", "()I");

    cls = env->FindClass ("java/awt/event/WindowEvent");

    if (cls == NULL)
	return;
    
    WCachedIDs.java_awt_event_WindowEventClass = (jclass)
	env->NewGlobalRef(cls);
    GET_METHOD_ID (java_awt_event_WindowEvent_constructorMID, "<init>",
		   "(Ljava/awt/Window;I)V");

    cls = env->FindClass ("java/lang/System");

    if (cls == NULL)
	return;

    WCachedIDs.java_lang_SystemClass = (jclass)
	env->NewGlobalRef(cls);
    
    GET_STATIC_METHOD_ID (java_lang_System_gcMID, "gc", "()V");
    GET_STATIC_METHOD_ID (java_lang_System_runFinalizationMID,
			  "runFinalization", "()V");

    cls = env->FindClass ("java/lang/Thread");

    if (cls == NULL)
	return;

    WCachedIDs.java_lang_ThreadClass = (jclass)
	env->NewGlobalRef(cls);

    GET_STATIC_METHOD_ID(java_lang_Thread_dumpStackMID, "dumpStack",
			 "()V");

    /* Font stuff */
    cls = env->FindClass("sun/awt/PlatformFont");
    
    if (cls == NULL) 
	return;

    
    GET_FIELD_ID(sun_awt_PlatformFont_componentFontsFID, "componentFonts", 
                 "[Lsun/awt/FontDescriptor;");
    GET_FIELD_ID(sun_awt_PlatformFont_propsFID, "props", "Ljava/util/Properties;");
    GET_METHOD_ID(sun_awt_PlatformFont_makeMultiCharsetStringMID, 
                  "makeMultiCharsetString", "(Ljava/lang/String;)[Lsun/awt/CharsetString;");

    cls = env->FindClass("sun/awt/CharsetString");
    if (cls == NULL)
        return;

    GET_FIELD_ID(sun_awt_CharsetString_fontDescriptorFID, "fontDescriptor", 
                 "Lsun/awt/FontDescriptor;");
    GET_FIELD_ID(sun_awt_CharsetString_charsetCharsFID, "charsetChars", "[C");
    GET_FIELD_ID(sun_awt_CharsetString_lengthFID, "length", "I");
    GET_FIELD_ID(sun_awt_CharsetString_offsetFID, "offset", "I");
    
    cls = env->FindClass("sun/awt/FontDescriptor");
    if (cls == NULL)
        return;

    GET_FIELD_ID(sun_awt_FontDescriptor_nativeNameFID, "nativeName", 
                 "Ljava/lang/String;");
    
    GET_FIELD_ID(sun_awt_FontDescriptor_fontCharsetFID, "fontCharset", 
    "Lsun/io/CharToByteConverter;"); 
}

/////////////////////////////////////////////////////////////////////////////
// Toolkit native methods
JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCToolkit_init(JNIEnv *env,
					   jobject self, 
					   jobject thread) 
{
    // Debug settings
    AwtToolkit::GetInstance().SetVerbose(env->GetBooleanField(self, WCachedIDs.PPCToolkit_dbgTraceFID));
    AwtToolkit::GetInstance().SetVerify(env->GetBooleanField(self, WCachedIDs.PPCToolkit_dbgVerifyFID));
    AwtToolkit::GetInstance().SetBreak(env->GetBooleanField(self, WCachedIDs.PPCToolkit_dbgBreakFID));
    AwtToolkit::GetInstance().SetHeapCheck(env->GetBooleanField(self, WCachedIDs.PPCToolkit_dbgHeapCheckFID));

    AwtToolkit::GetInstance().SetPeer(self);
    VERIFY(AwtToolkit::GetInstance().Initialize(env, TRUE));

    env->SetIntField(self, WCachedIDs.PPCToolkit_frameWidthFID, GetSystemMetrics(SM_CXDLGFRAME));
    env->SetIntField(self, WCachedIDs.PPCToolkit_frameHeightFID, GetSystemMetrics(SM_CYDLGFRAME));
 
    env->SetIntField(self, WCachedIDs.PPCToolkit_vsbMinHeightFID,
		     GetSystemMetrics(SM_CYVSCROLL));
    env->SetIntField(self, WCachedIDs.PPCToolkit_vsbWidthFID, GetSystemMetrics(SM_CXVSCROLL));
    env->SetIntField(self, WCachedIDs.PPCToolkit_hsbMinWidthFID, GetSystemMetrics(SM_CXHSCROLL));
    env->SetIntField(self, WCachedIDs.PPCToolkit_hsbHeightFID, GetSystemMetrics(SM_CYHSCROLL));
}


/* FIX FOR SCROLLBARS - WINCE */
JNIEXPORT jobject JNICALL
Java_sun_awt_pocketpc_PPCToolkit_getComponentFromNativeWindowHandle(JNIEnv *env,
								    jobject self,
								    jint hwnd)
{
    HWND hWnd = (HWND)hwnd;
    if (!::IsWindow(hWnd))
        return NULL;

    AwtComponent* comp = AwtComponent::GetComponent(hWnd);
    if (comp == NULL)
        return NULL;

    jobject peer = comp->GetPeer();
    if (peer == NULL)
        return NULL;

    //global ref?
    return env->GetObjectField(peer, WCachedIDs.PPCObjectPeer_targetFID);
}    

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCToolkit_getNativeWindowHandleFromPeer(JNIEnv *env,
							       jobject self,
							       jobject peer)
{
    AwtComponent* comp = PDATA(AwtComponent, peer);
    return (long)(comp->GetHWnd());
}    

/* END FIX FOR SCROLLBARS _ WINCE */

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCToolkit_getComboHeightOffset(JNIEnv *env,
						      jclass cls)
{
    return AwtToolkit::sm_nComboHeightOffset;
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCToolkit_eventLoop(JNIEnv *env,
					   jobject self)
{
    // Add our AtExit handler, now that Java is initialized.

    if (!AwtToolkit::GetInstance().localPump()) {
	return;
    }

    AWT_TRACE((TEXT("AWT event loop started\n")));
    AwtToolkit::GetInstance().MessageLoop();
    AWT_TRACE((TEXT("AWT event loop ended\n")));

    AwtToolkit::GetInstance().Finalize();
    /*
     * The AwtToolkit instance has been destructed.
     * Do NOT call any method of AwtToolkit from now on.
     */
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCToolkit_sync(JNIEnv *env,
				      jobject self)
{
#ifndef WINCE
    VERIFY(::GdiFlush());
#endif
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCToolkit_getScreenHeight(JNIEnv *env,
						 jobject self)
{
#ifndef WINCE
    return ::GetSystemMetrics(SM_CYSCREEN);
#else
    /* return client work area */
    RECT rect;

    if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, FALSE)) {
       return (rect.bottom - rect.top);
    } else {
       return ::GetSystemMetrics(SM_CYSCREEN);
    }

#endif
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCToolkit_getScreenWidth(JNIEnv *env,
						jobject self)
{
    return ::GetSystemMetrics(SM_CXSCREEN);
}

JNIEXPORT jint JNICALL
Java_sun_awt_pocketpc_PPCToolkit_getScreenResolution(JNIEnv *env,
						     jobject self)
{
#ifndef WINCE
    HWND hWnd = ::GetDesktopWindow();  
#else
    HWND hWnd = NULL;
#endif // WINCE
    HDC hDC = ::GetDC(hWnd);
    long result = ::GetDeviceCaps(hDC, LOGPIXELSX);
    ::ReleaseDC(hWnd, hDC);
    return result;
}

JNIEXPORT jobject JNICALL
Java_sun_awt_pocketpc_PPCToolkit_makeColorModel(JNIEnv *env,
						jclass cls)
{
    /* TEMPORARY ONLY */
  /*
    cls = env->FindClass("java/awt/image/ColorModel");
    
    jmethodID defMID = env->GetStaticMethodID(cls, "getRGBdefault",
					      "()Ljava/awt/image/ColorModel;");
    

    return env->CallStaticObjectMethod(cls, defMID);
  */

  // FIX not implemented
    return AwtImage::getColorModel(env); 
    
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCToolkit_beep(JNIEnv *env,
				      jobject self)
{
    VERIFY(::MessageBeep(MB_OK));
}

//  BUFFERED IMAGE CALLS 

JNIEXPORT jobject JNICALL
Java_sun_awt_pocketpc_PPCToolkit_createBufferedImage (JNIEnv * env, jclass cls, jobject peer)
{
    return env->NewObject (WCachedIDs.java_awt_image_BufferedImageClass,
                           WCachedIDs.java_awt_image_BufferedImage_constructorMID,
                           peer);
}

JNIEXPORT jobject JNICALL
Java_sun_awt_pocketpc_PPCToolkit_getBufferedImagePeer (JNIEnv * env, jclass cls, jobject bufferedImage)
{
    return env->GetObjectField (bufferedImage,
                                WCachedIDs.java_awt_image_BufferedImage_peerFID);
}




JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCToolkit_loadSystemColors(JNIEnv *env,
						  jobject self,
						  jintArray hcolors)
{
    static int indexMap[] = {
        COLOR_DESKTOP, // DESKTOP
        COLOR_ACTIVECAPTION, // ACTIVE_CAPTION
        COLOR_CAPTIONTEXT, // ACTIVE_CAPTION_TEXT
        COLOR_ACTIVEBORDER, // ACTIVE_CAPTION_BORDER
        COLOR_INACTIVECAPTION, // INACTIVE_CAPTION
        COLOR_INACTIVECAPTIONTEXT, // INACTIVE_CAPTION_TEXT
        COLOR_INACTIVEBORDER, // INACTIVE_CAPTION_BORDER
        COLOR_WINDOW, // WINDOW
        COLOR_WINDOWFRAME, // WINDOW_BORDER
        COLOR_WINDOWTEXT, // WINDOW_TEXT
        COLOR_MENU, // MENU
        COLOR_MENUTEXT, // MENU_TEXT
        COLOR_3DFACE, // TEXT
        COLOR_BTNTEXT, // TEXT_TEXT
        COLOR_HIGHLIGHT, // TEXT_HIGHLIGHT
        COLOR_HIGHLIGHTTEXT, // TEXT_HIGHLIGHT_TEXT
        COLOR_GRAYTEXT, // TEXT_INACTIVE_TEXT
        COLOR_3DFACE, // CONTROL
        COLOR_BTNTEXT, // CONTROL_TEXT
        COLOR_3DLIGHT, // CONTROL_HIGHLIGHT
        COLOR_3DHILIGHT, // CONTROL_LT_HIGHLIGHT
        COLOR_3DSHADOW, // CONTROL_SHADOW
        COLOR_3DDKSHADOW, // CONTROL_DK_SHADOW
        COLOR_SCROLLBAR, // SCROLLBAR
        COLOR_INFOBK, // INFO
        COLOR_INFOTEXT, // INFO_TEXT
    };
    int   colorLen = env->GetArrayLength(hcolors);
    jint *colors = new jint[colorLen];
    env->GetIntArrayRegion(hcolors, 0, colorLen, colors);
    int i;
    for ( i = 0; i < sizeof indexMap && i < colorLen; i++) {
      colors[i] = (0xFF000000 | DesktopColor2RGB(indexMap[i]));
    }
    env->SetIntArrayRegion(hcolors, 0, i, colors);
    delete colors;

    //what is this doing?
    //KEEP_POINTER_ALIVE(colors);
}

void AwtToolkit::DeleteInstance() {
    if (theInstance != NULL) {
        delete theInstance;
    }
}

AwtToolkit& AwtToolkit::GetInstance() {
    if (theInstance == NULL) {
        theInstance = new AwtToolkit();
        theInstance->SetModuleHandle(awtDllHandle);
    }
    return *theInstance;
}

//
//
//
UINT AwtToolkit::GetAsyncMouseKeyState()
{
#ifndef WINCE
    static BOOL mbSwapped = ::GetSystemMetrics(SM_SWAPBUTTON);
#else
    static BOOL mbSwapped = FALSE; // TODO
#endif
    UINT mouseKeyState = 0;

    if (HIBYTE(::GetAsyncKeyState(VK_CONTROL)))
        mouseKeyState |= MK_CONTROL;
    if (HIBYTE(::GetAsyncKeyState(VK_SHIFT)))
        mouseKeyState |= MK_SHIFT;
    if (HIBYTE(::GetAsyncKeyState(VK_LBUTTON)))
        mouseKeyState |= (mbSwapped ? MK_RBUTTON : MK_LBUTTON);
    if (HIBYTE(::GetAsyncKeyState(VK_RBUTTON)))
        mouseKeyState |= (mbSwapped ? MK_LBUTTON : MK_RBUTTON);
    if (HIBYTE(::GetAsyncKeyState(VK_MBUTTON)))
        mouseKeyState |= MK_MBUTTON;
    return mouseKeyState;
}

//
// Normal ::GetKeyboardState call only works if current thread has
// a message pump, so provide a way for other threads to get
// the keyboard state
//
void AwtToolkit::GetAsyncKeyboardState(PBYTE keyboardState)
{
    CriticalSection::Lock	l(GetInstance().m_lockKB);
    ASSERT(!IsBadWritePtr(keyboardState, KB_STATE_SIZE));
    memcpy(keyboardState, GetInstance().m_lastKeyboardState, KB_STATE_SIZE);
}

///////////////////////////////////////////////////////////////////////////
// JavaStringBuffer method

JavaStringBuffer::JavaStringBuffer(JNIEnv* env, jstring s) {
    int size = env->GetStringLength(s) + 1;
    buffer = new TCHAR[size];
#ifndef UNICODE
    javaString2CString(s, buffer, size);
#else
    env->GetStringRegion(s, 0, size - 1, (jchar *) buffer);
    buffer[size-1] = L'\0';
#endif // UNICODE
}

/////////////////////////////////////////////////////////////////////////////
// AwtToolkit methods

AwtToolkit::AwtToolkit() :
    m_lockKB("kbstate lock")
{
    m_localPump = FALSE;
    m_mainThreadId = 0;
    m_toolkitHWnd = NULL;
    m_verbose = FALSE;
    m_verifyComponents = FALSE;
    m_breakOnError = FALSE;

    m_breakMessageLoop = FALSE;
    m_messageLoopResult = 0;

    m_lastMouseOver = NULL;
    m_mouseDown = FALSE;

    m_hGetMessageHook = 0;
    m_timer = 0;

    m_cmdIDs = new AwtCmdIDList();
    m_pModalDialog = NULL;
    m_peer = NULL;
    m_dllHandle = NULL;

    // initialiaze kb state array
#ifndef WINCE
    ::GetKeyboardState(m_lastKeyboardState);
#else
    /* TODO !*/
#endif
    // Register this toolkit's helper window
    //
    VERIFY(RegisterClass() != NULL);

    AwtComponent::Init();
    
    HDC hDC = ::GetDC(NULL);
    ::ReleaseDC(NULL, hDC);
#ifndef WINCE
	//Netscape : Set system version and initialize the 
	//sm_nComboBoxHeightOffset variable based on the specific OS version
	// we're running.  
	#ifdef _WIN32
		DWORD dwVersion = ::GetVersion();
		if (dwVersion >= 0x80000000){
			//Windows 95 or win32 on 3.1
			AwtToolkit::sm_nComboHeightOffset = 4;
		}
		else {
			DWORD majVer = (DWORD)(LOBYTE(LOWORD(dwVersion)));
			//windows NT
			if (majVer == 3){
				//NT3.51
				AwtToolkit::sm_nComboHeightOffset = 7;
			}
			else if (majVer == 4){ 			
				//NT 4.0
				AwtToolkit::sm_nComboHeightOffset = 4;
			}
		}
	#else
		AwtToolkit::sm_nComboHeightOffset = 6;
	#endif
#else // WINCE
		AwtToolkit::sm_nComboHeightOffset = 6;
#endif
}

AwtToolkit::~AwtToolkit() {
    /*
     * The code has been moved to AwtToolkit::Finalize().
     */
}

#ifdef WINCE
#define WS_TKWIN 0
#else
#define WS_TKWIN WS_DISABLED
#endif

HWND AwtToolkit::CreateToolkitWnd(const TCHAR* name)
{
    HWND hwnd = CreateWindow(
	szAwtToolkitClassName,
	name,	                          // window name
	WS_TKWIN,                         // window style
	-1, -1,				  // position of window
	0, 0,				  // width and height
	NULL, NULL, 			  // hWndParent and hWndMenu
	::GetModuleHandle(NULL),	  // hInstance
	NULL);				  // lpParam
    ASSERT(hwnd != NULL);
#ifdef WINCE
    ::ShowWindow(hwnd, SW_HIDE);
    ::UpdateWindow(hwnd);
    ::SetFocus(hwnd);
#endif
    return hwnd;
}

BOOL AwtToolkit::Initialize(JNIEnv *env, BOOL localPump) {
    if (m_mainThreadId != 0) {
	// Already initialized.
	return FALSE;
    }

    //\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
    // Bugs 4032109, 4047966, and 4071991 to fix AWT
    //	crash in 16 color display mode.  16 color mode is supported.  Less
    //	than 16 color is not.
    // 
    // Check for at least 16 colors
    HDC hDC = ::GetDC(NULL); 
	if ((::GetDeviceCaps(hDC, BITSPIXEL) * ::GetDeviceCaps(hDC, PLANES)) < 4) {
		::MessageBox(NULL,
		     TEXT("Sorry, but this release of Java requires at least 16 colors"),
		     TEXT("AWT Initialization Error"),
		     MB_ICONHAND | MB_APPLMODAL);	
		::DeleteDC(hDC);
		env->ThrowNew(WCachedIDs.InternalErrorClass,
			      "unsupported screen depth");
		return FALSE;
	}
    ::ReleaseDC(NULL, hDC);
    ///////////////////////////////////////////////////////////////////////////

    m_localPump = localPump;
    m_mainThreadId = ::GetCurrentThreadId();

    // Create the one-and-only toolkit window.  This window isn't displayed,
    // but is used to route messages to this thread.
    m_toolkitHWnd = CreateToolkitWnd(TEXT("theAwtToolkitWindow"));
    ASSERT(m_toolkitHWnd != NULL);

    // Setup a GetMessage filter to watch all messages coming out of our 
    // queue from PreProcessMsg()
#ifndef WINCE
    m_hGetMessageHook = ::SetWindowsHookEx(WH_GETMESSAGE,
                                           (HOOKPROC)GetMessageFilter,
                                           0, GetCurrentThreadId());
#else
    // TODO
#endif

    return TRUE;
}

void AwtToolkit::Finalize() {
    AWT_TRACE((TEXT("In AwtToolkit::Finalize\n")));
    Cleanup();
    AwtImage::CleanUp();
    delete m_cmdIDs;
}

ATOM AwtToolkit::RegisterClass() {
    WNDCLASS  wc;

    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = ::GetModuleHandle(NULL);
    wc.hIcon         = NULL;
    wc.hCursor       = NULL;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.lpszMenuName  = NULL;
    wc.lpszClassName = szAwtToolkitClassName;

    ATOM ret = ::RegisterClass(&wc);
    ASSERT(ret != NULL);
    return ret;
}

void AwtToolkit::UnregisterClass() {
    VERIFY(::UnregisterClass(szAwtToolkitClassName, ::GetModuleHandle(NULL)));
}

//
// Structure holding the information to create a component. This packet is 
// sent to the toolkit window.
//
struct ComponentCreatePacket {
    void* hComponent;
    void* hParent;
    void (*factory)(void*, void*);
};

//
// Create an AwtXyz component using a given factory function
// Implemented by sending a message to the toolkit window to invoke the 
// factory function from that thread
//
void AwtToolkit::CreateComponent(void* hComponent, void* hParent, ComponentFactory compFactory)
{
    ComponentCreatePacket ccp = { hComponent, hParent, compFactory };
    ::SendMessage(GetInstance().GetHWnd(), WM_AWT_COMPONENT_CREATE, 
                  0, (long)&ccp);
}

//
// Destroy an HWND that was created in the toolkit thread. Can be used on 
// Components and the toolkit window itself.
//
void AwtToolkit::DestroyComponent(AwtComponent* comp)
{
    AwtToolkit& tk = GetInstance();
    if (comp == tk.m_lastMouseOver) {
        tk.m_lastMouseOver = NULL;
    }

    // Don't filter any post-destroy messages, such as WM_NCDESTROY
    comp->UnsubclassWindow();

    HWND hwnd = comp->GetHWnd();
    if (hwnd != NULL && ::IsWindow(hwnd)) // Bug #4048664 (Hide does not work on modal dialog)
    {
        ::PostMessage(GetInstance().m_toolkitHWnd, WM_AWT_DESTROY_WINDOW, 
                      (WPARAM)hwnd, 0);
    }
}

//
// An AwtToolkit window is just a means of routing toolkit messages to here.
//
LRESULT CALLBACK AwtToolkit::WndProc(HWND hWnd, UINT message, 
                                     WPARAM wParam, LPARAM lParam)
{
    /*
     * Awt widget creation messages are routed here so that all
     * widgets are created on the main thread.  Java allows widgets
     * to live beyond their creating thread -- by creating them on
     * the main thread, a widget can always be properly disposed.
     */
    switch (message) {
      case WM_AWT_EXECUTE_SYNC: {
	  AwtObject * 			object = (AwtObject *)wParam;
	  AwtObject::ExecuteArgs *	args = (AwtObject::ExecuteArgs *)lParam;
	  ASSERT(!IsBadReadPtr(object, sizeof(AwtObject)) );
	  ASSERT(!IsBadReadPtr(args, sizeof(AwtObject::ExecuteArgs)) );
	  return object->WinThreadExecProc(args);
      }
      case WM_AWT_COMPONENT_CREATE: {
          ComponentCreatePacket* ccp = (ComponentCreatePacket*)lParam;
          ASSERT(ccp->factory != NULL);
          ASSERT(ccp->hComponent != NULL);
          (*ccp->factory)(ccp->hComponent, ccp->hParent);
          return 0;
      }
      case WM_AWT_DESTROY_WINDOW: {
          if (wParam != NULL && ::IsWindow((HWND)wParam)) { // Bug #4048664 (Hide does not work on modal dialog)

	    // Destroy widgets from this same thread that created them
	    VERIFY(::DestroyWindow((HWND)wParam) != NULL);
	  }
          return 0;
      }
      case WM_AWT_DISPOSE:
      {
	  JNIEnv *env;
	  
	  if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	      return 0;
	  }
          jobject self = (jobject)wParam;
	  //ASSERT(self != NULL && self->obj != 0);
          AwtObject *p = PDATA(AwtObject, self);
          ASSERT(p != NULL);
          delete p;
          ASSERT(PDATA(AwtObject, self) == NULL);
          return 0;
      }
      case WM_AWT_TOOLKIT_CLEANUP: {
          // When this window is being destroyed, clean up the system
          Cleanup();
          return 0;
      }
      case WM_TIMER: {
	  /*
          // Create an artifical MouseExit message if the mouse left to 
          // a non-java window (bad mouse!)
          POINT pt;
	  AwtToolkit& tk = GetInstance();
#ifdef WINCE
	  // Get CursorPos may fail sometimes on WINCE
	  if (!GetCursorPos(&pt)) {
	      break;
	  }
#else  // ! WINCE
	  VERIFY(::GetCursorPos(&pt));
#endif // ! WINCE	  
	  
	  HWND hWndOver = ::WindowFromPoint(pt);
	  AwtComponent * last_M;
	  if ( AwtComponent::GetComponent(hWndOver) == NULL && tk.m_lastMouseOver != NULL ) {
	      last_M = tk.m_lastMouseOver;
	      // translate point to target window
	      MapWindowPoints(HWND_DESKTOP, last_M->GetHWnd(), &pt, 1);
	      last_M->SendMessage(WM_AWT_MOUSEEXIT,
				  GetAsyncMouseKeyState(), 
				  POINTTOPOINTS(pt));
	      tk.m_lastMouseOver = 0;
	      ::KillTimer(tk.m_toolkitHWnd, tk.m_timer);
	      tk.m_timer = 0;
	  }
	  */
          return 0;
      }
      case WM_AWT_POPUPMENU_SHOW: {
          AwtPopupMenu* popup = (AwtPopupMenu*)wParam;
          popup->Show((jobject)lParam);
          return 0;
      }
      case WM_DESTROYCLIPBOARD: {
	  if (AwtClipboard::ignoreWmDestroyClipboard == FALSE)
	      AwtClipboard::LostOwnership();
          return 0;
      }
      case WM_AWT_LIST_SETMULTISELECT: {
          AwtList* list = (AwtList*)wParam;
          list->SetMultiSelect(lParam);
          return 0;
      }
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LRESULT CALLBACK AwtToolkit::GetMessageFilter(int code, WPARAM wParam, LPARAM lParam)
{
    if (code >= 0 && wParam == PM_REMOVE && lParam != 0) {
       if (GetInstance().PreProcessMsg(*(MSG*)lParam) != mrPassAlong) {
           ((MSG*)lParam)->message = WM_NULL;  // PreProcessMsg() wants us to eat it
       }
    }
#ifndef WINCE
    return ::CallNextHookEx(GetInstance().m_hGetMessageHook, code, wParam, lParam);
#else // WINCE
    // TODO
    return 0;
#endif
}

//
// Cleanup this toolkit & free all the resources allocated. This function may
// be called from a number of places an in different situations:
//
//   1. Normal sysAtExit() callback, probably on our original thread
//   2. Error exit causing Toolkit destructor to be called, probably 
//      different thread
//
void AwtToolkit::Cleanup() {
    AwtToolkit& tk = GetInstance();

    // Only cleanup if it has not already been done
    //
    if (tk.m_mainThreadId != 0 && tk.m_toolkitHWnd) {
        AWT_TRACE((TEXT("In AwtToolkit::Cleanup: mainThreadId:%d toolkitHWnd:%X CurrentThreadId:%d\n"),
		   tk.m_mainThreadId, 
		   tk.m_toolkitHWnd, ::GetCurrentThreadId()));

        // If this is not the correct thread, use SendMessage to switch to the
        // toolkit thread before continuing. This will fail, however, if the
        // main thread was terminated due to an abort.
        //
        if (tk.m_mainThreadId != ::GetCurrentThreadId()) {
            if (::SendMessage(tk.m_toolkitHWnd, WM_AWT_TOOLKIT_CLEANUP, 0, 0) == 0)
                return;  // cleaned up in message 
            AWT_TRACE((TEXT("  Cleanup failed due to terminated Toolkit thread\n")));
        }
        // Now perform the cleanup from within the correct thread
        else {
	    /*
            AwtObjectList::Cleanup();
            AwtFont::Cleanup();
            AwtComponent::Term();
	    */

            VERIFY(::DestroyWindow(tk.m_toolkitHWnd) != NULL);
            tk.m_toolkitHWnd = 0;

            UnregisterClass();

#ifndef WINCE
            ::UnhookWindowsHookEx(tk.m_hGetMessageHook);
#else
	    // TODO
#endif
            
            tk.m_mainThreadId = 0;
        }
    }
}

void AwtToolkit::AtExit() {
    AWT_TRACE((TEXT("In AwtToolkit::AtExit\n")));
    Cleanup();
}

//
// The main message loop
//
UINT AwtToolkit::MessageLoop()
{
    m_messageLoopResult = 0;
    while (!m_breakMessageLoop) {            
#ifndef WINCE
        ::WaitMessage();               // allow system to go idle
#else
	// WINCE's pump will block with a getMessage
        PumpWaitingMessages();         // pumps any waiting messages
#endif
    }
    ::PostQuitMessage(-1);
    m_breakMessageLoop = FALSE;
    return m_messageLoopResult;
}

//
// Called by the message loop to pump the message queue when there are messages
// waiting. Can also be called anywhere to pump messages
//
BOOL AwtToolkit::PumpWaitingMessages()
{
    MSG  msg;
    BOOL foundOne = FALSE;
#ifdef WINCE
do {
        ::GetMessage(&msg, 0, 0, 0); // will block if no message
        foundOne = TRUE;
        if (msg.message == WM_QUIT) {
            m_breakMessageLoop = TRUE;
            m_messageLoopResult = msg.wParam;
            ::PostQuitMessage(msg.wParam);  // make sure all loops exit
            break;
        }
        else if (msg.message != WM_NULL) {
            // The AWT in standalone mode (that is, dynamically loaded from the
            // Java VM) doesn't have any translation tables to worry about, so
            // TranslateAccelerator isn't called.

			// Stylus devices - recognise enter and exit events
			if(!PreProcessMsg(msg)){
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
		}
    } while (::PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE));
#else
    while (::PeekMessage(&msg, 0, 0, 0, PM_REMOVE)) {
        foundOne = TRUE;
        if (msg.message == WM_QUIT) {
            m_breakMessageLoop = TRUE;
            m_messageLoopResult = msg.wParam;
            ::PostQuitMessage(msg.wParam);  // make sure all loops exit
            break;
        }
        else if (msg.message != WM_NULL) {
            // The AWT in standalone mode (that is, dynamically loaded from the
            // Java VM) doesn't have any translation tables to worry about, so
            // TranslateAccelerator isn't called.
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
    }
#endif // WINCE
    return foundOne;
}

//
// Perform pre-processing on a message before it is translated & dispatched.
// Returns true to eat the message
//
BOOL AwtToolkit::PreProcessMsg(MSG& msg)
{
    // Bug #4039858 (Selecting menu item causes bogus mouse click
    // event)
    if (g_bMenuLoop) {
        return FALSE;
    }
    // Offer preprocessing first to the target component, then call out to
    // specific mouse and key preprocessor methods
    //
    AwtComponent* p = AwtComponent::GetComponent(msg.hwnd);
    if (p && p->PreProcessMsg(msg) == mrConsume)
        return TRUE;
#ifndef WINCE
    if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST ||
        msg.message >= WM_NCMOUSEMOVE && msg.message <= WM_NCMBUTTONDBLCLK) {
#else // WINCE
      if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST) {
#endif
        if (PreProcessMouseMsg(p, msg))
            return TRUE;
    }
    else if (msg.message >= WM_KEYFIRST && msg.message <= WM_KEYLAST) {
        if (PreProcessKeyMsg(p, msg))
            return TRUE;
    }
    
    return FALSE;
}

//
//
BOOL AwtToolkit::PreProcessMouseMsg(AwtComponent* p, MSG& msg)
{
    
    WPARAM mouseWParam;
    LPARAM mouseLParam;
    if (msg.message >= WM_MOUSEFIRST && msg.message <= WM_MOUSELAST) {
        mouseWParam = msg.wParam;
        mouseLParam = msg.lParam;
    } else {
        mouseWParam = GetAsyncMouseKeyState();
    }

    // Get the window under the mouse, as it will be different if its captured.
    DWORD dwCurPos = ::GetMessagePos();
    DWORD dwScreenPos = dwCurPos; //Bug # 4092421
    POINT curPos;
    curPos.x = LOWORD(dwCurPos);
    curPos.y = HIWORD(dwCurPos);
    HWND hWndFromPoint = ::WindowFromPoint(curPos);
    AwtComponent* mouseComp = 
        AwtComponent::GetComponent(hWndFromPoint);
    // If the point under the mouse isn't in the client area,
    // ignore it to maintain compatibility with Solaris (#4095172)
    RECT windowRect;
    ::GetClientRect(hWndFromPoint, &windowRect);
    POINT topLeft;
    topLeft.x = 0;
    topLeft.y = 0;
    ::ClientToScreen(hWndFromPoint, &topLeft);
    windowRect.top += topLeft.y;
    windowRect.bottom += topLeft.y;
    windowRect.left += topLeft.x;
    windowRect.right += topLeft.x;
    if ((curPos.y < windowRect.top) ||
        (curPos.y >= windowRect.bottom) ||
        (curPos.x < windowRect.left) ||
        (curPos.x >= windowRect.right)) {
        mouseComp = NULL;
	hWndFromPoint = NULL;
    }

    // Look for mouse transitions between windows & create 
    // MouseExit & MouseEnter messages
    if (mouseComp != m_lastMouseOver) {
        // Send the messages right to the windows so that they are in 
        // the right sequence.
        // Bug #4092421 - send the correct message to the window !!
        if (m_lastMouseOver) {
            dwCurPos = dwScreenPos;
            curPos.x = LOWORD(dwCurPos);
            curPos.y = HIWORD(dwCurPos);
            ::MapWindowPoints(HWND_DESKTOP, m_lastMouseOver->GetHWnd(),
                              &curPos, 1);
            mouseLParam = MAKELPARAM((WORD)curPos.x, (WORD)curPos.y);
            m_lastMouseOver->SendMessage(WM_AWT_MOUSEEXIT, mouseWParam, 
                                         mouseLParam);
        }
        if (mouseComp) {
                dwCurPos = dwScreenPos;
                curPos.x = LOWORD(dwCurPos);
                curPos.y = HIWORD(dwCurPos);
                ::MapWindowPoints(HWND_DESKTOP, mouseComp->GetHWnd(),
                                  &curPos, 1);
                mouseLParam = MAKELPARAM((WORD)curPos.x, (WORD)curPos.y);
            mouseComp->SendMessage(WM_AWT_MOUSEENTER, mouseWParam, mouseLParam);
        }
        m_lastMouseOver = mouseComp;
    }

    // Make sure we get at least one last chance to check for transitions 
    // before we sleep
    if (m_lastMouseOver && !m_timer) {
        m_timer = ::SetTimer(m_toolkitHWnd, IDT_AWT_MOUSECHECK, 200, 0);
    }
    
    return FALSE;  // Now go ahead and process current message as usual
}

BOOL AwtToolkit::PreProcessKeyMsg(AwtComponent* p, MSG& msg)
{
    CriticalSection::Lock	l(m_lockKB);
#ifndef WINCE
    ::GetKeyboardState(m_lastKeyboardState);
#else
    // TODO
#endif
    return FALSE;
}

UINT AwtToolkit::CreateCmdID(AwtObject* object) 
{ 
    return m_cmdIDs->Add(object);
}

AwtObject* AwtToolkit::LookupCmdID(UINT id)
{
    return m_cmdIDs->Lookup(id);
}

void AwtToolkit::RemoveCmdID(UINT id)
{
    m_cmdIDs->Remove(id);
}

HICON AwtToolkit::GetAwtIcon()
{
#ifndef WINCE
    return ::LoadIcon((HINSTANCE)GetModuleHandle(), TEXT("AWT_ICON"));
#else
    // WINCE can't handle Icons that don't conform to its size
    // we also load by ID

    return ::LoadIcon((HINSTANCE)GetModuleHandle(), (LPCTSTR)AWT_ICON_ID);

#endif // WINCE
}

// Enable VC++ 4.X heap checking
#ifndef WINCE
#if defined(_DEBUG) && defined(_MSC_VER) && _MSC_VER >= 1000
#include <crtdbg.h>
#endif

void AwtToolkit::SetHeapCheck(jboolean flag) {
    int dbgFlag = _CRTDBG_ALLOC_MEM_DF;
    if (flag != JNI_FALSE) {
        dbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
    }
    _CrtSetDbgFlag(dbgFlag);
}
#else
void AwtToolkit::SetHeapCheck(jboolean flag) {
    if (flag == JNI_TRUE) {
        printf("heap checking not supported with this build\n");
    }
}
#endif

// Convert a Windows desktop color index into an RGB value.
COLORREF DesktopColor2RGB(int colorIndex) {
    long sysColor = ::GetSysColor(colorIndex);
    return ((GetRValue(sysColor)<<16) | 
            (GetGValue(sysColor)<<8) | GetBValue(sysColor));
}

#ifdef WINCE
/* Wince cannot enumerate windows by thread and it's not possible
 * to store properties, so we keep track of top level windows
 * here as well as their "disabled" counts, which is used by
 * the modal dialog code
 */
#include "ppcError.h" // for ppcPrintError

#define MAX_TOP_LEVEL_WINDOWS 32

static int numTopLevelWindows = 0;
static struct {
    int disabledCount;
    HWND hwnd;
} topLevelWindows[MAX_TOP_LEVEL_WINDOWS];

void AwtToolkit::RegisterTopLevelWindow(HWND hwnd)
{
    int i;
    if (numTopLevelWindows < MAX_TOP_LEVEL_WINDOWS) {
	for (i=0; i < MAX_TOP_LEVEL_WINDOWS; i++) {
	    if (topLevelWindows[i].hwnd == NULL) { // empty slot
		topLevelWindows[i].hwnd = hwnd;
		topLevelWindows[i].disabledCount = 0;
		numTopLevelWindows++;
		return;
	    }
	}
	/* FIX
	ppcPrintError("RegisterTopLevelWindow: no empty slot");
	*/
    } else {
	/* FIX
	ppcPrintError("RegisterTopLevelWindow: too many top level
	windows");
	*/
	// an error, but it will only effect modal dialogs
    }
}

void AwtToolkit::UnregisterTopLevelWindow(HWND hwnd)
{
    int i;
    
    if ((numTopLevelWindows > 0) && (hwnd != NULL)) {
	for (i=0; i < MAX_TOP_LEVEL_WINDOWS; i++) {
	    if (topLevelWindows[i].hwnd == hwnd) {
		topLevelWindows[i].hwnd = NULL;
		topLevelWindows[i].disabledCount = 0;
		numTopLevelWindows--;
		return;
	    }
	}
    }
    /* FIX
    ppcPrintError("unregisterTopLevelWindow: not found");
    */
}

void AwtToolkit::EnumTopLevelWindows(WNDENUMPROC callBack, LPARAM lParam)
{
    int i;
    int n=numTopLevelWindows;

    if (n) {
	for (i=0; i < MAX_TOP_LEVEL_WINDOWS; i++) {
	    if (topLevelWindows[i].hwnd != NULL) {
		callBack(topLevelWindows[i].hwnd, lParam);
		if (--n == 0) {
		    return;
		}
	    }
	}
	/* FIX
	ppcPrintError("RegisterTopLevelWindow: countWrong");
	*/
    }

}

void AwtToolkit::ResetDisabledLevel(HWND hwnd)
{
    int i;
    if (hwnd) {
	for (i=0; i < numTopLevelWindows; i++) {
	    if (topLevelWindows[i].hwnd == hwnd) {
		topLevelWindows[i].disabledCount = 0;
		return;
	    }
	}
    }
}

void AwtToolkit::ChangeDisabledLevel(HWND hwnd, int delta)
{
    int i;
    if (hwnd) {
	for (i=0; i < numTopLevelWindows; i++) {
	    if (topLevelWindows[i].hwnd == hwnd) {
		topLevelWindows[i].disabledCount += delta;
	        if (topLevelWindows[i].disabledCount <= 0) {
		    topLevelWindows[i].disabledCount = 0; /* sanity ?*/
		}
		return;
	    }
	}
    }
}

int AwtToolkit::GetDisabledLevel(HWND hwnd)
{
    int i;
    if (hwnd) {
	for (i=0; i < numTopLevelWindows; i++) {
	    if (topLevelWindows[i].hwnd == hwnd) {
		return(topLevelWindows[i].disabledCount);
	    }
	}
    }
    return 0;
}

void AwtToolkit::SetDisabledLevel(HWND hwnd, int value)
{
    int i;
    if (hwnd) {
	for (i=0; i < numTopLevelWindows; i++) {
	    if (topLevelWindows[i].hwnd == hwnd) {
		topLevelWindows[i].disabledCount = value;
		return;
	    }
	}
    }
}



#endif // WINCE

/////////////////////////////////////////////////////////////////////////////
// Diagnostic routines
#if defined(DEBUG)

void CallDebugger(char* expr, char* file, unsigned line) {
    DWORD   lastError = GetLastError();
    WCHAR    lastErrorMsg[256];

    FormatMessage(
	FORMAT_MESSAGE_FROM_SYSTEM |
	FORMAT_MESSAGE_IGNORE_INSERTS,
	NULL,
        GetLastError(),
	MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
	lastErrorMsg,
	255,
	NULL );

    static BOOL checkedRegistry = FALSE;
    static BOOL showAsserts;

    // Check HKEY_LOCAL_MACHINE\SOFTWARE\JavaSoft\Java Development Kit\1.1\showAsserts
    // to determine if assertion dialogs are desired
#ifndef WINCE
    if (!checkedRegistry)
    {
	    HKEY hkey;
	    if (ERROR_SUCCESS == RegOpenKeyA(HKEY_LOCAL_MACHINE, 
		    "SOFTWARE\\JavaSoft\\Java Development Kit\\1.1", &hkey))
	    {
		    char sz[32];
		    DWORD type;
		    DWORD size;
		    size = sizeof sz;
		    RegQueryValueExA(hkey, "showAsserts", NULL, &type, (LPBYTE)sz, &size);
		    showAsserts = !strcmp(sz, "1");
		    RegCloseKey(hkey);
	    }
	    else 
	    {
		    showAsserts = TRUE;
	    }
    }
#endif
    if (showAsserts)
    {
	    if (AwtToolkit::GetInstance().BreakOnError() == FALSE) {
#ifndef UNICODE
		    printf("Fatal error: %d, %s\n", lastError, lastErrorMsg);
#else
		    printf("Fatal error: %d, %ls\n", lastError, lastErrorMsg);
#endif
#ifndef WINCE
		    _assert(expr, file, line);
#endif
		    return;
	    }

	    BOOL ignoreError = FALSE;
    # if defined(_M_IX86)
	    _asm { int 3 };
    # else
	    DebugBreak();
    # endif
	    // Modify ignoreError in the debugger to continue
#ifndef WINCE
	    if (!ignoreError) {
		    _assert(expr, file, line);
	    }
#endif
    }
    else
    {
	    DWORD lastError = GetLastError();    
	    printf("Internal error: %d, %s\n", lastError, lastErrorMsg);
	    printf("%s:%d %s\n", file, line, expr);    
	    return;
    }
}

void DumpJavaStack(JNIEnv *env)
{
    env->CallStaticVoidMethod(WCachedIDs.java_lang_ThreadClass,
			      WCachedIDs.java_lang_Thread_dumpStackMID);
    ASSERT(!env->ExceptionCheck());
}
#endif // DEBUG
