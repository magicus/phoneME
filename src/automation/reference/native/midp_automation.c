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

#include <string.h>

static struct KeyCodeNameToMIDPKeyCode {
    const char* name;
    int midpKeyCode;
} keyCodeNameToMIDPKeyCode[] = {
    { "BACKSPACE", KEYMAP_KEY_BACKSPACE },
    { "UP",        KEYMAP_KEY_UP        },
    { "DOWN",      KEYMAP_KEY_DOWN      },
    { "LEFT",      KEYMAP_KEY_LEFT      },
    { "RIGHT",     KEYMAP_KEY_RIGHT     },
    { "SELECT",    KEYMAP_KEY_SELECT    },
    { "SOFT1",     KEYMAP_KEY_SOFT1     },
    { "SOFT2",     KEYMAP_KEY_SOFT2     },
    { "CLEAR",     KEYMAP_KEY_CLEAR     },
    { "SEND",      KEYMAP_KEY_SEND      },
    { "END",       KEYMAP_KEY_END       },
    { "POWER",     KEYMAP_KEY_POWER     },
    { "GAMEA",     KEYMAP_KEY_GAMEA     },
    { "GAMEB",     KEYMAP_KEY_GAMEB     },
    { "GAMEC",     KEYMAP_KEY_GAMEB     },
    { "GAMED",     KEYMAP_KEY_GAMEB     },
};

KNIEXPORT KNI_RETURNTYPE_INT
KNIDECL(com_sun_midp_automation_AutoKeyCode_getMIDPKeyCodeFromName) {
    int i;
    int sz = sizeof(keyCodeNameToMIDPKeyCode)/
        sizeof(struct KeyCodeNameToMIDPKeyCode);

    int midpKeyCode = KEYMAP_KEY_INVALID;
    const char* keyCodeName;

    KNI_StartHandles(1);
    GET_PARAMETER_AS_PCSL_STRING(1, keyCodeNamePCSL)

    keyCodeName = pcsl_string_get_utf8_data(&keyCodeNamePCSL);
 
    for (i = 0; i < sz; ++i) {
        const char* name = keyCodeNameToMIDPKeyCode[i].name;
        if (strcmp(keyCodeName, name) == 0) {
            midpKeyCode = keyCodeNameToMIDPKeyCode[i].midpKeyCode;
            break;
        }
    }

    if (midpKeyCode == KEYMAP_KEY_INVALID) {
         REPORT_ERROR1(LC_CORE, 
            "AutoKeyEventImpl_getMIDPKeyCodeFromName: unknown key code %s", 
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

