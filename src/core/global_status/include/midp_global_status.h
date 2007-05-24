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
#ifndef _MIDPERROR_H_
#define _MIDPERROR_H_


/**
 * @defgroup core MIDP Services and Utilities
 * @ingroup subsystems
 */

/**
 * @defgroup core_global Global State External Interface
 * @ingroup core
 */

/**
 * @file
 * @ingroup core_global
 *
 * @brief Definition of error conditions and exceptions that platform
 *  dependent layer can use.
 *
 */

/**
 * Error code for MIDP inter subsytem  and porting interface functions.
 */
typedef enum {
    /* Common Errors */
    MIDP_ERROR_NONE = 0, /**< no error */
    MIDP_ERROR_OUT_MEM, /**< out of memory (sometimes other non-recoverable
                             errors with missing details are reported
                             as out-of-memory) */
    MIDP_ERROR_ILLEGAL_ARGUMENT,
    MIDP_ERROR_UNSUPPORTED,
    MIDP_ERROR_OUT_OF_RESOURCE,
    MIDP_ERROR_PERMISSION_DENIED,
    /* error reported by another library or subsystem, details omitted */
    /* Why:
     * 1. A library cannot have error codes for all error events in all
     *    libraries that it invokes, maybe, alternatively invokes.
     * 2. It would be conceptually wrong to abuse "out of memory" error
     *    code for "unknown fatal error"
     * 3. If we introduce a new error code, we must change all client code
     *    that analyses error codes. This is not desirable right now.
     * Therefore, MIDP_ERROR_FOREIGN is mapped onto MIDP_ERROR_OUT_MEM. */
    MIDP_ERROR_FOREIGN = MIDP_ERROR_OUT_MEM,

    /* AMS Errors */
    MIDP_ERROR_AMS_SUITE_NOT_FOUND = 1000,
    MIDP_ERROR_AMS_SUITE_CORRUPTED,
    /**
     * MIDlet class cannot be found in the current MIDlet suite
     * or if this class is not included in any of the MIDlet- records
     * in the descriptor file or the JAR file manifest
     */
    MIDP_ERROR_AMS_MIDLET_NOT_FOUND,

    /* PUSH Errors */
    MIDP_ERROR_PUSH_CONNECTION_IN_USE = 2000,

    /* GRAPHICS and IMAGE Errors */
    MIDP_ERROR_IMAGE_CORRUPTED = 3000,

} MIDP_ERROR;


/**
 * Error codes.
 */
typedef enum {
    GENERAL_ERROR = -256,
    SUITE_CORRUPTED_ERROR = -34,
    SUITE_LOCKED = -33,
    END_OF_JAD = -32,
    END_OF_MF = -31,
    OUT_OF_MEM_LEN = -30,
    IO_ERROR_LEN = -29,
    JAD_AND_MANIFEST_DOESNT_MATCH = -28,
    SUITE_NAME_PROP_NOT_MATCH = -27,
    SUITE_VENDOR_PROP_NOT_MATCH = -26,
    SUITE_VERSION_PROP_NOT_MATCH = -25,
    NO_MF_FILE = -24,
    NO_JAR_FILE = -23,
    NO_JAD_FILE = -22,
    NO_SUITE_NAME_PROP = -21,
    NO_SUITE_VENDOR_PROP = -20,
    NO_SUITE_VERSION_PROP = -19,
    NO_JAR_URL_PROP = -18,
    NO_JAR_SIZE_PROP = -17,
    NO_MIDLET_ONE_PROP = -16,
    NO_MICROEDITION_PROFILE_PROP = -15,
    NO_MICROEDITION_CONFIGURATION_PROP = -14,
    BAD_SUITE_VERSION_PROP = -13,
    BAD_PARAMS = -12,
    BAD_JAD_KEY = -11,
    BAD_JAD_VALUE = -10,
    BAD_MF_KEY = -9,
    BAD_MF_VALUE = -8,
    RESOURCE_NOT_FOUND = -7,
    NOT_FOUND = -6,
    NUMBER_ERROR = -5,
    OUT_OF_STORAGE = -4,
    IO_ERROR = -3,
    OUT_OF_MEMORY = -2,
    NULL_LEN = -1, /*
                    * NULL_LEN must match the value for a null length
                    * string returned from KNI_GetStringLength() which is -1.
                    * since that where the length of a MidpString comes from
                    * when built from Java string use as a parameter to a
                    * native method.
                    */
    ALL_OK = 0,
} MIDPError;

/* @} */

#endif /* _MIDPERROR_H_ */

