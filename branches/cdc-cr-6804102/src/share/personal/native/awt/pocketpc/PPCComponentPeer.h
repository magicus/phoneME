/*
 * @(#)PPCComponentPeer.h	1.9 06/10/10
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
#ifndef _WINCECOMPONENT_PEER_H_
#define _WINCECOMPONENT_PEER_H_

#include "awt.h"

#include "awtmsg.h"
#include "PPCObjectPeer.h"
#include "PPCFontPeer.h"
#include "PPCBrush.h"
#include "PPCPen.h"

#include "java_awt_Component.h"
#include "sun_awt_pocketpc_PPCComponentPeer.h"
#include "java_awt_event_KeyEvent.h"
#include "java_awt_event_FocusEvent.h"
#include "java_awt_event_MouseEvent.h"
#include "java_awt_event_WindowEvent.h"
#include "java_awt_Dimension.h"

extern const char* szAwtComponentClassName;
extern ATOM thisPropertyAtom;

const UINT IGNORE_KEY = (UINT)-1;
const UINT MAX_ACP_STR_LEN = 7; // ANSI CP identifiers are no longer than this

#define LEFT_BUTTON 1
#define MIDDLE_BUTTON 2
#define RIGHT_BUTTON 3

class AwtSharedDC;
class AwtPopupMenu;

#ifdef WINCE
#include <wceCompat.h>
#endif


// Message routing codes
//
enum MsgRouting {
    mrPassAlong,    // pass along to next in chain
    mrDoDefault,    // skip right to underlying default behavior
    mrConsume,      // consume msg & terminate routing immediatly, don't pass anywhere
};

class AwtComponent : public AwtObject {
public:
    enum {
	// combination of all mouse button flags
	ALL_MK_BUTTONS = MK_LBUTTON|MK_MBUTTON|MK_RBUTTON
    };

    AwtComponent();
    virtual ~AwtComponent();

    //////// Static component subsystem initialization & termination

    static void Init();
    static void Term();

    //////// Dynamic class registration & creation

    virtual void RegisterClass();
    virtual void UnregisterClass();
    virtual const TCHAR* GetClassName() = 0;
    void CreateHWnd(const TCHAR *title, long windowStyle, long windowExStyle,
                    int x, int y, int w, int h,
                    HWND hWndParent, HMENU hMenu, 
                    COLORREF colorForeground, COLORREF colorBackground,
                    jobject peer);
    void DestroyHWnd();
    void SubclassWindow();
    void UnsubclassWindow();

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, 
	WPARAM wParam, LPARAM lParam);
    INLINE static char* ThisProperty() { 
	return (char*)MAKELONG(thisPropertyAtom, 0); 
    }

    //////// Access to the various objects of this aggregate component
    
    INLINE HWND GetHWnd() { return m_hwnd; }
    INLINE void SetHWnd(HWND hwnd) { m_hwnd = hwnd; }

    INLINE static AwtComponent* GetComponent(HWND hWnd) {
#ifndef WINCE
#ifdef DEBUG
        if (hWnd == NULL) {
            return NULL;
        }
        AwtComponent* c = (AwtComponent*)::GetProp(hWnd, ThisProperty());
        ASSERT(c == NULL || c->GetHWnd() == hWnd);
        return c;
#else
        return hWnd ? (AwtComponent*)::GetProp(hWnd, ThisProperty()) : 0;
#endif
#else /* WINCE */
        /* WINCE has no GetProp, so we store it in USERDATA */
        /* but command bar has it's own private data */
        if ( (hWnd != NULL) /*FIX && (!ppcIsCmdBarWnd(hWnd)) */) {
	    return (AwtComponent*)::GetWindowLong(hWnd, GWL_USERDATA);
        } else {
            return 0;
        } 
#endif /* WINCE */
    }

    // Return the associated AWT peer object.
    INLINE jobject GetPeer() { 
	return m_peerObject; 
    }

    INLINE CriticalSection& GetLock() { return m_lock; }

    //////// Access to the properties of the component

    INLINE COLORREF GetColor() { return m_colorForeground; }
    void SetColor(COLORREF c);
    HPEN GetForegroundPen();

    COLORREF GetBackgroundColor();
    void SetBackgroundColor(COLORREF c);
    HBRUSH GetBackgroundBrush();
    INLINE BOOL IsBackgroundColorSet() { return m_backgroundColorSet; }

    virtual void SetFont(AwtFont *pFont);

    INLINE void SetText(const TCHAR* text) { ::SetWindowText(GetHWnd(), text); }
    INLINE int GetText(TCHAR *buffer, int size) { 
        return ::GetWindowText(GetHWnd(), buffer, size); 
    }
    INLINE int GetTextLength() { return ::GetWindowTextLength(GetHWnd()); }

    virtual void GetInsets(RECT* rect) { 
        VERIFY(::SetRectEmpty(rect));
    }

    static HWND GetTopLevelParentForWindow(HWND hwndDescendant);

    //////// Access to related components & other objects

    AwtSharedDC* GetSharedDC();

    // Returns the parent component.  If no parent window, or the parent
    // window isn't an AwtComponent, returns NULL.
    AwtComponent* GetParent();

    // Get the component's immediate container.
    class AwtWindow* GetContainer();

    // Is a component a container? Used by above method
    virtual BOOL IsContainer() { return FALSE;} // Plain components can't

    // Returns an increasing unsigned value used for child control IDs.  
    // There is no attempt to reclaim command ID's.
    INLINE UINT CreateControlID() { return m_nextControlID++; }

    // Returns the current cursor.  If NULL, returns parent's cursor.
    // Top-level windows always have a default cursor defined.
    INLINE HCURSOR GetCursor(BOOL checkParent) {
        return (checkParent && m_hCursor == NULL && GetParent()) ? 
            GetParent()->GetCursor(TRUE) : m_hCursor;
    }

    // returns the current code page that should be used in 
    // all MultiByteToWideChar and WideCharToMultiByte calls.
    // This code page should also be use in IsDBCSLeadByteEx.
    INLINE static UINT GetCodePage()
    {
        return m_CodePage;
    }

    //////// methods on this component
    
    virtual void Show();
    virtual void Hide();
    virtual void Reshape(int x, int y, int w, int h);

    // Sets the scrollbar values.  'bar' can be either SB_VERT or SB_HORZ.
    // 'min', 'value', and 'max' can have the value INT_MAX which means that the
    // value should not be changed.
    void SetScrollValues(UINT bar, int min, int value, int max);

    INLINE LRESULT SendMessage(UINT msg, WPARAM wParam=0, LPARAM lParam=0) {
        ASSERT(GetHWnd());
        return ::SendMessage(GetHWnd(), msg, wParam, lParam);
    }
    INLINE long GetStyle() {
        ASSERT(GetHWnd());
        return ::GetWindowLong(GetHWnd(), GWL_STYLE);
    }
    INLINE void SetStyle(long style) {
        ASSERT(GetHWnd());
        // SetWindowLong() error handling as recommended by Win32 API doc.
        ::SetLastError(0);
        LONG ret = ::SetWindowLong(GetHWnd(), GWL_STYLE, style);
        ASSERT(ret != 0 || ::GetLastError() == 0);
    }
    INLINE long GetStyleEx() {
        ASSERT(GetHWnd());
        return ::GetWindowLong(GetHWnd(), GWL_EXSTYLE);
    }
    INLINE void SetStyleEx(long style) {
        ASSERT(GetHWnd());
        // SetWindowLong() error handling as recommended by Win32 API doc.
        ::SetLastError(0);
        LONG ret = ::SetWindowLong(GetHWnd(), GWL_EXSTYLE, style);
        ASSERT(ret != 0 || ::GetLastError() == 0);
    }

    INLINE void SetCursor(HCURSOR hCursor) { 
        ASSERT(hCursor); 
	m_hCursor = hCursor;
        ::SetCursor(m_hCursor);
    }

    virtual BOOL NeedDblClick() { return FALSE; }

    //for multifont component
    static void DrawWindowText(HDC hDC, jobject javaFont, 
			       jstring javaString, int x, int y);
    static void DrawGrayText(HDC hDC, jobject javaFont, 
			     jstring javaString, int x, int y);
    void DrawListItem(DRAWITEMSTRUCT far& drawInfo);
    void MeasureListItem(MEASUREITEMSTRUCT far& measureInfo);
    jstring GetItemString(jobject target, jint index);
    jint GetFontHeight();
    virtual jobject PreferredItemSize(JNIEnv* env) {ASSERT(FALSE); return NULL; }

    INLINE BOOL isEnabled() {
	JNIEnv *env;
	if (JVM->AttachCurrentThread((void **) &env, 0) != 0) {
	    return FALSE;
	}

        jobject self = GetPeer();
        jobject target = env->GetObjectField(self, WCachedIDs.PPCObjectPeer_targetFID);

        BOOL e = (BOOL)env->CallBooleanMethod(target,
					      WCachedIDs.java_awt_Component_isEnabledMID);
        ASSERT(!env->ExceptionCheck());
        return e;
    }

    // Allocate and initialize a new java.awt.event.KeyEvent, and 
    // post it to the peer's target object.  No response is expected 
    // from the target.
    void SendKeyEvent(jint id, jlong when, jint raw, jchar cooked,
                      jint modifiers, MSG* msg=NULL);

    // Allocate and initialize a new java.awt.event.MouseEvent, and 
    // post it to the peer's target object.  No response is expected 
    // from the target.
    void SendMouseEvent(jint id, jlong when, jint x, jint y, 
                        jint modifiers, jint clickCount, 
			BOOL popupTrigger, MSG* msg=NULL);

    // Allocate and initialize a new java.awt.event.FocusEvent, and 
    // post it to the peer's target object.  No response is expected 
    // from the target.
    void SendFocusEvent(jint id, BOOL temporary=FALSE);

    // Allocate and initialize a new java.awt.event.WindowEvent, and 
    // post it to the peer's target object.  No response is expected 
    // from the target.
    void SendWindowEvent(jint id, MSG* msg=NULL);

    // Forward a filtered event directly to the subclassed window.
    virtual MsgRouting HandleEvent(MSG* msg);

    // Event->message synthesizer methods.
    void SynthesizeKeyMessage(jobject hEvent);
    void SynthesizeMouseMessage(jobject hEvent);

    // Invalidate the specified rectangle.
    virtual void Invalidate(RECT* r);

    // Begin and end deferred window positioning.
    virtual void BeginValidate();
    virtual void EndValidate();

    // Keyboard conversion routines.
    static long GetJavaModifiers(UINT flags, int mouseButton);
    static UINT WindowsKeyToJavaKey(UINT wkey);
    static UINT JavaKeyToWindowsKey(UINT jkey);
#ifndef WINCE
    UINT WindowsKeyToJavaChar(UINT wkey, UINT modifiers);
#endif

    //////// Windows message handler functions

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT DefWindowProc(UINT msg, WPARAM wParam, LPARAM lParam);

    virtual MsgRouting PreProcessMsg(MSG& msg); // return true if msg is processed

    virtual MsgRouting WmCreate() {return mrDoDefault;}
    virtual MsgRouting WmClose() {return mrDoDefault;}
    virtual MsgRouting WmDestroy();
    virtual MsgRouting WmActivate(UINT nState, BOOL fMinimized) {return mrDoDefault;}

    virtual MsgRouting WmEraseBkgnd(HDC hDC, BOOL& didErase) {return mrDoDefault;}
    virtual MsgRouting WmPaint(HDC hDC);
    virtual MsgRouting WmMove(int x, int y);
    virtual MsgRouting WmSize(int type, int w, int h);
    virtual MsgRouting WmShowWindow(int show, long status);
    virtual MsgRouting WmSetFocus(HWND hWndLost);
    virtual MsgRouting WmKillFocus(HWND hWndGot);

    virtual MsgRouting WmCtlColor(HDC hDC, HWND hCtrl, UINT ctlColor, HBRUSH& retBrush);
    virtual MsgRouting WmHScroll(UINT scrollCode, UINT pos, HWND hScrollBar);
    virtual MsgRouting WmVScroll(UINT scrollCode, UINT pos, HWND hScrollBar);
    virtual MsgRouting WmSetCursor(HWND hWnd, UINT hitTest, UINT message, 
                                   BOOL& retVal);
    
    virtual MsgRouting WmMouseEnter(UINT flags, int x, int y);
    virtual MsgRouting WmMouseDown(UINT flags, int x, int y, int button);
    virtual MsgRouting WmMouseUp(UINT flags, int x, int y, int button);
    virtual MsgRouting WmMouseMove(UINT flags, int x, int y);
    virtual MsgRouting WmMouseExit(UINT flags, int x, int y);

    virtual MsgRouting WmKeyDown(UINT character, UINT repCnt, UINT flags, BOOL system);
    virtual MsgRouting WmKeyUp(UINT character, UINT repCnt, UINT flags, BOOL system);
    virtual MsgRouting WmChar(UINT character, UINT repCnt, UINT flags, BOOL system);
    virtual MsgRouting WmIMEChar(UINT character, UINT repCnt, UINT flags, BOOL system);
    virtual MsgRouting WmInputLangChange(UINT charset, UINT hKeyBoardLayout);
    virtual MsgRouting WmForwardChar(UINT character, UINT lParam);

    virtual MsgRouting WmTimer(UINT timerID) {return mrDoDefault;}

    virtual MsgRouting WmCommand(UINT id, HWND hWndCtrl, UINT notifyCode);
    virtual MsgRouting WmNotify(UINT notifyCode);  // reflected WmCommand from parent

    virtual MsgRouting WmCompareItem(UINT /*ctrlId*/, 
                                     COMPAREITEMSTRUCT far& compareInfo, 
                                     long& result);
    virtual MsgRouting WmDeleteItem(UINT /*ctrlId*/, 
                                    DELETEITEMSTRUCT far& deleteInfo);
    virtual MsgRouting WmDrawItem(UINT /*ctrlId*/, 
                                  DRAWITEMSTRUCT far& drawInfo);
    virtual MsgRouting WmMeasureItem(UINT ctrlId, 
                                     MEASUREITEMSTRUCT far& measureInfo);

    virtual MsgRouting WmPrint(HDC hDC, UINT flags);
    virtual MsgRouting WmPrintClient(HDC hDC, UINT flags);
    virtual MsgRouting PrintComponent(class AwtGraphics* g);

#ifndef WINCE
    virtual MsgRouting WmNcCalcSize(BOOL fCalcValidRects, 
				    LPNCCALCSIZE_PARAMS lpncsp, UINT& retVal);
#endif /* WINCE */
    virtual MsgRouting WmNcPaint(HRGN hrgn);
    virtual MsgRouting WmNcHitTest(UINT x, UINT y, UINT& retVal);
    virtual MsgRouting WmSysCommand(UINT uCmdType, UINT xPos, UINT yPos);
    virtual MsgRouting WmExitSizeMove();
    virtual MsgRouting WmEnterMenuLoop(BOOL isTrackPopupMenu);
    virtual MsgRouting WmExitMenuLoop(BOOL isTrackPopupMenu);

    virtual MsgRouting WmQueryNewPalette(UINT& retVal);
    virtual MsgRouting WmPaletteChanged(HWND hwndPalChg);
    virtual MsgRouting WmStyleChanged(WORD wStyleType, LPSTYLESTRUCT lpss);
    virtual MsgRouting WmSettingChange(WORD wFlag, LPCTSTR pszSection);

    //////// HWND, AwtComponent and Java Peer interaction

    // Link the C++, Java peer, and HWNDs together.
    void LinkObjects(JNIEnv* env, jobject hPeer);
    void UnlinkObjects();

#ifdef DEBUG
    virtual void VerifyState(); // verify component and peer are in sync.
#else
    void VerifyState() {}       // no-op
#endif

protected:
    HWND     m_hwnd;
    UINT     m_myControlID;	// its own ID from the view point of parent
    BOOL     m_backgroundColorSet;

private:
    COLORREF m_colorForeground;
    COLORREF m_colorBackground;

    AwtPen*  m_penForeground;
    AwtBrush* m_brushBackground;

    AwtSharedDC* m_sharedDC;

    WNDPROC  m_DefWindowProc;

    UINT     m_nextControlID;   // provides a unique ID for child controls

    HCURSOR  m_hCursor;

    CriticalSection m_lock;

    // DeferWindowPos handle for batched-up window positioning
    HDWP     m_hdwp;
    // Counter to handle nested calls to Begin/EndValidate
    UINT     m_validationNestCount;


    // when we process WM_INPUTLANGCHANGE we need to change the input
    // codepage. all MultiByteToWideChar and WideCharToMultiByte
    // calls should use this code page
    static UINT    m_CodePage;

    
private:
    // The association list of children's IDs and corresponding components. 
    // Some components like Choice or List are required their sizes while
    // the creations of themselfs are in progress.
    class ChildListItem {
    public:
        ChildListItem(UINT id, AwtComponent* component) {
            m_ID = id;
            m_Component = component;
            m_next = NULL;
        }
        ~ChildListItem() {
            if (m_next != NULL)
                delete m_next;
        }

        UINT m_ID;
        AwtComponent* m_Component;
        ChildListItem* m_next;
    };

public:
    INLINE void PushChild(UINT id, AwtComponent* component) {
        ChildListItem* child = new ChildListItem(id, component);
        child->m_next = m_childList;
        m_childList = child;
    }


private:
    AwtComponent* SearchChild(UINT id);
    void RemoveChild(UINT id) ;

    ChildListItem* m_childList;
};

#include "ObjectList.h"

#endif
