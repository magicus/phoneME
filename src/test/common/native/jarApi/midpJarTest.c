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

#include <midpMalloc.h>
#include <midpStorage.h>
#include <midpJar.h>

/** Filename for the suites JAR. */
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(JAR_FILENAME)
    {'t', 'e', 's', 't', '.', 'j', 'a', 'r', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(JAR_FILENAME);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(ENTRY_NAME)
    {'M', 'E', 'T', 'A', '-', 'I', 'N', 'F', '/', 'M',
     'A', 'N', 'I', 'F', 'E', 'S', 'T', '.', 'M', 'F', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(ENTRY_NAME);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(UNKNOWN_ENTRY_NAME)
    {'X', 'X', 'T', 'A', '-', 'I', 'N', 'F', '/', 'M',
     'A', 'N', 'I', 'F', 'E', 'S', 'T', '.', 'X', 'X', '\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(UNKNOWN_ENTRY_NAME);

int
main() {
    void* jarHandle;
    int error;
    char* entryData = NULL;
    long entryLen;

    initializeStorage(".");

    /* open for read binary. */
    jarHandle = midpOpenJar(&error, &JAR_FILENAME);
    if (error != 0) {
        printf("open error = %d\n", error);
        puts("Test failed");
        return -1;
    }

    error = midpJarEntryExists(jarHandle, &UNKNOWN_ENTRY_NAME);
    if (error != 0) {
        printf("Jar entry was not found error = %d\n", error);
        puts("Test failed");
        return -1;
    }

    error = midpJarEntryExists(jarHandle, &ENTRY_NAME);
    if (error <= 0) {
        printf("Jar entry was not found error = %d\n", error);
        puts("Test failed");
        return -1;
    }

    entryLen = midpGetJarEntry(jarHandle, &UNKNOWN_ENTRY_NAME, &entryData);
    if (entryLen != 0 || entryData != NULL) {
        printf("Jar entry error = %ld, %d\n", entryLen, entryData);
        puts("Test failed");
        return -1;
    }

    entryLen = midpGetJarEntry(jarHandle, &ENTRY_NAME, &entryData);
    if (entryLen < 0) {
        printf("Jar entry error = %ld\n", entryLen);
        puts("Test failed");
        return -1;
    }

    if (entryData == NULL) {
        puts("Jar entry was not found");
        puts("Test failed");
        return -1;
    }

    printf("%*.*s\n", (int)entryLen, (int)entryLen, entryData);

    midpFree(entryData);

    puts("Test passed");
    return 0;
}


