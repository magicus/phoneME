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

/**
 * @file
 *
 * This header file contains RMS API of the MIDlet suite storage subsystem.
 */

#ifndef _SUITESTORE_RMS_H_
#define _SUITESTORE_RMS_H_

#include <suitestore_common.h>

/** @name Constants for the midpport_suite_rms_filename function
 * Constants for extension definition for rms resources
 * @see midpport_suite_rms_filename
 * @{
 */
/** specifies the extension .db to the function midp_suite_rms_filename */
#define MIDP_RMS_DB_EXT       0
/** specifies the extension .idx to the function midp_suite_rms_filename */
#define MIDP_RMS_IDX_EXT      1

/** @} */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Gets location of the resource with specified type and name
 * for the suite with the specified suiteId.
 *
 * Note that the implementation of this function MUST allocate the memory
 * for the in/out parameter filename using pcsl_mem_malloc().
 * The caller is responsible for freeing the memory associated
 * with filename parameter.
 *
 * @param suiteId The application suite ID
 * @param storageId storage ID where the RMS will be located
 * NOTE: currently this parameter is ignored due to limitation of our
 * implementation: RMS is always located at the same storage as the suite.
 * @param extension rms extension that can be MIDP_RMS_DB_EXT or
 * MIDP_RMS_IDX_EXT
 * @param pResourceName RMS name
 * @param pFileName The in/out parameter that contains returned filename
 *
 * @return error code that should be one of the following:
 * <pre>
 *     ALL_OK, OUT_OF_MEMORY, NOT_FOUND,
 *     SUITE_CORRUPTED_ERROR, BAD_PARAMS
 * </pre>
 */
MIDPError midp_suite_get_rms_filename(SuiteIdType suiteId,
                                      StorageIdType storageId,
                                      jint extension,
                                      const pcsl_string* pResourceName,
                                      pcsl_string* pFileName);

#ifdef __cplusplus
}
#endif

#endif /* _SUITESTORE_RMS_H_ */
