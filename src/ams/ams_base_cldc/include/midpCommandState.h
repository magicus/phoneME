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

#ifndef _MIDP_COMMANDSTATE_H_
#define _MIDP_COMMANDSTATE_H_

/**
 * @defgroup ams_base Application Management System External Interface
 * @ingroup ams
 */
/**
 * @file
 * @ingroup ams_base
 *
 * @brief Command state header file.
 */

#include <kni.h>
#include <midpString.h>
#include <suitestore_common.h>
#include <midp_runtime_info.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * MUST MATCH the values in CommandState.java
 */
#define MAIN_EXIT (2001)

/** The state for the MIDlet suite loader. */
typedef struct _MIDPCommandState {
    /** status of the last command. */
    int status;
    /** the ID given to a suite to load. */
    SuiteIdType suiteId;
    /** Class name of MIDlet. */
    pcsl_string midletClassName;
    /** Has the application manager MIDlet displayed the Java logo yet? */
    jboolean logoDisplayed;
    /** true, if VM has been restarted to let next MIDlet run **/
    jboolean vmRestarted;
    /** The ID of suite to load when there is no other queued. */
    SuiteIdType lastSuiteId;
    /** The MIDlet class name for the suite to load. */
    pcsl_string lastMidletClassName;
    /** The argument for a last MIDlet, will be app property arg-0. */
    pcsl_string lastArg0;
    /** The argument for a last MIDlet, will be app property arg-1. */
    pcsl_string lastArg1;
    /** The argument for a MIDlet in the suite, will be app property arg-0. */
    pcsl_string arg0;
    /** The argument for a MIDlet in the suite, will be app property arg-1. */
    pcsl_string arg1;
    /** The argument for a MIDlet in the suite, will be app property arg-2. */
    pcsl_string arg2;
    /**
     * Name of the profile to set before starting the VM.
     * IMPL_NOTE: currently this field is duplicated in runtimeInfo member
     *            because the MidletRuntimeInfo structure is also used in
     *            NAMS external API so its profileName member is declared
     *            as jchar.
     */
    pcsl_string profileName;
    /** Memory quotas to set before starting the VM. */
    MidletRuntimeInfo runtimeInfo;
    /**
     * A debug option for the new MIDlet: one of
     * MIDP_NO_DEBUG, MIDP_DEBUG_SUSPEND, MIDP_DEBUG_NO_SUSPEND
     */
    jint debugMode;
} MIDPCommandState;

/**
 * Get the command state.
 *
 * @return current command state
 */
MIDPCommandState* midpGetCommandState();

/**
 * Perform any need finalization and free the command state.
 */
void finalizeCommandState();

#ifdef __cplusplus
}
#endif

#endif /* _MIDP_COMMANDSTATE_H_ */
