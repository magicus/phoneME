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

#define  ID_MENU_MAIN                5
#define  ID_MENU_POPUP_MIDLET       10
#define  ID_MENU_POPUP_SUITE        15
#define  ID_MENU_POPUP_FOLDER       20

// was in javacall/lcd.h
#define  WM_DEBUGGER      (WM_USER)
#define  WM_HOST_RESOLVED (WM_USER + 1)
#define  WM_NETWORK       (WM_USER + 2)

#define  WM_NOTIFY_SUITE_INSTALLED (WM_USER + 3)

#define  WM_JAVA_AMS_INSTALL_ASK      (WM_USER + 100)
#define  WM_JAVA_AMS_INSTALL_STATUS   (WM_USER + 101)
#define  WM_JAVA_AMS_INSTALL_FINISHED (WM_USER + 102)

#define  IDM_INFO                  100

#define  IDM_MIDLET_START_STOP     110
#define  IDM_MIDLET_TO_FOREGROUND  120
#define  IDM_MIDLET_INFO           130

#define  IDM_SUITE_INFO            200
#define  IDM_SUITE_SETTINGS        210
#define  IDM_SUITE_INSTALL         220
#define  IDM_SUITE_REMOVE          230
#define  IDM_SUITE_UPDATE          240
#define  IDM_SUITE_EXIT            250
#define  IDM_SUITE_CUT             260

#define  IDM_FOLDER_INFO           300
#define  IDM_FOLDER_INSTALL_INTO   310
#define  IDM_FOLDER_REMOVE_ALL     320
#define  IDM_FOLDER_PASTE          330

#define  IDM_WINDOW_APP_MANAGER    400

#define  IDM_WINDOW_FIRST_ITEM     410
#define  IDM_WINDOW_LAST_ITEM      450

#define  IDM_HELP_ABOUT            500

#define  IDD_INFO                  600
#define  IDD_PERMISSIONS           610
#define  IDD_INSTALL_PATH          620
#define  IDD_INSTALL_PROGRESS      630

#define  IDC_TREEVIEW_MIDLETS      901
#define  IDC_TREEVIEW              902

#define  IDC_EDIT_URL              903
#define  IDC_STATIC_URL            904
#define  IDC_STATIC_BROWSE         905
#define  IDC_STATIC_FOLDER         906
#define  IDC_COMBO_FOLDER          908
#define  IDC_BUTTON_FILE           909

#define  IDC_PROGRESS_OPERATION    910
#define  IDC_PROGRESS_TOTAL        911
#define  IDC_EDIT_INFO             912
#define  IDC_STATIC_DETAILS        913
#define  IDC_STATIC_TOTAL          914
#define  IDC_STATIC_OPERATION      915

#define  IDC_MAIN_TOOLBAR          930

#define  IDB_MAIN_TOOLBAR_BUTTONS  940
#define  IDB_DEF_SUITE_ICON        950
#define  IDB_DEF_MIDLET_ICON       960
#define  IDB_DEF_MIDLET_ICON_1     961
#define  IDB_DEF_MIDLET_ICON_2     962
#define  IDB_DEF_MIDLET_ICON_3     963
#define  IDB_FOLDER_ICON           970

#define  IDI_PERMISSON_DENY       1000
#define  IDI_PERMISSON_ONESHOT    1001
#define  IDI_PERMISSON_SESSION    1002
#define  IDI_PERMISSON_ALLOW      1003
