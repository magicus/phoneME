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
#include <string.h>
#include <midpStorage.h>
#include <pcsl_memory.h>

#define MAX_AMS_PARAMS 100

/**
 * Name of file containing the parameters to be passed to runMidlet.
 */
PCSL_DEFINE_ASCII_STRING_LITERAL_START(PARAMS_FILE_NAME)
    {'a', 'm', 's', '_', 'p', 'a', 'r', 'a', 'm', 's', '.', 't', 'x', 't', '\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(PARAMS_FILE_NAME);

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
    int handle, status = ALL_OK;
    long fileSize, len;
    char* pszTemp;
    char* pBuffer = NULL;
    char* pszError = NULL;
    char** ppParamsRead = NULL;
    int i, lastPos, linesNum = 0;

    if (pppParams == NULL || pNumberOfParams == NULL) {
        return BAD_PARAMS;
    }

    *pppParams = NULL;
    *pNumberOfParams = 0;

    /* open the file */
    handle = storage_open(&pszError, &PARAMS_FILE_NAME, OPEN_READ);
    if (pszError != NULL) {
        storageFreeError(pszError);
        if (!storage_file_exists(&PARAMS_FILE_NAME)) {
            return NOT_FOUND;
        }
        return IO_ERROR;
    }

    do {
        /* get the size of file */
        fileSize = storageSizeOf(&pszError, handle);
        if (pszError != NULL) {
            storageFreeError(pszError);
            status = IO_ERROR;
            break;
        }

        if (fileSize > 0) {
            /* allocate a buffer to store the file contents */
            pBuffer = (char*)pcsl_mem_malloc(fileSize);
            if (pBuffer == NULL) {
                status = OUT_OF_MEMORY;
                break;
            }

            /* read the whole file */
            len = storageRead(&pszError, handle, pBuffer, fileSize);
            if (pszError != NULL || len != fileSize) {
	        storageFreeError(pszError);
                status = IO_ERROR;
                break;
            }

            /* parse the file */
            ppParamsRead = pcsl_mem_malloc(sizeof(char*) * MAX_AMS_PARAMS);

            linesNum = 0;
            lastPos = 0;
            i = 0;
	    
            while (i < fileSize) {
                if (pBuffer[i] == 0x0a) {
		    if (i == lastPos) {
		        lastPos++;
  		        i++;
		        continue;
		    }
                    ppParamsRead[linesNum] = pcsl_mem_malloc(i - lastPos + 1);
                    if (ppParamsRead[linesNum] == NULL) {
                        int j;
                        for (j = 0; j < linesNum; j++) {
                            pcsl_mem_free(ppParamsRead[j]);
                        }
                        linesNum = 0;
                        status = OUT_OF_MEMORY;
                        break;
                    }

                    memcpy(ppParamsRead[linesNum], &pBuffer[lastPos], i - lastPos);
                    ppParamsRead[linesNum][i - lastPos] = 0;

                    linesNum++;
		    
		    if (linesNum > MAX_AMS_PARAMS) {
		        status = ALL_OK;
		        break;
		    }
		    
		    if (i + 1 < fileSize && pBuffer[i + 1] == 0x0d) {
		        i++;
		    }

                    lastPos = i + 1;
                }
		
		i++;
            }

            if (i < fileSize) {
                break;
            }
        }

        status = ALL_OK;
    } while (0);

    /* close the file */
    storageClose(&pszTemp, handle);
    storageFreeError(pszTemp);

    if (status == ALL_OK) {
        *pppParams = ppParamsRead;
        *pNumberOfParams = linesNum;
    }

    if (pBuffer != NULL) {
        pcsl_mem_free(pBuffer);
    }

    return status;
}

/**
 * Frees memory previously allocated by ams_get_startup_params().
 * 
 * @param ppParams arrays of parameters to be freed
 * @param numberOfParams number of elements in pParams array
 */
void ams_free_startup_params(char** ppParams, int numberOfParams) {
    if (ppParams != NULL) {
        int i;
        for (i = 0; i < numberOfParams; i++) {
            pcsl_mem_free(ppParams[i]);
        }
        pcsl_mem_free(ppParams);
    }
}
