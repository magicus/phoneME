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

#include "appManager.h"
#include "appManagerProgress.h"
#include "appManagerUtils.h"

#include <javacall_memory.h>
#include <javacall_ams_installer.h>


static HWND g_hProgressDlg = NULL;


static LONG g_szInstallRequestText[][2] = 
{ 
{(LONG)JAVACALL_INSTALL_REQUEST_WARNING, (LONG)_T("Warning!")},
{(LONG)JAVACALL_INSTALL_REQUEST_CONFIRM_JAR_DOWNLOAD, (LONG)_T("Download the JAR file?")},
{(LONG)JAVACALL_INSTALL_REQUEST_KEEP_RMS, (LONG)_T("Keep the RMS?")},
{(LONG)JAVACALL_INSTALL_REQUEST_CONFIRM_AUTH_PATH, (LONG)_T("Trust the authorization path?")},
{(LONG)JAVACALL_INSTALL_REQUEST_CONFIRM_REDIRECTION, (LONG)_T("Allow redirection?")}
};

#define INSTALL_REQUEST_NUM \
    ((int) (sizeof(g_szInstallRequestText) / sizeof(g_szInstallRequestText[0])))


INT_PTR CALLBACK ProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,
                                LPARAM lParam);

void ShowProgressDialog(BOOL fShow) {
    if (g_hProgressDlg) {
        ShowWindow(g_hProgressDlg, (fShow) ? SW_SHOW : SW_HIDE);
    }
}

PostProgressMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
  if (g_hProgressDlg) {
      return PostMessage(g_hProgressDlg, uMsg, wParam, lParam);
  }
  return FALSE;
}

BOOL CreateProgressDialog(HINSTANCE hInstance, HWND hWndParent) {
    HWND hDlg;
    RECT rcClient;
    HWND hBtnNo;
    SIZE sizeButtonN; 
    javacall_result res;

    hDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_INSTALL_PROGRESS),
                        hWndParent, ProgressDlgProc);

    if (!hDlg) {
        MessageBox(hWndParent, _T("Create install progress dialog failed!"),
                   g_szTitle, NULL);
        return FALSE;
    }

    // Get the dimensions of the parent window's client area
    GetClientRect(hWndParent, &rcClient); 

    // Set actual dialog size
    SetWindowPos(hDlg,
                 0, // ignored by means of SWP_NOZORDER
                 0, 0, // x, y
                 rcClient.right, rcClient.bottom, // w, h
                 SWP_NOZORDER | SWP_NOOWNERZORDER |
                     SWP_NOACTIVATE);

    PrintWindowSize(hDlg, _T("Progress dialog"));

    // Get handle to Cancel button (the Cancel button may be absent
    // on the dialog)
    hBtnNo = GetDlgItem(hDlg, IDCANCEL);

    if (hBtnNo) {
        sizeButtonN = GetButtonSize(hBtnNo);

        SetWindowPos(hBtnNo,
                     0, // ignored by means of SWP_NOZORDER
                     rcClient.right - sizeButtonN.cx,  // x
                     rcClient.bottom - sizeButtonN.cy, // y
                     sizeButtonN.cx, sizeButtonN.cy,   // w, h
                     SWP_NOZORDER | SWP_NOOWNERZORDER |
                         SWP_NOACTIVATE);


        PrintWindowSize(hBtnNo, _T("Cancel button"));
    }

    // TODO: implement dynamic positioning and resize for the rest controls of
    // the dialog.

    g_hProgressDlg = hDlg;

    return TRUE;
}

INT_PTR CALLBACK
ProgressDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    javacall_result res = JAVACALL_FAIL;

    switch (uMsg) {

    case WM_COMMAND: {
        WORD wCmd = LOWORD(wParam);

        switch (wCmd) {

        case IDCANCEL: {
            CloseInstallerDlg(hwndDlg);
            break;
        }

        default: {
            return FALSE;
        }

        } // end of switch (wCmd)

        return TRUE;
    }

    case WM_JAVA_AMS_INSTALL_ASK: {
        javacall_ams_install_data resultData;
        javacall_ams_install_request_code requestCode;
        javacall_ams_install_state* pInstallState;
        int nRes;
        LPTSTR pszText = NULL;

        requestCode = (javacall_ams_install_request_code)wParam;
        pInstallState = (javacall_ams_install_state*)lParam;

        for (int i = 0; i < INSTALL_REQUEST_NUM; i++) {
            if (g_szInstallRequestText[i][0]  == (LONG)requestCode) {
                pszText = (LPTSTR)g_szInstallRequestText[i][1];
                break;
            }
        }

        if (pszText) {
            nRes = MessageBox(hwndDlg, pszText, g_szTitle,
                              MB_ICONQUESTION | MB_YESNO);

            resultData.fAnswer = (nRes == IDYES) ?
                JAVACALL_TRUE : JAVACALL_FALSE;
        } else {
            MessageBox(hwndDlg,
                       _T("Unknown confirmation has been requested!"),
                       g_szTitle, NULL);
            resultData.fAnswer = JAVACALL_TRUE;
        }

        res = java_ams_install_answer(requestCode, pInstallState, &resultData);

        if (res != JAVACALL_OK) {
            wprintf(_T("ERROR: java_ams_install_answer() ")
                    _T("returned %d\n"), (int)res);
        }

        break;
    }

    case WM_JAVA_AMS_INSTALL_STATUS: {
       static nDownloaded = 0;

       HWND hOperProgress, hTotalProgress, hEditInfo;
       WORD wCurProgress, wTotalProgress;
       javacall_ams_install_status status;
       TCHAR szBuf[127];
       LPTSTR pszInfo;

       hOperProgress = GetDlgItem(hwndDlg, IDC_PROGRESS_OPERATION);

       if (hOperProgress) {
           wCurProgress = LOWORD(wParam);

            // Validate progress values
           if (wCurProgress < 0) {
               wCurProgress = 0;
           } else if (wCurProgress > 100) {
              wCurProgress = 100;
           }
       
           SendMessage(hOperProgress, PBM_SETPOS, (WPARAM)wCurProgress, 0);
       }

       hTotalProgress = GetDlgItem(hwndDlg, IDC_PROGRESS_TOTAL);

       if (hTotalProgress) {
           wTotalProgress = HIWORD(wParam);

           if (wTotalProgress < 0) {
              wTotalProgress = 0;
           } else if (wTotalProgress > 100) {
              wTotalProgress = 100;
           }

           SendMessage(hTotalProgress, PBM_SETPOS, (WPARAM)wTotalProgress, 0);
       }

       hEditInfo = GetDlgItem(hwndDlg, IDC_EDIT_INFO);

       if (hEditInfo) {
           status = (javacall_ams_install_status)lParam;

           switch (status) {

           case JAVACALL_INSTALL_STATUS_DOWNLOADING_JAD:
           case JAVACALL_INSTALL_STATUS_DOWNLOADING_JAR: {
               nDownloaded = 0;

               pszInfo = (status == JAVACALL_INSTALL_STATUS_DOWNLOADING_JAD) ?
                   _T("JAD") : _T("JAR");
               wsprintf(szBuf, _T("Downloading of the %s is started"), pszInfo);
               break;
           }

           case JAVACALL_INSTALL_STATUS_DOWNLOADED_1K_OF_JAD:
           case JAVACALL_INSTALL_STATUS_DOWNLOADED_1K_OF_JAR: {
               nDownloaded++;

               pszInfo = 
                   (status == JAVACALL_INSTALL_STATUS_DOWNLOADED_1K_OF_JAD) ?
                   _T("JAD") : _T("JAR");
               wsprintf(szBuf, _T("%dK of %s downloaded"), nDownloaded, pszInfo);
               break;
           }

           case JAVACALL_INSTALL_STATUS_VERIFYING_SUITE: {
               wsprintf(szBuf, _T("Verifing the suite..."));
               break;
           }

           case JAVACALL_INSTALL_STATUS_GENERATING_APP_IMAGE: {
               wsprintf(szBuf, _T("Generating application image..."));
               break;
           }

           case JAVACALL_INSTALL_STATUS_VERIFYING_SUITE_CLASSES: {
               wsprintf(szBuf, _T("Verifing classes of the suite..."));
               break;
           }

           case JAVACALL_INSTALL_STATUS_STORING_SUITE: {
               wsprintf(szBuf, _T("Storing the suite..."));
               break;
           }

           default: {
               // Show no text if status is unknown
               wsprintf(szBuf, _T(""));
               break;
           }

           } // end of switch (installStatus)

            SendMessage(hEditInfo, WM_SETTEXT, 0, (LPARAM)szBuf);
       }

       break;
    }

    case WM_JAVA_AMS_INSTALL_FINISHED: {
       TCHAR szBuf[127];
       javacall_ams_install_data* pResult = (javacall_ams_install_data*)lParam;

       if (pResult && 
               (pResult->installStatus == JAVACALL_INSTALL_STATUS_COMPLETED) &&
               (pResult->installResultCode == JAVACALL_INSTALL_EXC_ALL_OK)) {
           MessageBox(hwndDlg, _T("Installation completed!"), g_szTitle,
                      MB_ICONINFORMATION | MB_OK);
       } else {
           wsprintf(szBuf,
                    _T("Installation failed!\n\n Error status %d, code %d"),
                    pResult->installStatus, pResult->installResultCode);
           MessageBox(hwndDlg, szBuf, g_szTitle, MB_ICONERROR | MB_OK);
       }

       // Free memeory alloced by us in java_ams_operation_completed
       javacall_free(pResult);

       CloseInstallerDlg(hwndDlg);

       break;
    }

    default: {
        return FALSE;
    }

    } // end of switch (uMsg)

    return TRUE;
}
