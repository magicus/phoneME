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

#include <pcsl_directory.h>
#include <pcsl_memory.h>
#include <donuts.h>


PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(file1)
  {'t','e','s','t','f','i','l','e','\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(file1);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(dir1)
  {'.','\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(dir1);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(dummy1)
  {'n','o','n','e','x','i','s','t','e','n','t','\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(dummy1);

PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_START(dir3)
  {'t','e','s','t','d','i','r','\0'}
PCSL_DEFINE_STATIC_ASCII_STRING_LITERAL_END(dir3);

static int init() {
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

static void tearDown() {
    pcsl_file_finalize();
}

/**
 * Tests for pcsl_file_is_directory(), pcsl_file_mkdir() and pcsl_file_rmdir().
 */
void testDirectories() {
    int res;

    res = pcsl_file_is_directory(&dir1);
    assertTrue("\".\" is not recognized as a directory", res == 1);
    res = pcsl_file_is_directory(&dummy1);
    assertTrue("Non-existent directory is erroneously found", res != 1);

    res = pcsl_file_mkdir(&dir3);
    assertTrue("mkdir failed", res == 0);
    res = pcsl_file_is_directory(&dir3);
    assertTrue("Directory wasn't created", res == 1);
    res = pcsl_file_mkdir(&dir3);
    assertTrue("mkdir succeeded for directory that already exists", res != 0);

    res = pcsl_file_rmdir(&dir3);
    assertTrue("rmdir failed", res == 0);
    res = pcsl_file_is_directory(&dir3);
    assertTrue("Directory wasn't deleted", res != 1);
    res = pcsl_file_rmdir(&dummy1);
    assertTrue("rmdir succeeded for non-exisent directory", res != 0);
}

/**
 * Tests for pcsl_file_getfreesize() and pcsl_file_gettotalsize().
 */
void testSizes() {
    jlong res;

    res = pcsl_file_getfreesize(&dir1);
    assertTrue("Querying free size failed", res >= 0);
    printf("Free bytes on file system: %lld\n", res);
    res = pcsl_file_gettotalsize(&dir1);
    assertTrue("Querying total size failed", res >= 0);
    printf("Total bytes on file system: %lld\n", res);

    /*
     * IMPL_NOTE: to do further testing (counting exact number of bytes).
     */
}

/**
 * Tests for pcsl_file_get_attribute() and pcsl_file_set_attribute().
 */
void testAttributes() {
    int res;
    int fileID;
    int attr;

    res = pcsl_file_open(&file1, PCSL_FILE_O_RDWR | PCSL_FILE_O_TRUNC | PCSL_FILE_O_CREAT, (void **)&fileID);
    pcsl_file_close((void *)fileID);
    assertTrue("File creation failed", res == 0);

    /* All files are considered readable on win32 */
    res = pcsl_file_set_attribute(&file1, PCSL_FILE_ATTR_READ, 0);
    assertTrue("Attributes setting failed", res == 0);
    res = pcsl_file_get_attribute(&file1, PCSL_FILE_ATTR_READ, &attr);
    assertTrue("Attributes query failed", res == 0);
    assertTrue("File isn't reported as readable", attr == 1);

    res = pcsl_file_set_attribute(&file1, PCSL_FILE_ATTR_READ, 1);
    assertTrue("Attributes setting failed", res == 0);
    res = pcsl_file_get_attribute(&file1, PCSL_FILE_ATTR_READ, &attr);
    assertTrue("Attributes query failed", res == 0);
    assertTrue("File isn't reported as readable", attr == 1);

    res = pcsl_file_set_attribute(&file1, PCSL_FILE_ATTR_WRITE, 0);
    assertTrue("Attributes setting failed", res == 0);
    res = pcsl_file_get_attribute(&file1, PCSL_FILE_ATTR_WRITE, &attr);
    assertTrue("Attributes query failed", res == 0);
    assertTrue("Read-only file is reported as writable", attr == 0);

    res = pcsl_file_set_attribute(&file1, PCSL_FILE_ATTR_WRITE, 1);
    assertTrue("Attributes setting failed", res == 0);
    res = pcsl_file_get_attribute(&file1, PCSL_FILE_ATTR_WRITE, &attr);
    assertTrue("Attributes query failed", res == 0);
    assertTrue("Writable file is reported as read-only", attr == 1);

    /* All files are considered executable on win32 */
    res = pcsl_file_set_attribute(&file1, PCSL_FILE_ATTR_EXECUTE, 0);
    assertTrue("Attributes setting failed", res == 0);
    res = pcsl_file_get_attribute(&file1, PCSL_FILE_ATTR_EXECUTE, &attr);
    assertTrue("Attributes query failed", res == 0);
    assertTrue("File isn't reported as executable", attr == 1);

    res = pcsl_file_set_attribute(&file1, PCSL_FILE_ATTR_EXECUTE, 1);
    assertTrue("Attributes setting failed", res == 0);
    res = pcsl_file_get_attribute(&file1, PCSL_FILE_ATTR_EXECUTE, &attr);
    assertTrue("Attributes query failed", res == 0);
    assertTrue("File isn't reported as executable", attr == 1);

    res = pcsl_file_set_attribute(&file1, PCSL_FILE_ATTR_HIDDEN, 1);
    assertTrue("Attributes setting failed", res == 0);
    res = pcsl_file_get_attribute(&file1, PCSL_FILE_ATTR_HIDDEN, &attr);
    assertTrue("Attributes query failed", res == 0);
    assertTrue("File is reported as executable", attr == 1);

    res = pcsl_file_set_attribute(&file1, PCSL_FILE_ATTR_HIDDEN, 0);
    assertTrue("Attributes setting failed", res == 0);
    res = pcsl_file_get_attribute(&file1, PCSL_FILE_ATTR_HIDDEN, &attr);
    assertTrue("Attributes query failed", res == 0);
    assertTrue("File is reported as executable", attr == 0);

    pcsl_file_unlink(&file1);
}

/**
 * Tests for pcsl_file_get_time().
 */
void testTimes() {
    int res;
    long time1, time2;
    int fileID;
    int i, j;

    res = pcsl_file_get_time(&dummy1, PCSL_FILE_TIME_LAST_MODIFIED, &time1);
    assertTrue("Time query succeeded for non-existent file", res == -1);

    res = pcsl_file_open(&file1, PCSL_FILE_O_RDWR | PCSL_FILE_O_TRUNC | PCSL_FILE_O_CREAT, (void **)&fileID);
    pcsl_file_close((void *)fileID);
    assertTrue("File creation failed", res == 0);

    res = pcsl_file_get_time(&file1, PCSL_FILE_TIME_LAST_MODIFIED, &time1);
    assertTrue("Time query failed", res == 0);
    res = pcsl_file_open(&file1, PCSL_FILE_O_RDWR | PCSL_FILE_O_APPEND, (void **)&fileID);
    assertTrue("File opening failed", res == 0);
    for (i = 0; i < 1000000; i++) {
        res = pcsl_file_write((void *)fileID, (unsigned char *)&time1, 4);
        assertTrue("Write failed", res == 4);
    }
    pcsl_file_close((void *)fileID);

    /* Writing 4 MB to a file should take several seconds */
    res = pcsl_file_get_time(&file1, PCSL_FILE_TIME_LAST_MODIFIED, &time2);
    assertTrue("Time query failed", res == 0);
    assertTrue("Modification time should be greater", time2 > time1);

    for (j = 0; j < 10; j++) {
        res = pcsl_file_open(&file1, PCSL_FILE_O_RDWR | PCSL_FILE_O_APPEND, (void **)&fileID);
        assertTrue("File opening failed", res == 0);
        for (i = 0; i < 1000000; i++) {
            res = pcsl_file_read((void *)fileID, (unsigned char *)&time1, 4);
            assertTrue("Read failed", res == 4);
        }
        pcsl_file_close((void *)fileID);
    }

    /* Reading anything from a file shouldn't lead to modification time change */
    res = pcsl_file_get_time(&file1, PCSL_FILE_TIME_LAST_MODIFIED, &time1);
    assertTrue("Time query failed", res == 0);
    assertTrue("Modification time shouldn't change", time1 == time2);

    pcsl_file_unlink(&file1);
}


/**
 * Unit test framework entry point for this set of unit tests.
 */
void testDir_win32_runTests() {

    int status;

    pcsl_mem_initialize(NULL, 0);

    pcsl_string_initialize();

    status = init();
    assertTrue("init failed", status == 0);

    /* Tests for additional API needed for JSR-75 */
    testDirectories();

    // Temporarily disabled for POSIX
    //testSizes();

    // Temporarily disabled for POSIX
    //testAttributes();

    // Temporarily disabled for POSIX
    //testTimes();

    tearDown();

    pcsl_string_finalize();

    pcsl_mem_finalize();

}
