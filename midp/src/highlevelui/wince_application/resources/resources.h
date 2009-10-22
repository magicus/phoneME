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

/*
 * Here are the IDs for the statically assigned menus and buttons. Make sure
 * that no IDs are are above ID_DYNAMIC_MENU.
 */
#define  ID_MENU_SIMPLE    10
#define  ID_MENU_POPUP     11
#define  IDM_SOFTBTN_0     100
#define  IDM_SOFTBTN_1     101
#define  IDS_EMPTY         1001
#define  IDS_QUIT          1002
#define  IDS_MENU          1003

/*
 * If a MIDP command is added to the popup menu, its ID will be
 * D_DYNAMIC_MENU + javax.microedition.lcdui.Command.id
 */
#define ID_DYNAMIC_MENU 2000
