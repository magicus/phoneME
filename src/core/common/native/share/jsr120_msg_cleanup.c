/*
 *   
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

#include <stdio.h>

#include <kni.h>
#include <midpServices.h>

#include <sni.h>
#include <commonKNIMacros.h>
#include <ROMStructs.h>
#include <midpError.h>
#include <midp_properties_port.h>
#include <midp_logging.h>
#include <midpResourceLimit.h>
#include <pcsl_memory.h>
#include <jsr120_sms_listeners.h>
#include <jsr120_cbs_listeners.h>

#if ENABLE_JSR_205
#include <jsr205_mms_listeners.h>
#endif

#include <ROMStructs.h>

/**
 * Delete all messages registered against specified suite ID.
 *
 * @param msid The MIDlet suite ID.
 *
 */
KNIEXPORT KNI_RETURNTYPE_VOID
Java_com_sun_midp_wma_WMACleanupMonitor_deleteMessages0(void) { 

    /** The MIDP String version of the Midlet suite ID. */
    MidpString msMsid = NULL_MIDP_STRING;

    /* The midlet suite name for this connection. */
    unsigned char* msid = NULL;

    /* Create handles for all Java objects. */
    KNI_StartHandles(1);
    KNI_DeclareHandle(javaStringMsid);

    /* Pick up the Midlet Suite ID string. */
    KNI_GetParameterAsObject(1, javaStringMsid);

    do {
        /* Get the Midlet suite name. */
        if (!KNI_IsNullHandle(javaStringMsid)) {

	    msMsid.len = KNI_GetStringLength(javaStringMsid);
	    msMsid.data = (jchar*)pcsl_mem_malloc(msMsid.len * sizeof(jchar));
	    if (msMsid.data == NULL) {
                /* Couldn't allocate space for the Midlet suite name string. */
                KNI_ThrowNew(midpOutOfMemoryError, NULL);
                break;
	    } else {
                /* Convert the MIDP string contents to a character array. */
                KNI_GetStringRegion(javaStringMsid, 0, msMsid.len, msMsid.data);
                msid = (unsigned char*)midpJcharsToChars(msMsid);
                pcsl_mem_free(msMsid.data);
            }

            /*
             * Invoke a native function that will delete all messages
             * registered against msid.
             */
            jsr120_sms_delete_midlet_suite_msg(msid);
            jsr120_cbs_delete_midlet_suite_msg(msid);
#if ENABLE_JSR_205
            jsr205_mms_delete_midlet_suite_msg(msid);
#endif
        }

    } while (0);

    /* Memory clean-up (The string can be NULL). */
    pcsl_mem_free(msid);

    KNI_EndHandles();

    KNI_ReturnVoid();
}
