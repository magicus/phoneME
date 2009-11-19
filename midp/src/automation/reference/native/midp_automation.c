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

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>

#include <jvmspi.h>
#include <sni.h>

#include <midpError.h>
#include <midpUtilKni.h>
#include <keymap_input.h>
#include <pcsl_string.h>
#include <midp_logging.h>
#include <midp_foreground_id.h>
#include <midpCommandState.h>
#include <midpEventUtil.h>
#include <midpServices.h>


#include <string.h>

static struct KeyCodeNameToMIDPKeyCode {
    const char* name;
    int midpKeyCode;
} keyCodeNameToMIDPKeyCode[] = {
    { "backspace", KEYMAP_KEY_BACKSPACE },
    { "up",        KEYMAP_KEY_UP        },
    { "down",      KEYMAP_KEY_DOWN      },
    { "left",      KEYMAP_KEY_LEFT      },
    { "right",     KEYMAP_KEY_RIGHT     },
    { "select",    KEYMAP_KEY_SELECT    },
    { "soft1",     KEYMAP_KEY_SOFT1     },
    { "soft2",     KEYMAP_KEY_SOFT2     },
    { "clear",     KEYMAP_KEY_CLEAR     },
    { "send",      KEYMAP_KEY_SEND      },
    { "end",       KEYMAP_KEY_END       },
    { "power",     KEYMAP_KEY_POWER     },
    { "gamea",     KEYMAP_KEY_GAMEA     },
    { "gameb",     KEYMAP_KEY_GAMEB     },
    { "gamec",     KEYMAP_KEY_GAMEB     },
    { "gamed",     KEYMAP_KEY_GAMEB     },
};

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_automation_AutoKeyCode_getMIDPKeyCodeForName) {
    int i;
    int sz = sizeof(keyCodeNameToMIDPKeyCode)/
        sizeof(struct KeyCodeNameToMIDPKeyCode);

    int midpKeyCode = KEYMAP_KEY_INVALID;
    const char* keyCodeName;

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, keyCodeNamePCSL)

    keyCodeName = (const char*)pcsl_string_get_utf8_data(&keyCodeNamePCSL);
 
    for (i = 0; i < sz; ++i) {
        const char* name = keyCodeNameToMIDPKeyCode[i].name;
        if (strcmp(keyCodeName, name) == 0) {
            midpKeyCode = keyCodeNameToMIDPKeyCode[i].midpKeyCode;
            break;
        }
    }

    if (midpKeyCode == KEYMAP_KEY_INVALID) {
         REPORT_ERROR1(LC_CORE, 
            "AutoKeyEventImpl_getMIDPKeyCodeForName: unknown key code %s", 
            keyCodeName);
    }

    RELEASE_PCSL_STRING_PARAMETER
    KNI_EndHandles();
        
    KNI_ReturnInt(midpKeyCode);
}


KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_automation_AutomationImpl_getForegroundIsolateAndDisplay) {

    KNI_StartHandles(1);
    KNI_DeclareHandle(foregroundIsolateAndDisplay);
    KNI_GetParameterAsObject(1, foregroundIsolateAndDisplay);

    KNI_SetIntArrayElement(foregroundIsolateAndDisplay, 
            0, (jint)gForegroundIsolateId);

    KNI_SetIntArrayElement(foregroundIsolateAndDisplay, 
            1, (jint)gForegroundDisplayId);
    
    KNI_EndHandles();
    KNI_ReturnVoid();    
}

KNIEXPORT KNI_RETURNTYPE_BOOLEAN
KNIDECL(com_sun_midp_automation_AutomationInitializer_isVMRestarted) {
#if ENABLE_MULTIPLE_ISOLATES
    KNI_ReturnBoolean(KNI_FALSE);
#else
    MIDPCommandState* state = midpGetCommandState();
    KNI_ReturnBoolean(state->vmRestarted);
#endif
}

KNIEXPORT KNI_RETURNTYPE_VOID
KNIDECL(com_sun_midp_automation_AutomationImpl_sendShutdownEvent) {
    MidpEvent evt;

    MIDP_EVENT_INITIALIZE(evt);
    evt.type = SHUTDOWN_EVENT;
    midpStoreEventAndSignalAms(evt);
}

