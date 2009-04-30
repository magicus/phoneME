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
#include <native_virt_keyboard.h>
#include <javacall_input.h>
#include <midpEvents.h>
#include <midp_foreground_id.h>
#include <pcsl_string.h>
#include <cdc_natives.h>

#include <javavm/include/porting/sync.h>



#ifdef __cplusplus
extern "C" {
#endif

static int stringEnteredLength;
static void callbackStringEntered(javacall_utf16_string ptr, int str_len);
static javacall_utf16_string stringEntered;
static CVMMutex   mutex;
static CVMCondVar condVar;

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_input_VirtualKeyboardInputMode_showNativeKeyboard) {
	int str_len = 0;
    javacall_utf16_string retStr = get_str_from_virt_kbd(NULL, 0, 200, &str_len, _ee); //TBD - set the input string
	if (str_len > 0) {
      MidpEvent event;
      MIDP_EVENT_INITIALIZE(event);
      event.intParam1 = 44 /* EventConstants.IME2 */;
      event.type = 580; // EventTypes.VIRTUAL_KEYBOARD_RETURN_DATA_EVENT
      event.DISPLAY = gForegroundDisplayId;

      event.stringParam1.length = str_len;
      pcsl_string_convert_from_utf16(retStr, str_len, &event.stringParam1); // TBD: check return status
      sendMidpKeyEvent(&event);
    }
	
    KNI_ReturnVoid();
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_chameleon_input_VirtualKeyboardInputMode_hideNativeKeyboard) {
    KNI_ReturnVoid();
}

/**
 * Displays the native keyboard and returns the entered string.
 * @param prepopulate prepopulate string
 * @param mode  mode value: 0  - qwerty alpha mode,
 *                          1  - qwerty number mode,
 * @param maxSize  the maximal string length
 * @param string_length ptr to the length of entered string
 * @return string which entered from the keyboard
 */
javacall_utf16_string get_str_from_virt_kbd(javacall_utf16_string prepopulate, int mode, int maxSize, int* string_length, CVMExecEnv* _ee) {
  stringEntered = NULL;
  stringEnteredLength = 0;
  CVMmutexInit(&mutex);
  CVMcondvarInit(&condVar, &mutex);
  if (JAVACALL_OK == javacall_native_virtual_keyboard(
	                  JAVACALL_TRUE, prepopulate,
	                  mode, maxSize, callbackStringEntered)) {
    CVMD_gcSafeExec(_ee, {
      CVMmutexLock(&mutex);
      while (stringEntered == NULL) {
        CVMcondvarWait(&condVar, &mutex, 0);
      }
      CVMmutexUnlock(&mutex);
    });
  }
  CVMcondvarDestroy(&condVar);
  CVMmutexDestroy(&mutex);
  *string_length = stringEnteredLength;
  return stringEntered;
}

void callbackStringEntered(javacall_utf16_string ptr, int str_len) {
  stringEntered = ptr;
  stringEnteredLength = str_len;
  CVMcondvarNotifyAll(&condVar);
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_chameleon_input_VirtualKeyboardInputMode_isPopUp) {
  KNI_ReturnBoolean(KNI_FALSE);
}


#ifdef __cplusplus
}
#endif
