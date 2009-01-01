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

/*
 * @(#)wceConsole.c	1.6 06/10/10
 */

/*
 * TheJavaConsole is a very simple terminal to be used on systems without
 * console support. It is a 24*80 character buffer that scrolls. LF is the
 * only control character recognized.
 */

#include <windows.h>
#include <commctrl.h>

#include "javavm/include/io_md.h"
#include "javavm/include/porting/int.h"

//extern HINSTANCE hJavaInst;
HINSTANCE hJavaInst;

#define MAXROWS 24
#define MAXCOLS 80

#define IDC_CMDBAR 1
#define IBUFSIZE 256

static TCHAR consoleName[]=TEXT("Java Console");
static TCHAR consolePausedString[]=TEXT("Java Console: PAUSED");
static TCHAR consoleExitedString[]=TEXT("Java Console: EXITING");

static int initialized=0;

static struct JavaConsole {
    HWND hwnd;
    HFONT hFont;
    HANDLE hThread;
    HANDLE createEvent;	/* to signal initialization */
    HANDLE exitEvent;   /* to signal exit */
    HANDLE unPauseEvent; /* signal to un-pause */
    int numRows, numCols;
    int row, col;
    int startRow; /* For scrolling, we arap around */
    TCHAR buffer[MAXROWS][MAXCOLS + 1]; /* Screen buffer */
    TCHAR iBuffer[IBUFSIZE]; /* input buffer */
    int iCount;
    int iAvail; /* number of bytes actually available */
    int iRead, iWrite;
    HANDLE inputEvent;
    CRITICAL_SECTION cs; /* for updating iCount and iAvail */
    int closed;
    char paused;
    HCURSOR normCursor, pauseCursor;
} theConsole;

void JavaConsolePutStr(LPCTSTR str); /* Put a string to the java console */

static void wceInitConsole();
static int initConsole();
JavaConsoleCreateProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
JavaConsolePaintProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
static  HFONT wceGetFont(HDC hdc);


/* Must be called before console can actually be initialized */
static void wceInitConsole()
{
   InitializeCriticalSection(&theConsole.cs);
}

static int
initConsole()
{
    DWORD threadID;
    DWORD WINAPI consoleThread (PVOID arg); /* forward */
    int ret = 1;
    
    EnterCriticalSection(&theConsole.cs);
    if (!initialized) {
	initialized =1 ;
	theConsole.createEvent  = CreateEvent(NULL, TRUE, FALSE, TEXT("Created"));
	theConsole.inputEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("InputEvent"));
	theConsole.exitEvent = CreateEvent(NULL, FALSE, FALSE, TEXT("ConsoleExit"));
	theConsole.unPauseEvent = CreateEvent(NULL, FALSE, TRUE, TEXT("ConsoleExit"));
	theConsole.numCols=MAXCOLS; /* default, until window created */
	theConsole.numRows=MAXROWS; 
	theConsole.startRow = theConsole.row = theConsole.col =
	    theConsole.iCount = theConsole.iRead = 
	    theConsole.iWrite = theConsole.iAvail = 0;
	/* start the console thread */
	theConsole.hThread = CreateThread(NULL, 0, consoleThread,
					  0, 0, &threadID);
	/* now wait (3 seconds max) for it to be initialized */
	if (WaitForSingleObject(theConsole.createEvent, 3000) == WAIT_OBJECT_0 ) {
	    ret = 1; /* OK */
	} else {
	    ret = 0;
	}
    } else {
	/* already initialized , but maybe closed? */
	if (theConsole.closed) {
	    SendMessage(theConsole.hwnd, WM_USER, (WPARAM)0, (LPARAM)0);
	}
    }
    LeaveCriticalSection(&theConsole.cs);
    return ret;
}

/* Console Initialization and event thread */

static DWORD WINAPI consoleThread(PVOID pArg)
{
    WNDCLASS wc;
    MSG msg;
    LRESULT CALLBACK JavaConsoleWndProc(HWND wnd, UINT wMsg, WPARAM wParam, LPARAM lParam);
    wc.style = 0;
    wc.lpfnWndProc = JavaConsoleWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hJavaInst;
    wc.hIcon = NULL;
    wc.hCursor = NULL;
    wc.hbrBackground = GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = consoleName; /* use Console name as class name */
    RegisterClass(&wc);
    theConsole.hwnd  = CreateWindow(wc.lpszClassName,
				    consoleName,
				    WS_CAPTION | WS_MINIMIZEBOX |
				    WS_VISIBLE | WS_BORDER,
				    CW_USEDEFAULT, //x
				    CW_USEDEFAULT,  //y
				    CW_USEDEFAULT,  //w
				    CW_USEDEFAULT,  //h
				    NULL,/*Parent */	
				    NULL, /* menu */
				    hJavaInst, /* WARNING: NOT INITIALIZED!! */
				    NULL);
    theConsole.normCursor = LoadCursor(NULL, IDC_ARROW);
    theConsole.pauseCursor = LoadCursor(NULL, IDC_HAND);
    if (!theConsole.hwnd) {
	return -1;
    } else {
	ShowWindow(theConsole.hwnd, 1);   
	UpdateWindow(theConsole.hwnd);
	
	/* Let calling thread know we're */
        SetEvent(theConsole.createEvent);
	while (GetMessage(&msg, NULL, 0, 0)) {
	    TranslateMessage(&msg);
	    DispatchMessage(&msg);
	}
	/* we have exited */
	if (theConsole.hFont) {
	    DeleteObject(theConsole.hFont);
	}
	DeleteCriticalSection(&theConsole.cs);
	SetEvent(theConsole.exitEvent);
	return 0;
    }
}

static LRESULT CALLBACK
JavaConsoleWndProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    switch (wMsg) {
        case WM_CREATE:
	    return JavaConsoleCreateProc(hwnd, wMsg, wParam, lParam);
        case WM_PAINT:
	    return JavaConsolePaintProc(hwnd, wMsg, wParam, lParam);
        case WM_CHAR:
            return JavaConsoleCharProc(hwnd, wMsg, wParam, lParam);
        case WM_CLOSE:
	case WM_DESTROY:
	    /* don't actually destroy the window */
	    ShowWindow(theConsole.hwnd, SW_HIDE);
	    theConsole.closed = TRUE;
	    return 0;
	case WM_USER: /* used to re-open the console */
	    ShowWindow(theConsole.hwnd, SW_SHOWNORMAL);
	    theConsole.closed = FALSE;
	    return 0;
        case WM_LBUTTONDOWN: /* pause the console */
	    ResetEvent(theConsole.unPauseEvent);
	    theConsole.paused = TRUE;
	    SetCursor(theConsole.pauseCursor);
	    SetWindowText(theConsole.hwnd, consolePausedString); 
	    return 0;
        case WM_LBUTTONUP: /* unpause */
	    theConsole.paused = FALSE;
	    SetEvent(theConsole.unPauseEvent);
	    SetCursor(theConsole.normCursor);
	    SetWindowText(theConsole.hwnd, consoleName); 
	    return 0;
	    
	default:
	    return DefWindowProc(hwnd, wMsg, wParam, lParam);
     }
}

void
JavaConsolePutStr(LPCTSTR str) {
    TCHAR ch;

    if (!initConsole()) {
	return;
    }
    if (!str) {
	 return;
    }
    /* if console is paused block */
    if (theConsole.paused) {
	WaitForSingleObject(theConsole.unPauseEvent, INFINITE);
    }
    while (ch = *str++) {
	if (ch < (TCHAR)' ') {
	    /* Control Character */
            switch (ch) {
	        case '\n':
		    if (theConsole.row < theConsole.numRows-1) {
			theConsole.row++;
		    } else {
			/* scroll */
			theConsole.startRow =
			    (theConsole.startRow+1) % theConsole.numRows;
			memset((char *)(theConsole.buffer[
			    (theConsole.startRow + theConsole.numRows - 1)%theConsole.numRows]),
			       (int)'\0', theConsole.numCols*sizeof(TCHAR));
		    }
		    theConsole.col = 0;
		    break;
	    default:
		break;
	    }
	} else if ((ch == 0xffff) || (ch == 0xfffe)) {
            /* ignore */
        } else {
            if (theConsole.col == theConsole.numCols) { /* wrap */
		theConsole.col = 0;
		if (theConsole.row < theConsole.numRows-1) {
		    theConsole.row++;
		} else {
		    /* scroll */
		    theConsole.startRow = (theConsole.startRow+1) % theConsole.numRows;
		    memset((char *)(theConsole.buffer[(theConsole.startRow
						       + theConsole.numRows - 1) %
						       theConsole.numRows]),
			   0, theConsole.numCols*sizeof(TCHAR));
		}
	    }
	    theConsole.buffer[(theConsole.row+theConsole.startRow) % theConsole.numRows]
		[theConsole.col] = ch;
	    theConsole.col++;
	}
    }
    if (theConsole.hwnd) {
	InvalidateRect(theConsole.hwnd, NULL, TRUE);
	// doesn't need to call: UpdateWindow(theConsole.hwnd);
    }
}

JavaConsoleCreateProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    TEXTMETRIC tm;
    LPCREATESTRUCT lpcs;
    HWND cmdBar;
    HDC hdc;
    int desiredHeight, desiredWidth;
    int borderHeight, borderWidth;
    int maxWidth; int maxHeight, cmdBarHeight, titleHeight;
    int charWidth;
    RECT rect;

    lpcs = (LPCREATESTRUCT) lParam;
#ifdef HASCMDBAR
    cmdBar = CommandBar_Create(lpcs->hInstance, hwnd, IDC_CMDBAR);
    cmdBarHeight = CommandBar_Height(GetDlgItem(hwnd, IDC_CMDBAR));
    CommandBar_Destroy(cmdBar); /* we will create it again later */
#else
    cmdBarHeight = 0;
#endif

    hdc = GetDC(hwnd);
    theConsole.hFont = wceGetFont(hdc);
    if (theConsole.hFont) {
	SelectObject(hdc, theConsole.hFont);
    }
    GetTextMetrics(hdc, &tm);
    ReleaseDC(hwnd, hdc);
    
    borderWidth = GetSystemMetrics(SM_CXEDGE);
    borderHeight = GetSystemMetrics(SM_CYEDGE);
    titleHeight = GetSystemMetrics(SM_CYCAPTION);
    SystemParametersInfo(SPI_GETWORKAREA, 0, (void *)&rect, 0);
    
    if (tm.tmPitchAndFamily | TMPF_FIXED_PITCH) {
	/* Variable width font (flag is named oddly) */
	/* average character is too small */
	/* MAX is too big: combine them, weighted towards average */
	charWidth = (3*tm.tmAveCharWidth + tm.tmMaxCharWidth)/4;
    } else {
	/* Fixed width font */
	charWidth = tm.tmAveCharWidth;
    }
    desiredWidth = MAXCOLS * charWidth + 2 * borderWidth;
    desiredHeight = MAXROWS * tm.tmHeight + cmdBarHeight + 3 * borderHeight +titleHeight;
    maxWidth = (8*(rect.right - rect.left))/10; /* 80 percent of sceen dims */
    maxHeight = (8*(rect.bottom - rect.top))/10;
    if (desiredHeight > maxHeight) {
	/* won't fit shrink window */
	desiredHeight = maxHeight;
	theConsole.numRows =
	    (desiredHeight - (cmdBarHeight + 3 * borderHeight +titleHeight))/tm.tmHeight;
	/* re-adjust height */
	desiredHeight = theConsole.numRows * tm.tmHeight + cmdBarHeight + 3 * borderHeight +titleHeight;
    } else {
	theConsole.numRows = MAXROWS;
    }
    if (desiredWidth > maxWidth) {
	desiredWidth = maxWidth;
	theConsole.numCols =
	    (desiredWidth - (2 * borderWidth)) /  charWidth;
	desiredWidth = (theConsole.numCols * charWidth)
	    + (2 * borderWidth);
    } else {
	theConsole.numCols = MAXCOLS;
    }
    SetWindowPos( hwnd, NULL, (rect.left+rect.right-desiredWidth)/2, (rect.top+rect.bottom-desiredHeight)/2, desiredWidth, desiredHeight, 0);
#ifdef HADCMDBAR
    /* ComnmandBar must be created after sizing */
    cmdBar = CommandBar_Create(lpcs->hInstance, hwnd, IDC_CMDBAR);
    CommandBar_AddAdornments(cmdBar, 0, 0);
#endif
    theConsole.hwnd = hwnd;

    return 0;
}

JavaConsolePaintProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    PAINTSTRUCT ps;
    RECT rect, clientRect;
    TEXTMETRIC tm;
    WCHAR *line;
    HDC hdc;
    int i, len;
    HFONT prevFont= (HFONT) 0;

    hdc = BeginPaint(hwnd, &ps);
    if (theConsole.hFont) {
	prevFont = SelectObject(hdc, theConsole.hFont);
    }
    GetClientRect(hwnd, &clientRect);
    
    GetTextMetrics(hdc, &tm);
#ifdef HADCMDBAR
    clientRect.top += CommandBar_Height(GetDlgItem(hwnd, IDC_CMDBAR));
#endif
    for (i=0; i < theConsole.numRows; i++) {
	rect.top = clientRect.top + (i * tm.tmHeight);
//	rect.bottom = rect.top + tm.tmHeight;

        line = theConsole.buffer[(i+theConsole.startRow)%theConsole.numRows];
        len = wcslen(theConsole.buffer[(i+theConsole.startRow)%theConsole.numRows]);
	rect.left = clientRect.left;
//	rect.right = rect.left + len * tm.tmMaxCharWidth;
	DrawTextW(hdc, line,
		 -1, /* count: null terminated */
		 &rect,
		 DT_LEFT | DT_TOP | DT_NOCLIP |DT_NOPREFIX |DT_SINGLELINE /* Formatting options */
		);
    }

    /* Draw cursor */
    rect.top = clientRect.top + tm.tmHeight*theConsole.row;
    rect.left = 0;
    rect.right = 0;
    rect.bottom = rect.top + tm.tmHeight;

    /* Compute where cursor goes */
    /* Since font is variable width, we must calculate length of string up */
    /* to where the cursor is */
    DrawText(hdc,
	     theConsole.buffer[(theConsole.row+theConsole.startRow)%theConsole.numRows],
	     theConsole.col, &rect,
	     DT_CALCRECT | DT_LEFT | DT_TOP |DT_NOPREFIX |DT_SINGLELINE);
    /* Simply draw an underscore: note if cursor positioning is implemented */
    /* may need to change the paint mode (to xor?) in the GC */
    rect.left = rect.right;
    rect.right = rect.left + tm.tmMaxCharWidth;
    rect.bottom = rect.top + tm.tmHeight;
    DrawTextW(hdc, TEXT("_"), -1, &rect,
	      DT_LEFT | DT_TOP |DT_NOPREFIX |DT_SINGLELINE); 

    EndPaint(hwnd, &ps);
    if (prevFont) {
      SelectObject(hdc, prevFont);
    }
    return 0;
}

/* JavaConsoleCharProc: called when user types a character */
JavaConsoleCharProc(HWND hwnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{
    TCHAR str[2];

    if (wParam == '\r') {
      wParam = '\n'; /* Translate CR to LF */
    }
    str[1] = 0;
    str[0] = (TCHAR) wParam;

    /* Echo */
    JavaConsolePutStr(str);

    if (theConsole.iCount < IBUFSIZE) {
	theConsole.iBuffer[theConsole.iWrite] = (TCHAR) wParam;
	theConsole.iWrite = (theConsole.iWrite +1) % IBUFSIZE;
	theConsole.iCount++;
	if (wParam == '\n') {
	    /* we are line buffered */
	    EnterCriticalSection(&theConsole.cs);
	    theConsole.iAvail = theConsole.iCount; /* update available */
	    LeaveCriticalSection(&theConsole.cs);
	    SetEvent(theConsole.inputEvent); /* wake up reader */
	}
    }
    return 0;
}

DWORD wceConsoleRead(void *buf, size_t nbytes, DWORD *nRead)
{
    int count;
    int i, j;
    char *cp = buf;

    if (!initConsole()) {
	*nRead = 0;
	return 0;
    }
    if (theConsole.iAvail <= 0) {
	/* Block, waiting for input */
	if (WaitForSingleObject(theConsole.inputEvent, INFINITE) != WAIT_OBJECT_0 ) {
	    *nRead = 0;
	    return 0;
	}
    }
    /* Get count, once, as it may change as characters arrive */
     
    EnterCriticalSection(&theConsole.cs);
    count = theConsole.iAvail;
    LeaveCriticalSection(&theConsole.cs);

    if (count * sizeof(TCHAR) > nbytes) {
	count = nbytes / sizeof(TCHAR);
    }

    /* Take unicode characters and put them into byte buffer */

    j = 0;
    for (i=0; i < theConsole.iAvail; i++) {
	/* Little endian, should match file.encoding property */
	cp[j++] = theConsole.iBuffer[(i + theConsole.iRead)%IBUFSIZE] & 0xFF;
#ifdef UNICODE_IO
	cp[j++] = theConsole.iBuffer[i%IBUFSIZE] >> 8;
#endif
//	cp[j++] = theConsole.iBuffer[(i + theConsole.iRead)%IBUFSIZE] & 0xFF;
    }

    theConsole.iRead += count;
    EnterCriticalSection(&theConsole.cs);
    theConsole.iCount -= count;
    theConsole.iAvail -= count;
    LeaveCriticalSection(&theConsole.cs);
#ifdef UNICODE_IO
    *nRead = count * sizeof(TCHAR);
#else
    *nRead = count;
#endif
    return 0;
}

/* Called when system wants to exit */
/* if there is information on the console, wait */
void wceConsoleExit()
{
    if (theConsole.createEvent) { /* if there is a console */
        SetWindowText(theConsole.hwnd, consoleExitedString); 
	/* Just pop up a dilog (message box) */
	MessageBox((HWND)0, TEXT("Java Program has exited."),
		   TEXT("Java Exit"), MB_OK | MB_TOPMOST);
    }
}


/* Windows CE does not support the FIXED_FONT
 * Also soes not support PROOF_QUALITY, which
 * would insure that it we could select a reasonable
 * screen font. The problem with the default font is
 * that it is variable width which makes it diffiult to
 * size to console properly.
 */
#define WCE_FONT_MIN_HEIGHT 12
static 
HFONT wceGetFont(HDC hdc)
{
    LOGFONT lFont;

    memset(&lFont, 0,  sizeof(LOGFONT));
    /* non-zero members */
    
    lFont.lfHeight = GetDeviceCaps(hdc, VERTRES)/32;
    if (lFont.lfHeight < WCE_FONT_MIN_HEIGHT) {
      /* select a 9 point font */
      lFont.lfHeight = (-9*GetDeviceCaps(hdc, LOGPIXELSY))/72;
    } 
    lFont.lfWeight = FW_NORMAL;
    lFont.lfQuality = DRAFT_QUALITY;
    lFont.lfCharSet = ANSI_CHARSET;
    lFont.lfOutPrecision = OUT_RASTER_PRECIS;
    lFont.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;

    return CreateFontIndirect(&lFont);
}

int 
writeStandardIO(CVMInt32 fd, const char* buf, CVMUint32 nbyte) {
    /* Console is unicode */
    /* string may be unicode */
    /* but there may be alignement problems */
    /* also, need to translate LF */
    #define WRITE_BUF_SIZE 80
    TCHAR wbuf[WRITE_BUF_SIZE+2];/* for CR and null */
    int wc;
    int c;
    TCHAR wchar;
#ifdef UNICODE_IO
    if (nbyte%2) {
      nbyte -=1; /* make it even */
    }
#endif
    wc = 0;
#ifdef UNICODE_IO
    for (c = 0; c < (int)nbyte; c+=2) {
        wchar = buf[c] | (buf[c+1]<< 8);
#else
    for (c = 0; c < (int)nbyte; c++) {
        wchar = buf[c];
#endif
        if (wchar == '\n') {
            wbuf[wc++] = _T('\r'); /* insert CR */
        }
        wbuf[wc++] = wchar;
        if (wc >= WRITE_BUF_SIZE) {
            /* flush */
            wbuf[wc] = _T('\0');
            JavaConsolePutStr(wbuf);
            wc = 0;
        }
    }
    if (wc) {
        /* flush */
        wbuf[wc] = _T('\0');
        JavaConsolePutStr(wbuf);
        wc = 0;
    }
    return nbyte;
}

int 
readStandardIO(CVMInt32 fd, void* buf, CVMUint32 nbyte) {
   DWORD nRead;
   if (fd == 0) {
      if (wceConsoleRead(buf, nbyte, &nRead) < 0) {
         return nRead;
      } 
   }
   return 0;
}

void 
initializeStandardIO() {
   wceInitConsole();
}
