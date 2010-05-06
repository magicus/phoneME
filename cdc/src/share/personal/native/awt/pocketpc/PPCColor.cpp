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

#include "awt.h"
#include "java_awt_Color.h"
#include "sun_awt_pocketpc_PPCColor.h"

JNIEXPORT jobject JNICALL
Java_sun_awt_pocketpc_PPCColor_getDefaultColor(JNIEnv *env,
					       jclass cls, 
					       jint index)
{
    int iColor = 0;
    switch(index) {
      case sun_awt_pocketpc_PPCColor_WINDOW_BKGND: 
        iColor = COLOR_WINDOW; 
        break;
      case sun_awt_pocketpc_PPCColor_WINDOW_TEXT:
        iColor = COLOR_WINDOWTEXT; 
        break;
      case sun_awt_pocketpc_PPCColor_FRAME:
        iColor = COLOR_WINDOWFRAME; 
        break;
      case sun_awt_pocketpc_PPCColor_SCROLLBAR:
        iColor = COLOR_SCROLLBAR; 
        break;
      case sun_awt_pocketpc_PPCColor_MENU_BKGND:
        iColor = COLOR_MENU; 
        break;
      case sun_awt_pocketpc_PPCColor_MENU_TEXT:
        iColor = COLOR_MENUTEXT; 
        break;
      case sun_awt_pocketpc_PPCColor_BUTTON_BKGND:
        iColor = (IS_NT) ? COLOR_BTNFACE : COLOR_3DFACE; 
        break;
      case sun_awt_pocketpc_PPCColor_BUTTON_TEXT:
        iColor = COLOR_BTNTEXT; 
        break;
      case sun_awt_pocketpc_PPCColor_HIGHLIGHT:
        iColor = COLOR_HIGHLIGHT; 
        break;

      default:
          return NULL;
    }
    DWORD c = ::GetSysColor(iColor);
    jobject hColor = env->NewObject(WCachedIDs.java_awt_ColorClass,
				    WCachedIDs.java_awt_Color_constructorMID,
				    GetRValue(c), 
				    GetGValue(c), 
				    GetBValue(c));
    ASSERT(!env->ExceptionCheck());
    ASSERT(hColor != NULL);
    return hColor;
}
