/*
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

/**
 * @file
 * Holds the state for the MIDletSuiteLoader between VM starts.
 */

#include <stdlib.h>
#include <string.h>

#include <kni.h>

#include <midpMalloc.h>
#include <suitestore_kni_util.h>
#include <midpCommandState.h>

#ifdef __cplusplus
extern "C" {
#endif

/** The singleton command state. */
static MIDPCommandState MidpCommandState;
/** If true the state has been initialized. */
static jboolean state_initialized = KNI_FALSE;

/**
 * Get the command state.
 *
 * @return current command state
 */
MIDPCommandState* midpGetCommandState() {
    if (KNI_FALSE == state_initialized) {
        memset((unsigned char*)&MidpCommandState, 0,
               sizeof (MidpCommandState));
        MidpCommandState.midletClassName = PCSL_STRING_NULL;
        MidpCommandState.suiteID = PCSL_STRING_NULL;
        MidpCommandState.lastSuiteID = PCSL_STRING_NULL;
        MidpCommandState.lastMidletClassName = PCSL_STRING_NULL;
        MidpCommandState.arg0 = PCSL_STRING_NULL;
        MidpCommandState.arg1 = PCSL_STRING_NULL;
        MidpCommandState.arg2 = PCSL_STRING_NULL;
        state_initialized = KNI_TRUE;
    }

    return &MidpCommandState;
}

/**
 * Perform any need finalization and free the command state.
 */
void finalizeCommandState(void) {
    pcsl_string_free(&MidpCommandState.midletClassName);
    pcsl_string_free(&MidpCommandState.suiteID);
    pcsl_string_free(&MidpCommandState.lastSuiteID);
    pcsl_string_free(&MidpCommandState.lastMidletClassName);
    pcsl_string_free(&MidpCommandState.arg0);
    pcsl_string_free(&MidpCommandState.arg1);
    pcsl_string_free(&MidpCommandState.arg2);
    state_initialized = KNI_FALSE;
}


/**
 * Restore the state of the MIDlet suite loader.
 * <p>
 * Java declaration:
 * <pre>
 *   restoreCommandState(Lcom/sun/midp/CommandState;)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_CommandState_restoreCommandState() {
    KNI_StartHandles(3);
    KNI_DeclareHandle(commandState);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(string);

    KNI_GetParameterAsObject(1, commandState);
    
    KNI_GetObjectClass(commandState, clazz);

    KNI_RESTORE_INT_FIELD(commandState, clazz, "status",
			  MidpCommandState.status);
    KNI_RESTORE_BOOLEAN_FIELD(commandState, clazz, "logoDisplayed", 
			      (jboolean)MidpCommandState.logoDisplayed);
    KNI_RESTORE_PCSL_STRING_FIELD(commandState, clazz, "midletClassName",
                                  &MidpCommandState.midletClassName, string);
    KNI_RESTORE_PCSL_STRING_FIELD(commandState, clazz, "suiteID",
                                  &MidpCommandState.suiteID, string);
    KNI_RESTORE_PCSL_STRING_FIELD(commandState, clazz, "lastSuiteID",
                                  &MidpCommandState.lastSuiteID, string);
    KNI_RESTORE_PCSL_STRING_FIELD(commandState, clazz, "lastMidletClassName",
                                  &MidpCommandState.lastMidletClassName,
                                  string);
    KNI_RESTORE_PCSL_STRING_FIELD(commandState, clazz, "arg0",
                                  &MidpCommandState.arg0, string);
    KNI_RESTORE_PCSL_STRING_FIELD(commandState, clazz, "arg1",
                                  &MidpCommandState.arg1, string);
    KNI_RESTORE_PCSL_STRING_FIELD(commandState, clazz, "arg2",
                                  &MidpCommandState.arg2, string);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

/**
 * Save the state of the MIDlet suite loader.
 * <p>
 * Java declaration:
 * <pre>
 *    saveCommandState(Lcom/sun/midp/CommandState;)V
 * </pre>
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_main_CommandState_saveCommandState() {
    KNI_StartHandles(3);
    KNI_DeclareHandle(commandState);
    KNI_DeclareHandle(clazz);
    KNI_DeclareHandle(string);

    KNI_GetParameterAsObject(1, commandState);
    KNI_GetObjectClass(commandState, clazz);

    KNI_SAVE_INT_FIELD(commandState, clazz, 
		       "status", MidpCommandState.status);
    KNI_SAVE_BOOLEAN_FIELD(commandState, clazz, 
			  "logoDisplayed", MidpCommandState.logoDisplayed);

    /*
     * We need to put these in the do/while block since the SAVE_STRING
     * macros may throw an OutOfMemoryException. If this happens, we
     * must exit the native function immediately.
     */
    do {
        KNI_SAVE_PCSL_STRING_FIELD(commandState, clazz, "midletClassName",
                                   &MidpCommandState.midletClassName, string);
        KNI_SAVE_PCSL_STRING_FIELD(commandState, clazz, "suiteID",
                                   &MidpCommandState.suiteID, string);
        KNI_SAVE_PCSL_STRING_FIELD(commandState, clazz, "lastSuiteID",
                                   &MidpCommandState.lastSuiteID, string);
        KNI_SAVE_PCSL_STRING_FIELD(commandState, clazz, "lastMidletClassName",
                                   &MidpCommandState.lastMidletClassName,
                                   string);
        KNI_SAVE_PCSL_STRING_FIELD(commandState, clazz, "arg0",
                                   &MidpCommandState.arg0, string);
        KNI_SAVE_PCSL_STRING_FIELD(commandState, clazz, "arg1",
                                   &MidpCommandState.arg1, string);
        KNI_SAVE_PCSL_STRING_FIELD(commandState, clazz, "arg2",
                                   &MidpCommandState.arg2, string);
    } while (0);

    KNI_EndHandles();
    KNI_ReturnVoid();
}

#ifdef __cplusplus
}
#endif
