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

#include <kni.h>
#include <javacall_input.h>
#include <midpEvents.h>
#include <midp_foreground_id.h>
#include <pcsl_string.h>
#include <cdc_natives.h>

#ifdef __cplusplus
extern "C" {
#endif

static void callbackStringEntered(javacall_utf16_string ptr, int str_len);

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_input_VirtualKeyboardInputMode_showNativeKeyboard) {
    javacall_native_virtual_keyboard(JAVACALL_TRUE, NULL, 0, 200, callbackStringEntered);
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_input_VirtualKeyboardInputMode_hideNativeKeyboard) {
    javacall_native_virtual_keyboard(JAVACALL_FALSE, NULL, 0, 0, NULL);
    KNI_ReturnVoid();
}

static void callbackStringEntered(javacall_utf16_string ptr, int str_len) {
  MidpEvent event;
  if (!str_len) {
	return;
  }
  MIDP_EVENT_INITIALIZE(event);
  event.intParam1 = 44 /* EventConstants.IME2 */;
  event.type = 580; // EventTypes.VIRTUAL_KEYBOARD_RETURN_DATA_EVENT
  event.DISPLAY = gForegroundDisplayId;

  event.stringParam1.length = str_len;
  pcsl_string_convert_from_utf16(ptr, str_len, &event.stringParam1); // TBD: check return status
  sendMidpKeyEvent(&event);
}

#ifdef __cplusplus
}
#endif
