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
#include <malloc.h>

#include <jar.h>

static long
sizeOfFile(void* state) {
    long current = ftell((FILE*)state);
    long size;

    fseek((FILE*)state, 0, SEEK_END);
    size = ftell((FILE*)state);
    fseek((FILE*)state, current, SEEK_SET);

    return size;
}

static int
readChars(void* state, unsigned char* buffer, long n) {
    return fread(buffer, sizeof (char), (size_t)n, (FILE*)state);
}

static int
seekChars(void* state, long offset, int whence) {
    return fseek((FILE*)state, offset, whence);
}

static int
readChar(void* state) {
    return fgetc((FILE*)state);
}

static void*
allocFunction(void* state, int n) {
    return malloc(n);
}

static void
freeFunction(void* state, void* handle) {
    if (handle == NULL) {
        return;
    }

    free(handle);
}

static void*
addrFromHandleFunction(void* state, void* handle) {
    /* alloc function just returns the address. */
    return handle;
}

int
main() {
    FileObj fileObj;
    HeapManObj heapManObj;
    char   fileName[] = "test.jar";
    JarInfo jarInfo;
    char   entryName[] = "META-INF/MANIFEST.MF";
    JarEntryInfo entryInfo;
    char*  entryData = NULL;
    int status;
    unsigned char* compBuffer;

    /* open for read binary. */
    fileObj.state = fopen(fileName, "rb");
    if (fileObj.state == NULL) {
        perror("can't open file");
        puts("Test failed");
        return -1;
    }

    fileObj.size = sizeOfFile;
    fileObj.read = readChars;
    fileObj.seek = seekChars;
    fileObj.readChar = readChar;

    jarInfo = getJarInfo(&fileObj);
    if (jarInfo.status != 0) {
        printf("Jar error = %d\n", jarInfo.status);
        puts("Test failed");
        return -1;
    }

    heapManObj.state = NULL;
    heapManObj.alloc = allocFunction;
    heapManObj.free = freeFunction;
    heapManObj.addrFromHandle = addrFromHandleFunction;

    compBuffer = malloc(strlen(entryName));
    if (compBuffer == NULL) {
        printf("Can't malloc %ld bytes for name comp buffer.\n",
               strlen(entryName));
        puts("Test failed");
        return -1;
    }

    entryInfo = findJarEntryInfo(&fileObj, &jarInfo, entryName,
                                 strlen(entryName), compBuffer);
    free(compBuffer);
    if (entryInfo.status != 0) {
        printf("Jar entry error = %d\n", entryInfo.status);
        puts("Test failed");
        return -2;
    }

    entryData = malloc((size_t)entryInfo.decompLen);
    if (entryData == NULL) {
        printf("Can't malloc %ld bytes for entry.\n", entryInfo.decompLen);
        puts("Test failed");
        return -3;
    }

    status = inflateJarEntry(&fileObj, &heapManObj, &entryInfo, entryData, 0);
    if (status != 0) {
        printf("Inflate error = %d\n", status);
        puts("Test failed");
        return -5;
    }

    printf("%*.*s", (int)entryInfo.decompLen, (int)entryInfo.decompLen,
        entryData);

    puts("Test passed");
    return status;
}


