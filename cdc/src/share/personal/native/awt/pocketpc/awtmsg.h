/*
 * @(#)awtmsg.h	1.5 06/10/10
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

#ifndef AWTMSG_H
#define AWTMSG_H

// AwtComponent messages
#define AWT_MSG_BASE                 WM_USER+100
#define WM_AWT_COMPONENT_CREATE	     AWT_MSG_BASE+0
#define WM_AWT_DESTROY_WINDOW        AWT_MSG_BASE+1
#define WM_AWT_TOOLKIT_CLEANUP       AWT_MSG_BASE+2
#define WM_AWT_CALLBACK              AWT_MSG_BASE+3
#define WM_AWT_MOUSEENTER 	     AWT_MSG_BASE+4
#define WM_AWT_MOUSEEXIT 	     AWT_MSG_BASE+5
#define WM_AWT_MOUSECHECK 	     AWT_MSG_BASE+6
#define WM_AWT_COMPONENT_SHOW        AWT_MSG_BASE+7
#define WM_AWT_POPUPMENU_SHOW        AWT_MSG_BASE+8
#define WM_AWT_COMPONENT_SETFOCUS    AWT_MSG_BASE+9
#define WM_AWT_LIST_SETMULTISELECT   AWT_MSG_BASE+10
#define WM_AWT_HANDLE_EVENT          AWT_MSG_BASE+11
#define WM_AWT_PRINT_COMPONENT       AWT_MSG_BASE+12
#define WM_AWT_RESHAPE_COMPONENT     AWT_MSG_BASE+13
#define WM_AWT_BEGIN_VALIDATE        AWT_MSG_BASE+14
#define WM_AWT_END_VALIDATE          AWT_MSG_BASE+15
//	#define WM_AWT_COMPONENT_ENABLE      AWT_MSG_BASE+16
// 	#define WM_AWT_QUEUE_SYNC            AWT_MSG_BASE+17 // Bug #4048664 (Hide does not work on modal dialog)
#define WM_AWT_FORWARD_CHAR          AWT_MSG_BASE+18
#define WM_AWT_FORWARD_BYTE          AWT_MSG_BASE+19
#define WM_AWT_SET_SCROLL_INFO       AWT_MSG_BASE+20
//	#define WM_AWT_WINDOWTOFRONT         AWT_MSG_BASE+21    // used to avoid using
                                                        // setforegroundwindow in 
                                                        // awt_dialog.
#define WM_AWT_DISPOSE               AWT_MSG_BASE+22
#define WM_AWT_DLG_SHOWMODAL	     AWT_MSG_BASE+23
#define WM_AWT_DLG_ENDMODAL	     AWT_MSG_BASE+24
#define WM_AWT_EXECUTE_SYNC	     AWT_MSG_BASE+25


#endif  // AWTMSG_H
