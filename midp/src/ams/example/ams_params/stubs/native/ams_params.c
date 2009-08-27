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

#include <string.h>
#include <midp_global_status.h>

/**
 * Retrieves an array of parameters to be handled by runMidlet.
 *
 * If this function returns non-zero number of parameters, then all parameters
 * passed via the command-line are ignored (this behavior is implemented in
 * the calling function).
 *
 * Note that the memory to hold the parameters is allocated in this
 * function and must be freed by the caller using ams_get_startup_params().
 *
 * @param pppParams       [out] if successful, will hold an array of
 *                              startup parameters
 * @param pNumberOfParams [out] if successful, will hold a number of
 *                              parameters read
 *
 * @return ALL_OK if no errors or an implementation-specific error code
 */
MIDPError ams_get_startup_params(char*** pppParams, int* pNumberOfParams) {
    if (pppParams == NULL || pNumberOfParams == NULL) {
        return BAD_PARAMS;
    }
    
    *pNumberOfParams = 0;
    *pppParams = NULL;

    return ALL_OK;
}

/**
 * Frees memory previously allocated by ams_get_startup_params().
 * 
 * @param ppParams arrays of parameters to be freed
 * @param numberOfParams number of elements in pParams array
 */
void ams_free_startup_params(char** ppParams, int numberOfParams) {
    (void)ppParams;
    (void)numberOfParams;
}
