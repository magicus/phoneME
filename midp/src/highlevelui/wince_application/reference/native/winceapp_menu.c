/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 * This file implements the handling of soft buttons and menus on
 * the WinCE platform.
 */

#include <windows.h>
#include <windowsx.h>
#include <aygshell.h>
#include <commctrl.h>
#include <sipapi.h>

#include <jvmconfig.h>
#include <kni.h>
#include <midpError.h>
#include <midp_logging.h>
#include <winceapp_export.h>
#include <gxj_putpixel.h>
#include <midpMalloc.h>
#include <midp_properties_port.h>
#include <midp_constants_data.h>
#include <keymap_input.h>
#include <commonKNIMacros.h>
#include "resources.h"
#include <midpEventUtil.h>

/* defined in winceapp_export.cpp */
extern HWND hwndMenuBar;       /* The currently displayed menu bar. */
extern HWND hwndMenuBarSimple; /* The menu bar with only 2 soft buttons */
extern HWND hwndMenuBarPopup;  /* The menu bar with a popup menu */
extern HMENU hmenuPopup;       /* the menu used to display the MIDP commands */
extern HMENU hmenuMain;        /* Handle to the main menu */


KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_layers_SoftButtonLayer_setNativeSoftButton) {
    int index;
    int labelLen;
    jchar *label;

    KNI_StartHandles(1);
    KNI_DeclareHandle(labelHandle);
    KNI_GetParameterAsObject(2, labelHandle);

    labelLen = KNI_GetStringLength(labelHandle);
    if (labelLen > 0) {
        label = (jchar*)midpMalloc(labelLen * sizeof(jchar));
        KNI_GetStringRegion(labelHandle, 0, labelLen, label);
        index = KNI_GetParameterAsInt(1);
        native_set_softbutton(index, label, labelLen);
        midpFree(label);
    }
    KNI_EndHandles();
    KNI_ReturnVoid();
}

#define MAX_MENU_LEN 40

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_layers_SoftButtonLayer_setNativePopupMenu) {
/** 
 * IMPL_NOTE: The events from the menu are not transfer up to java without displaying and
 * addition menu.  Disable this feature for now
 */
#if 0
    int i;
    int numMenus = 0;
    jchar* str;
    DWORD err;
    TCHAR buff[MAX_MENU_LEN+1]; /* can't fit more than 40 chars on screen */

    KNI_StartHandles(4);
    KNI_DeclareHandle(menu);
    KNI_DeclareHandle(command);
    KNI_DeclareHandle(strHandle);
    KNI_DeclareHandle(commandClass);

    KNI_GetParameterAsObject(1, menu);
    numMenus = (int)KNI_GetArrayLength(menu);

    if (menu != NULL && numMenus > 0) {
        /* Delete all existing menu items */
        while (DeleteMenu(hmenuPopup, 0, MF_BYPOSITION)) {;}

        /* Append new menu items */
        for (i=0; i<numMenus; i++) {
            int count;
            jfieldID field;
            /**
             * It's OK to have tons of menu items here. Windows will
             * scroll the menu vertically if they cannot all fit on
             * the screen.
             */
            KNI_GetObjectArrayElement(menu, i, command);
            KNI_GetObjectClass(command, commandClass);

            field = KNI_GetFieldID(commandClass, "shortLabel", "Ljava/lang/String;");
            KNI_GetObjectField(command, field, strHandle);

            count = KNI_GetStringLength(strHandle);
            if (count > MAX_MENU_LEN) {
                count = MAX_MENU_LEN;
            }

            str = (jchar*)midpMalloc(count * sizeof(jchar));
            if (str != NULL) {
                KNI_GetStringRegion(strHandle, 0, count, str);
            }

            memcpy(buff, str, count*sizeof(jchar));
            buff[count] = 0;

            if (!AppendMenu(hmenuPopup, MF_STRING, ID_DYNAMIC_MENU + i, buff)) {
                /* Probably ran out of memory */
                break;
            }
            midpFree(str);
        }
    }

    if (numMenus == 0) {
        if (hwndMenuBar != hwndMenuBarSimple) {
            ShowWindow(hwndMenuBar, SW_HIDE);
            hwndMenuBar = hwndMenuBarSimple;
            ShowWindow(hwndMenuBar, SW_SHOW);
        }
    } else {
        if (hwndMenuBar != hwndMenuBarPopup) {
            ShowWindow(hwndMenuBar, SW_HIDE);
            hwndMenuBar = hwndMenuBarPopup;
            ShowWindow(hwndMenuBar, SW_SHOW);
        }
    }
    KNI_EndHandles();
#endif
    while (DeleteMenu(hmenuPopup, 0, MF_BYPOSITION)) {;}
    if (!AppendMenu(hmenuPopup, MF_STRING, ID_DYNAMIC_MENU, TEXT("Menu"))) {
        NKDbgPrintfW(TEXT("Append Menu Failed\n"));              
    }
    if (hwndMenuBar != hwndMenuBarPopup) {
        ShowWindow(hwndMenuBar, SW_HIDE);
        hwndMenuBar = hwndMenuBarPopup;
        ShowWindow(hwndMenuBar, SW_SHOW);
    }
    KNI_ReturnVoid();
}

