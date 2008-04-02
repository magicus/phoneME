/*
 *   
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

#include "incls/_precompiled.incl"
#include "incls/_Main_wince.cpp.incl"

/*
 * This file implements the launching of the stand-along VM
 * on the WinCE platform.
 */

#include <windows.h>
#include <windowsx.h>
#if _WIN32_WCE < 300
#include "Afx.h"
#endif
#include <aygshell.h>
#include "resources.h"

BOOL WINAPI SHSipInfo (
  UINT uiAction, 
  UINT uiParam, 
  PVOID pvParam, 
  UINT fWinIni 
);

extern "C" {
extern bool LogConsole;
extern bool WriteConsole;
extern bool StickyConsole;
}

// height of PocketPC menu bar
#define MENU_HEIGHT 26

#define MAX_LINE_BUFFER 128

static PTCHAR      _szAppName    = TEXT("cldc_vm");
static PTCHAR      _szTitle      = TEXT("cldc_vm");
static HINSTANCE   _hAppInstance = NULL;
static HWND        _hwndMain     = NULL;
static HWND        _hwndEdit     = NULL;
static const int   _max_args     = 32;
static char*       _argv[_max_args];
static int         _argc;

static int         _WindowSystem = 1;
static FILE *      _log_file     = NULL ;
static int         _line_pos     = 0;
static char        _line_buffer[MAX_LINE_BUFFER];
static char *      logfilename = "\\cldc_vm.log";

extern "C" {
  extern int     _quit_now; // defined in Scheduler.cpp
}

// Display the message on the on-screen console
static void write_console(const char* s) {
  if (!_WindowSystem) {
    return;
  }
  // convert \n to \r\n
  bool flush_line = false;
  // we add one or two characters per iteration
  for (int si = 0; _line_pos < MAX_LINE_BUFFER-2; _line_pos++, si++) {
    const char ch = s[si];
    if (ch == '\n') {
      flush_line = true;
      _line_buffer[_line_pos++] = '\r';
    }
    _line_buffer[_line_pos] = ch;
    if (ch == 0x0) {
      break;
    }
  }

  // flush line only at new line characters
  if (flush_line || _line_pos >= MAX_LINE_BUFFER-2) {
    _line_pos = 0;
    // convert to Unicode and display
    TCHAR tmp[MAX_LINE_BUFFER];
    int res = MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, _line_buffer,
                                  -1, tmp, MAX_LINE_BUFFER);
    int lng = GetWindowTextLength(_hwndEdit);
    SetFocus(_hwndEdit);
    SendMessage(_hwndEdit, EM_SETSEL, (WPARAM)lng, (LPARAM)lng);
    SendMessage(_hwndEdit, EM_REPLACESEL, 0, (LPARAM) ((LPSTR) tmp));
    MSG msg;
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) == TRUE) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
}

// Log the message to /jvm.log
static void log_console(const char* s) {
  static bool first = true;

  if (first && _log_file == NULL) {
    char buf[128], buf2[140];
    if (LogByDate) {

      SYSTEMTIME st;
      GetSystemTime(&st);
      // example: jvm.log.2002-09-10.13-11-05
      sprintf(buf, "%s.%4d-%2d-%2d.%2d-%2d-%2d",
              logfilename, st.wYear, st.wMonth, st.wDay,
              st.wHour, st.wMinute, st.wSecond);
      for (int i=0; i<sizeof(buf); i++) {
        if (buf[i] == ' ') {
          buf[i] = '0';
        }
      }
    } else {
      sprintf(buf, "%s", logfilename);
    }

    // Try to save to "\Storage Card\cldc_vm.log" if possible. This
    // makes it easier to view the output on an ARM device emulator.
    sprintf(buf2, "%s%s", "\\Storage Card", buf);
    if ((_log_file = fopen(buf2, "w")) == NULL) {
        _log_file = fopen(buf, "w");
    }

    if (_log_file == NULL) {
      write_console("Console creation failed: \n");
      write_console(logfilename);
      write_console("\n");
    }
    first = false;
  }
  if (_log_file != NULL) {
    fprintf(_log_file, "%s", s);
    fflush(_log_file);
  }
}

extern "C"
void JVMSPI_PrintRaw(const char* s) {
  if (WriteConsole) {
    write_console(s);
  }
  if (LogConsole) {
    log_console(s);
  }
}

void JVMSPI_Exit(int code) {
  if (_log_file) {
    fclose(_log_file);
    _log_file = NULL;
  }

  Os::stop_ticks();

  if (_WindowSystem && StickyConsole) {
    write_console("VM has terminated\n");
  } else {
    write_console("\nBye Bye\n");
    if (_hwndMain) {
      SendMessage(_hwndMain, WM_CLOSE, 0, 0);
    }
  }
  ExitThread(0);
}

// Translate LPWSTR command line to chars.
int translate_command_line(LPWSTR lpCmdLine, char** argv, int max_args,
                           char* tmp, int max_line) {
  int argc = 1;
  size_t index;
  int res = WideCharToMultiByte(CP_ACP, 0, lpCmdLine, -1, tmp,
                                max_line, NULL, NULL);
  char seps[] = " ,\t\n\r";

  char *token;
  char *cmdStart = tmp;
  char *cmdEnd = tmp + strlen(tmp);
  while (cmdStart < cmdEnd) {
    index = strcspn(cmdStart, " ,\t\n\r");    // see if any separators
    if (index == 0) {
      // leading terminators, skip
      cmdStart++;
    } else {
      // found something...
      token = cmdStart;
      if (token[0] == '"') {
        /*
         * Some minimal parsing of quoted strings. This implementation
         * is incomplete.  You cannot quote the double-quote character
         * (").
         * Double-quotes must be balanced.
         * Here are some examples that work:
         *
         *    cldc_vm.exe -classpath "\My Document\classes" helloworld
         *    cldc_vm.exe helloworld pass an empty string arg ""
         */
        token++;  // skip "
        size_t endIndex = strcspn(token, "\"");  // look for trailing "
        if ((token + endIndex) == cmdEnd) {
          // Huh?  no terminating " oh well pretend it's at the end
          cmdStart = cmdEnd;  // already a null at the end
          argv[argc++] = token;
        } else {
          cmdStart = token + endIndex + 1; // point to char after quote
          token[endIndex] = '\0';
          argv[argc++] = token;
        }
      } else {
        // no quote, just a normal terminator
        token[index] = '\0';
        cmdStart = token + index + 1;
        argv[argc++] = token;
      }
    }
  }
  return argc;
}

// Check (and remove) a word is in the _argv[] array. This function is used
// for parsing the command-line before passing it to JVM_Start().
int check_and_pop(char *word) {
  int i;
  for (i=1; i<_argc; i++) {
    if (strcmp(_argv[i], word) == 0) {
      while (i+1 < _argc) {
        _argv[i] = _argv[i+1];
        i++;
      }
      _argc --;
      return 1;
    }
  }
  return 0;
}

void process_command_line(LPWSTR lpCmdLine) {
  const int max_line = 1024;
  static char line_buffer[max_line];

  int cmdLen = wcslen(lpCmdLine);
  _argv[0] = "cldc_vm";
  _argc = translate_command_line(lpCmdLine, _argv, _max_args,
                                 line_buffer, max_line);

  if (check_and_pop("-WindowSystem")) {
    // We need to process this specially because we need to know its
    // value before calling JVM_ParseOneArg().
    _WindowSystem = 0;
  }
}

/*
 * We can't run the VM in the main thread (because it has to handle events).
 * So we run the VM in this thread.
 */
static DWORD WINAPI vm_thread_routine(LPVOID lpvParam) {
  // Print arguments that we are using
  JVMSPI_PrintRaw("Running VM");
  JVMSPI_PrintRaw("\n");

  for (int i = 1; i < _argc; i++) {
    JVMSPI_PrintRaw(" ");
    JVMSPI_PrintRaw(_argv[i]);
    JVMSPI_PrintRaw("\n");
  }

  // Call this before any other Jvm_ functions.
  JVM_Initialize();

  int argc = _argc;
  char ** argv = _argv;
  // Ignore arg[0] -- the name of the program.
  argc --;
  argv ++;

  while (true) {
    int n = JVM_ParseOneArg(argc, argv);
    if (n < 0) {
      JVMSPI_DisplayUsage(NULL);
      return -1;
    } else if (n == 0) {
      break;
    }
    argc -= n;
    argv += n;
  }

  if (LogConsole) {
    write_console("Console output logged at \n");
    write_console(logfilename);
    write_console("\n");
    for (int index=0; index<_argc; index++) {
      log_console(_argv[index]);
      log_console(" ");
    }
    log_console("\n");
  }
  if (!WriteConsole) {
    write_console("On-screen console output disabled.\n");
  }

  int code = JVM_Start(NULL, NULL, argc, argv);
  JVMSPI_Exit(code);
  SHOULD_NOT_REACH_HERE();
  return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
  LRESULT lResult = 0;

  switch(msg){
  case WM_CREATE:
    {
      SHMENUBARINFO mbi;
      ZeroMemory(&mbi, sizeof(SHMENUBARINFO));
      mbi.cbSize = sizeof(SHMENUBARINFO);
      mbi.hwndParent = hwnd;
      mbi.nToolBarId = ID_MENU;
      mbi.hInstRes = _hAppInstance;

      if (SHCreateMenuBar(&mbi) == FALSE) {
         // Couldn't create the menu bar.  Fail creation of the window.
          return -1;
      }
    }
    break;

  case WM_COMMAND:
    switch (GET_WM_COMMAND_ID(wp, lp)) {
    case IDM_EXIT:
    case IDOK:
      _quit_now = 1;
      SendMessage(hwnd, WM_CLOSE, 0, 0);
      break;
    default:
      return DefWindowProc(hwnd, msg, wp, lp);
    }
    break;
  case WM_CLOSE:
    DestroyWindow(hwnd);
    break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;

  default:
    lResult = DefWindowProc(hwnd, msg, wp, lp);
    break;
  }
  return (lResult);
}

BOOL InitApplication(HINSTANCE hInstance) {
  WNDCLASS wc;

  wc.style = CS_HREDRAW | CS_VREDRAW ;
  wc.lpfnWndProc = (WNDPROC)WndProc;
  wc.cbClsExtra = 0;
  wc.cbWndExtra = 0;
  wc.hIcon = NULL;
  wc.hInstance = hInstance;
  wc.hCursor = NULL;
  wc.hbrBackground = (HBRUSH) GetStockObject( WHITE_BRUSH );
  wc.lpszMenuName = NULL;
  wc.lpszClassName = _szAppName;

  return RegisterClass(&wc);
}

bool InitInstance(HINSTANCE hInstance, int CmdShow) {
  _hwndMain = CreateWindowEx(WS_EX_CAPTIONOKBTN,
                             _szAppName,
                             _szTitle,
                             WS_VISIBLE,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             NULL, NULL, hInstance, NULL);

  if (!_hwndMain) {
    return false;
  }

  DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL |
                  WS_BORDER | ES_LEFT | ES_MULTILINE | ES_NOHIDESEL |
                  ES_AUTOHSCROLL | ES_AUTOVSCROLL;

  int x = 0, y = 0;

  SIPINFO si = {0};
  SHSipInfo(SPI_GETSIPINFO, 0, &si, 0);
  int cx = si.rcVisibleDesktop.right - si.rcVisibleDesktop.left;
  int cy = si.rcVisibleDesktop.bottom - si.rcVisibleDesktop.top - MENU_HEIGHT;

  // Create the window for the edit control window.
  _hwndEdit = CreateWindow(TEXT("edit"),  // Class name
                           NULL,          // Window text
                           dwStyle,       // Window style
                           x,             // upper-left corner
                           y,             // upper-left corner
                           cx,            // Width of the window for the edit
                           cy,            // Height of the window for the edit
                           _hwndMain,     // Window handle to the parent window
                           0,             // Control identifier
                           _hAppInstance, // Instance handle
                           NULL);

  SendMessage(_hwndEdit, EM_SETLIMITTEXT, 0x10000, 0);
  ShowWindow(_hwndMain, CmdShow );
  UpdateWindow(_hwndMain);
  SetForegroundWindow((HWND)(((DWORD)_hwndMain) | 0x01));
  return true;
}

bool init_gui(HINSTANCE hInstance, HINSTANCE hPrevInstance, int nShowCmd) {
  if (!InitApplication(hInstance)) {
    return false;
  }
  if (!InitInstance(hInstance, nShowCmd)) {
    return false;
  }
  return true;
}

HRESULT ActivatePreviousInstance(
    const TCHAR* lptszClass,
    const TCHAR* lptszTitle,
    BOOL* pfActivated)
{
  HRESULT hr = S_OK;
  int cTries;
  HANDLE hMutex = NULL;

  *pfActivated = FALSE;
  cTries = 5;

  while (cTries > 0) {
    // NOTE: We don't want to own the object.
    hMutex = CreateMutex(NULL, FALSE, lptszClass);

    if (NULL == hMutex) {
      // Something bad happened, fail.
        hr = E_FAIL;
        goto Exit;
    }

    if (GetLastError() == ERROR_ALREADY_EXISTS) {
      HWND hwnd;
      CloseHandle(hMutex);
      hMutex = NULL;

      // There is already an instance of this app
      // running.  Try to bring it to the foreground.

      hwnd = FindWindow(lptszClass, lptszTitle);
      if (NULL == hwnd) {
        // It's possible that the other window is in the process of
        // being created...
        Sleep(500);
        hwnd = FindWindow(lptszClass, lptszTitle);
      }

      if (NULL != hwnd) {
        // Set the previous instance as the foreground window
        // The "| 0x01" in the code below activates
        // the correct owned window of the
        // previous instance's main window.
        SetForegroundWindow((HWND) (((ULONG) hwnd) | 0x01));

        // We are done.
        *pfActivated = TRUE;
        break;
      }

      // It's possible that the instance we found isn't coming up,
      // but rather is going down.  Try again.
      cTries--;
    } else {
      // We were the first one to create the mutex
      // so that makes us the main instance.  'leak'
      // the mutex in this function so it gets cleaned
      // up by the OS when this instance exits.
      break;
    }
  }

  if (cTries <= 0) {
    // Someone else owns the mutex but we cannot find
    // their main window to activate.
    hr = E_FAIL;
    goto Exit;
  }

Exit:
  return(hr);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPWSTR lpCmdLine, int nShowCmd) {
  DWORD tmp;
  BOOL fActivated;
  _hAppInstance = hInstance;

  if (FAILED(ActivatePreviousInstance(_szAppName, _szTitle, &fActivated)) ||
      fActivated) {
    // Exit immediately if previous instance exists
    return 0;
  }

  process_command_line(lpCmdLine);
  if (_WindowSystem && _argc == 1) {
    int eembc = 0, hello = 0, mixed = 1, done = 0;

    init_gui(hInstance, hPrevInstance, nShowCmd);
    if (!done && MessageBox(NULL, TEXT("EEMBC (mixed)?"),
                            TEXT("Pick a program"), MB_YESNO) == IDYES) {
      done = 1; eembc = 1;
    }
    if (!done && MessageBox(NULL, TEXT("EEMBC (int)?"),
                            TEXT("Pick a program"), MB_YESNO) == IDYES) {
      done = 1; eembc = 1; mixed = 0;
    }
    if (!done && MessageBox(NULL, TEXT("HelloWorld (mixed)?"),
                            TEXT("Pick a program"), MB_YESNO) == IDYES) {
      done = 1; hello = 1;
    }
    if (!done && MessageBox(NULL, TEXT("HelloWorld (int)?"),
                            TEXT("Pick a program"), MB_YESNO) == IDYES) {
      done = 1; hello = 1; mixed = 0;
    }

    if (!done) {
      exit(0);
    }
    //_argv[_argc++] = "=HeapMin1500K";
    //_argv[_argc++] = "=HeapCapacity1500K";
    if (!mixed) {
      _argv[_argc++] = "-int";
    }
    if (eembc) {
      _argv[_argc++] = "-cp";
      _argv[_argc++] = "/eembcBM.jar";
      _argv[_argc++] = "com.sun.mep.bench.main";
    }
    if (hello) {
      _argv[_argc++] = "-cp";
      _argv[_argc++] = "/HelloWorld.jar";
      _argv[_argc++] = "HelloWorld";
    }
  }

  SetForegroundWindow((HWND)(((DWORD)_hwndMain) | 0x01));

  HANDLE vm_thread_handle = CreateThread(NULL, 0, 
        (LPTHREAD_START_ROUTINE)&vm_thread_routine, 0, 0, &tmp);

  // Handle events until we're done
  if (_WindowSystem) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0,0) == TRUE) {
      TranslateMessage (&msg);
      DispatchMessage(&msg);
    }
  } else {
    if (vm_thread_handle != NULL) {
      WaitForSingleObject(vm_thread_handle, INFINITE);
    }
  }

  CloseHandle(vm_thread_handle);

  if (_log_file) {
    fclose(_log_file);
    _log_file = NULL;
  }
  TerminateProcess(GetCurrentProcess(), 0);

  return 0;
}
