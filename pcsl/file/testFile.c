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
#include <donuts.h>

PCSL_DEFINE_ASCII_STRING_LITERAL_START(file1)
  {'t','e','s','t','1','.','j','a','r','\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(file1);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(file2)
  {'t','e','s','t','2','.','j','a','r','\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(file2);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(file3)
  {'t','e','s','t','3','.','j','a','r','\0'} 
PCSL_DEFINE_ASCII_STRING_LITERAL_END(file3);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(file4)
  {'t','e','s','t','4','.','j','a','r','\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(file4);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(file5)
  {'a','f','i','l','e','.','j','a','r','\0'} 
PCSL_DEFINE_ASCII_STRING_LITERAL_END(file5);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(file6)
  {'t','f','i','l','e','.','j','a','r','\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(file6);

PCSL_DEFINE_ASCII_STRING_LITERAL_START(dir1)
  {'.','\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(dir1);

// search string for file iterator
PCSL_DEFINE_ASCII_STRING_LITERAL_START(search)
  {'t','e','s','t','\0'}
PCSL_DEFINE_ASCII_STRING_LITERAL_END(search);

static int TOTALSIZE = 4 * 1024 * 1024;

#ifndef MAX_PATH
#define MAX_PATH 256
#endif

void testSizeOf() {
    int fileID, size, stat;

    stat = pcsl_file_open(&file5, 
			  PCSL_FILE_O_RDWR | PCSL_FILE_O_TRUNC | 
			  PCSL_FILE_O_CREAT, 
			  (void **)(&fileID));
    size = pcsl_file_sizeofopenfile((void *)fileID);
    assertTrue("File size not zero", size == 0);
    pcsl_file_close((void *)fileID);
    pcsl_file_unlink(&file5);

}

void printFileName(const pcsl_string * name) {
    const jbyte * string = pcsl_string_get_utf8_data(name);

    printf("Next file from iterator = %s\n", string);

    pcsl_string_release_utf8_data(string, name);
}

void testExistence() {
    int stat;

    /* Should return false for a directory */ 
    stat = pcsl_file_exist(&dir1);
    assertTrue("Should return false for a directory", stat == 0);
}

void testFilelistIteration() {
    int fd, stat;
    pcsl_string result = PCSL_STRING_NULL;
    int fileID;
    int found = 0;
    int notFound = 0;

    /* open and close a few files, before trying the iterator */
    stat = pcsl_file_open(&file1, 
			  PCSL_FILE_O_RDWR | 
			  PCSL_FILE_O_TRUNC | 
			  PCSL_FILE_O_CREAT, 
                          (void **)&fileID);
    pcsl_file_close((void *)fileID);
    stat = pcsl_file_open(&file2, 
			  PCSL_FILE_O_RDWR | 
			  PCSL_FILE_O_TRUNC | 
			  PCSL_FILE_O_CREAT,                           
			  (void **)&fileID);
    pcsl_file_close((void *)fileID);
    stat = pcsl_file_open(&file3, 
			  PCSL_FILE_O_RDWR | 
			  PCSL_FILE_O_TRUNC | 
			  PCSL_FILE_O_CREAT,                           
			  (void **)&fileID);
    pcsl_file_close((void *)fileID);
    stat = pcsl_file_open(&file4, 
			  PCSL_FILE_O_RDWR | 
			  PCSL_FILE_O_TRUNC | 
			  PCSL_FILE_O_CREAT,                           
			  (void **)&fileID);
    pcsl_file_close((void *)fileID);
    stat = pcsl_file_open(&file5, 
			  PCSL_FILE_O_RDWR | 
			  PCSL_FILE_O_TRUNC | 
			  PCSL_FILE_O_CREAT,                           
			  (void **)&fileID);
    pcsl_file_close((void *)fileID);
    stat = pcsl_file_open(&file6, 
			  PCSL_FILE_O_RDWR | 
			  PCSL_FILE_O_TRUNC | 
			  PCSL_FILE_O_CREAT,                           
			  (void **)&fileID);
    pcsl_file_close((void *)fileID);

    fd = (int)pcsl_file_openfilelist(&search);

    do {
        // look for files that begin with "test"
        stat = pcsl_file_getnextentry((void *)fd, &search, &result);

        if (stat >= 0) {
            // a match has been found
            printFileName(&result);
            // There are 4 files that begin with "test"
            // Check if all 4 have been found 

            if (pcsl_string_equals(&result, &file1) == PCSL_TRUE ||
                pcsl_string_equals(&result, &file2) == PCSL_TRUE ||
                pcsl_string_equals(&result, &file3) == PCSL_TRUE ||
                pcsl_string_equals(&result, &file4) == PCSL_TRUE) {
                found++;
                pcsl_string_free(&result);
                continue;
             }

             // There are 2 files that don't start with
             // "test". Make sure these aren't found

            if (pcsl_string_equals(&result, &file5) ||
                pcsl_string_equals(&result, &file6) ) {
                notFound++;
                printf("wrong file found\n");
                printFileName(&result);
             }

             pcsl_string_free(&result);

        }
    } while (stat >= 0);

    pcsl_file_unlink(&file1);
    pcsl_file_unlink(&file2);
    pcsl_file_unlink(&file3);
    pcsl_file_unlink(&file4);
    pcsl_file_unlink(&file5);
    pcsl_file_unlink(&file6);
    pcsl_file_closefilelist((void *)fd);

    assertTrue("File not found by iterator", found == 4); 
    assertTrue("Wrong file found by iterator", notFound == 0); 
}

/*
 * - Call and store the size as initialSpace, expect > 0
 * - Create a file "A" and write 1KB data, and call size again
 *	 as lessSpace
 *	 expect: 0 < diff - 1KB < 512B
 * - Delete file "A" and call size again as moreSpace
  expect moreSpace - lessSpace > 1KB
  expect abs(moreSpace - lessSpace) < 512B
 * - Repeat from step 2 for 50 times
 */
void testAvailableSpace(int totalSpace) {

    int SIZE = 2048; // bytes
    int OVERHEAD = 512; // bytes
    int fileID;
    int i, nWrite, stat;
    unsigned char buff[2048];
    int initialSpace, lessSpace, moreSpace, delta;
    pcsl_string dir1 = PCSL_STRING_NULL;
    jchar dir1chars[2]; 

    dir1chars[0] = '.';
    dir1chars[1] = pcsl_file_getfileseparator();

    if (pcsl_string_convert_from_utf16(dir1chars, 2, &dir1) 
	!= PCSL_STRING_OK) {
      assertTrue("Failed to create string", 0);
      return;
    }

    initialSpace = totalSpace - pcsl_file_getusedspace(&dir1);
    assertTrue("initialSpace <= 0", initialSpace > 0);

    // Loop many times to ensure no space leak
    for (i = 0; i < 50; i++) {
	// Create a file with certain size
	// expect overhead be less than allowed number
        stat = pcsl_file_open(&file6, 
			      PCSL_FILE_O_RDWR | 
			      PCSL_FILE_O_TRUNC | 
                              PCSL_FILE_O_CREAT, 
			      (void **)(&fileID));
        assertTrue("file open failed",stat >= 0);
        nWrite = pcsl_file_write((void *)fileID, buff, SIZE);

        pcsl_file_commitwrite((void *)fileID); /* flush write to filesystem */

        assertTrue("write failed",nWrite == SIZE);

        assertTrue("incorrect size of open file",pcsl_file_sizeofopenfile((void *)fileID) >= SIZE);

	lessSpace = totalSpace - pcsl_file_getusedspace(&dir1);
	
	delta = initialSpace - lessSpace;

	assertTrue("space used less than file size", delta >= SIZE);

	assertTrue("create overhead over 512 bytes",
        	delta < SIZE + OVERHEAD);

	// expect delete a file will give back some space
	// expect less than overhead bytes will be kept
        pcsl_file_close((void *)fileID);
	pcsl_file_unlink(&file6);
	moreSpace = totalSpace - pcsl_file_getusedspace(&dir1);

	delta = moreSpace - lessSpace;

	assertTrue("freeup space not enough", delta >= SIZE);

	if (moreSpace < initialSpace) {
	    assertTrue("overhead not deleted",
			initialSpace - moreSpace < OVERHEAD);
	} else {
	    assertTrue("delete overhead",
			moreSpace - initialSpace < OVERHEAD);
	}
    }

    pcsl_string_free(&dir1);
}


int init() {
    int res;

    res = pcsl_file_init();
    if (res < 0 ) {
    	printf("Can't init the heap\n");
    	return -1;
    } else if (res == 0) {
    	printf("Already initialized\n");
    }
    return 0;

}

void tearDown() {

    pcsl_file_finalize();
}

/**
 * Test the file system
 *
 */
int testFileReadWrite() {

    int fileID, stat;
    unsigned char* dataBuffer1;
    unsigned char* dataBuffer2;
    unsigned char* dataBuffer3;
    unsigned char* dataBuffer4;
    unsigned char dataBuffer5[1025];
    int nRead;
    int nWrite;
    int i, j;
    long seekStatus;

    dataBuffer1 = pcsl_mem_malloc(1024*256);
    if(dataBuffer1 == NULL) {
        printf("Failed to allocate data buffer1\n");
        return -1;
    }
    dataBuffer2 = pcsl_mem_malloc(1024*312);
    if(dataBuffer2 == NULL) {
        pcsl_mem_free(dataBuffer1);
        printf("Failed to allocate data buffer2\n");
        return -1;
    }
    dataBuffer3 = pcsl_mem_malloc(1024*256);
    if(dataBuffer3 == NULL) {
        pcsl_mem_free(dataBuffer1);
        pcsl_mem_free(dataBuffer2);
        printf("Failed to allocate data buffer3\n");
        return -1;
    }
    dataBuffer4 = pcsl_mem_malloc(1024*256);
    if(dataBuffer4 == NULL) {
        pcsl_mem_free(dataBuffer1);
        pcsl_mem_free(dataBuffer2);
        pcsl_mem_free(dataBuffer3);
        printf("Failed to allocate data buffer4\n");
        return -1;
    }

    memset(dataBuffer1, 0x31, 1024 * 256);
    memset(dataBuffer2, 0x32, 1024 * 312);
    memset(dataBuffer3, 0x33, 1024 * 256);
    memset(dataBuffer4, 0x34, 1024 * 256);


    stat = (int)pcsl_file_open(&file1, 
			       PCSL_FILE_O_RDWR | 
			       PCSL_FILE_O_TRUNC | 
                               PCSL_FILE_O_CREAT, 
			       (void **)(&fileID));

    for(i = 0; i < 256; i++) {
        nWrite = pcsl_file_write((void *)fileID,dataBuffer1, 1024);
        assertTrue("Write failure", nWrite == 1024);
	if(nWrite != 1024) {
	    return -1;
	}

	dataBuffer1 = (unsigned char*)((int)dataBuffer1 + 1024);

    }

    pcsl_file_close((void *)fileID);
    stat = (int)pcsl_file_open(&file2, 
			       PCSL_FILE_O_RDWR | 
			       PCSL_FILE_O_TRUNC | 
                               PCSL_FILE_O_CREAT, 
			       (void **)(&fileID));

    for(i = 0; i < 312; i++) {
        nWrite = pcsl_file_write((void *)fileID,dataBuffer2, 1024);
        assertTrue("Write failure", nWrite == 1024);
	if(nWrite != 1024) {
	    return -1;
        }
	dataBuffer2 = (unsigned char*)((int)dataBuffer2 + 1024);

    }

    pcsl_file_close((void *)fileID);

    stat = pcsl_file_open(&file3, 
			  PCSL_FILE_O_RDWR | 
			  PCSL_FILE_O_TRUNC | 
			  PCSL_FILE_O_CREAT,                           
			  (void **)(&fileID));

    for(i = 0; i < 256; i++) {
        nWrite = pcsl_file_write((void *)fileID,dataBuffer3, 1024);
        assertTrue("Write failure", nWrite == 1024);
	if(nWrite != 1024) {
	    return -1;
        }
	dataBuffer3 = (unsigned char*)((int)dataBuffer3 + 1024);

    }

    pcsl_file_close((void *)fileID);
	
    pcsl_file_unlink(&file2);

    stat = pcsl_file_open(&file4, 
			  PCSL_FILE_O_RDWR | 
			  PCSL_FILE_O_TRUNC | 
			  PCSL_FILE_O_CREAT,                           
			  (void **)(&fileID));

    for(i = 0; i < 256; i++) {
        nWrite = pcsl_file_write((void *)fileID,dataBuffer4, 1024);
        assertTrue("Write failure", nWrite == 1024);
	if(nWrite != 1024) {
	    return -1;
        }
	dataBuffer4 = (unsigned char*)((int)dataBuffer4 + 1024);

    }

    pcsl_file_commitwrite((void *)fileID);

    seekStatus = pcsl_file_seek((void *)fileID, 0, 0);

    for(i = 0; i < 256; i++) {
        nRead = pcsl_file_read((void *)fileID,dataBuffer5, 1024);
        assertTrue("Read failure", nRead == 1024);
	if(nRead != 1024) {
	    return -1;
        }

	for(j = 0; j < 1024; j++) {
            assertTrue("Read byte has wrong value", dataBuffer5[j] == 0x34);
	    if(dataBuffer5[j] != 0x34) {
	        return -1;
            }

        }

    }
	
    pcsl_file_close((void *)fileID);
    pcsl_file_unlink(&file1);
    pcsl_file_unlink(&file3);
    pcsl_file_unlink(&file4);

    dataBuffer1 = (void*)((int)dataBuffer1 - 1024*256);
    dataBuffer2 = (void*)((int)dataBuffer2 - 1024*312);
    dataBuffer3 = (void*)((int)dataBuffer3 - 1024*256);
    dataBuffer4 = (void*)((int)dataBuffer4 - 1024*256);

    pcsl_mem_free(dataBuffer1);
    pcsl_mem_free(dataBuffer2);
    pcsl_mem_free(dataBuffer3);
    pcsl_mem_free(dataBuffer4);

    return 1;

}

/**
 * Unit test framework entry point for this set of unit tests.
 *
 */
void testFile_runTests() {

    int status;

    pcsl_mem_initialize(NULL, 100*1024*1024);

    pcsl_string_initialize();

    status = init();
    
    assertTrue("init failed", status == 0);

    testFilelistIteration();

    status = testFileReadWrite();
    assertTrue("File test failed", (status != -1));

    testSizeOf();

    testAvailableSpace(TOTALSIZE);

    testExistence();

    tearDown();

    pcsl_string_finalize();

    pcsl_mem_finalize();

}
