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

#include <stdarg.h>
#include <stdio.h>

#include <jvmconfig.h>
#include <kni.h>
#include <jvm.h>
#include <jvmspi.h>
#include <sni.h>

#include <midpMalloc.h>
#include <midpServices.h>
#include <midp_logging.h>
#include <midp_constants_data.h>
#include <midp_properties_port.h>
#include <midpMidletSuiteUtils.h>
#include <midpUtilKni.h>

/**
 * @file
 *
 * Basic system services, such as event monitoring, VM status,
 * png and jar handling, etc.
 */

/*=========================================================================
 * VM helper functions
 *=======================================================================*/

/**
 * Immediately terminates the VM.
 * <p>
 * <b>NOTE:</b> This may not necessarily terminate the process.
 *
 * @param status The return code of the VM.
 */
void
midp_exitVM(int status) {
    JVM_Stop(status);
}

/**
 * Gets the current system time in milliseconds.
 *
 * @return The current system time in milliseconds
 */
jlong
midp_getCurrentTime(void) {
    return JVM_JavaMilliSeconds();
}

/*=========================================================================
 * JAR helper functions
 *=======================================================================*/
#if 0
/*
 * IMPL_NOTE: this code will be removed as soon as we find out
 * that the full build does not break
 */
/* *
 * Reads an entry from a JAR file.
 *
 * @param jarName The name of the JAR file to read
 * @param entryName The name of the file inside the JAR file to read
 * @param entry An <tt>Object</tt> representing the entry
 *
 * @return <tt>true</tt> if the entry was read, otherwise <tt>false</tt>
 */
/*
* IMPL_NOTE: this function is the same as midpGetJarEntry() in midpJar.c,
* but it uses only CLDC stuff
*/
jboolean
midp_readJarEntry(const pcsl_string* jarName, const pcsl_string* entryName, jobject* entry) {
    JvmPathChar* platformJarName;
    int i;
    jboolean noerror = KNI_TRUE;

    GET_PCSL_STRING_DATA_AND_LENGTH(jarName)
    /* Entry names in JARs are UTF-8. */
    const jbyte * const platformEntryName = pcsl_string_get_utf8_data(entryName);

    do {
        if (platformEntryName == NULL) {
            noerror = KNI_FALSE;
            break;
        }
        /*
         * JvmPathChars can be either 16 bits or 8 bits so we can't
         * assume either.
         */

        /*
         * Conversion to JvmPathChars is only temporary, we should change the VM
         * should take a jchar array and a length and pass that to
         * PCSL which will then perform
         * a platform specific conversion (DBCS on Win32 or UTF8 on Linux)
         */
        platformJarName = midpMalloc((jarName_len + 1) * sizeof (JvmPathChar));
        if (platformJarName == NULL) {
            noerror = KNI_FALSE;
            break;
        }

        for (i = 0; i < jarName_len; i++) {
            platformJarName[i] = (JvmPathChar)jarName_data[i];
        }

        platformJarName[i] = 0;

        Jvm_read_jar_entry(platformJarName, (char*)platformEntryName, (jobject)*entry);
        midpFree(platformJarName);
    } while(0);
    pcsl_string_release_utf8_data(platformEntryName, entryName);
    RELEASE_PCSL_STRING_DATA_AND_LENGTH
    if (noerror) {
        return (KNI_IsNullHandle((jobject)*entry) == KNI_TRUE)
               ? KNI_FALSE : KNI_TRUE;
    } else {
        return KNI_FALSE;
    }
}
#endif
/**
 * Get the current isolate id from VM in case of MVM mode. 
 * For SVM, simply return 0 as an isolate ID.
 *
 * @return isolated : Isolate ID
 * 
 */
int getCurrentIsolateId() {
    int amsIsolateId = midpGetAmsIsolateId();
    /*
     * midpGetAmsIsolateId() returns 0 when VM is not running at all. It 
     * also returns 0 when VM is running but it is in SVM mode. 
     *
     * When VM is running in MVM mode, midpGetAmsIsolateId() returns 
     * isolateID of an AMS isolate
     */
#if ENABLE_MULTIPLE_ISOLATES
    return ((amsIsolateId == 0)? 0:JVM_CurrentIsolateId());
#else
    return amsIsolateId;
#endif /* ENABLE_MULTIPLE_ISOLATES */
}

#if ENABLE_MULTIPLE_ISOLATES
/** Sets new value of MAX_ISOLATES property */
static void setMaxIsolates(int value) {
    char valueStr[5];
    sprintf(valueStr, "%d", value);
    setInternalProperty("MAX_ISOLATES", valueStr);
}
#endif /* ENABLE_MULTIPLE_ISOLATES */

/**
 * Get maximal allowed number of isolates in the case of MVM mode.
 * For SVM, simply return 1.
 */
int getMaxIsolates() {

#if ENABLE_MULTIPLE_ISOLATES

    static int midpMaxIsolates = 0;
    if (midpMaxIsolates == 0) {
        int jvmMaxIsolates = JVM_MaxIsolates();
        midpMaxIsolates  = getInternalPropertyInt("MAX_ISOLATES");
        if (0 == midpMaxIsolates) {
            REPORT_INFO(LC_AMS, "MAX_ISOLATES property not set");
            midpMaxIsolates = MAX_ISOLATES;
        }
        if (midpMaxIsolates > jvmMaxIsolates){
            REPORT_WARN1(LC_AMS,
                "MAX_ISOLATES exceeds VM limit %d, decreased", jvmMaxIsolates);
            midpMaxIsolates = jvmMaxIsolates;
        }
        setMaxIsolates(midpMaxIsolates);
    }
    return midpMaxIsolates;

#else

    return 1;

#endif /* ENABLE_MULTIPLE_ISOLATES */
}

/*=========================================================================
 * Debugger helper functions
 *=======================================================================*/

#if ENABLE_JAVA_DEBUGGER
/**
 * Determines if the debugger is active.
 *
 * @return <tt>true</tt> if the debugger is active, otherwise <tt>false</tt>
 */
jboolean
midp_isDebuggerActive(void) {
    return JVM_IsDebuggerActive();
}
#endif /* ENABLE_JAVA_DEBUGGER */


/*=========================================================================
 * Memory manager helper function
 *=======================================================================*/

#if NO_STRDUP
/**
 * Duplicates a NULL-terminated string.
 * <p>
 * <b>NOTE:</b> It is important that the caller of this function
 * properly dispose of the returned string.
 *
 * @param str The string to duplicate
 *
 * @return The duplicate string upon success. If the system is out of
 *         memory, <tt>NULL</tt> is returned
 */
char* strdup(const char* str) {
    int   len = strlen(str);
    char* p   = (char*) midpMalloc(len + 1);
    if (p != NULL){
        memcpy(p, str, len);
        p[len] = '\0';
    }
    return p;                            /* null if p could not be allocated */
}
#endif /* NO_STRDUP */
