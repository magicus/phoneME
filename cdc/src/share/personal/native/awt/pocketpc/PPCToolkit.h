/*
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

#ifndef _WINCE_TOOLKIT_H
#define _WINCE_TOOLKIT_H

#include "awt.h"
#include "awtmsg.h"

class AwtObject;
class AwtDialog;
class AwtCmdIDList;

//
// class CriticalSection
// ~~~~~ ~~~~~~~~~~~~~~~~
// Lightweight intra-process thread synchronization. Can only be used with
// other critical sections, and only within the same process.
//
class CriticalSection {
  public:
    // ctor argument provides a static string tag for tracing when enabled,
    // unused otherwise
#ifdef CSDEBUG
    INLINE  CriticalSection(const char* name) { ::InitializeCriticalSection(&rep); this->name = name; }
#else
    INLINE  CriticalSection(const char*) { ::InitializeCriticalSection(&rep); }
#endif
    INLINE ~CriticalSection() { ::DeleteCriticalSection(&rep); }

    class Lock {
        public:
            INLINE Lock(const CriticalSection& cs) 
	        : critSec(cs)
	    {
	        const_cast<CriticalSection &>(critSec).Enter();
	    } 
	    INLINE ~Lock()
	    { 
	        const_cast<CriticalSection &>(critSec).Leave();
	    } 

        private:
	    const CriticalSection& critSec;
    };
    friend class Lock;

  private:
    CRITICAL_SECTION rep;
#ifdef CSDEBUG
    const char* name;
#endif

    CriticalSection(const CriticalSection&);
    const CriticalSection& operator =(const CriticalSection&);

  public:
    void    Enter           (void)
    {
        AWT_CS_TRACE((stderr, "> %2X locking  (cnt:%d,rec:%d,own:%X) [%s]\n", ::GetCurrentThreadId(),
		      rep.LockCount, rep.RecursionCount, rep.OwningThread, name));
        ::EnterCriticalSection(&rep);
	AWT_CS_TRACE((stderr, "> %2X  locked  (cnt:%d,rec:%d,own:%X) [%s]\n", ::GetCurrentThreadId(),
		      rep.LockCount, rep.RecursionCount, rep.OwningThread, name));
    }
    void    Leave           (void)
    {
        ::LeaveCriticalSection(&rep);
	AWT_CS_TRACE((stderr, "> %2X unlocked (cnt:%d,rec:%d,own:%X) [%s]\n", ::GetCurrentThreadId(),
		      rep.LockCount, rep.RecursionCount, rep.OwningThread, name));
    }
};

//
// Class to encapsulate the extraction of the java string contents into a buffer
// and the cleanup of the buffer
//
class JavaStringBuffer {
  public:
    JavaStringBuffer(JNIEnv* env, jstring s);
    INLINE ~JavaStringBuffer() { delete[] buffer; }
    INLINE operator TCHAR*() {return buffer;}
    INLINE operator LPARAM() {return (LPARAM)buffer;}  // for SendMessage

  private:
    TCHAR* buffer;
};

class AwtToolkit {
public:
    enum {
	KB_STATE_SIZE = 256
    };

    AwtToolkit();
   ~AwtToolkit();

    BOOL Initialize(JNIEnv *env, BOOL localPump);
    void Finalize();

    INLINE BOOL localPump() { return m_localPump; }
    INLINE BOOL Verbose() { return m_verbose; }
    INLINE BOOL VerifyComponents() { return m_verifyComponents; }
    INLINE BOOL BreakOnError() { return m_breakOnError; }
    INLINE HWND GetHWnd() { return m_toolkitHWnd; }

    INLINE HANDLE GetModuleHandle() { return m_dllHandle; }
    INLINE void SetModuleHandle(HANDLE h) { m_dllHandle = h; }

    INLINE static DWORD MainThread() { return GetInstance().m_mainThreadId; }
    static void Cleanup();
    static void AtExit();
    static UINT GetAsyncMouseKeyState();
    static void GetAsyncKeyboardState(PBYTE keyboardState);

    static ATOM RegisterClass();
    static void UnregisterClass();
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, 
                                    LPARAM lParam);
    static LRESULT CALLBACK GetMessageFilter(int code, WPARAM wParam, 
                                             LPARAM lParam);
    static LRESULT CALLBACK ForegroundIdleFilter(int code, WPARAM wParam, 
                                                 LPARAM lParam);
    static AwtToolkit& GetInstance();
    static void AwtToolkit::DeleteInstance();

    INLINE jobject GetPeer() { return m_peer; }
    INLINE void SetPeer(jobject p) { m_peer = p; }

    // Create an AwtXyz C++ component using a given factory
    //
    typedef void (*ComponentFactory)(void*, void*);
    static void CreateComponent(void* hComponent, void* hParent, 
                                ComponentFactory compFactory);
    static void DestroyComponent(class AwtComponent* comp);

    UINT MessageLoop();
    BOOL PumpWaitingMessages();
    BOOL PreProcessMsg(MSG& msg);
    BOOL PreProcessMouseMsg(class AwtComponent* p, MSG& msg);
    BOOL PreProcessKeyMsg(class AwtComponent* p, MSG& msg);
    
    // Create an ID which maps to an AwtObject pointer, such as a menu.
    UINT CreateCmdID(AwtObject* object);

    // removes cmd id mapping
    void RemoveCmdID(UINT id);

    // Return the AwtObject associated with its ID.
    AwtObject* LookupCmdID(UINT id);

    // Return the current application icon.
    HICON GetAwtIcon();

    // Turns on/off dialog modality for the system.
    INLINE AwtDialog* SetModal(AwtDialog* frame) { 
	AwtDialog* previousDialog = m_pModalDialog;
	m_pModalDialog = frame;
	return previousDialog;
    };
    INLINE void ResetModal(AwtDialog* oldFrame) { m_pModalDialog = oldFrame; };
    INLINE BOOL IsModal() { return (m_pModalDialog != NULL); };
    INLINE AwtDialog* GetModalDialog(void) { return m_pModalDialog; };

    // Stops the current message pump (normally a modal dialog pump)
    INLINE void StopMessagePump() { m_breakOnError = TRUE; }

    // Debug settings
    INLINE void SetVerbose(jboolean flag)   { m_verbose = (flag != JNI_FALSE); }
    INLINE void SetVerify(jboolean flag)    { m_verifyComponents = (flag != JNI_FALSE); }
    INLINE void SetBreak(jboolean flag)     { m_breakOnError = (flag != JNI_FALSE); }
    INLINE void SetHeapCheck(jboolean flag);

private:
    HWND CreateToolkitWnd(const TCHAR* name);

    BOOL m_localPump;
    DWORD m_mainThreadId;
    HWND m_toolkitHWnd;
    BOOL m_verbose;

    BOOL m_verifyComponents;
    BOOL m_breakOnError;

    BOOL  m_breakMessageLoop;
    UINT  m_messageLoopResult;

    class AwtComponent* m_lastMouseOver;
    BOOL                m_mouseDown;

    HHOOK m_hGetMessageHook;
    UINT  m_timer;

    AwtCmdIDList* m_cmdIDs;
    BYTE		m_lastKeyboardState[KB_STATE_SIZE];
    CriticalSection	m_lockKB;

    static AwtToolkit *theInstance;

    // The current modal dialog frame (normally NULL).
    AwtDialog* m_pModalDialog;

    // The PPCToolkit peer instance
    jobject m_peer;

    HANDLE m_dllHandle;  // The module handle.
	public:
	static int sm_nComboHeightOffset;
#ifdef WINCE
    static void RegisterTopLevelWindow(HWND hwnd);
    static void UnregisterTopLevelWindow(HWND hwnd);
    static void EnumTopLevelWindows(WNDENUMPROC callBack, LPARAM lParam);
    static void ResetDisabledLevel(HWND hWnd);
    static void ChangeDisabledLevel(HWND hWnd, int delta);
    static int  GetDisabledLevel(HWND hWnd);
    static void SetDisabledLevel(HWND hWnd, int value);
#endif /* WINCE */
};

#endif
