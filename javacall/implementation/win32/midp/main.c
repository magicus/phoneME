/*
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

/**
 * @file
 *
 * Main function to start the es-gate simulator.
 */
#include <windows.h>
#include <stdio.h>
#include <process.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>

#include "javacall_lifecycle.h"
#include "javacall_logging.h"
#include "javacall_events.h"
#include "javacall_properties.h"
#include "javautil_jad_parser.h"
#include "javacall_lcd.h"
#if ENABLE_NATIVE_AMS_UI
#include "javacall_ams_installer.h"
#endif
#include "lcd.h"
#include "res/resource.h"

static char usage[] = "\t runMidlet \n"
                      "\t runMidlet help \n"
                      "\t runMidlet manager \n"
                      "\t runMidlet [debug] loop \n"
                      "\t runMidlet [debug] tck \n"
                      "\t runMidlet [debug] tck <url> \n"
                      "\t runMidlet [debug] install_url  <url> \n"
                      "\t runMidlet [debug] install_file <path> \n"
                      "\t runMidlet [debug] install_wap <path> <url> \n"
                      "\t runMidlet [debug] install_content <path> <url> [<mime>]\n"
                      "\t runMidlet [debug] i3test [<arg1>] [<arg2>]\n"
                      "\t runMidlet [debug] invoke <handlerID> <url> [action]\n"
                      "\t For example: \n"
                      "\t runMidlet tck http://host:port/test/getNextApp.jad [trusted | untrusted | minimum | maximum]\n"
                      "\t runMidlet install_url http://host:port/rmsT1.jad \n"
                      "\t runMidlet debug install_url http://host:port/rmsT1.jad \n"
                      "\t runMidlet install_file file:///C:/WTK21/apps/rmsT1/bin/rmsT1.jad \n";

static char controlLoopInfo[] = "\t To control:\n"
                                "\t '1' to send Start \n"
                                "\t '2' to send Shutdown \n"
                                "\t '3' to send Pause \n"
                                "\t '4' to send Resume \n"
                                "\t \n"
                                "\t '0' to quit";
                                

/* forward declaration */
void main_install_content(int argc, char *argv[]);
void CreateEmulatorWindow();
LRESULT CALLBACK main_dlgproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

HANDLE lifecycle_shutdown_event;

DWORD WINAPI ThreadProc( LPVOID lpParam ) {
     javacall_lcd_init();
     CreateEmulatorWindow();
     JavaTask();
     SetEvent(lifecycle_shutdown_event);
     javacall_print("Done JavaTask\n");
     return 0; // Terminate the thread
}

/*
    The function processes the command line arguments:
    tck ..., install_...
    Returns JAVACALL_OK to go on
         or JAVACALL_FAIL to stop
*/
javacall_bool mainArgumentsHandle(int argc, char *argv[]) {

    if (argc == 1) {

        /* no arguments */
        javanotify_start_java_with_arbitrary_args(argc, argv);

    } else if((argc == 2) && (strcmp(argv[1], "manager") == 0)) {

        /* appmanager.Manager */
        javacall_print("main() Starting Manager\n");
        javanotify_start();

    } else if((argc == 2) && (strcmp(argv[1], "help") == 0)) {

        javacall_print(usage);
        return JAVACALL_FAIL;

    } else if((argc == 2) && (strcmp(argv[1], "loop") == 0)) {
        javacall_print("main() Device emulation mode. No UI will be displayed till start event arrives.\n");
    } else if((argc == 2) && (strcmp(argv[1], "tck") == 0)) {

        /* installer.AutoTester */
        javacall_print("main() Starting AutoTester without url\n");
        javanotify_start_tck("none", JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED_MAX);

    } else if(((argc == 3) || (argc == 4)) && (strcmp(argv[1], "tck") == 0)) {

        /* installer.AutoTester */

        if(argc == 3) {
            javacall_print("main() Starting AutoTester with url and default domain.\n");
            //javanotify_start_tck(argv[2], JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED_MAX);
//## here is a stub.
//## actually JAVACALL_LIFECYCLE_TCK_DOMAIN_IDENTIFIED should be added accordingly to com/sun/midp/security/Permissions.java
//## None of trusted/untrusted/maximum/minimum permits midlet to use wma2.0 (MMS_SERVER permission)
            javanotify_start_tck(argv[2], -1);
        } else if(argc == 4){

            javacall_print("main() Starting AutoTester with url and ");

            if((strcmp(argv[3], "trusted")) == 0) {

                javacall_print("trusted domain.\n");
                javanotify_start_tck(argv[2], JAVACALL_LIFECYCLE_TCK_DOMAIN_TRUSTED);

            } else if((strcmp(argv[3], "untrusted")) == 0) {

                javacall_print("untrusted domain.\n");
                javanotify_start_tck(argv[2], JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED);

            } else if((strcmp(argv[3], "maximum")) == 0) {

                javacall_print("maximum domain.\n");
                javanotify_start_tck(argv[2], JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED_MAX);

            } else if((strcmp(argv[3], "minimum")) == 0) {

                javacall_print("minimum domain.\n");
                javanotify_start_tck(argv[2], JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED_MIN);

            } else {

                javacall_print("no domain defined. Use system default\n");
                javanotify_start_tck(argv[2], JAVACALL_LIFECYCLE_TCK_DOMAIN_UNTRUSTED_MAX);

            }
        } /* end of else if */

    } else if ((argc == 3) && (strcmp(argv[1], "install_url") == 0) ||
                              (strcmp(argv[1], "install_file") == 0)) {
        char *path;
        int pathLen;
        int i;
        javacall_utf16 utf16Path[256];

        if (strcmp(argv[1], "install_url") == 0) {
            /* installer.GraphicalInstaller URL */
            javacall_print("main() Starting Graphical Installer from URL\n");
            /* javanotify_install_midlet(argv[2]); */
        } else {
            javacall_print("main() Starting Graphical Installer from file\n");
            javacall_print("main() Calling to Graphical Installer from URL\n");

            /* installer.GraphicalInstaller FILE */
            /*
            README PLEASE
            Why do we call to install from url instead of calling to install
            from file?
            The main difference between both installs is that in file install
            path supposed to be unicode and in url install it will be asci
            When working with ES-Gate on Win32 we type path in plain english.
            In javanotify_install_midlet_from_filesystem() it converted to
            asci from unicode since unicode expected and string becomes unusable.
            So for win32 we will use same javanotify_install_midlet() for both.
            once ES-Gate ported to platform, we will ask the platform to call
            to javanotify_install_midlet_from_filesystem() with unicode path
            and than it should work as expected.
            This code will not go to platform so it's ok to make here
            platform dependent tricks.
            */

            /*
            javanotify_install_midlet_from_filesystem(
                                                 (const javacall_utf16*)argv[2],
                                                 strlen(argv[2]));
            */
        }

        path = argv[2];
        pathLen = strlen(path);

        for (i=0; i < pathLen; i++) {
            utf16Path[i] = path[i];
        }

        utf16Path[i++] = 0;
        utf16Path[i]   = 0;

        /*
        javanotify_install_midlet_from_filesystem (utf16Path, pathLen, 0);
        */

#if ENABLE_NATIVE_AMS_UI
        javanotify_ams_install_suite(1, JAVACALL_INSTALL_SRC_ANY, utf16Path,
            JAVACALL_INVALID_STORAGE_ID, JAVACALL_INVALID_FOLDER_ID);
#endif
    } else if ((argc == 4) && (strcmp(argv[1], "install_wap") == 0)) {
        {
            char* jadPath = argv[2];
            int jadPathLen = strlen(jadPath);
            char* jadUrl = argv[3];
            char* jarUrl;
            javacall_parse_result status;
            javacall_utf16 jadPathUnicode[256];
            int i;
            javacall_result res;

            for (i=0; i < jadPathLen; i++) {
                jadPathUnicode[i] = jadPath[i];
            }

            res = javautil_get_jar_url_from_jad(jadPathUnicode, jadPathLen, jadUrl, &jarUrl, &status);
            if (res != JAVACALL_OK) {
                javacall_print("main(): Failed to parse JAD");
            } else {
                jadPathUnicode[0] = 'f';
                jadPathUnicode[1] = 'i';
                jadPathUnicode[2] = 'l';
                jadPathUnicode[3] = 'e';
                jadPathUnicode[4] = ':';
                jadPathUnicode[5] = '/';
                jadPathUnicode[6] = '/';

                for (i = 0; i < jadPathLen; i++) {
                    jadPathUnicode[i+7] = jadPath[i];
                }

                /*
                javanotify_install_midlet_from_filesystem(jadPathUnicode,
                                                          jadPathLen + 7, 1);
                */

                jadPathUnicode[i + 7] = 0;
                i++;
                jadPathUnicode[i + 7] = 0;

#if ENABLE_NATIVE_AMS_UI
                javanotify_ams_install_suite(1, JAVACALL_INSTALL_SRC_ANY,
                    jadPathUnicode, JAVACALL_INVALID_STORAGE_ID,
                        JAVACALL_INVALID_FOLDER_ID);
#endif
            }
        }
    } else if ((argc >= 3) && (strcmp(argv[1], "install_content") == 0)) {

        main_install_content(argc - 2, argv + 2);

    } else if (strcmp(argv[1], "i3test") == 0) {
        if (argc > 4) {
            javacall_print("i3test: too many arguments\n");
            javacall_print(usage);
            return JAVACALL_FAIL;
        }

        javanotify_start_i3test(argc > 2? argv[2]: NULL, argc == 4? argv[3]: NULL);

    } else if (strcmp(argv[1], "invoke") == 0) {
        // runMidlet invoke <handlerID> <url> [action]
        char *handlerID, *url, *action = NULL;
        switch(argc) {
            case 5:
                action = argv[4];
            case 4:
                url = argv[3];
                handlerID = argv[2];
                break;
            default:
                javacall_print("invoke: wrong number of arguments\n");
                javacall_print(usage);
                return JAVACALL_FAIL;
        }
        javanotify_start_handler(handlerID, url, action);
    } else {
        javanotify_start_java_with_arbitrary_args(argc, argv);
    }
    return JAVACALL_OK;
}

//extern char *destinationHost;
//extern int destinationPort, localPort;
//extern unsigned short defaultDestinationPort, defaultLocalPort;
char* _phonenum;

#define MAIN_ARGS_MAX  14
char  maindlg_args[4096]; /* buffer to recieve all arguments */
char* maindlg_argv[MAIN_ARGS_MAX];
int   maindlg_argc;

int main(int argc, char *main_argv[]) {

    DWORD dwJavaThreadId;
    HANDLE hJavaThread;
    char** argv = main_argv;

    _phonenum = getenv("JSR_120_PHONE_NUMBER");
    _phonenum = _phonenum ? _phonenum : "5550000";

    if ((argc == 2) && (0 == strcmp(argv[1], "dlg"))) {
        /* show UI modal dialog box to request main arguments */
        if (DialogBox(
                GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDD_DIALOG_MAIN_ARGS),
                     NULL, main_dlgproc)) {
            maindlg_argv[0] = argv[0];
            argv = maindlg_argv;
            argc = maindlg_argc;
        } else {
            argc = 1;
        }
    }

    while (1) {
        /*
        if (1 < argc && 0 == strcmp(argv[1], "destinationHost")) {
            destinationHost = argv[2];
            argc -= 2 ;
            argv += 2;
            continue;
        } else if (1 < argc && 0 == strcmp(argv[1], "destinationPort")) {
            destinationPort = atoi(argv[2]);
            argc -= 2;
            argv += 2;
            continue;
        } else if (1 < argc && 0 == strcmp(argv[1], "localPort")) {
            localPort = atoi(argv[2]);
            argc -= 2;
            argv += 2;
            continue;
        } else 
        */
        if (1 < argc && 0 == strcmp(argv[1], "debug")) {
            argc -= 1;
            argv += 1;
            continue;
        } else if (1 < argc && 0 == strcmp(argv[1], "phonenumber")) {
            _phonenum = argv[2];
	     argc -= 2;
            argv += 2;
            continue;
        } else {
            break;
        }
    } /* end of while(1) */

    /*
    if (0 >= localPort || 65535 < localPort) {
        localPort = defaultLocalPort;
    }

    if (0 >= destinationPort || 65535 < destinationPort) {
        if (NULL == destinationHost) {
            destinationPort = defaultDestinationPort;
        } else {
            destinationPort = defaultLocalPort;
        }
    }

    if (NULL == destinationHost && localPort == destinationPort) {
        printf("Destination port must different with local port\n", stderr);
        return -2;
    }
    */

    javacall_events_init();
    javacall_initialize_configurations();

#if !ENABLE_MULTIPLE_INSTANCES
    if (isSecondaryInstance()) {
        enqueueInterprocessMessage(argc, argv);
        return 0;
    }
#endif

    hJavaThread = CreateThread(
                      NULL,              // default security attributes
                      0,                 // use default stack size
                      ThreadProc,        // thread function
                      0,                 // argument to thread function
                      0,                 // use default creation flags
                      &dwJavaThreadId);   // returns the thread identifier


    lifecycle_shutdown_event = CreateEvent(NULL,FALSE,FALSE,NULL);

    if (mainArgumentsHandle(argc, argv) == JAVACALL_FAIL) {
        return -1;
    }

#if ENABLE_MULTIPLE_INSTANCES
    WaitForSingleObject(lifecycle_shutdown_event, INFINITE);
#else
    while (WaitForSingleObject(lifecycle_shutdown_event, 50) != WAIT_OBJECT_0) {
        /* Check for Interprocess event */
        int iarvc, i;
        char** iargv;

        iarvc = dequeueInterprocessMessage(&iargv);
        if (iarvc) {
            javacall_print("main(): Secondary instance parameters:\n");
            for(i = 0; i < iarvc; i++) {
                javacall_print("\t");javacall_print(iargv[i]);javacall_print("\n");
            }

            mainArgumentsHandle(iarvc, iargv);
        }
    } /* end of while(WaitForSingleObject(...)) */
#endif    

    CloseHandle(lifecycle_shutdown_event);
    return 1;
}

void main_install_content(int argc, char *argv[])
{
    javacall_utf16 descFilePath[2080]; // Max URL (2048) + Max Schema (32)
    int descFilePathLen;
    javacall_bool isJadFile, isSilent;

    MultiByteToWideChar(
        CP_UTF8, 0,
        argv[0], -1,
        descFilePath, sizeof(descFilePath) / sizeof(javacall_utf16));

    descFilePathLen = (wcslen(descFilePath) + 1) * sizeof(javacall_utf16);
    isJadFile = (argc > 2) && (strcmp(argv[2], "mimejad") == 0);
    isSilent = JAVACALL_FALSE;

    javanotify_install_content(argv[1],
        descFilePath, descFilePathLen, isJadFile, isSilent);
}

/*
 *    The function processes windows messages
 *    of the UI modal dialog box "Main Arguments"
 */

static const char* maindlg_course[] = {
    "tck", "install_file", "install_url", "install_wap"
};
#define maindlg_course_cnt (sizeof(maindlg_course) / sizeof(maindlg_course[0]))

static const UINT maindlg_items[] = {
    IDC_EDIT_URL, IDC_STATIC_URL,
    IDC_STATIC_TRUST_DOMAIN, IDC_COMBO_TRUST_HOST,
    IDC_EDIT_LOCAL_FILE, IDC_BUTTON_BROWSE, IDC_STATIC_LOCAL_FILE,
    IDC_CHECK_DEBUG
};
#define maindlg_items_cnt (sizeof(maindlg_items) / sizeof(maindlg_items[0]))

#define INDX_DEBUG      7
#define INDX_PATH       4
#define INDX_URL        0
#define INDX_TRUST      3

static const int maindlg_enable[maindlg_course_cnt][maindlg_items_cnt] = {
    { 1, 1, 1, 1, 0, 0, 0, 1 }, // tck
    { 0, 0, 0, 0, 1, 1, 1, 1 }, // install_file
    { 1, 1, 0, 0, 0, 0, 0, 1 }, // install_url
    { 1, 1, 0, 0, 0, 0, 0, 1 }  // install_wap
};

static const char* maindlg_trustdmn[] = {
    "operator", "identified", "unidentified", "minimum", "maximum"
};
#define maindlg_trustdmn_cnt (sizeof(maindlg_trustdmn) / sizeof(maindlg_trustdmn[0]))

static const char* maindlg_default_url = "http://129.156.62.219:8088/test/getNextApp.jad";
static const char* maindlg_default_path = "C:/WTK21/apps/rmsT1/bin/rmsT1.jad";

LRESULT CALLBACK main_dlgproc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int i, j, itemId = LOWORD(wParam), msgId = HIWORD(wParam);

    switch (message) {
    case WM_INITDIALOG:

        for (i = 0; i < maindlg_course_cnt; i++) {
            SendDlgItemMessage(hDlg, IDC_LIST_MODE,
                LB_ADDSTRING, 0, (LPARAM) maindlg_course[i]);
        }
        for (i = 0; i < maindlg_trustdmn_cnt; i++) {
            SendDlgItemMessage(hDlg, IDC_COMBO_TRUST_HOST,
                CB_ADDSTRING, 0, (LPARAM) maindlg_trustdmn[i]);
        }
        SendDlgItemMessage(hDlg, IDC_LIST_MODE, LB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hDlg, IDC_COMBO_TRUST_HOST, CB_SETCURSEL, 0, 0);
        SendDlgItemMessage(hDlg, IDC_EDIT_URL, WM_SETTEXT, 0, (LPARAM) maindlg_default_url);
        SendDlgItemMessage(hDlg, IDC_EDIT_LOCAL_FILE, WM_SETTEXT, 0, (LPARAM) maindlg_default_path);
        PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_LIST_MODE, LBN_SELCHANGE), 0);
        return TRUE;

    case WM_COMMAND:

        if ((itemId == IDCANCEL) && (msgId == 0)) {
            EndDialog(hDlg, FALSE);
            break;
        }

        if ((itemId == IDOK) && (msgId == 0)) {

            int len; /* length of an argument */ 
            int lenbuf = 0; /* length of the buffer */
            i = SendDlgItemMessage(hDlg, IDC_LIST_MODE, LB_GETCURSEL, 0, 0);
            if (i < 0) {
                break;
            }

            maindlg_argc = 1;

            if (maindlg_enable[i][INDX_DEBUG]) {
                len = SendDlgItemMessage(hDlg, IDC_CHECK_DEBUG, BM_GETCHECK, 0, 0);
                if (len & BST_CHECKED) {
                    maindlg_argv[maindlg_argc++] = "debug";
                }
            }

            len = SendDlgItemMessage(hDlg, IDC_LIST_MODE, LB_GETTEXT,
                i, (LPARAM) maindlg_args + lenbuf);
            maindlg_argv[maindlg_argc++] = maindlg_args + lenbuf;
            lenbuf += len + 1;

            if (maindlg_enable[i][INDX_PATH]) {
                len = SendDlgItemMessage(hDlg, IDC_EDIT_LOCAL_FILE, WM_GETTEXT,
                    sizeof(maindlg_args) - lenbuf, (LPARAM) maindlg_args + lenbuf);
                if (len) {
                    maindlg_argv[maindlg_argc++] = maindlg_args + lenbuf;
                    lenbuf += len + 1;
                }
            }

            if (maindlg_enable[i][INDX_URL]) {
                len = SendDlgItemMessage(hDlg, IDC_EDIT_URL, WM_GETTEXT,
                    sizeof(maindlg_args) - lenbuf, (LPARAM) maindlg_args + lenbuf);
                if (len) {
                    maindlg_argv[maindlg_argc++] = maindlg_args + lenbuf;
                    lenbuf += len + 1;
                }
            }

            if (maindlg_enable[i][INDX_TRUST]) {
                len = SendDlgItemMessage(hDlg, IDC_COMBO_TRUST_HOST, WM_GETTEXT,
                    sizeof(maindlg_args) - lenbuf, (LPARAM) maindlg_args + lenbuf);
                if (len) {
                    maindlg_argv[maindlg_argc++] = maindlg_args + lenbuf;
                    lenbuf += len + 1;
                }
            }

            EndDialog(hDlg, TRUE);

#ifdef _DEBUG
            javacall_print("[main dialog] the arguments: \n");
            for (i = 1; i < maindlg_argc; i++) {
                javacall_print("\t\t");
                javacall_print(maindlg_argv[i]);
                javacall_print("\n");
            }
#endif
            break;
        }

        if ((itemId == IDC_BUTTON_BROWSE) && (msgId == 0)) {

            char path[MAX_PATH];
            int  length;
            OPENFILENAME ofn = {
                sizeof (OPENFILENAME), NULL, GetModuleHandle(NULL),
                "JAD Files\0*.jad\0GCD Files\0*.gcd\0All Files\0*.*\0\0",
                NULL, 0, 0,
                path, MAX_PATH,
                NULL, 0, NULL,
                "Open descriptor file",
                OFN_FILEMUSTEXIST,
                0, 0, NULL, 0, NULL, NULL
            };

            HWND wndPath = GetDlgItem(hDlg, IDC_EDIT_LOCAL_FILE);
            length = SendMessage(wndPath, WM_GETTEXT, MAX_PATH, (LPARAM) path);
            if (length > 0) {
                HANDLE hFile = CreateFile(path, FILE_READ_DATA, FILE_SHARE_READ, NULL,
					OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
                if (hFile == INVALID_HANDLE_VALUE) {
                    path[0] = 0;
                }
                CloseHandle(hFile);
            }
            
            if (GetOpenFileName(&ofn) && *path) {
                SendMessage(wndPath, WM_SETTEXT, 0, (LPARAM) path);
            }
            break;
        }

    
        if ((itemId == IDC_LIST_MODE) && (msgId == LBN_SELCHANGE)) {
 
                i = SendDlgItemMessage(hDlg, IDC_LIST_MODE, LB_GETCURSEL, 0, 0);
                for (j = 0; j < maindlg_items_cnt; j++) {
                    EnableWindow(GetDlgItem(hDlg, maindlg_items[j]),
                        maindlg_enable[i][j]);
                }
        }
        // End of case WM_COMMAND
    
    } // End of switch(message)

    return 0;
} // End of main_dlgproc(...)
