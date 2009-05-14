/*
 * @(#)PPCComponentPeer.cpp	1.21 06/10/10
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

#ifdef WINCE
/*
 * CVM include file undef of "interface" will cause this
 * needed Win32 include to fail compilation unless we
 * place it first. See:
 * $(CVM_TOP)/src/win32/javavm/include/ansi/stddef.h.
 */
#include <aygshell.h>
#define TAP_AND_HOLD_APERTURE 8
/* PocketPC has two timeouts. A regular timeout is
 * 500ms and a long timeout is 1000.
 * Some MS apps, like pocket word, seem to use the long
 * while others, like the file explorer use the short timeout.
 * We choose to to split the diference.
 */ 
#define TAP_AND_HOLD_TIMEOUT 750
#define MENU_MOUSE_BUTTON (RIGHT_BUTTON+2)
#endif WINCE

#include "jni.h"
#include "sun_awt_pocketpc_PPCComponentPeer.h"
#include "java_awt_AWTEvent.h"
#include "java_awt_Cursor.h"
#include "PPCToolkit.h"
#include "PPCComponentPeer.h"
#include "PPCWindowPeer.h"
#include "PPCMenuItemPeer.h"
#include "PPCMenuPeer.h"
#include "PPCGraphics.h"

#ifdef DEBUG
#include "PPCUnicode.h"
#endif

#include "java_awt_event_KeyEvent.h"
#include "java_awt_event_InputEvent.h"
/*
#include <java_awt_Toolkit.h>
#include <java_awt_FontMetrics.h>
#include <java_awt_Color.h>
#include <java_awt_Cursor.h>
#include <java_awt_Event.h>
#include <java_awt_event_KeyEvent.h>
#include <java_awt_Insets.h>

#include <sun_awt_windows_WPanelPeer.h>
*/

// Begin -- Win32 SDK include files
#include <tchar.h>
// End -- Win32 SDK include files

// Double-click variables.
static ULONG multiClickTime = ::GetDoubleClickTime();
static int multiClickMaxX = ::GetSystemMetrics(SM_CXDOUBLECLK);
static int multiClickMaxY = ::GetSystemMetrics(SM_CYDOUBLECLK);
static AwtComponent* lastClickWnd = NULL;
static ULONG lastTime = 0;
static int lastClickX = 0;
static int lastClickY = 0;
static int lastX = 0;
static int lastY = 0;
static int m_dragged = 0;
static int clickCount = 0;
static AwtComponent* lastComp = NULL;

const char* szAwtComponentClassName = "SunAwtComponent";

// Bug #4039858 (Selecting menu item causes bogus mouse click event)
BOOL g_bMenuLoop = FALSE;       
static HWND g_hwndDown = NULL;

// initialize static variable
UINT AwtComponent::m_CodePage = GetACP();

// global variables to compensate for the limitations of
// seeing Ctrl + Shift + key in WmChar
UINT globalCharacter;
BOOL globalCtrlShift = FALSE;

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_initIDs (JNIEnv *env, jclass cls)
{
    GET_METHOD_ID (PPCComponentPeer_handlePaintMID,
		   "handlePaint", "(IIII)V");
    GET_METHOD_ID (PPCComponentPeer_postEventMID,
		   "postEvent", "(Ljava/awt/AWTEvent;)V");
    GET_METHOD_ID (PPCComponentPeer_setBackgroundMID,
		   "setBackground", "(Ljava/awt/Color;)V");
   
    // used by the CanvasPeer
    GET_METHOD_ID (PPCComponentPeer_handleExposeMID,
		   "handleExpose", "(IIII)V");


    cls = env->FindClass ("java/awt/AWTEvent");
    
    if (cls == NULL)
	return;
    
    GET_FIELD_ID (java_awt_AWTEvent_dataFID, "data", "I");
    GET_METHOD_ID (java_awt_AWTEvent_getIDMID, "getID", "()I");
    GET_METHOD_ID (java_awt_AWTEvent_isConsumedMID, "isConsumed",
		   "()Z");
}

//////////////////////////////////////////////////////////////////////////////
// ComponentPeer native methods

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_show(JNIEnv *env,
					    jobject self)
{
    CHECK_PEER(self);
    AwtComponent* p = PDATA(AwtComponent, self);
    CriticalSection::Lock l(p->GetLock());
    p->Show();
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_hide(JNIEnv *env,
					    jobject self)
{
    CHECK_PEER(self);
    AwtComponent* p = PDATA(AwtComponent, self);
    CriticalSection::Lock l(p->GetLock());
    p->Hide();
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_enable(JNIEnv *env,
					      jobject self)
{
    CHECK_PEER(self);
    AwtComponent* p = PDATA(AwtComponent, self);
    ::EnableWindow(p->GetHWnd(), TRUE);
    ::InvalidateRect(p->GetHWnd(), NULL, TRUE); // Bug #4038881 Labels don't enable and disable properly
    CriticalSection::Lock l(p->GetLock());
    p->VerifyState();
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_disable(JNIEnv *env,
					       jobject self)
{
    CHECK_PEER(self);
    AwtComponent* p = PDATA(AwtComponent, self);
    ::EnableWindow(p->GetHWnd(), FALSE);
    ::InvalidateRect(p->GetHWnd(), NULL, TRUE); // Bug #4038881 Labels don't enable and disable properly
    CriticalSection::Lock l(p->GetLock());
    p->VerifyState();
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_reshape(JNIEnv *env,
					       jobject self,
					       jint x,
					       jint y,
					       jint w,
					       jint h)
{
    CHECK_PEER(self);
    AwtComponent* p = PDATA(AwtComponent, self);
    RECT* r = new RECT;
    ::SetRect(r, x, y, x+w, y+h);
    ::PostMessage(p->GetHWnd(), WM_AWT_RESHAPE_COMPONENT, 0,
    (LPARAM)r);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_nativeHandleEvent(JNIEnv *env,
							 jobject self,
							 jobject hEvent)
{
    CHECK_NULL(hEvent, "null AWTEvent");

    jint hEventData = env->GetIntField(hEvent,
				       WCachedIDs.java_awt_AWTEvent_dataFID);
    jboolean hEventConsumed = env->CallBooleanMethod(hEvent,
						     WCachedIDs.java_awt_AWTEvent_isConsumedMID);
    jint hEventId = env->CallIntMethod(hEvent,
				       WCachedIDs.java_awt_AWTEvent_getIDMID);

    if (hEventData != 0) 
    {
        static BOOL keyDownConsumed = FALSE;
#ifndef WINCE
	static BOOL bCharChanged = FALSE;
#endif
	static WCHAR modifiedChar;	
	WCHAR unicodeChar;
        MSG* pMsg = (MSG*)hEventData;
        hEventData = 0;

        // Remember if a KEY_PRESSED event is consumed, as an old model 
        // program won't consume a subsequent KEY_TYPED event.
        if (hEventConsumed) 
        {
            keyDownConsumed = 
                (hEventId == java_awt_event_KeyEvent_KEY_PRESSED);
            delete pMsg;
            return;
        }

        // Consume a KEY_TYPED event if a KEY_PRESSED had been, to support 
        // the old model.
        if (hEventId == java_awt_event_KeyEvent_KEY_TYPED && 
          keyDownConsumed) 
        {
            delete pMsg;
            keyDownConsumed = FALSE;
            return;
        }

        // Modify any event parameters, if necessary.
        if (self && PDATA(AwtComponent, self) && 
            hEventId >= java_awt_event_KeyEvent_KEY_FIRST &&
            hEventId <= java_awt_event_KeyEvent_KEY_LAST) 
        {

            AwtComponent* p = PDATA(AwtComponent, self);
            CriticalSection::Lock l(p->GetLock());

            jobject keyEvent = hEvent;

	    jint keyEventCode = env->CallIntMethod(keyEvent,
					      WCachedIDs.java_awt_event_KeyEvent_getKeyCodeMID);
	    jchar keyEventChar = env->CallCharMethod(keyEvent,
						WCachedIDs.java_awt_event_KeyEvent_getKeyCharMID);
	    jint keyEventModifiers = env->CallIntMethod(keyEvent,
						WCachedIDs.java_awt_event_InputEvent_getModifiersMID);
	    
            switch (hEventId) 
            {
              case java_awt_event_KeyEvent_KEY_PRESSED:
	      {
                  UINT key = p->JavaKeyToWindowsKey(keyEventCode);
	          if (!key) 
	          {
	              key = keyEventCode;
	          }
	          UINT modifiers = keyEventModifiers;
#ifndef WINCE
	          modifiedChar = p->WindowsKeyToJavaChar(key, modifiers);
	          bCharChanged = (keyEventChar != modifiedChar);
#else
		  modifiedChar = keyEventChar;
#endif
	      }

                  break;
              case java_awt_event_KeyEvent_KEY_RELEASED:
                  keyDownConsumed = FALSE;
#ifndef WINCE
		  bCharChanged = FALSE;
#endif
		  break;
              case java_awt_event_KeyEvent_KEY_TYPED: 
              {
#ifndef WINCE
		  if (bCharChanged) 
		  {
		      unicodeChar = modifiedChar;
		  } 
		  else
		  {
		      unicodeChar = (WCHAR)keyEventChar;
		  }
		  bCharChanged = FALSE;
#else
		  unicodeChar = modifiedChar;
#endif

#ifndef UNICODE
                  char mbChar[2] = {'\0','\0',};
                  int len;
                  len = ::WideCharToMultiByte(p->GetCodePage(), 0, &unicodeChar, 1, mbChar, 2, NULL, NULL);
                  // after converting back to ANSI, send the character back to the native
                  // window for processing.
                  if (len>1)
                  {
                      // if DBCS, then we'll package wParam up like wParam in a WM_IME_CHAR
                      // message
                      pMsg->wParam = ((mbChar[0] & 0x00FF) << 8) | (mbChar[1] & 0x00FF);
                  }
		          else
                  {
                      pMsg->wParam = mbChar[0] & 0x00ff;
                  }
#else
                  pMsg->wParam = unicodeChar;
#endif
                  // the WM_AWT_FORWARD_CHAR handler will send this character to the
                  // default window proc
                  ::PostMessage(p->GetHWnd(), WM_AWT_FORWARD_CHAR, pMsg->wParam, pMsg->lParam);
                  delete pMsg;
                  return;
              }
                  break;
              default:
                  break;
            }
        }

        // Post the message directly to the subclassed component.
        // KEY_TYPED events get posted above.
        if (self && PDATA(AwtComponent, self)) 
	    {
            AwtComponent* p = PDATA(AwtComponent, self);
            ::PostMessage(p->GetHWnd(), WM_AWT_HANDLE_EVENT, 0, (LPARAM)pMsg);
        }
        return;
    } 
    
    // Forward any valid synthesized events.  Currently only mouse and key events
    // are supported.
    if (self == NULL || PDATA(AwtComponent, self) == NULL) 
    {
	    return;
    }
    
    AwtComponent* p = PDATA(AwtComponent, self);
    if (hEventId >= java_awt_event_KeyEvent_KEY_FIRST &&
        hEventId <= java_awt_event_KeyEvent_KEY_LAST) 
    {
	    p->SynthesizeKeyMessage(hEvent);
    } else if (hEventId >= java_awt_event_MouseEvent_MOUSE_FIRST &&
	       hEventId <= java_awt_event_MouseEvent_MOUSE_LAST) 
    {
	    p->SynthesizeMouseMessage(hEvent);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_disposeNative(JNIEnv *env,
						     jobject self)
{
    ASSERT(self != NULL);
    AwtComponent* p = PDATA(AwtComponent, self);
    if (p == NULL) {
        return;
    }

    ::SendMessage(AwtToolkit::GetInstance().GetHWnd(), WM_AWT_DISPOSE,
		  (WPARAM)self, 0);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer__1setForeground(JNIEnv *env,
						      jobject self,
						      jint rgb)
{
    CHECK_PEER(self);
    AwtComponent* p = PDATA(AwtComponent, self);
    p->SetColor(PALETTERGB((rgb>>16)&0xff, 
		           (rgb>>8)&0xff, 
		           (rgb)&0xff));
    p->VerifyState();
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer__1setBackground(JNIEnv *env,
						      jobject self,
						      jint rgb)
{
    CHECK_PEER(self);
    AwtComponent* p = PDATA(AwtComponent, self);
    p->SetBackgroundColor(PALETTERGB((rgb>>16)&0xff, 
		                     (rgb>>8)&0xff, 
		                     (rgb)&0xff));
    p->VerifyState();
}

JNIEXPORT jobject JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_getLocationOnScreen(JNIEnv *env,
							   jobject self)
{
    CHECK_PEER_RETURN(self);
    AwtComponent* component = PDATA(AwtComponent, self);

    RECT rect;
    VERIFY(::GetWindowRect(component->GetHWnd(), &rect));

    return env->NewObject(WCachedIDs.java_awt_PointClass,
			  WCachedIDs.java_awt_Point_constructorMID,
			  rect.left,
			  rect.top);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_setFont(JNIEnv *env,
					       jobject self,
					       jobject hFont)
{
    CHECK_PEER(self);
    ASSERT(hFont != NULL);

    AwtFont* font = (AwtFont *) 
	env->GetIntField(hFont, WCachedIDs.java_awt_Font_pDataFID);

    if (font == NULL) {
	//arguments of AwtFont::Create are changed for multifont component
	font = AwtFont::Create(env, hFont);
    }
    env->SetIntField(hFont, WCachedIDs.java_awt_Font_pDataFID, (jint) font);

    AwtComponent* p = PDATA(AwtComponent, self);
    p->SetFont(font);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_requestFocus(JNIEnv *env,
						    jobject self)
{
    CHECK_PEER(self);
    AwtComponent* p = PDATA(AwtComponent, self);
    VERIFY(::PostMessage(p->GetHWnd(), WM_AWT_COMPONENT_SETFOCUS, 0, 0));
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_start(JNIEnv *env,
					     jobject self)
{
    CHECK_PEER(self);
    AwtComponent* p = PDATA(AwtComponent, self);
    jobject target = p->GetTarget();

    // Disable window if specified -- windows are enabled by default.
    if (!env->CallBooleanMethod(target, WCachedIDs.java_awt_Component_isEnabledMID)) {
	::EnableWindow(p->GetHWnd(), FALSE);
    }

    // The peer is now ready for callbacks, since this is the last 
    // initialization call
    p->EnableCallbacks(TRUE);
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_setCursor(JNIEnv *env,
						 jobject self,
						 jobject hJavaCursor)
{
    /* There is no cursor in iPaq
    CHECK_PEER(thisObj);
    CHECK_NULL(cursor, "null cursor");
    AwtComponent* comp = PDATA(AwtComponent, thisObj);
    CriticalSection::Lock l(comp->GetLock());
    LPCTSTR cursorStr;

    jint cursorType = env->CallIntMethod(cursor,
					 WCachedIDs.java_awt_Cursor_getTypeMID);

    switch (cursorType) {
      case java_awt_Cursor_DEFAULT_CURSOR:
      default:
          cursorStr = IDC_ARROW;
          break;
      case java_awt_Cursor_CROSSHAIR_CURSOR:
          cursorStr = IDC_CROSS;
          break;
      case java_awt_Cursor_TEXT_CURSOR:
          cursorStr = IDC_IBEAM;
          break;
      case java_awt_Cursor_WAIT_CURSOR:
          cursorStr = IDC_WAIT;
          break;
      case java_awt_Cursor_NE_RESIZE_CURSOR:
      case java_awt_Cursor_SW_RESIZE_CURSOR:
          cursorStr = IDC_SIZENESW;
          break;
      case java_awt_Cursor_SE_RESIZE_CURSOR:
      case java_awt_Cursor_NW_RESIZE_CURSOR:
          cursorStr = IDC_SIZENWSE;
          break;
      case java_awt_Cursor_N_RESIZE_CURSOR:
      case java_awt_Cursor_S_RESIZE_CURSOR:
          cursorStr = IDC_SIZENS;
          break;
      case java_awt_Cursor_W_RESIZE_CURSOR:
      case java_awt_Cursor_E_RESIZE_CURSOR:
          cursorStr = IDC_SIZEWE;
          break;
      case java_awt_Cursor_HAND_CURSOR:
          cursorStr = TEXT("HAND_CURSOR");
          break;
      case java_awt_Cursor_MOVE_CURSOR:
          cursorStr = IDC_SIZEALL;
          break;
    }

    HCURSOR hCursor;

#if 0
    hCursor = comp->GetCursor(FALSE);
    // DestroyCursor is only valid on cursors created with CreateCursor...
    if (hCursor != NULL && unhand(hCursor)->type == <custom type when it exists>) {
        VERIFY(::DestroyCursor(hCursor));
    }
#endif

    hCursor = ::LoadCursor(NULL, cursorStr);
    if (hCursor == NULL) {
        // Not a system cursor, check for resource.
        hCursor = ::LoadCursor((HINSTANCE)AwtToolkit::GetInstance().GetModuleHandle(), 
                               cursorStr);
    }
    if (hCursor == NULL) {
        // Not found anywhere, use default.
        hCursor = ::LoadCursor(NULL, IDC_ARROW);
        ASSERT(hCursor != NULL);
    }
    comp->SetCursor(hCursor);

    // Force the cursor to change if mouse is over it.
    POINT pt;
    ::GetCursorPos(&pt);
    // removed fix for 4040388, which caused 4117036 and
    // used Oracle's suggested fix - robi.khan@eng
    ::SetCursorPos(pt.x, pt.y);    
    */
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer__1beginValidate(JNIEnv *env,
						      jobject self)
{
    CHECK_OBJ(self);
    if (PDATA(AwtComponent, self)) {
        AwtComponent* p = PDATA(AwtComponent, self);
        ::PostMessage(p->GetHWnd(), WM_AWT_BEGIN_VALIDATE, 0, 0);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_endValidate(JNIEnv *env,
						   jobject self)
{
    CHECK_OBJ(self);
    if (PDATA(AwtComponent, self)) {
        AwtComponent* p = PDATA(AwtComponent, self);
        ::PostMessage(p->GetHWnd(), WM_AWT_END_VALIDATE, 0, 0);
    }
}

JNIEXPORT void JNICALL
Java_sun_awt_pocketpc_PPCComponentPeer_setZOrderPosition(JNIEnv *env,
							 jobject self,
							 jobject aboveComp)
{
    CHECK_PEER(self);
    if (PDATA(AwtComponent, self)) {
        AwtComponent *p = PDATA(AwtComponent, self);
	HWND hWndSelf = p->GetHWnd();
	HWND hWndAbove = NULL;
	HWND hWndInsertAfter = NULL;
	LONG selfStyle = GetWindowLong(hWndSelf, GWL_STYLE);

	if (!(selfStyle & WS_CHILD)) {
	// frames/windows/dialogs do not follow component front-to-back z-ordering rules
	    return;
	}

	// aboveComp refers to the component that should appear on top
	// of this one, so we must insert ourselves below it
        if (aboveComp != NULL && PDATA(AwtComponent, aboveComp)) {
            AwtComponent* aboveW = PDATA(AwtComponent, aboveComp);
            hWndAbove = aboveW->GetHWnd();
        }
	
	// fixed 4059430 (re-opened) -- previous 1.1.7 fix was wrong, restored
	// 1.1.6 code and just added a couple of checks
	if (hWndAbove != NULL) {
	// need to insert in the middle of the z-order
	    ASSERT(IsWindow(hWndAbove));
	    hWndInsertAfter = hWndAbove;
	} else if (hWndAbove == NULL) {
	// insert at top of z-order when aboveComp is null
	    hWndInsertAfter = HWND_TOP;
	}

	// move the window into the correct position in the z-order
	::SetWindowPos(hWndSelf, hWndInsertAfter, 
		       0, 0, 0, 0, SWP_NOACTIVATE |SWP_NOMOVE | SWP_NOSIZE);
    }
}

//////////////////////////////////////////////////////////////////////////
// AwtComponent class methods

AwtComponent::AwtComponent() : m_lock("AwtComponent")
{
    m_callbacksEnabled = FALSE;
    m_hwnd = NULL;

    m_colorForeground = 0;
    m_colorBackground = 0;
    m_backgroundColorSet = FALSE;
    m_penForeground = NULL;
    m_brushBackground = NULL;
    m_sharedDC = NULL;
    m_DefWindowProc = NULL;
    m_nextControlID = 1;
    m_hCursor = NULL;
    m_childList = NULL;
    m_myControlID = 0;
    m_dragged = FALSE;
    m_hdwp = NULL;
    m_validationNestCount = 0;

}

AwtComponent::~AwtComponent()
{
    // all component deletion should be done on the toolkit thread
    // to avoid race conditions (see bug 4051487)
    ASSERT(::GetCurrentThreadId() == AwtToolkit::MainThread());

    CriticalSection::Lock l(GetLock());

    if (m_hdwp != NULL) {
    // end any deferred window positioning, regardless
    // of m_validationNestCount
	::EndDeferWindowPos(m_hdwp);
    }
    
    // Stop message filtering.
    UnsubclassWindow();

    // Disconnect all links.
    UnlinkObjects();

    if (m_childList != NULL)
	delete m_childList;

    if (m_myControlID != 0) {
	AwtComponent* parent = GetParent();
	if (parent != NULL)
	    parent->RemoveChild(m_myControlID);
    }

    // Waiting until this base destructor assumes that we don't care about 
    // getting destroy related messages in derived classes, as this object 
    // is already torn down.
    DestroyHWnd();

    // Release any allocated resources.
    if (m_penForeground != NULL) {
        m_penForeground->Release();
        m_penForeground = NULL;
    }
    if (m_brushBackground != NULL) {
        m_brushBackground->Release();
        m_brushBackground = NULL;
    }
#if 0
    // DestroyCursor is only valid on cursors created with CreateCursor...
    if (m_hCursor != NULL) {
        ::DestroyCursor(m_hCursor);
        m_hCursor = NULL;
    }
#endif
}

///////////////////////////////////////////////////////////////////////////
// AwtComponent window class routines

ATOM thisPropertyAtom;

void AwtComponent::Init()
{
    char buf[40];
    VERIFY(sprintf(buf, "%s-0x%x", szAwtComponentClassName, 
                   ::GetCurrentThreadId()) < 40);
#ifndef WINCE
    thisPropertyAtom = ::GlobalAddAtom(buf);
    ASSERT(thisPropertyAtom != 0);
#endif /* !WINCE */
}

void AwtComponent::Term()
{
#ifndef WINCE
    VERIFY(::GlobalDeleteAtom(thisPropertyAtom) == NULL);
#else
    return;
#endif /* WINCE */
}

//
// Single window proc for all the components. Delegates real work to the component's
// WindowProc() member function.
//
LRESULT CALLBACK AwtComponent::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    ASSERT(::IsWindow(hWnd));

    AwtComponent* self = GetComponent(hWnd);

    if (!self || self->GetHWnd() != hWnd) {
        return ::DefWindowProc(hWnd, message, wParam, lParam);
    }

    return self->WindowProc(message, wParam, lParam);
}

///////////////////////////////////////////////////////////////////////////
// AwtComponent instance routines

////////////////////////////////////////
// Window class registration routines

// Register window class; returns whether registry actually happened.
void AwtComponent::RegisterClass()
{
    const TCHAR* className = GetClassName();
    WNDCLASS  wc;

    if (!::GetClassInfo(::GetModuleHandle(NULL), className, &wc)) {
#ifndef WINCE
        wc.style         = CS_OWNDC;
#else
	wc.style = CS_HREDRAW | CS_VREDRAW;
#endif
	wc.lpfnWndProc   = (WNDPROC)::DefWindowProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 0;
	wc.hInstance     = ::GetModuleHandle(NULL);
	wc.hIcon         = AwtToolkit::GetInstance().GetAwtIcon();
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = NULL;
	wc.lpszClassName = className;

        ATOM ret = ::RegisterClass(&wc);
        ASSERT(ret != 0);
    }
}

void AwtComponent::UnregisterClass()
{
    ::UnregisterClass(GetClassName(), ::GetModuleHandle(NULL));
}

//
// Create the HWND for this component, & link up everyone
//
void 
AwtComponent::CreateHWnd(const TCHAR *title, long windowStyle, long windowExStyle,
                         int x, int y, int w, int h,
                         HWND hWndParent, HMENU hMenu, 
                         COLORREF colorForeground, COLORREF colorBackground,
                         jobject peer)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }

    CriticalSection::Lock l(GetLock());
    //The window class of multifont label must be "BUTTON" because "STATIC" class
    //can't get WM_DRAWITEM message, and m_peerObject member is referred in the
    //GetClassName method of AwtLabel class.
    //So m_peerObject member must be set here.

    m_peerObject = env->NewGlobalRef(peer); // C++ -> JavaPeer
    RegisterClass();

    jobject target = env->GetObjectField(peer,
					 WCachedIDs.PPCObjectPeer_targetFID);
    jboolean isVisible = env->CallBooleanMethod(target,
						WCachedIDs.java_awt_Component_isVisibleMID);

    if (isVisible) {
        windowStyle |= WS_VISIBLE;
    } else {
        windowStyle &= ~WS_VISIBLE;
    }

    HWND hwnd = ::CreateWindowEx(windowExStyle,
				 GetClassName(),
				 title,
				 windowStyle,
				 x, y, w, h,
				 hWndParent,
				 hMenu,
				 ::GetModuleHandle(NULL),
				 NULL);
    ASSERT(hwnd != NULL);
    m_hwnd = hwnd;

    LinkObjects(env, peer);

    // Subclass the window now so that we can snoop on its messages
    SubclassWindow();

    // Set default colors.
    m_colorForeground = colorForeground;
    m_colorBackground = colorBackground;

    // Only set background color if the color is actually set on the target -- this
    // avoids inheriting a parent's color unnecessarily, and has to be done here
    // because there isn't an API to get the real background color from outside the
    // AWT package.

    jobject background = env->CallObjectMethod(target,
					       WCachedIDs.java_awt_Component_getBackgroundMID);
    
    if (background) {
	env->CallVoidMethod(peer,
			    WCachedIDs.PPCComponentPeer_setBackgroundMID, background);
    }
}

//
// Destroy this window's HWND
//
void AwtComponent::DestroyHWnd() {
    if (m_hwnd != NULL) {
        AwtToolkit::DestroyComponent(this);
        m_hwnd = NULL;
    }
}

//
// Install our window proc as the proc for our HWND, and save off the 
// previous proc as the default
//
void AwtComponent::SubclassWindow()
{
    m_DefWindowProc = (WNDPROC)::SetWindowLong(GetHWnd(), GWL_WNDPROC, (long)WndProc);
}

//
// Reinstall the original window proc as the proc for our HWND
//
void AwtComponent::UnsubclassWindow()
{
    if (m_DefWindowProc) {
        ::SetWindowLong(GetHWnd(), GWL_WNDPROC, (long)m_DefWindowProc);
        m_DefWindowProc = NULL;
    }
}

/////////////////////////////////////
// (static method)
// Determines the top-level ancestor for a given window. If the given
// window is a top-level window, return itself.
//
// 'Top-level' includes dialogs as well.
//
HWND AwtComponent::GetTopLevelParentForWindow(HWND hwndDescendant) {
    if (hwndDescendant == NULL) {
	return NULL;
    }

    ASSERT(IsWindow(hwndDescendant));
    HWND hwnd = hwndDescendant;
    for(;;) {
	LONG style = ::GetWindowLong(hwnd, GWL_STYLE);
	if ( (style & WS_CHILD) == 0 ) {
	    // found a non-child window so terminate
	    break;
	}
	hwnd = ::GetParent(hwnd);
    }

    ASSERT( !(::GetWindowLong(hwnd, GWL_STYLE) & WS_CHILD) );
    return hwnd;
}

////////////////////////////////////////
// 

void AwtComponent::SetColor(COLORREF c)
{
    if (m_colorForeground == c) {
        return;
    }
    m_colorForeground = c;
    if (m_penForeground != NULL) {
        m_penForeground->Release();
        m_penForeground = NULL;
    }
    VERIFY(::InvalidateRect(GetHWnd(), NULL, FALSE));
}

void AwtComponent::SetBackgroundColor(COLORREF c)
{
    if (m_colorBackground == c) {
        return;
    }
    m_colorBackground = c;
    m_backgroundColorSet = TRUE;
    if (m_brushBackground != NULL) {
        m_brushBackground->Release();
        m_brushBackground = NULL;
    }
    VERIFY(::InvalidateRect(GetHWnd(), NULL, TRUE));
}

HPEN AwtComponent::GetForegroundPen()
{
    if (m_penForeground == NULL) {
        m_penForeground = AwtPen::Get(m_colorForeground);
    }
    return (HPEN) m_penForeground->GetHandle();
}

COLORREF AwtComponent::GetBackgroundColor()
{
    if (m_backgroundColorSet == FALSE) {
        AwtComponent* c = this;
        while ((c = c->GetParent()) != NULL) {
            if (c->IsBackgroundColorSet()) {
                return c->GetBackgroundColor();
            }
        }
    }
    return m_colorBackground;
}

HBRUSH AwtComponent::GetBackgroundBrush()
{
    if (m_backgroundColorSet == FALSE) {
        if (m_brushBackground != NULL) {
            m_brushBackground->Release();
            m_brushBackground = NULL;
        }
          AwtComponent* c = this;
          while ((c = c->GetParent()) != NULL) {
              if (c->IsBackgroundColorSet()) {
                  m_brushBackground = 
                      AwtBrush::Get(c->GetBackgroundColor());
		  m_brushBackground = NULL;
                  break;
              }
          }
    }
    if (m_brushBackground == NULL) {
        m_brushBackground = AwtBrush::Get(m_colorBackground);
    }
    return (HBRUSH) m_brushBackground->GetHandle();
}

void AwtComponent::SetFont(AwtFont* font)
{
    ASSERT(font != NULL);
    if (font->GetAscent() < 0) {
	AwtFont::SetupAscent(font);
    }
    SendMessage(WM_SETFONT, (WPARAM)font->GetHFont(), MAKELPARAM(FALSE, 0));
    VERIFY(::InvalidateRect(GetHWnd(), NULL, TRUE));
}

AwtSharedDC* AwtComponent::GetSharedDC()
{
    if (m_sharedDC == NULL) {
        HDC hWndDC = ::GetDC(GetHWnd());
        if (hWndDC != 0) {
            ::SetBkMode(hWndDC, TRANSPARENT);
            ::SelectObject(hWndDC, ::GetStockObject(NULL_BRUSH));
            ::SelectObject(hWndDC, ::GetStockObject(NULL_PEN));
#ifndef WINCE
            ::GdiFlush();
#endif
            m_sharedDC = new AwtSharedDC(this, hWndDC);
        }
    }
    return m_sharedDC;
}

AwtComponent* AwtComponent::GetParent() 
{
    HWND hwnd = ::GetParent(GetHWnd());
    if (hwnd == NULL) {
        return NULL;
    }
    return GetComponent(hwnd);
}

AwtWindow* AwtComponent::GetContainer() 
{
    AwtComponent* comp = this;
    while (comp != NULL) {
        if (comp->IsContainer()) {
            return (AwtWindow*)comp;
        }
        comp = comp->GetParent();
    }
    return NULL;
}

////////////////////////////////////////
// 

void AwtComponent::Show()
{
    VERIFY(::PostMessage(GetHWnd(), WM_AWT_COMPONENT_SHOW, SW_SHOWNA, 0));
}

void AwtComponent::Hide()
{
    VERIFY(::PostMessage(GetHWnd(), WM_AWT_COMPONENT_SHOW, SW_HIDE, 0));
}

void AwtComponent::Reshape(int x, int y, int w, int h)
{
    AwtWindow* container = GetContainer();
    AwtComponent* parent = GetParent();
    if (container != NULL && (void *) container == (void *) parent) {
#ifndef WINCE
        container->SubtractInsetPoint(x, y);
#else
	container->TranslateToClient(x, y);
#endif
    }
#ifndef WINCE
    long flags = SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS;
#else
    long flags = SWP_NOACTIVATE | SWP_NOZORDER;
#endif
#ifndef UNICODE
    if (parent && strcmp(parent->GetClassName(), "SunAwtScrollPane") == 0) {
#else
    if (parent && wcscmp(parent->GetClassName(), TEXT("SunAwtScrollPane"))
	== 0) {
#endif 
        if (x > 0) {
            x = 0;
        }
        if (y > 0) {
            y = 0;
        }

	RECT childRect;
	::GetClientRect(GetHWnd(), &childRect);
 
	/* if Reshape() is called and it is not as a result of a move, 
	   suppress repaints.  Given that scrollpane always adjusts
	   the dimensions of its child to be atleast as large as the
	   veiwport,  we can safely assume that these resizes
	   will be outside the viewrect. */
#ifndef WINCE
	if ((childRect.right != w) || (childRect.bottom != h)) {
	    flags |= SWP_NOREDRAW;
	}
#endif
    }
    if (m_hdwp != NULL) {
	m_hdwp = ::DeferWindowPos(m_hdwp, GetHWnd(), 0, x, y, w, h, flags);
	ASSERT(m_hdwp != NULL);
    } else {
	VERIFY(::SetWindowPos(GetHWnd(), 0, x, y, w, h, flags));
    }
}

void AwtComponent::SetScrollValues(UINT bar, int min, int value, int max)
{
#ifndef WINCE
    int minTmp, maxTmp;

    ::GetScrollRange(GetHWnd(), bar, &minTmp, &maxTmp);
    if (min == INT_MAX) {
        min = minTmp;
    }
    if (value == INT_MAX) {
        value = ::GetScrollPos(GetHWnd(), bar);
    }
    if (max == INT_MAX) {
        max = maxTmp;
    }
    if (min == max) {
        max++;
    }
    ::SetScrollRange(GetHWnd(), bar, min, max, FALSE);
    ::SetScrollPos(GetHWnd(), bar, value, TRUE);
#endif /* WINCE */
}

////////////////////////////////////////
// 

//
// Oportunity to process and/or eat a message before it is dispatched
//
MsgRouting AwtComponent::PreProcessMsg(MSG& msg)
{
    return mrPassAlong;
}

static UINT lastMessage = WM_NULL;
#ifdef WINCE
static int wasMenuButton = 0;
static int wasRightButton = 0;
static int wasVK_F23=0;

#include <dbgapi.h>
static int stopme=0;
#define MY_TIMER_ID  0x0FCE
static UINT myTimer=0;
static struct thm {
    HWND hwnd;
    int x;
    int y;
    int lastX;
    int lastY;
    int flags;
    int button;
    int lastMessage;
} thmon;

void ppcKillTHTimer()
{
    if (myTimer) {
	KillTimer(thmon.hwnd, myTimer);
	myTimer = 0; thmon.hwnd=0;
    }
}

#endif /* WINCE */

#define FT2INT64(ft) \
	((__int64)(ft).dwHighDateTime << 32 | (__int64)(ft).dwLowDateTime)

__int64 nowMillis()
{
    SYSTEMTIME st, st0;
    FILETIME   ft, ft0;

    (void)memset(&st0, 0, sizeof st0);
    st0.wYear  = 1970;
    st0.wMonth = 1;
    st0.wDay   = 1;
    SystemTimeToFileTime(&st0, &ft0);

    GetSystemTime(&st);
    SystemTimeToFileTime(&st, &ft);

    return (FT2INT64(ft) - FT2INT64(ft0)) / 10000;
    return 0;
}

static MSG* CreateMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    MSG* msg = new MSG();
    msg->message = message;
    msg->wParam = wParam;
    msg->lParam = lParam;
    return msg;
}

//
// Dispatch messages for this window class--general component
//
static BOOL imeStartComp = FALSE;

LRESULT AwtComponent::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
static BOOL composeSwitch = FALSE;
    LRESULT retValue = 0;
    MsgRouting mr = mrDoDefault;

    lastMessage = message;

    switch (message) {
      case WM_IME_STARTCOMPOSITION: 
	imeStartComp=TRUE;
        break;
      case WM_IME_ENDCOMPOSITION:
        imeStartComp=FALSE;
        break;
      case WM_CREATE:     mr = WmCreate(); break;
      case WM_CLOSE:      mr = WmClose(); break;
      case WM_DESTROY:    mr = WmDestroy(); break;

      case WM_ERASEBKGND: 
          mr = WmEraseBkgnd((HDC)wParam, *(BOOL*)&retValue); break;
      case WM_PAINT:      mr = WmPaint((HDC)wParam); break;
      case WM_MOVE:       mr = WmMove(LOWORD(lParam), HIWORD(lParam)); break;
      case WM_SIZE:       
      {
	  RECT r;
	  // fix 4128317 : use GetClientRect for full 32-bit int precision and
	  // to avoid negative client area dimensions overflowing 16-bit params - robi
	  ::GetClientRect( GetHWnd(), &r );
          mr = WmSize(wParam, r.right - r.left, r.bottom - r.top);
	  //mr = WmSize(wParam, LOWORD(lParam), HIWORD(lParam));
	  break;
      }
      case WM_SHOWWINDOW:
          mr = WmShowWindow(wParam, lParam); break;
      case WM_SYSCOMMAND: 
          mr = WmSysCommand(wParam & 0xFFF0, LOWORD(lParam), HIWORD(lParam));
	  break;
#ifndef WINCE
      case WM_EXITSIZEMOVE:
          mr = WmExitSizeMove();
	  break;
#endif
      // Bug #4039858 (Selecting menu item causes bogus mouse click event)
      case WM_ENTERMENULOOP:
	  mr = WmEnterMenuLoop((BOOL)wParam);
          g_bMenuLoop = TRUE;
          break;     
      case WM_EXITMENULOOP:
	  mr = WmExitMenuLoop((BOOL)wParam);
          g_bMenuLoop = FALSE;
          break;     

      case WM_SETFOCUS:
	  mr = WmSetFocus((HWND)wParam);
	  break;
      case WM_KILLFOCUS:
	  mr = WmKillFocus((HWND)wParam);
	  break;
      case WM_ACTIVATE:   
          mr = WmActivate(LOWORD(wParam), (BOOL)HIWORD(wParam)); 
          SHFullScreen(AwtComponent::GetTopLevelParentForWindow(GetHWnd()), SHFS_HIDESTARTICON | SHFS_HIDETASKBAR | SHFS_SHOWSIPBUTTON);
          break;

#if defined(WIN32)
      case WM_CTLCOLOREDIT:
          mr = mrDoDefault;
          break;
      case WM_CTLCOLORMSGBOX:
      case WM_CTLCOLORLISTBOX:
      case WM_CTLCOLORBTN:
      case WM_CTLCOLORDLG:
      case WM_CTLCOLORSCROLLBAR:
      case WM_CTLCOLORSTATIC:
          mr = WmCtlColor((HDC)wParam, (HWND)lParam, 
                          message-WM_CTLCOLORMSGBOX+CTLCOLOR_MSGBOX, 
                          *(HBRUSH*)&retValue); 
          break;
      case WM_HSCROLL:    
          mr = WmHScroll(LOWORD(wParam), HIWORD(wParam), (HWND)lParam); 
          break;
      case WM_VSCROLL:    
          mr = WmVScroll(LOWORD(wParam), HIWORD(wParam), (HWND)lParam); 
          break;
#else
      case WM_CTLCOLOR:   
          mr = WmCtlColor((HDC)wParam, (HWND)LOWORD(lParam), HIWORD(lParam),
                          *(HBRUSH*)&retValue); 
          break;
      case WM_HSCROLL:    
          mr = WmHScroll(wParam, LOWORD(lParam), (HWND)HIWORD(lParam)); 
          break;
      case WM_VSCROLL:
          mr = WmVScroll(wParam, LOWORD(lParam), (HWND)HIWORD(lParam)); 
          break;
#endif
      case WM_SETCURSOR:  
          mr = WmSetCursor((HWND)wParam, LOWORD(lParam), HIWORD(lParam), 
                           *(BOOL*)&retValue); 
          break;
      case WM_AWT_MOUSEENTER:
          mr = WmMouseEnter(wParam, LO_INT(lParam), HI_INT(lParam)); break;
      case WM_LBUTTONDOWN:
      case WM_LBUTTONDBLCLK:
          mr = WmMouseDown(wParam, LO_INT(lParam), HI_INT(lParam), LEFT_BUTTON);
          break;
      case WM_MBUTTONDOWN:
      case WM_MBUTTONDBLCLK:
          mr = WmMouseDown(wParam, LO_INT(lParam), HI_INT(lParam), MIDDLE_BUTTON); break;
      case WM_RBUTTONDOWN:
      case WM_RBUTTONDBLCLK:
          mr = WmMouseDown(wParam, LO_INT(lParam), HI_INT(lParam), RIGHT_BUTTON); break;
      case WM_LBUTTONUP:
          mr = WmMouseUp(wParam, LO_INT(lParam), HI_INT(lParam), LEFT_BUTTON); break;
      case WM_RBUTTONUP:
          mr = WmMouseUp(wParam, LO_INT(lParam), HI_INT(lParam), RIGHT_BUTTON);
          break;
      case WM_MBUTTONUP:      
          mr = WmMouseUp(wParam, LO_INT(lParam), HI_INT(lParam), MIDDLE_BUTTON);
          break;
      case WM_MOUSEMOVE:
          mr = WmMouseMove(wParam, LO_INT(lParam), HI_INT(lParam)); 
          break;
      case WM_AWT_MOUSEEXIT:
          mr = WmMouseExit(wParam, LO_INT(lParam), HI_INT(lParam)); 
          break;

      case WM_KEYDOWN:
          mr = WmKeyDown(wParam, LOWORD(lParam), HIWORD(lParam), FALSE); 
          break;
      case WM_KEYUP:
          mr = WmKeyUp(wParam, LOWORD(lParam), HIWORD(lParam), FALSE); 
          break;
      case WM_CHAR:
#ifdef WINCE
	  /* ignore the enter key if was generated by an ACTION+CLICK */
	  if ((wasVK_F23) && (VK_RETURN == (int) wParam)) {
	      wasVK_F23 = 0;
	      mr = mrConsume;
	  } else {
	      mr = WmChar(wParam, LOWORD(lParam), HIWORD(lParam), FALSE);
	  }
#else /* !WINCE: same as else */
	  mr = WmChar(wParam, LOWORD(lParam), HIWORD(lParam), FALSE);
#endif /* WINCE */
          break;

      case WM_IME_CHAR:
          mr = WmIMEChar(wParam, LOWORD(lParam), HIWORD(lParam), FALSE); 
          break;
#ifndef WINCE
      case WM_INPUTLANGCHANGE:
      	  mr = WmInputLangChange(wParam, lParam);
	  // should return non-zero if we process this message
	  retValue = 1;
	  break;
#endif
      case WM_AWT_FORWARD_CHAR:
      	  mr = WmForwardChar(wParam, lParam);
	  break;

      case WM_AWT_FORWARD_BYTE:
          mr = HandleEvent((MSG*)lParam);
          break;

      case WM_SYSKEYDOWN:
          mr = WmKeyDown(wParam, LOWORD(lParam), HIWORD(lParam), TRUE); 
          break;
      case WM_SYSKEYUP:
          mr = WmKeyUp(wParam, LOWORD(lParam), HIWORD(lParam), TRUE); 
          break;
      case WM_SYSCHAR:
          mr = WmChar(wParam, LOWORD(lParam), HIWORD(lParam), TRUE); 
          break;

      case WM_TIMER:
#ifdef WINCE
	  if ((myTimer) && ((UINT)wParam == myTimer)) {
	      if (thmon.hwnd == GetHWnd()) {
		  /* Fake a menu trigger by sending a right button down */
		  /* followed by a right button up, with the popup trigger */
		  /* set. */
		  MSG* msg = CreateMessage(thmon.lastMessage, thmon.flags,
					 MAKELPARAM(thmon.lastX, thmon.lastY));
		  MSG* msg2 = CreateMessage(thmon.lastMessage, thmon.flags,
					 MAKELPARAM(thmon.lastX, thmon.lastY));
		 
		  SendMouseEvent(java_awt_event_MouseEvent_MOUSE_PRESSED,
				 nowMillis(), thmon.lastX, thmon.lastY, 
				 GetJavaModifiers(thmon.flags, MENU_MOUSE_BUTTON),
				 1, FALSE, msg);
		  SendMouseEvent(java_awt_event_MouseEvent_MOUSE_RELEASED,
				 nowMillis(), thmon.lastX, thmon.lastY, 
				 GetJavaModifiers(thmon.flags, MENU_MOUSE_BUTTON),
				 1, TRUE, msg2);
	      }
	      KillTimer(GetHWnd(), myTimer);
	  } else { /* another timer */
	      mr = WmTimer(wParam);
	  }
#else  /* ! WINCE */
	  mr = WmTimer(wParam);
#endif /* ! WINCE */
          break;

      case WM_COMMAND:    
#if defined(WIN32)
          mr = WmCommand(LOWORD(wParam), (HWND)lParam, HIWORD(wParam));
#else
          mr = WmCommand(LOWORD(wParam), (HWND)LOWORD(lParam), HIWORD(lParam));
#endif
          break;

      case WM_COMPAREITEM:
          mr = WmCompareItem(wParam, *(COMPAREITEMSTRUCT*)lParam, retValue);
          break;
      case WM_DELETEITEM: 
          mr = WmDeleteItem(wParam, *(DELETEITEMSTRUCT*)lParam);
          break;
      case WM_DRAWITEM:
          mr = WmDrawItem(wParam, *(DRAWITEMSTRUCT*)lParam);
          break;
      case WM_MEASUREITEM:
          mr = WmMeasureItem(wParam, *(MEASUREITEMSTRUCT*)lParam);
          break;

      case WM_AWT_HANDLE_EVENT:
          mr = HandleEvent((MSG*)lParam);
          break;

      case WM_AWT_PRINT_COMPONENT:
	  mr = PrintComponent((AwtGraphics*)wParam);
	  break;
#ifndef WINCE
      case WM_PRINT:
	  mr = WmPrint((HDC)wParam, lParam);
	  break;

       case WM_PRINTCLIENT:
	  mr = WmPrintClient((HDC)wParam, lParam);
	  break;

      case WM_NCCALCSIZE:
	  mr = WmNcCalcSize((BOOL)wParam, (LPNCCALCSIZE_PARAMS)lParam, 
			    *(UINT*)&retValue);
	  break;
      case WM_NCPAINT:
	  mr = WmNcPaint((HRGN)wParam);
	  break;
      case WM_NCHITTEST:
	  mr = WmNcHitTest(LOWORD(lParam), HIWORD(lParam), *(UINT*)&retValue);
	  break;
#endif
      case WM_AWT_RESHAPE_COMPONENT: {
          RECT* r = (RECT*)lParam;
          Reshape(r->left, r->top, r->right - r->left, r->bottom - r->top);
          delete r;
          mr = mrConsume;
          break;
      }

      case WM_AWT_BEGIN_VALIDATE:
          BeginValidate();
          mr = mrConsume;
          break;
      case WM_AWT_END_VALIDATE:
          EndValidate();
          mr = mrConsume;
          break;
			  
      case WM_QUERYNEWPALETTE:
	  mr = WmQueryNewPalette(*(UINT*)&retValue);
          break;
      case WM_PALETTECHANGED:
          mr = WmPaletteChanged((HWND)wParam);
          break;
      case WM_STYLECHANGED:
	  mr = WmStyleChanged(wParam, (LPSTYLESTRUCT)lParam);
	  break;
      case WM_SETTINGCHANGE:
	  mr = WmSettingChange(wParam, (LPCTSTR)lParam);
	  break;
      // These messages are used to route Win32 calls to the creating thread,
      // since these calls fail unless executed there.
      case WM_AWT_COMPONENT_SHOW:
          ::ShowWindow(GetHWnd(), wParam);
          mr = mrConsume;
          break;
      case WM_AWT_COMPONENT_SETFOCUS:
          if (wParam != NULL) {
              if ( ((HWND) wParam) == ::GetActiveWindow()) {
                  ::SetFocus(GetHWnd());
              } else {
                  // Don't call SetFocus if our top-level window is not
                  // active since SetFocus would activate it.
                  ::SendMessage((HWND) lParam, WM_KILLFOCUS,
				(WPARAM)GetHWnd(), 0);
                  ::SendMessage(GetHWnd(), WM_SETFOCUS, lParam, 0);
              }
          } else {
              ::SetFocus(GetHWnd());
          }
          mr = mrConsume;
          break;

      case WM_AWT_SET_SCROLL_INFO:
	  SCROLLINFO *si = (SCROLLINFO *) lParam;
	  ::SetScrollInfo(GetHWnd(), (int) wParam, si, TRUE);
	  delete si;
	  mr = mrConsume;
	  break;
    }

    // If not a specific Consume, it was a specific DoDefault, or a 
    // PassAlong (since the default is the next in chain), then call the 
    // default proc
    //
    if (mr != mrConsume) {
        retValue = DefWindowProc(message, wParam, lParam);
    }
    return retValue;
}

//
// Call this instance's default window proc, or if none set, call the stock 
// Window's one.
//
LRESULT AwtComponent::DefWindowProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
    return m_DefWindowProc
        ? ::CallWindowProc((WNDPROC)m_DefWindowProc, GetHWnd(), msg, wParam, lParam)
        : ::DefWindowProc(GetHWnd(), msg, wParam, lParam);
}

// This message should only be received when a window is destroy by
// Windows, and not Java.  Window termination has been reworked so
// this method should never be called during termination.
MsgRouting AwtComponent::WmDestroy()
{
    delete this;
    return mrConsume;
}

MsgRouting AwtComponent::WmMove(int x, int y)
{
    return mrDoDefault;
}

MsgRouting AwtComponent::WmSize(int type, int w, int h)
{
    return mrDoDefault;
}
  
MsgRouting AwtComponent::WmSysCommand(UINT uCmdType, UINT xPos, UINT yPos)
{
    return mrDoDefault;
}

MsgRouting AwtComponent::WmExitSizeMove()
{
    return mrDoDefault;
}

MsgRouting AwtComponent::WmEnterMenuLoop(BOOL isTrackPopupMenu)
{
    return mrDoDefault;
}

MsgRouting AwtComponent::WmExitMenuLoop(BOOL isTrackPopupMenu)
{
    return mrDoDefault;
}

MsgRouting AwtComponent::WmShowWindow(int show, long status)
{
    return mrDoDefault;
}

MsgRouting AwtComponent::WmSetFocus(HWND hWndLost)
{
    SendFocusEvent(java_awt_event_FocusEvent_FOCUS_GAINED);
    return mrDoDefault;
}

MsgRouting AwtComponent::WmKillFocus(HWND hWndGotFocus)
{
    HWND newTop = NULL;
    HWND oldTop = NULL;

    // Determine new top-level window with component gaining the focus
    newTop = AwtComponent::GetTopLevelParentForWindow(hWndGotFocus);

    // Determine old top-level window with component losing the focus
    oldTop = AwtComponent::GetTopLevelParentForWindow(GetHWnd());

    // FOCUS_LOST is temporary if we are switching to another top-level window
    BOOL isTemporary = (newTop != oldTop);

    // Post the FOCUS_LOST event. It will be redispatched to lightweight
    // components at the java.awt level
    SendFocusEvent(java_awt_event_FocusEvent_FOCUS_LOST, isTemporary);

    return mrDoDefault;
}

MsgRouting AwtComponent::WmCtlColor(HDC hDC, HWND hCtrl, UINT ctlColor, HBRUSH& retBrush)
{
    AwtComponent* child = AwtComponent::GetComponent(hCtrl);
    if (child) {
        ::SetBkColor(hDC, child->GetBackgroundColor());
        ::SetTextColor(hDC, child->GetColor());
        retBrush = child->GetBackgroundBrush();
        return mrConsume;
    }
    return mrDoDefault;
/*
    switch (ctlColor) {
        case CTLCOLOR_MSGBOX:
        case CTLCOLOR_EDIT:
        case CTLCOLOR_LISTBOX:
        case CTLCOLOR_BTN:
        case CTLCOLOR_DLG:
        case CTLCOLOR_SCROLLBAR:
        case CTLCOLOR_STATIC:
    }
*/
}

MsgRouting AwtComponent::WmHScroll(UINT scrollCode, UINT pos, HWND hScrollbar) {
    if (hScrollbar && hScrollbar != GetHWnd()) {  // the last test should never happen
        AwtComponent* sb = GetComponent(hScrollbar);
        if (sb) {
            sb->WmHScroll(scrollCode, pos, hScrollbar);
        }
    }
    return mrDoDefault;
}

MsgRouting AwtComponent::WmVScroll(UINT scrollCode, UINT pos, HWND hScrollbar) {
    if (hScrollbar && hScrollbar != GetHWnd()) {  // the last test should never happen
        AwtComponent* sb = GetComponent(hScrollbar);
        if (sb) {
            sb->WmVScroll(scrollCode, pos, hScrollbar);
        }
    }
    return mrDoDefault;
}


MsgRouting AwtComponent::WmPaint(HDC)
{
    // Get the rectangle that covers all update regions, if any exist.
    RECT r;
    if (::GetUpdateRect(GetHWnd(), &r, FALSE)) {
        if ((r.right-r.left) > 0 && (r.bottom-r.top) > 0 &&
            m_peerObject != NULL && m_callbacksEnabled) {
            // Always call handlePaint, because the underlying control
            // will have painted itself (the "background") before any
            // paint method is called.
	    JNIEnv *env;
	    
	    if (JVM->AttachCurrentThread((void **) &env, 0) == 0) {
		env->CallVoidMethod(GetPeer(),
				    WCachedIDs.PPCComponentPeer_handlePaintMID, 
				    r.left, r.top, r.right-r.left,
				    r.bottom-r.top);
	    }
        }
    }
    return mrDoDefault;
}

MsgRouting AwtComponent::WmSetCursor(HWND hWnd, UINT hitTest, UINT message, 
                                     BOOL& retVal)
{
    if (hitTest == HTCLIENT) {
        ::SetCursor(GetCursor(TRUE));
        retVal = TRUE;
        return mrConsume;
    } else {
        return mrDoDefault;
    }
}

MsgRouting AwtComponent::WmMouseEnter(UINT flags, int x, int y)
{
    SendMouseEvent(java_awt_event_MouseEvent_MOUSE_ENTERED, 
                   nowMillis(), x, y, 0, 0, FALSE);
    return mrConsume;	// Don't pass our synthetic event on!
}



MsgRouting AwtComponent::WmMouseDown(UINT flags, int x, int y, int button)
{
    __int64 tm = nowMillis();

    ULONG now = (ULONG)tm;
#ifdef WINCE
    ppcKillTHTimer();
#endif /* WINCE */
    if (lastClickWnd == this &&
        (now - lastTime) <= multiClickTime && 
        abs(x - lastClickX) <= multiClickMaxX &&
        abs(y - lastClickY) <= multiClickMaxY) {
        clickCount++;
    } else {
	clickCount = 1;
    }
    lastClickWnd = this;
    lastTime = now;
    lastX = lastClickX = x;
    lastY = lastClickY = y;
	lastComp = this;
    m_dragged = FALSE;

#ifdef WINCE
    /* Windows CE Menu Handling Code: much weirdness because */
    /* CE doesn't send RBUTTON events and for device's w/o right */
    /* buttons, we check for ALT (MENU) key to invoke popups */
    /* Also the action key (F23) */
    
    if ((GetKeyState(VK_MENU) < 0) || (GetAsyncKeyState(VK_F23) < 0) ) {
	wasMenuButton = 1;
    } else if (GetAsyncKeyState(VK_F23) < 0) {
	wasVK_F23 = 1;
	wasMenuButton = 1;
    } else {
	wasMenuButton = 0; /* this doen't work, but we clear it elsewhere */
      /* probably because of the way events are dispatched while menus are */
      /* up */
    }
    if ((!(flags & MK_LBUTTON)) && (flags & MK_RBUTTON)) {
	/* not really the left button */
	button = RIGHT_BUTTON;
	wasRightButton = 1;
    } else {
	wasRightButton = 0;
    }
    MSG* msg = CreateMessage(lastMessage, flags, MAKELPARAM(x, y));
    
    SendMouseEvent(java_awt_event_MouseEvent_MOUSE_PRESSED, tm, x, y, 
                   GetJavaModifiers(flags, button), clickCount, FALSE, msg);
    if (myTimer) {
	ppcKillTHTimer();
    }
    myTimer = SetTimer(GetHWnd(), MY_TIMER_ID, TAP_AND_HOLD_TIMEOUT, NULL);
    thmon.x = thmon.lastX = x;
    thmon.y = thmon.lastY = y;
    thmon.flags = flags;
    thmon.button = button;
    thmon.hwnd = GetHWnd();
    thmon.lastMessage = lastMessage;
    return mrConsume;

#else /* WINCE */

    MSG* msg = CreateMessage(lastMessage, flags, MAKELPARAM(x, y));
    SendMouseEvent(java_awt_event_MouseEvent_MOUSE_PRESSED, tm, x, y, 
                   GetJavaModifiers(flags, button), clickCount, FALSE, msg);

    return mrConsume;
#endif
}

MsgRouting AwtComponent::WmMouseUp(UINT flags, int x, int y, int button)
{
    MSG* msg = CreateMessage(lastMessage, flags, MAKELPARAM(x, y));
#ifndef WINCE
    SendMouseEvent(java_awt_event_MouseEvent_MOUSE_RELEASED, nowMillis(), 
                   x, y, GetJavaModifiers(flags, button), clickCount, 
                   (button == RIGHT_BUTTON), msg);
#else
    ppcKillTHTimer();
    if (wasRightButton) {
	button = RIGHT_BUTTON;
	wasRightButton = 0;
    }
    /*  Left button, so see if VKMENU was set on press  */
    SendMouseEvent(java_awt_event_MouseEvent_MOUSE_RELEASED, nowMillis(), 
                   x, y, GetJavaModifiers(flags, button), clickCount, 
                   ((button == RIGHT_BUTTON) || wasMenuButton), msg);
    //need to reset this here, or menu never goes away */
    wasMenuButton = 0;
#endif
    // If no movement, then report a click following the button release
    //
    if (!m_dragged) {
        SendMouseEvent(java_awt_event_MouseEvent_MOUSE_CLICKED, 
                       nowMillis(), x, y, 
                       GetJavaModifiers(flags, button), clickCount, FALSE);
    }
    m_dragged = FALSE;

    return mrConsume;
}

MsgRouting AwtComponent::WmMouseMove(UINT flags, int x, int y)
{

    // Only report mouse move and drag events if a move or drag actually
    // happened -- Windows sends a WM_MOUSEMOVE in case the app wants to
    // modify the cursor.
    if (lastComp != this || x != lastX || y != lastY) {
        lastComp = this;
        lastX = x;
        lastY = y;
        jint id;
	/* check for tap and hold */
	if (myTimer) {
	  /* distance, square dx and dy and compare to square of aperture */
	  /* saves computing abs. and gives cirular aperture */
	    if (lastComp != this ||
		(((x-thmon.x)*(x-thmon.x)) + ((y-thmon.y)*(y-thmon.y))) >
		  (TAP_AND_HOLD_APERTURE * TAP_AND_HOLD_APERTURE)) {
	    /* we have exited the component, or moved more that 2 pixels */
		  ppcKillTHTimer();	    
	    } else {
		/* mouse has moved, but not very far, track it */
		/* so we send proper coordinates when we do send */
		/* the fake event */
		thmon.lastX = x;
		thmon.lastY = y;
	    }
	}
        if ( flags & ALL_MK_BUTTONS ) {
            m_dragged = TRUE;
            id = java_awt_event_MouseEvent_MOUSE_DRAGGED;
        } 
        else 
            id = java_awt_event_MouseEvent_MOUSE_MOVED;

        MSG* msg = CreateMessage(lastMessage, flags, MAKELPARAM(x, y));
        SendMouseEvent(id, nowMillis(), x, y, 
                       GetJavaModifiers(flags, 0), 0, FALSE, msg);
    } else {
        ::SetCursor(GetCursor(TRUE));
    }
    return mrConsume;
}

MsgRouting AwtComponent::WmMouseExit(UINT flags, int x, int y)
{
#ifdef WINCE
    /* Kill tap and hold timer */
    ppcKillTHTimer();
#endif
    SendMouseEvent(java_awt_event_MouseEvent_MOUSE_EXITED, 
                   nowMillis(), x, y, 0, 0, FALSE);
    return mrConsume;	// Don't pass our synthetic event on!
}

long AwtComponent::GetJavaModifiers(UINT flags, int mouseButton)
{
    int modifiers = 0;

    if ((1<<24) & flags) {
        modifiers |= java_awt_event_InputEvent_META_MASK;
    }	

    if (HIBYTE(::GetKeyState(VK_CONTROL)) || 
        mouseButton == MIDDLE_BUTTON) {
        modifiers |= java_awt_event_InputEvent_CTRL_MASK;
    }
    if (HIBYTE(::GetKeyState(VK_SHIFT))) {
        modifiers |= java_awt_event_InputEvent_SHIFT_MASK;
    }
    
    if (HIBYTE(::GetKeyState(VK_MBUTTON)) || 
        HIBYTE(::GetKeyState(VK_MENU))) {
        modifiers |= java_awt_event_InputEvent_ALT_MASK;
    }
#ifndef WINCE
    // set "right mouse" mask depending on whether system is set to be
    // left or right-handed.
    if (::GetSystemMetrics(SM_SWAPBUTTON)) {
	if (HIBYTE(::GetAsyncKeyState(VK_LBUTTON))) {
	    modifiers |= java_awt_event_InputEvent_META_MASK;
	} 

    } else {
	if (HIBYTE(::GetAsyncKeyState(VK_RBUTTON))) {
	    modifiers |= java_awt_event_InputEvent_META_MASK;
	}
    }
#endif
    // Mouse buttons are already set correctly for left/right handedness
    if (mouseButton == LEFT_BUTTON) {
        modifiers |= java_awt_event_InputEvent_BUTTON1_MASK;
    }
    if (mouseButton == MIDDLE_BUTTON) {
        modifiers |= java_awt_event_InputEvent_ALT_MASK;
    }
    if (mouseButton == RIGHT_BUTTON) {
        modifiers |= java_awt_event_InputEvent_META_MASK;
    }
    return modifiers;
}

UINT AwtComponent::JavaKeyToWindowsKey(UINT wkey)
{
    switch (wkey) {
      case java_awt_event_KeyEvent_VK_DELETE: return VK_DELETE;
      case java_awt_event_KeyEvent_VK_ENTER: return VK_RETURN;
      case java_awt_event_KeyEvent_VK_BACK_SPACE: return VK_BACK;
      case java_awt_event_KeyEvent_VK_TAB: return VK_TAB;
      case java_awt_event_KeyEvent_VK_PRINTSCREEN: return VK_SNAPSHOT;
      case java_awt_event_KeyEvent_VK_INSERT: return VK_INSERT;
      case java_awt_event_KeyEvent_VK_HELP: return VK_HELP;
    }
    return wkey;
}

UINT AwtComponent::WindowsKeyToJavaKey(UINT wkey)
{
    switch (wkey) {
      case VK_DELETE:    return java_awt_event_KeyEvent_VK_DELETE;
      case VK_RETURN:    return java_awt_event_KeyEvent_VK_ENTER;
      case VK_BACK:      return java_awt_event_KeyEvent_VK_BACK_SPACE;
      case VK_TAB:       return java_awt_event_KeyEvent_VK_TAB;
      case VK_SNAPSHOT:  return java_awt_event_KeyEvent_VK_PRINTSCREEN;
      case VK_INSERT:    return java_awt_event_KeyEvent_VK_INSERT;
      case VK_HELP:      return java_awt_event_KeyEvent_VK_HELP;
    }
    return wkey;
}

#ifndef WINCE
UINT AwtComponent::WindowsKeyToJavaChar(UINT wkey, UINT modifiers)
{
    BYTE    keyboardState[AwtToolkit::KB_STATE_SIZE];
    
    UINT    scancode;
    WCHAR   unicodeChar[2];
    char    mbChar[4];
    int     conversionResult;

    scancode = ::MapVirtualKey(wkey, 0);
    AwtToolkit::GetAsyncKeyboardState(keyboardState);
    
    // apply modifiers if necessary
    if (modifiers) {
        if (modifiers & java_awt_Event_SHIFT_MASK) {
            keyboardState[VK_SHIFT] |= 0x80;
        }
        if (modifiers & java_awt_Event_CTRL_MASK)  {
	    //fix for bug #4098210
	    keyboardState[VK_CONTROL] &= ~0x80;
	    if (wkey >= java_awt_event_KeyEvent_VK_A &&
		wkey <= java_awt_event_KeyEvent_VK_Z)  {
		keyboardState[VK_CONTROL] |= 0x80;
	    }
	    if ((((wkey & 0x7F) == java_awt_event_KeyEvent_VK_OPEN_BRACKET ) ||
		 ((wkey & 0x7F) == java_awt_event_KeyEvent_VK_BACK_SLASH   ) ||
		 ((wkey & 0x7F) == java_awt_event_KeyEvent_VK_CLOSE_BRACKET)) && 
		 ( (modifiers & java_awt_Event_SHIFT_MASK) == 0 ) ) {
		keyboardState[VK_CONTROL] |= 0x80;
	    }
	    if ( ( wkey == (UINT)::VkKeyScan('-')        ) && 
		 ( modifiers & java_awt_Event_SHIFT_MASK ) )  {
		keyboardState[VK_CONTROL] |= 0x80;
	    }
        }
        if (modifiers & java_awt_Event_ALT_MASK) {
 	    //fix for bug #4098210
	    if (keyboardState[VK_CONTROL] & 0x80) {
 		keyboardState[VK_MENU] &= ~0x80;
	    } else {
	        keyboardState[VK_MENU] |= 0x80;
	    }
        }
    }
    

    // instead of creating our own conversion tables, I'll let Win32
    // convert the character for me.
    conversionResult = ToAscii(wkey, scancode, keyboardState, (LPWORD)mbChar, 0);

    if (conversionResult == 0) {
        // No translation available -- try known conversions or else punt.
        if (wkey == java_awt_event_KeyEvent_VK_DELETE ||
            wkey == java_awt_event_KeyEvent_VK_ENTER) 
        {
	    return wkey;
        }
	if (wkey >= VK_NUMPAD0 && wkey <= VK_NUMPAD9) 
        {
            return wkey - VK_NUMPAD0 + '0';
        }
	return 0;
    }
    // the caller expects a Unicode character.
    VERIFY(::MultiByteToWideChar(GetCodePage(), MB_PRECOMPOSED, mbChar, 1, unicodeChar, 1));
    return unicodeChar[0];
}
#endif

MsgRouting AwtComponent::WmKeyDown(UINT wkey, UINT repCnt, UINT flags, BOOL system)
{
  //VK_PROCESSKEY is not a valid Java key and has already been consumed.
    if (wkey == VK_PROCESSKEY) return mrDoDefault;

    UINT modifiers = GetJavaModifiers(flags, 0);
    UINT jkey = WindowsKeyToJavaKey(wkey);
#ifndef WINCE
    UINT character = WindowsKeyToJavaChar(wkey, modifiers);
#else
    MSG dummyMsg;
    UINT character;
    
    // this should be put in the queue by TranslateMessage()
    BOOL charMsgResult = ::PeekMessage(&dummyMsg, GetHWnd(), WM_CHAR,
				       WM_CHAR, PM_NOREMOVE);
    
    if (charMsgResult) {
	character = dummyMsg.wParam;
    } else {
	character = java_awt_event_KeyEvent_CHAR_UNDEFINED;
    }
#endif

    globalCharacter = wkey;

    if (modifiers & java_awt_event_InputEvent_CTRL_MASK) {
    // if CTRL and SHIFT are pressed, save the state since WinCE
    // won't report them being pressed when the next key is entered
      if (modifiers & java_awt_event_InputEvent_SHIFT_MASK) {          
        globalCtrlShift = TRUE;
      } else if (globalCtrlShift) {   
    // if the next key was entered and CTRL is pressed, we need
    // to see if globalCtrlShift is true because it could be thta
    // both CTRL and SHIFT are down.       
          modifiers |= java_awt_event_InputEvent_SHIFT_MASK;
      }   
    }
 
    MSG* msg = CreateMessage((system ? WM_SYSKEYDOWN : WM_KEYDOWN), wkey, 
                             MAKELPARAM(repCnt, flags));
    SendKeyEvent(java_awt_event_KeyEvent_KEY_PRESSED, nowMillis(), jkey, 
                 character, modifiers, msg);
    return mrConsume;
}

MsgRouting AwtComponent::WmKeyUp(UINT wkey, UINT repCnt, UINT flags, BOOL system)
{
    if (wkey == VK_PROCESSKEY) return mrDoDefault;

    UINT modifiers = GetJavaModifiers(flags, 0);
    UINT jkey = WindowsKeyToJavaKey(wkey);
#ifndef WINCE
    UINT character = WindowsKeyToJavaChar(jkey, modifiers);
#else // platform limitation in Windows CE
    UINT character = java_awt_event_KeyEvent_CHAR_UNDEFINED;
#endif

    // reset global variables, see description of their use
    // at their declaration in the beginning of the file
    globalCharacter = NULL;
    globalCtrlShift = FALSE;

    MSG* msg = CreateMessage((system ? WM_SYSKEYUP : WM_KEYUP), wkey, 
                             MAKELPARAM(repCnt, flags));
    SendKeyEvent(java_awt_event_KeyEvent_KEY_RELEASED, nowMillis(), jkey,
                 character, modifiers, msg);
    return mrConsume;
}

static BOOL IsDBCSEditClass(AwtComponent *comp)
{
    const TCHAR* className = comp->GetClassName();
#ifndef UNICODE
    if (stricmp(className,"Edit")==0 || stricmp(className,"ListBox")==0
	  || stricmp(className,"ComboBox")==0 ) {
#else
      if (_wcsicmp(className, TEXT("Edit")) == 0 ||
	  _wcsicmp(className, TEXT("ListBox")) == 0 ||
	  _wcsicmp(className,TEXT("ComboBox"))==0 ) {
#endif
      return TRUE;
    } else {
      return FALSE;
    }
}


MsgRouting AwtComponent::WmInputLangChange(UINT charset, UINT hKeyboardLayout)
{
    // Normally we would be able to use charset and TranslateCharSetInfo
    // to get a code page that should be associated with this keyboard
    // layout change. However, there seems to be an NT 4.0 bug associated
    // with the WM_INPUTLANGCHANGE message, which makes the charset parameter
    // unreliable, especially on Asian systems. Our workaround uses the 
    // keyboard layout handle instead.

    TCHAR strCodePage[MAX_ACP_STR_LEN];
    LANGID idLang;
    LCID idLocale;

    // a LANGID is stored in the bottom word of hKeyboardLayout
    idLang = LOWORD(hKeyboardLayout);
    // use the LANGID to create a LCID
    idLocale = MAKELCID(idLang, SORT_DEFAULT);
    // get the ANSI code page associated with this locale
    if (GetLocaleInfo(idLocale, LOCALE_IDEFAULTANSICODEPAGE, strCodePage, sizeof(strCodePage)) > 0 )
    {
        m_CodePage = _ttoi(strCodePage);
    }
    return mrConsume;
}

MsgRouting AwtComponent::WmIMEChar(UINT character, UINT repCnt, UINT flags, BOOL system)
{

    // We will simply create Java events here.
    char mbChar[3] = { '\0', '\0', '\0'};
    WCHAR unicodeChar;

    mbChar[0] = HIBYTE(character)?HIBYTE(character):LOBYTE(character);
    mbChar[1] = HIBYTE(character)?LOBYTE(character):'\0';

    ::MultiByteToWideChar(GetCodePage(), 0, mbChar, -1, &unicodeChar, 1);
    MSG* pMsg = CreateMessage(WM_IME_CHAR, character, MAKELPARAM(repCnt, flags));
    SendKeyEvent(java_awt_event_KeyEvent_KEY_TYPED, nowMillis(), 
                 java_awt_event_KeyEvent_VK_UNDEFINED, unicodeChar, 
                 GetJavaModifiers(flags, 0), pMsg);
    return mrConsume;
}

MsgRouting AwtComponent::WmChar(UINT character, UINT repCnt, UINT flags, BOOL system)
{

    // Will only get WmChar messages with DBCS if we create them for an Edit class 
    // in the WmForwardChar method. These synthesized DBCS chars are ok to pass on 
    // directly to the default window procedure. They've already been filtered through 
    // the Java key event queue. We will never get the trail byte since the edit class 
    // will PeekMessage(&msg, hwnd, WM_CHAR, WM_CHAR, PM_REMOVE).
    // I would like to be able to pass this character off via WM_AWT_FORWARD_BYTE, but 
    // the Edit classes don't seem to like that.
    // Begin pollution
#ifndef UNICODE
    if (IsDBCSLeadByteEx(GetCodePage(), BYTE(character)))
    {  
        return mrDoDefault;
    }
    // End pollution
#endif
  
    // We will simply create Java events here.
    WCHAR unicodeChar = L'\0';
    UINT message = 0;

    message = system?WM_SYSCHAR:WM_CHAR;

    //fix for bug #4098210
    if (character == java_awt_event_KeyEvent_VK_DELETE) {
	if ((::GetKeyState(character) & 0x80) == 0)
	    unicodeChar = java_awt_event_KeyEvent_VK_BACK_SPACE;
    }
    else
    {
#ifndef UNICODE
        ::MultiByteToWideChar(GetCodePage(), 0, (char *)&character, 1, &unicodeChar, 1);
#else
        unicodeChar = character;
#endif
    }

    UINT modifiers = GetJavaModifiers(flags, 0);
    // this is the case where CTRL + SHIFT was pressed, but when WmChar
    // is reached after the next keypress (CTRL + SHIFT + key), WinCE only
    // reports the CTRL key being pressed.  See WmKeyDown for more details.
    if (globalCtrlShift && (modifiers & java_awt_event_InputEvent_CTRL_MASK)) {
        modifiers |= java_awt_event_InputEvent_SHIFT_MASK;    
    }

    MSG* pMsg = CreateMessage(message, character, MAKELPARAM(repCnt, flags));
    SendKeyEvent(java_awt_event_KeyEvent_KEY_TYPED, nowMillis(), 
                 java_awt_event_KeyEvent_VK_UNDEFINED, 
                 (unicodeChar == 0xffff) ? globalCharacter : unicodeChar, 
                 modifiers, pMsg);
    return mrConsume;
}

MsgRouting AwtComponent::WmForwardChar(UINT character, UINT lParam)
{
    // This message is sent from the Java key event handler.
    TCHAR mbChar[2];
    MSG* pMsg;
    mbChar[0] = HIBYTE(character)?HIBYTE(character):LOBYTE(character);
    mbChar[1] = HIBYTE(character)?LOBYTE(character):_T('\0');
    

#ifndef UNICODE
    if (IsDBCSEditClass(this) && mbChar[1])
    {
        // The first WM_CHAR message will get handled by the WmChar, but
        // the second WM_CHAR message will get picked off by the Edit class.
        // WmChar will never see it.
        // If an Edit class gets a lead byte, it immediately calls PeekMessage
        // and pulls the trail byte out of the message queue.
        ::PostMessage(GetHWnd(), WM_CHAR, mbChar[0] & 0x00ff, lParam);
        ::PostMessage(GetHWnd(), WM_CHAR, mbChar[1] & 0x00ff, lParam);
    }
    else
    {
        pMsg = CreateMessage(WM_CHAR, mbChar[0] & 0x00ff, lParam);
        ::PostMessage(GetHWnd(), WM_AWT_FORWARD_BYTE, 0, (LPARAM)pMsg);
        if (mbChar[1])
        {
            pMsg = CreateMessage(WM_CHAR, mbChar[1] & 0x00ff, lParam);
            ::PostMessage(GetHWnd(), WM_AWT_FORWARD_BYTE, 0, (LPARAM)pMsg);
        }
    }
#else
    if (!imeStartComp) 
    {
       pMsg = CreateMessage(WM_CHAR, character, lParam);
       ::PostMessage(GetHWnd(), WM_AWT_FORWARD_BYTE, 0, (LPARAM)pMsg);
    }
#endif
    return mrConsume;
}

MsgRouting AwtComponent::WmCommand(UINT id, HWND hWndChild, UINT notifyCode)
{
    // Menu/Accelerator
#ifndef WINCE
    if (hWndChild == 0) {
#else
    if ((hWndChild == 0) || wceIsCmdBarWnd(hWndChild)) {
#endif
        AwtObject* obj = AwtToolkit::GetInstance().LookupCmdID(id);
        ASSERT(obj);
	ASSERT(((AwtMenuItem*)obj)->GetID() == id);
        obj->DoCommand();
        return mrConsume;
    }
    // Child id notification
    //
    else {
        AwtComponent* child = AwtComponent::GetComponent(hWndChild);
        if (child) {
            child->WmNotify(notifyCode);
        }
    }
    return mrDoDefault;
}

MsgRouting AwtComponent::WmNotify(UINT notifyCode)
{
    return mrDoDefault;
}

MsgRouting AwtComponent::WmCompareItem(UINT ctrlId, 
                                       COMPAREITEMSTRUCT far& compareInfo, 
                                       long& result)
{
    AwtComponent* child = AwtComponent::GetComponent(compareInfo.hwndItem);
    if (child == this) {
        //DoCallback("handleItemDelete",
    }
    else if (child) {
        return child->WmCompareItem(ctrlId, compareInfo, result);
    }
    return mrConsume;
}

MsgRouting AwtComponent::WmDeleteItem(UINT ctrlId, DELETEITEMSTRUCT far& deleteInfo)
{
    CriticalSection::Lock l(GetLock());
    /* Workaround for NT 4.0 bug -- if SetWindowPos is called on a AwtList
     * window, a WM_DELETEITEM message is sent to its parent with a window
     * handle of one of the list's child windows.  The property lookup
     * succeeds, but the HWNDs don't match. */
    if (deleteInfo.hwndItem == NULL) {
        return mrConsume;
    }
#ifndef WINCE
    AwtComponent* child = (AwtComponent*)::GetProp(deleteInfo.hwndItem, 
                                                   ThisProperty());
#else
    AwtComponent* child = (AwtComponent*)::GetWindowLong(deleteInfo.hwndItem,
							 GWL_USERDATA);
#endif
    if (child && child->GetHWnd() != deleteInfo.hwndItem) {
        return mrConsume;
    }

    if (child == this) {
        //DoCallback("handleItemDelete",
    }
    else if (child) {
        return child->WmDeleteItem(ctrlId, deleteInfo);
    }
    return mrConsume;
}
                            
MsgRouting AwtComponent::WmDrawItem(UINT ctrlId, DRAWITEMSTRUCT far& drawInfo)
{
    if (drawInfo.CtlType == ODT_MENU) {
        if (drawInfo.itemData != 0) {
            AwtMenu* menu = (AwtMenu*)(drawInfo.itemData);
	    AWT_TRACE((TEXT("AwtComponent::WmDrawItem %x\n"), menu));
	    ASSERT( !IsBadReadPtr(menu, sizeof(*menu)) );
	    ASSERT( menu->GetHMenu() != NULL);
            menu->DrawItem(drawInfo);
        }
    } else {
        AwtComponent* child = AwtComponent::GetComponent(drawInfo.hwndItem);
        if (child == this) {
          //DoCallback("handleItemDelete",
        }
        else if (child != NULL) {
            return child->WmDrawItem(ctrlId, drawInfo);
        }
    }
    return mrConsume;
}
        
MsgRouting AwtComponent::WmMeasureItem(UINT ctrlId, MEASUREITEMSTRUCT far& measureInfo)
{
    if (measureInfo.CtlType == ODT_MENU) {
        if (measureInfo.itemData != 0) {
            AwtMenu* menu = (AwtMenu*)(measureInfo.itemData);
            HDC hDC = ::GetDC(GetHWnd());
            menu->MeasureItem(hDC, measureInfo);
            ::ReleaseDC(GetHWnd(), hDC);
        }
    } else {
        HWND  hChild = ::GetDlgItem(GetHWnd(), measureInfo.CtlID);
        AwtComponent* child = AwtComponent::GetComponent(hChild);
	// If the parent cannot find the child's instance from its handle,
	// maybe the child is in its creation.  So the child must be searched 
	// from the list linked before the child's creation.
        if (child == NULL)
            child = SearchChild((UINT)ctrlId);
        
        if (child == this) {
            //DoCallback("handleItemDelete", 
        }
        else if (child)
            return child->WmMeasureItem(ctrlId, measureInfo);
    }
    return mrConsume;
}

//for WmDrawItem method of Label, Button and Checkbox
void AwtComponent::DrawWindowText(HDC hDC, jobject hJavaFont, 
				  jstring hJavaString, int x, int y)
{
    int nOldBkMode = ::SetBkMode(hDC,TRANSPARENT);
    ASSERT(nOldBkMode != 0);
    AwtFont::drawMFString(hDC, hJavaFont, hJavaString, x, y);
    VERIFY(::SetBkMode(hDC,nOldBkMode));
}

// Draw text in gray (the color being set to COLOR_GRAYTEXT) when 
// the component is disabled.
// Used only for label, checkbox and button in OWNER_DRAW.
// It draws the text in emboss.
void AwtComponent::DrawGrayText(HDC hDC, jobject hJavaFont, 
			jstring hJavaString, int x, int y)
{
    ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNHIGHLIGHT));
    AwtComponent::DrawWindowText(hDC, hJavaFont, hJavaString, x+1, y+1);
    ::SetTextColor(hDC, ::GetSysColor(COLOR_BTNSHADOW));
    AwtComponent::DrawWindowText(hDC, hJavaFont, hJavaString, x, y);
}

//for WmMeasureItem method of List and Choice
jstring AwtComponent::GetItemString(jobject target, jint index)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return NULL;
    }

    jstring str = NULL;

    if (env->IsInstanceOf(target, WCachedIDs.java_awt_ChoiceClass)) {
	str = (jstring) env->CallObjectMethod(target,
					      WCachedIDs.java_awt_Choice_getItemMID, index);
    } else if (env->IsInstanceOf(target,
				 WCachedIDs.java_awt_ListClass)) {
	str = (jstring) env->CallObjectMethod(target,
					      WCachedIDs.java_awt_List_getItemMID, index);
    }
    // ignore exceptions, which may occur due to race-conditions between
    // when a item is added and displayed.
    return str;
}

//for WmMeasureItem method of List and Choice
void AwtComponent::MeasureListItem(MEASUREITEMSTRUCT far& measureInfo)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }
    
    jobject dimension = PreferredItemSize(env);
    measureInfo.itemWidth = env->GetIntField(dimension,
					     WCachedIDs.java_awt_Dimension_widthFID);
    measureInfo.itemHeight = env->GetIntField(dimension,
					      WCachedIDs.java_awt_Dimension_heightFID);
}

//for WmDrawItem method of List and Choice
void AwtComponent::DrawListItem(DRAWITEMSTRUCT far& drawInfo)
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }

    jobject target = GetTarget();

    HDC hDC = drawInfo.hDC;
    RECT rect = drawInfo.rcItem;
    
    BOOL bEnabled = isEnabled();

    DWORD crBack, crText;
    if (drawInfo.itemState & ODS_SELECTED) {
	// Set background and text colors for selected item
	crBack = ::GetSysColor (COLOR_HIGHLIGHT);
	crText = ::GetSysColor (COLOR_HIGHLIGHTTEXT);
    } else {
	// Set background and text colors for unselected item
        crBack = GetBackgroundColor();
        crText = bEnabled ? GetColor() : ::GetSysColor(COLOR_GRAYTEXT);
    }

    // Fill item rectangle with background color
    AwtBrush* brBack = AwtBrush::Get(crBack);
    ASSERT(brBack->GetHandle());
    VERIFY(::FillRect(hDC, &rect, (HBRUSH)brBack->GetHandle()));
    brBack->Release();
    
    // Set current background and text colors
    ::SetBkColor (hDC, crBack);
    ::SetTextColor (hDC, crText);
    
    //draw string (with left mergin of 1 point)
    if ((int) (drawInfo.itemID) >= 0) {
	    jobject hJavaFont = GET_FONT(target);
	    jstring hJavaString = GetItemString(target,
						drawInfo.itemID);
	    SIZE size = AwtFont::getMFStringSize(hDC,hJavaFont,hJavaString);
	    AwtFont::drawMFString(hDC, hJavaFont, hJavaString,
				  rect.left+1,
				  (rect.top + rect.bottom-size.cy) /
				  2);
    }
    if ((drawInfo.itemState & ODS_FOCUS)  &&
        (drawInfo.itemAction & (ODA_FOCUS | ODA_DRAWENTIRE))) {
        VERIFY(::DrawFocusRect(hDC, &rect));
    }
}

//for MeasureListItem method and WmDrawItem method of Checkbox
jint AwtComponent::GetFontHeight()
{
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return 0L;
    }
    
    jobject self = GetPeer();
    jobject target = env->GetObjectField(self, WCachedIDs.PPCObjectPeer_targetFID);

    jobject hJavaFont = GET_FONT(self);
    jobject toolkit = env->CallObjectMethod(self,
					    WCachedIDs.java_awt_Component_getToolkitMID);
    ASSERT(!env->ExceptionCheck());
    jobject fontMetrics = env->CallObjectMethod(toolkit,
						WCachedIDs.java_awt_Toolkit_getFontMetricsMID,
						hJavaFont);
    ASSERT(!env->ExceptionCheck());
    jint height = env->CallIntMethod(fontMetrics,
				     WCachedIDs.java_awt_FontMetrics_getHeightMID);
    ASSERT(!env->ExceptionCheck());

    return height;
    return 0L;
}

// Forward the print request to the Graphics object.  It's necessary
// to bounce the print request from WGraphics_print to here and back
// to avoid race conditions when sending print messages, by running on 
// the creating thread.
MsgRouting AwtComponent::PrintComponent(AwtGraphics* g)
{
    /* FIX
    g->PrintComponent(this);
    */
    return mrConsume;
}

// If you override WmPrint, make sure to save a copy of the DC on the GDI
// stack to be restored in WmPrintClient. Windows mangles the DC in
// ::DefWindowProc.
MsgRouting AwtComponent::WmPrint(HDC hDC, UINT flags)
{
#ifndef WINCE
    // Save two copies of the DC: one for WmPrintClient and one for
    // us to use after DefWindowProc returns
    ::SaveDC(hDC);
    ::SaveDC(hDC);

    DefWindowProc(WM_PRINT, (WPARAM) hDC, (LPARAM) flags);

    ::RestoreDC(hDC, -1);

    // Special case for components with a sunken border. Windows does not
    // print the border correctly, so we have to do it ourselves.
    if (IS_WIN4X && (::GetWindowLong(GetHWnd(), GWL_EXSTYLE) &
		     WS_EX_CLIENTEDGE)) {
        RECT r;
        ::GetClipBox(hDC, &r);
	::DrawEdge(hDC, &r, EDGE_SUNKEN, BF_RECT);
    }
#endif
    return mrConsume;
}

// If you override WmPrintClient, maker sure to obtain a valid copy of
// the DC from the GDI stack. The copy of the DC should have been placed
// there by WmPrint. Windows mangles the DC in ::DefWindowProc.
MsgRouting AwtComponent::WmPrintClient(HDC hDC, UINT)
{
    // obtain valid DC from GDI stack
    ::RestoreDC(hDC, -1);

    return mrDoDefault;
}

#ifndef WINCE
MsgRouting AwtComponent::WmNcCalcSize(BOOL fCalcValidRects, 
				      LPNCCALCSIZE_PARAMS lpncsp, UINT& retVal)
{
    return mrDoDefault;
}
#endif

MsgRouting AwtComponent::WmNcPaint(HRGN hrgn)
{
    return mrDoDefault;
}

MsgRouting AwtComponent::WmNcHitTest(UINT x, UINT y, UINT& retVal)
{
    return mrDoDefault;
}

MsgRouting AwtComponent::WmQueryNewPalette(UINT& retVal)
{
    /* FIX
    if (AwtImage::m_hPal != NULL) {
        GetSharedDC()->Lock();
        HDC hDC = GetSharedDC()->GrabDC(NULL);
        ASSERT(hDC);
		
		#if defined(NETSCAPE)
		 	BOOL bBackgroundPalette = AwtToolkit::m_bInsideNavigator;
		#else
			BOOL bBackgroundPalette = FALSE;
		#endif // defined (NETSCAPE)

		ASSERT(::SelectPalette(hDC, AwtImage::m_hPal, bBackgroundPalette) != NULL);
		//Netscape : We dont want to invalidate here - RealizePalette will
		//automatically post a WM_PALETTECHANGED message, where we can
		//invalidate if necessary.
		
        ::RealizePalette(hDC);
        GetSharedDC()->Unlock();
        retVal = TRUE;
    } else {
        retVal = FALSE;
    }
    */
    
    return mrDoDefault;
}

MsgRouting AwtComponent::WmPaletteChanged(HWND hwndPalChg)
{
//Netscape : We need to re-realize our palette here (unless we're the one
//that was realizing it in the first place).  That will let us match the
//remaining colors in the system palette as best we can.  We always invalidate
//because the palette will have changed when we recieve this message.

    if (hwndPalChg == GetHWnd()) {
        GetSharedDC()->Lock();
		GetSharedDC()->GrabDC(NULL);
		HDC hDC = GetSharedDC()->GetDC();
		ASSERT(hDC);
		ASSERT(::RealizePalette(hDC) != GDI_ERROR);
		GetSharedDC()->Unlock();
    }
    Invalidate(NULL);
    return mrDoDefault;
}

MsgRouting AwtComponent::WmStyleChanged(WORD wStyleType, LPSTYLESTRUCT lpss)
{
    ASSERT(!IsBadReadPtr(lpss, sizeof(STYLESTRUCT)));
    return mrDoDefault;
}

MsgRouting AwtComponent::WmSettingChange(WORD wFlag, LPCTSTR pszSection)
{
#ifndef WINCE
    ASSERT(!IsBadStringPtr(pszSection, 20));
#endif
    AWT_TRACE((TEXT("WM_SETTINGCHANGE: wFlag=%d pszSection=%s\n"), (int)wFlag, pszSection));
    return mrDoDefault;
}

AwtComponent* AwtComponent::SearchChild(UINT id) {
    ChildListItem* child;
    for (child = m_childList; child != NULL;child = child->m_next) {
	if (child->m_ID == id)
	    return child->m_Component;
    }
    //ASSERT(FALSE); // This should not be happend if all children are recorded
    return NULL;	// To avoid compiler error.
}

void AwtComponent::RemoveChild(UINT id) {
    ChildListItem* child = m_childList;
    ChildListItem* lastChild = NULL;
    while (child != NULL) {
	if (child->m_ID == id) {
	    if (lastChild == NULL) {
		m_childList = child->m_next;
	    } else {
		lastChild->m_next = child->m_next;
	    }
	    child->m_next = NULL;
	    ASSERT(child != NULL);
	    delete child;
	    return;
	}
	lastChild = child;
	child = child->m_next;
    }
}

void AwtComponent::SendKeyEvent(jint id, jlong when, jint raw, jchar cooked,
                                jint modifiers, MSG* msg)
{
    CriticalSection::Lock l(GetLock());
    if (GetPeer() == NULL) {
        // event received during termination.
        return;
    }

    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }

    jobject hEvent =
	env->NewObject(WCachedIDs.java_awt_event_KeyEventClass,
		       WCachedIDs.java_awt_event_KeyEvent_constructorMID,
		       GetTarget(),
		       id,
		       when,
		       modifiers,
		       raw, 
		       cooked);
		       
    ASSERT(!env->ExceptionCheck());
    ASSERT(hEvent != NULL);
    
    env->SetIntField(hEvent, WCachedIDs.java_awt_AWTEvent_dataFID,
		     (jint) msg);
    
    SendEvent(hEvent);
}

void AwtComponent::SendMouseEvent(jint id, jlong when, jint x, jint y, 
                                  jint modifiers, jint clickCount, 
                                  BOOL popupTrigger,
                                  MSG* msg)
{
    CriticalSection::Lock l(GetLock());
    if (GetPeer() == NULL) {
        // event received during termination.
        return;
    }

    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }

    RECT insets;
    GetInsets(&insets);

    jobject hEvent = 
	env->NewObject(WCachedIDs.java_awt_event_MouseEventClass,
		       WCachedIDs.java_awt_event_MouseEvent_constructorMID,
		       GetTarget(),
		       id, 
		       when, 
		       modifiers, 
		       x+insets.left, 
		       y+insets.top, 
		       clickCount, 
		       popupTrigger ? JNI_TRUE : JNI_FALSE);

    ASSERT(!env->ExceptionCheck());
    ASSERT(hEvent != NULL);

    env->SetIntField(hEvent, WCachedIDs.java_awt_AWTEvent_dataFID,
		     (jint) msg);
    
    SendEvent(hEvent);
}

void AwtComponent::SendFocusEvent(jint id, BOOL temporary)
{
    CriticalSection::Lock l(GetLock());
    if (GetPeer() == NULL) {
        // event received during termination.
        return;
    }

    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }

    jobject hEvent = 
	env->NewObject(WCachedIDs.java_awt_event_FocusEventClass,
		       WCachedIDs.java_awt_event_FocusEvent_constructorMID,
		       GetTarget(),
		       id, 
		       ((temporary) ? JNI_TRUE : JNI_FALSE));

    ASSERT(!env->ExceptionCheck());
    ASSERT(hEvent != NULL);

    SendEvent(hEvent);
}

// Forward a filtered event directly to the subclassed window.
// This method is needed so that DefWindowProc is invoked on the 
// component's owning thread.
MsgRouting AwtComponent::HandleEvent(MSG* msg)
{
    DefWindowProc(msg->message, msg->wParam, msg->lParam);
    delete msg;
    return mrConsume;
}

void AwtComponent::SynthesizeKeyMessage(jobject hEvent)
{
    UINT message;

    JNIEnv *env;

    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }

    jint id = env->CallIntMethod(hEvent,
				 WCachedIDs.java_awt_AWTEvent_getIDMID);
    jchar keyChar = env->CallCharMethod(hEvent,
					WCachedIDs.java_awt_event_KeyEvent_getKeyCharMID);
    jint keyCode = env->CallIntMethod(hEvent,
				      WCachedIDs.java_awt_event_KeyEvent_getKeyCodeMID);

    switch (id) {
      case java_awt_event_KeyEvent_KEY_PRESSED:
	  message = WM_KEYDOWN; 
	  break;
      case java_awt_event_KeyEvent_KEY_RELEASED:
	  message = WM_KEYUP; 
	  break;
      case java_awt_event_KeyEvent_KEY_TYPED:
	  message = WM_CHAR; 
	  break;
      default:
	  return;
    }

    // KeyEvent.modifiers aren't supported -- the Java app must send separate
    // KEY_PRESSED and KEY_RELEASED events for the modifier virtual keys.

    if (id == java_awt_event_KeyEvent_KEY_TYPED) {
	// Convert unicode character to DBCS chars, and send each DBCS char as
	// a separate message.
	UINT forwardChar;

#ifndef UNICODE
	char mbs[2];	
	int retValue = ::WideCharToMultiByte(GetCodePage(), 0, 
					    (WCHAR*)&keyChar),
					    1, mbs, 2, NULL, NULL);
	if (retValue == 2)
        {
            // if DBCS, then we'll package wParam up like wParam in a WM_IME_CHAR
            // message
            forwardChar = ((mbs[0] & 0x00FF) << 8) | (mbs[1] & 0x00FF);
        }
	else
        {
            forwardChar = mbs[0] & 0x00ff;
	}
#else
//        char * str = "SynthesizeKeyMessage:";
//        (void) consoleWrite(str, strlen(str));
	forwardChar = keyChar;
#endif

        ::PostMessage(GetHWnd(), WM_AWT_FORWARD_CHAR, forwardChar, 0);

    } else {
        UINT key = AwtComponent::JavaKeyToWindowsKey(keyCode);
        MSG* msg = CreateMessage(message, key, 0);
	::PostMessage(GetHWnd(), WM_AWT_HANDLE_EVENT, 0, (LPARAM)msg);
    }
}

void AwtComponent::SynthesizeMouseMessage(jobject hEvent)
{
    enum Buttons { left, middle, right } button;

    JNIEnv *env;

    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }

    jint id = env->CallIntMethod(hEvent,
				 WCachedIDs.java_awt_AWTEvent_getIDMID);
    jint modifiers = env->CallIntMethod(hEvent,
					WCachedIDs.java_awt_event_InputEvent_getModifiersMID);
    jint x = env->CallIntMethod(hEvent,
				WCachedIDs.java_awt_event_MouseEvent_getXMID);
    jint y = env->CallIntMethod(hEvent,
				WCachedIDs.java_awt_event_MouseEvent_getYMID);

    if ((modifiers & java_awt_event_InputEvent_META_MASK) > 0) {
	button = right;
    } else if ((modifiers & java_awt_event_InputEvent_ALT_MASK) > 0) {
	button = middle;
    } else {
	button = left;
    }

    UINT message;
    switch (id) {
      case java_awt_event_MouseEvent_MOUSE_PRESSED: {
	  switch (button) {
            case left:   message = WM_LBUTTONDOWN; break;
            case middle: message = WM_MBUTTONDOWN; break;
            case right:  message = WM_RBUTTONDOWN; break;
	  }
	  break;
      }
      case java_awt_event_MouseEvent_MOUSE_RELEASED: {
	  switch (button) {
            case left:   message = WM_LBUTTONUP; break;
            case middle: message = WM_MBUTTONUP; break;
            case right:  message = WM_RBUTTONUP; break;
	  }
	  break;
      }
      case java_awt_event_MouseEvent_MOUSE_MOVED:
      // MOUSE_DRAGGED events must first have sent a MOUSE_PRESSED event.
      case java_awt_event_MouseEvent_MOUSE_DRAGGED:
	  message = WM_MOUSEMOVE;
	  break;
      default:
	  return;
    }

    MSG* msg = CreateMessage(message, 0, 
                             MAKELPARAM(x, y));
    ::PostMessage(GetHWnd(), WM_AWT_HANDLE_EVENT, 0, (LPARAM)msg);
}

void AwtComponent::Invalidate(RECT* r)
{
    ::InvalidateRect(GetHWnd(), r, FALSE);
}

void AwtComponent::BeginValidate()
{
    ASSERT(m_validationNestCount >= 0 &&
    	   m_validationNestCount < 1000); // sanity check

    if (m_validationNestCount == 0) {
    // begin deferred window positioning if we're not inside
    // another Begin/EndValidate pair
	ASSERT(m_hdwp == NULL);
	m_hdwp = ::BeginDeferWindowPos(32);
    }

    m_validationNestCount++;
}

void AwtComponent::EndValidate()
{
    ASSERT(m_validationNestCount > 0 &&
    	   m_validationNestCount < 1000); // sanity check
    ASSERT(m_hdwp != NULL);

    m_validationNestCount--;
    if (m_validationNestCount == 0) {
    // if this call to EndValidate is not nested inside another
    // Begin/EndValidate pair, end deferred window positioning
        ::EndDeferWindowPos(m_hdwp);
        m_hdwp = NULL;
    }
}

////////////////////////////////////////
// HWND, AwtComponent and Java Peer interaction

// Link the C++, Java peer, and HWNDs together.
//
void AwtComponent::LinkObjects(JNIEnv* env, jobject hPeer)
{
    CriticalSection::Lock l(GetLock());

    // Any caller should hold the objects CRITICALSECTION.
    // Bind all three objects together thru this C++ object, two-way to each:
    //     JavaPeer <-> C++ <-> HWND
    //
    
    if (m_peerObject == NULL) {
	m_peerObject = env->NewGlobalRef(hPeer); // C++ -> JavaPeer
    }

    SET_PDATA(hPeer, (jint)this);                                // JavaPeer -> C++
#ifndef WINCE
    VERIFY(::SetProp(GetHWnd(), ThisProperty(), (HANDLE)this)); // HWND -> C++
#else
    SetWindowLong(GetHWnd(), GWL_USERDATA, (LONG)this);
#endif
}

void AwtComponent::UnlinkObjects()
{
    CriticalSection::Lock l(GetLock());

    // Any caller should hold the objects CRITICALSECTION.
    // Cleanup above binding
    //
    if (m_hwnd) {
#ifndef WINCE
        VERIFY(::RemoveProp(m_hwnd, ThisProperty()) != NULL);
#else
	SetWindowLong(m_hwnd, GWL_USERDATA, NULL);
#endif
    }
        // Note: order of operations should be the reverse of the link operation.
    if (m_peerObject) {
	JNIEnv *env;
	
	if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	    return;
	}
	
	SET_PDATA(m_peerObject, 0);
	// DeleteGlobalRef
	env->DeleteGlobalRef(m_peerObject);
        m_peerObject = NULL;
    }

    // Also cleanup any after-the-fact binding
    if (m_sharedDC) {
        AwtSharedDC* sdc = m_sharedDC;
        m_sharedDC = NULL;
        sdc->Detach(NULL);
    }
}

/////////////////////////////////////////////////////////////////////////////
// Diagnostic routines
#ifdef DEBUG

void AwtComponent::VerifyState()
{
/* FIX
    JNIEnv *env;
    
    if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	return;
    }
    
    if (AwtToolkit::GetInstance().VerifyComponents() == FALSE) {
        return;
    }

    if (m_callbacksEnabled == FALSE) {
        return;
    }

    // Get target bounds.
    jobject hTarget = GetTarget();
    long x = env->GetIntField(hTarget, WCachedIDs.java_awt_Component_xFID);
    long y = env->GetIntField(hTarget, WCachedIDs.java_awt_Component_yFID);
    long width = env->GetIntField(hTarget, WCachedIDs.java_awt_Component_widthFID);
    long height = env->GetIntField(hTarget, WCachedIDs.java_awt_Component_heightFID);

    // Convert target origin to absolute coordinates
    jobject hComp = hTarget;
    while (TRUE) {
        hParent = env->GetObjectField(hComp,
				      WCachedIDs.java_awt_Component_parentFID);
        if (hParent == NULL) {
            break;
        }
        x += env->GetIntField(hParent, WCachedIDs.java_awt_Component_xFID);
        y += env->GetIntField(hParent, WCachedIDs.java_awt_Component_xFID);

        // If this component has insets, factor them in, but ignore
        // top-level windows.
        if (unhand(hParent)->parent != NULL) {
            jobject peer = (Hsun_awt_windows_WPanelPeer*)
		GetPeerForTarget((HObject*)hParent);
            if (peer != NULL && IsInstanceOf((HObject*)peer, 
                                             "sun/awt/windows/WPanelPeer")) {
                jobject insets = (Hjava_awt_Insets*)
                    execute_java_dynamic_method(EE(), (HObject*)peer, "insets", 
                                                "()Ljava/awt/Insets;");
                x += unhand(insets)->left;
                y += unhand(insets)->top;
            }
        }

        hComp = hParent;
    }

    // Test whether component's bounds match the native window's
    RECT rect;
    VERIFY(::GetWindowRect(GetHWnd(), &rect));
#if 0
    ASSERT( (x == rect.left) && 
            (y == rect.top) &&
            (width == (rect.right-rect.left)) && 
            (height == (rect.bottom-rect.top)) );
#else
    BOOL fSizeValid = ( (x == rect.left) && 
            (y == rect.top) &&
            (width == (rect.right-rect.left)) && 
            (height == (rect.bottom-rect.top)) );
#endif

    // See if visible state matches
    long targetVisible = execute_java_dynamic_method(EE(), GetTarget(), 
                                                     "isShowing", "()Z");
    ASSERT(!env->ExceptionCheck());
    BOOL wndVisible = ::IsWindowVisible(GetHWnd());
#if 0
    ASSERT( (targetVisible && wndVisible) ||
            (!targetVisible && !wndVisible) );
#else
    BOOL fVisibleValid = ( (targetVisible && wndVisible) ||
            (!targetVisible && !wndVisible) );
#endif

    // Check enabled state
    BOOL wndEnabled = ::IsWindowEnabled(GetHWnd());
#if 0
    ASSERT( (target->enabled && wndEnabled) ||
            (!target->enabled && !wndEnabled) );
#else
    BOOL fEnabledValid = ( (target->enabled && wndEnabled) ||
            (!target->enabled && !wndEnabled) );

    if (!fSizeValid || !fVisibleValid || !fEnabledValid) {
        printf("AwtComponent::ValidateState() failed:\n");
        jstring targetStr =  (Hjava_lang_String*)
            execute_java_dynamic_method(EE(), GetTarget(), "getName", 
                                        "()Ljava/lang/String;");
        ASSERT(!env->ExceptionCheck());
        printf("\t%S\n", GET_WSTRING(targetStr));
        printf("\twas:       [%d,%d,%dx%d]\n", x, y, width, height);
        if (!fSizeValid) {
            printf("\tshould be: [%d,%d,%dx%d]\n", rect.left, rect.top,
                   rect.right-rect.left, rect.bottom-rect.top);
        }
        if (!fVisibleValid) {
            printf("\tshould be: %s\n", 
                   (targetVisible) ? "visible" : "hidden");
        }
        if (!fEnabledValid) {
            printf("\tshould be: %s\n", 
                   (target->enabled) ? "enabled" : "disabled");
        }
    }
#endif
*/
}
#endif //DEBUG
