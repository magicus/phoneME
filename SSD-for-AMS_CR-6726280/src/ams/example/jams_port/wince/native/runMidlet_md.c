/*
 *
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

#include <windows.h>
#include <windowsx.h>
#include <aygshell.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <logging.h>
#include <midpAMS.h>
#include <midpexport_suite_storage.h>
#include <midpMalloc.h>
#include <jvm.h>
#include <findMidlet.h>
#include <commandLineUtil.h>
#include <commandLineUtil_md.h>
#include <winceapp_export.h>

#if ENABLE_MULTIPLE_ISOLATES
#define MIDP_HEAP_REQUIREMENT (3 * 1024 * 1024)
#else
#define MIDP_HEAP_REQUIREMENT (1 * 1024 * 1024)
#endif

/** Maximum number of command line arguments. */
#define RUNMIDLET_MAX_ARGS 32


#if ENABLE_MULTIPLE_ISOLATES
static PTCHAR      _szAppName    = TEXT("jwc1.1.3 (mvm)");
static PTCHAR      _szTitle      = TEXT("jwc1.1.3 (mvm)");
#else
static PTCHAR      _szAppName    = TEXT("jwc1.1.3 (svm)");
static PTCHAR      _szTitle      = TEXT("jwc1.1.3 (svm)");
#endif
static HINSTANCE   _hAppInstance = NULL;
static HWND        _hwndMain     = NULL;
static char*       _argv[RUNMIDLET_MAX_ARGS];
static int         _argc;
int                quit_now     = 0; /* used by VM to quit immediately. */

/** Usage text for the run MIDlet executable. */
static const char* const runUsageText =
"\n"
"Usage: runMidlet [<VM args>] [-debug] [-loop] [-classpathext <path>]\n"
"           (<suite number> | <suite ID>)\n"
"           [<classname of MIDlet to run> [<arg0> [<arg1> [<arg2>]]]]\n"
"         Run a MIDlet of an installed suite. If the classname\n"
"         of the MIDlet is not provided and the suite has multiple MIDlets,\n"
"         the first MIDlet from the suite will berun.\n"
"          -debug: start the VM suspended in debug mode\n"
"          -loop: run the MIDlet in a loop until system shuts down\n"
"          -classpathext <path>: append <path> to classpath passed to VM\n"
"             (can access classes from <path> as if they were romized)\n"
"\n"
"  where <suite number> is the number of a suite as displayed by the\n"
"  listMidlets command, and <suite ID> is the unique ID a suite is \n"
"  referenced by\n\n";

/* Translate LPWSTR command line to chars. */
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
        index = strcspn(cmdStart, " ,\t\n\r");   /* see if any separators */
        if (index == 0) {
            /* leading terminators, skip */
            cmdStart++;
        } else {
            /* found something... */
            token = cmdStart;
            if (token[0] == '"') {
                size_t endIndex;
                /*
                 * Some minimal parsing of quoted strings. This implementation
                 * is incomplete.  You cannot quote the double-quote character
                 * (").
                 * Double-quotes must be balanced.
                 * Here are some examples that work:
                 *
                 *    cldc_hi.exe -classpath "\My Document\classes" helloworld
                 *    cldc_hi.exe helloworld pass an empty string arg ""
                 */
                token++;  /* skip " */
                endIndex = strcspn(token, "\"");  /* look for trailing " */
                if ((token + endIndex) == cmdEnd) {
                    /* Huh?  no terminating " oh well pretend it's at the end */
                    cmdStart = cmdEnd;  /* already a null at the end */
                    argv[argc++] = token;
                } else {
                    cmdStart = token + endIndex + 1; 
                    /* point to char after quote */
                    token[endIndex] = '\0';
                    argv[argc++] = token;
                }
            } else {
                /* no quote, just a normal terminator */
                token[index] = '\0';
                cmdStart = token + index + 1;
                argv[argc++] = token;
            }
        }
    }
    return argc;
}

void process_command_line(LPWSTR lpCmdLine) {
    static char line_buffer[1024];

    int cmdLen = wcslen(lpCmdLine);
    _argv[0] = "runMidler.exe";
    _argc = translate_command_line(lpCmdLine, _argv, RUNMIDLET_MAX_ARGS,
                                   line_buffer, sizeof(line_buffer));
}

HRESULT ActivatePreviousInstance(const TCHAR* lptszClass,
                                 const TCHAR* lptszTitle, BOOL* pfActivated)
{
    HRESULT hr = S_OK;
    int cTries;
    HANDLE hMutex = NULL;

    *pfActivated = FALSE;
    cTries = 5;

    while (cTries > 0) {
        /* NOTE: We don't want to own the object. */
        hMutex = CreateMutex(NULL, FALSE, lptszClass);

        if (NULL == hMutex) {
            /* Something bad happened, fail. */
            hr = E_FAIL;
            goto Exit;
        }

      if (GetLastError() == ERROR_ALREADY_EXISTS) {
          HWND hwnd;
          CloseHandle(hMutex);
          hMutex = NULL;

          /* There is already an instance of this app
           * running.  Try to bring it to the foreground.
           */

          hwnd = FindWindow(lptszClass, lptszTitle);
          if (NULL == hwnd) {
              /* It's possible that the other window is in the process of
               * being created...
               */
              Sleep(500);
              hwnd = FindWindow(lptszClass, lptszTitle);
          }

          if (NULL != hwnd) {
              /* Set the previous instance as the foreground window
               * The "| 0x01" in the code below activates
               * the correct owned window of the
               * previous instance's main window.
               */
              SetForegroundWindow((HWND) (((ULONG) hwnd) | 0x01));

              /* We are done. */
              *pfActivated = TRUE;
              break;
          }

          /* It's possible that the instance we found isn't coming up,
           * but rather is going down.  Try again.
           */
          cTries--;
      } else {
          /* We were the first one to create the mutex
           * so that makes us the main instance.  'leak'
           * the mutex in this function so it gets cleaned
           * up by the OS when this instance exits.
           */
          break;
      }
    }

    if (cTries <= 0) {
        /* Someone else owns the mutex but we cannot find
         * their main window to activate.
         */
        hr = E_FAIL;
        goto Exit;
    }

Exit:
    return(hr);
}

static BOOL InitApplication(HINSTANCE hInstance) {
    WNDCLASS wc;

    wc.style = CS_HREDRAW | CS_VREDRAW ;
    wc.lpfnWndProc = (WNDPROC)winceapp_wndproc;
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

static BOOL InitInstance(HINSTANCE hInstance, int CmdShow) {
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
        return FALSE;
    }

    winceapp_set_window_handle(_hwndMain);
    ShowWindow(_hwndMain, CmdShow );
    UpdateWindow(_hwndMain);
    return TRUE;
}

static BOOL init_gui(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     int nShowCmd) {
    if (!InitApplication(hInstance)) {
        return FALSE;
    }
    if (!InitInstance(hInstance, nShowCmd)) {
        return FALSE;
    }
    return TRUE;
}


/**
 * Runs a MIDlet from an installed MIDlet suite. This is an example of
 * how to use the public MIDP API.
 *
 * @param argc The total number of arguments
 * @param argv An array of 'C' strings containing the arguments
 *
 * @return <tt>0</tt> for success, otherwise <tt>-1</tt>
 *
 * @todo determine if it is desirable for user targeted output
 *       messages to be sent via the log/trace service, or if
 *       they should remain as printf calls
 */

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPWSTR lpCmdLine, int nShowCmd) {
    int argc;
    char** commandlineArgs;
    BOOL fActivated;
    int status = -1;
    MidpString suiteID = NULL_MIDP_STRING;
    MidpString classname = NULL_MIDP_STRING;
    MidpString arg0 = NULL_MIDP_STRING;
    MidpString arg1 = NULL_MIDP_STRING;
    MidpString arg2 = NULL_MIDP_STRING;
    int repeatMidlet = 0;
    char* argv[RUNMIDLET_MAX_ARGS];
    int i, used;
    int debugOption = MIDP_NO_DEBUG;
    char *progName;
    char* appDir = NULL;
    char* confDir = NULL;
    char* additionalPath;
    MidpString* pSuites = NULL;
    int numberOfSuites = 0;

    if (FAILED(ActivatePreviousInstance(_szAppName, _szTitle, &fActivated)) ||
        fActivated) {
        /* Exit immediately if previous instance exists */
        return 0;
    }

    _hAppInstance = hInstance;
    process_command_line(lpCmdLine);
    if (!init_gui(hInstance, hPrevInstance, nShowCmd)) {
        REPORT_ERROR(LC_AMS, "init_gui() failed");
        MessageBox(NULL, TEXT("Failed to start JWC"), TEXT("Bye"), MB_OK);
        return 0;
    }

    argc = _argc;
    commandlineArgs = _argv;
    progName = commandlineArgs[0];

    JVM_Initialize(); /* It's OK to call this more than once */

    /*
     * Set Java heap capacity now so it can been overridden from command line.
     */
    JVM_SetConfig(JVM_CONFIG_HEAP_CAPACITY, MIDP_HEAP_REQUIREMENT);
    JVM_SetConfig(JVM_CONFIG_HEAP_MINIMUM, MIDP_HEAP_REQUIREMENT);

    /*
     * Parse options for the VM. This is desirable on a 'development' platform
     * such as linux_qte. For actual device ports, copy this block of code only
     * if your device can handle command-line arguments.
     */

    /* JVM_ParseOneArg expects commandlineArgs[0] to contain the first actual
     * parameter */
    argc --;
    commandlineArgs ++;

    while ((used = JVM_ParseOneArg(argc, commandlineArgs)) > 0) {
        argc -= used;
        commandlineArgs += used;
    }

    /* Restore commandlineArgs[0] to contain the program name. */
    argc ++;
    commandlineArgs --;
    commandlineArgs[0] = progName;

    /*
     * Not all platforms allow rewriting the command line arg array,
     * make a copy
     */
    if (argc > RUNMIDLET_MAX_ARGS) {
        REPORT_ERROR(LC_AMS, "Number of arguments exceeds supported limit");
        fprintf(stderr, "Number of arguments exceeds supported limit\n");
        return -1;
    }
    for (i = 0; i < argc; i++) {
        argv[i] = commandlineArgs[i];
    }

    if (midpRemoveOptionFlag("-debug", argv, &argc) != NULL) {
        debugOption = MIDP_DEBUG_SUSPEND;
    }

    if (midpRemoveOptionFlag("-loop", argv, &argc) != NULL) {
        repeatMidlet = 1;
    }

    /* additionalPath gets appended to the classpath */
    additionalPath = midpRemoveCommandOption("-classpathext", argv, &argc);

    if (argc == 1) {
#if ENABLE_MULTIPLE_ISOLATES
        argv[argc++] = "internal";
        argv[argc++] = "com.sun.midp.appmanager.MVMManager";
#else
        argv[argc++] = "internal";
        argv[argc++] = "com.sun.midp.appmanager.Manager";
#endif
    }

    if (argc > 6) {
        REPORT_ERROR(LC_AMS, "Too many arguments given\n");
        fprintf(stderr, "Too many arguments given\n%s", runUsageText);
        return -1;
    }

    /* get midp home directory, set it */
    appDir = getApplicationDir(argv[0]);
    if (appDir == NULL) {
        return -1;
    }
    /* set up appDir before calling initialize */
    midpSetAppDir(appDir);

    /* get midp config directory, set it */
    confDir = getConfigurationDir(argv[0]);
    if (confDir == NULL) {
        return -1;
    }
    /* set up confDir before calling initialize */
    midpSetConfigDir(confDir);

    if (midpInitialize() != 0) {
        REPORT_ERROR(LC_AMS, "Not enough memory");
        fprintf(stderr, "Not enough memory\n");
        return -1;
    }

    do {
        int onlyDigits;
        int len;
        int i;

        if (argc > 5) {
            arg2 = midpCharsToJchars(argv[5]);
            if (arg2.len == OUT_OF_MEM_LEN) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory\n");
                break;
            }
        }

        if (argc > 4) {
            arg1 = midpCharsToJchars(argv[4]);
            if (arg1.len == OUT_OF_MEM_LEN) {
            REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory\n");
                break;
            }
        }

        if (argc > 3) {
            arg0 = midpCharsToJchars(argv[3]);
            if (arg0.len == OUT_OF_MEM_LEN) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory\n");
                break;
            }
        }

        if (argc > 2) {
            classname = midpCharsToJchars(argv[2]);
            if (classname.len == OUT_OF_MEM_LEN) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory\n");
                break;
            }

        }

        /* if the storage name only digits, convert it */
        onlyDigits = 1;
        len = strlen(argv[1]);
        for (i = 0; i < len; i++) {
            if (!isdigit((argv[1])[i])) {
                onlyDigits = 0;
                break;
            }
        }

        if (onlyDigits) {
            /* Run by number */
            int suiteNumber;

            /* the format of the string is "number:" */
            if (sscanf(argv[1], "%d", &suiteNumber) != 1) {
                REPORT_ERROR(LC_AMS, "Invalid suite number format");
                fprintf(stderr, "Invalid suite number format\n");
                break;
            }

            numberOfSuites = midpGetSuiteIDs(&pSuites);
            if (numberOfSuites < 0) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory\n");
                break;
            }

            if (suiteNumber > numberOfSuites || suiteNumber < 1) {
                REPORT_ERROR(LC_AMS, "Suite number out of range");
                fprintf(stderr, "Suite number out of range\n");
                midpFreeSuiteIDs(pSuites, numberOfSuites);
                break;
            }

            suiteID = pSuites[suiteNumber - 1];
        } else {
            /* Run by ID */
            suiteID = midpCharsToJchars(argv[1]);

            if (suiteID.data == NULL) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory\n");
                break;
            }
        }

        if (classname.len == NULL_LEN) {
            classname = findMidletClass(suiteID, 1);
            if (classname.len == OUT_OF_MEM_LEN) {
                REPORT_ERROR(LC_AMS, "Out of Memory");
                fprintf(stderr, "Out Of Memory 7\n");
                break;
            }

            if (classname.len == NULL_LEN) {
                REPORT_ERROR(LC_AMS, "Could not find the first MIDlet");
                fprintf(stderr, "Could not find the first MIDlet\n");
                break;
            }
        }

        do {
            status = midpRunMidletWithArgsCp(suiteID, classname,
                                             arg0, arg1, arg2,
                                             debugOption, additionalPath);
        } while (repeatMidlet && status != MIDP_SHUTDOWN_STATUS);

        if (pSuites != NULL) {
            midpFreeSuiteIDs(pSuites, numberOfSuites);
            suiteID = NULL_MIDP_STRING;
        } else {
            midpFreeString(suiteID);
        }
    } while (0);

    midpFreeString(arg0);
    midpFreeString(arg1);
    midpFreeString(arg2);
    midpFreeString(classname);

    switch (status) {
    case MIDP_SHUTDOWN_STATUS:
        break;

    case MIDP_ERROR_STATUS:
        REPORT_ERROR(LC_AMS, "The MIDlet suite could not be run.");
        fprintf(stderr, "The MIDlet suite could not be run.\n");
        break;

    case SUITE_NOT_FOUND_STATUS:
        REPORT_ERROR(LC_AMS, "The MIDlet suite was not found.");
        fprintf(stderr, "The MIDlet suite was not found.\n");
        break;

    default:
        break;
    }

    midpFinalize();

    return status;
}

