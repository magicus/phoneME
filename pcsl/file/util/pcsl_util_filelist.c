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

#include <stdio.h>
#include <string.h>

#include <pcsl_file.h>
#include <pcsl_memory.h>
#include <pcsl_util_filelist.h>

/**
 * @file
 *
 * Useful posix file list utilities.
 */

extern void* pcsl_file_opendir(const pcsl_string * dirName);
extern int pcsl_file_closedir(void *handle);

void *pcsl_util_openfileiterator(const pcsl_string * filelist) {
    int rootLength;
    PCSLStorageDirInfo* pPCSLStorageDirInfo;
    void* dir;
    const jsize filelistLen = pcsl_string_length(filelist);

    pPCSLStorageDirInfo = (PCSLStorageDirInfo*)pcsl_mem_malloc(sizeof (PCSLStorageDirInfo));
    if (pPCSLStorageDirInfo == NULL) {
	//Error in allocation
	return NULL;
    }

    memset(pPCSLStorageDirInfo, 0, sizeof(PCSLStorageDirInfo));

    /*
     * find the root dir of the string
     */
    rootLength = pcsl_string_last_index_of(filelist, pcsl_file_getfileseparator());
    
    if (-1 == rootLength) {
        rootLength = 0;
    } else {
        /* Include the file separator. */
        rootLength++;
    }

    pPCSLStorageDirInfo->savedRootLength = rootLength;

    pPCSLStorageDirInfo->savedMatchLength = filelistLen - rootLength;

    /* Do not include the file separator for dirOpen. */
    if (rootLength > 0) {
        pcsl_string dirName = PCSL_STRING_NULL;
	if (pcsl_string_substring(filelist, 0, rootLength, &dirName) 
	    != PCSL_STRING_OK) {
	  pcsl_mem_free(pPCSLStorageDirInfo);
	  return NULL;
	}

	dir = pcsl_file_opendir(&dirName);

	pcsl_string_free(&dirName);
	
	if (dir == NULL)
	{
	    pcsl_mem_free(pPCSLStorageDirInfo);
	    return NULL;
	}
	
        pPCSLStorageDirInfo->savedDirectory = dir;
    } else {
        // open '.' dir
        jchar dotDir[2];
        pcsl_string dirName = PCSL_STRING_NULL;
     
        dotDir[0] = '.';
        dotDir[1] = pcsl_file_getfileseparator();

	if (pcsl_string_convert_from_utf16(dotDir, 2, &dirName) 
	    != PCSL_STRING_OK) {
	    pcsl_mem_free(pPCSLStorageDirInfo);
	    return NULL;
	}

        pPCSLStorageDirInfo->savedDirectory = pcsl_file_opendir(&dirName);

	pcsl_string_free(&dirName);
    }
     

    return pPCSLStorageDirInfo;

}

int pcsl_util_closefileiterator(void *handle) {

    int retVal;

    PCSLStorageDirInfo* pPCSLStorageDirInfo = (PCSLStorageDirInfo *)handle;

    if (handle == NULL) {
	return -1;
    }

    retVal = pcsl_file_closedir(pPCSLStorageDirInfo->savedDirectory);

    pcsl_mem_free(pPCSLStorageDirInfo);

    return retVal;
}
