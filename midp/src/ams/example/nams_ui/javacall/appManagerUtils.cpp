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

#include "appManagerUtils.h"

#include <tchar.h>


#define USE_DYNAMIC_BUTTON_SIZE


void PrintWindowSize(HWND hWnd, LPTSTR pszName) {
    RECT rcWnd, rcOwner;
    HWND hOwner;

    if ((hOwner = GetParent(hWnd)) == NULL) {
        hOwner = GetDesktopWindow();
    }

    GetWindowRect(hOwner, &rcOwner);
    GetWindowRect(hWnd, &rcWnd);

    wprintf(_T("%s size: x=%d, y=%d, w=%d, h=%d\n"), pszName,
            rcWnd.left - rcOwner.left,
            rcWnd.top - rcOwner.top,
            rcWnd.right - rcWnd.left,
            rcWnd.bottom - rcWnd.top);
}

SIZE GetButtonSize(HWND hBtn) {
    SIZE res;

    res.cx = 0;
    res.cx = 0;
    
    if (hBtn) {
#ifdef USE_DYNAMIC_BUTTON_SIZE
        int nBtnTextLen, nBtnHeight, nBtnWidth;
        HDC hdc;
        TEXTMETRIC tm;
        TCHAR szBuf[127];

        hdc = GetDC(hBtn);

        if (GetTextMetrics(hdc, &tm)) {

            nBtnTextLen = GetWindowText(hBtn, szBuf, sizeof(szBuf));

            nBtnWidth = (tm.tmAveCharWidth * nBtnTextLen) +
                (2 * DLG_BUTTON_MARGIN);

            nBtnHeight = (tm.tmHeight + tm.tmExternalLeading) +
                DLG_BUTTON_MARGIN;

            ReleaseDC(hBtn, hdc);

            res.cx = nBtnWidth;
            res.cy = nBtnHeight;
        }
#else
        RECT rc;

        GetClientRect(hBtn, &rc);

        res.cx = rc.right;
        res.cy = rc.bottom;     
#endif

    }

    return res;
}
