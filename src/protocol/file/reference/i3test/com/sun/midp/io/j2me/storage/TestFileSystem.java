/*
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

package com.sun.midp.io.j2me.storage;

import java.io.*;
import javax.microedition.io.*;
import com.sun.midp.i3test.*;
import com.sun.midp.configurator.Constants;


/**
 * Unit test for File.java.
 */
public class TestFileSystem extends TestCase {

    final static String EMPTY_STR = "";
    final static String TEST_FILE_A = File.getStorageRoot(Constants.INTERNAL_STORAGE_ID) + "A";
    final static String TEST_FILE_B = File.getStorageRoot(Constants.INTERNAL_STORAGE_ID) + "B";
    
    File fs;
    RandomAccessStream ras;

    void setUp() {
	fs = new File();
	ras = new RandomAccessStream();
    }
 
 
    /*
     * - Check storage root for null
     * - Create a file "A" under the root and write to it
     */
    public void testGetStorageRoot() {

	// Storage root should not be null
        assertTrue("getStorageRoot == null", File.getStorageRoot(Constants.INTERNAL_STORAGE_ID) != null);

	// Storage root should be writable
        try {
            createTestFile(TEST_FILE_A, 1024);
	    assertTrue(TEST_FILE_A + " exists", fs.exists(TEST_FILE_A));
	    // Leave "A" on FS, so a later test can use it
        } catch (IOException ioe) {
            fail("Could not create a file in storage root");
        }
    }

    /*
     * - Check config root for null
     */
    public void testGetConfigRoot() {
        assertTrue("getConfigRoot != null", File.getConfigRoot(Constants.INTERNAL_STORAGE_ID) != null);
    }
  
    /*
     * - a-z, 0-9 unchanged, convert back unchanged
     * - A-Z convert to #A-#Z, convert back correctly
     * - 0-1, 9+1, a-1, z+1, A-1, Z+1, 0xFFFF to not be unchanged,
     *   convert back correctly
     * - empty string unchanged, convert back unchanged
     * - 0x100 convert to "%0100", convert back correctly
     */
    public void testUnicodeToAsciiFilenameAndReverse() {
        String unicode;
        String ascii;
        String result;
        char[] buf = new char[1];

        for (char c = 'a'; c <= 'z'; c++) {
            buf[0] = c;
            unicode = new String(buf);
            ascii = File.unicodeToAsciiFilename(unicode);
            assertEquals(unicode, ascii);
            result = File.asciiFilenameToUnicode(ascii);
            assertEquals(unicode, result);
        }

        for (char c = '0'; c <= '9'; c++) {
            buf[0] = c;
            unicode = new String(buf);
            ascii = File.unicodeToAsciiFilename(unicode);
            assertEquals(unicode, ascii);
            result = File.asciiFilenameToUnicode(ascii);
            assertEquals(unicode, result);
        }

        for (char c = 'A'; c <= 'Z'; c++) {
            buf[0] = c;
            unicode = new String(buf);
            ascii = File.unicodeToAsciiFilename(unicode);
            assertEquals('#', ascii.charAt(0));
            assertEquals(c, ascii.charAt(1));
            result = File.asciiFilenameToUnicode(ascii);
            assertEquals(unicode, result);
        }

        unicode = EMPTY_STR;
        ascii = File.unicodeToAsciiFilename(unicode);
        assertEquals(unicode, ascii);

        buf[0] = 'a' - 1;
        unicode = new String(buf);
        ascii = File.unicodeToAsciiFilename(unicode);
        assertNotEquals(unicode, ascii);

        buf[0] = 'z' + 1;
        unicode = new String(buf);
        ascii = File.unicodeToAsciiFilename(unicode);
        assertNotEquals(unicode, ascii);

        buf[0] = '0' - 1;
        unicode = new String(buf);
        ascii = File.unicodeToAsciiFilename(unicode);
        assertNotEquals(unicode, ascii);

        buf[0] = '9' + 1;
        unicode = new String(buf);
        ascii = File.unicodeToAsciiFilename(unicode);
        assertNotEquals(unicode, ascii);

        buf[0] = 'A' - 1;
        unicode = new String(buf);
        ascii = File.unicodeToAsciiFilename(unicode);
        assertNotEquals(unicode, ascii);

        buf[0] = 'Z' + 1;
        unicode = new String(buf);
        ascii = File.unicodeToAsciiFilename(unicode);
        assertNotEquals(unicode, ascii);

        buf[0] = 0xFFFF;
        unicode = new String(buf);
        ascii = File.unicodeToAsciiFilename(unicode);
        assertNotEquals(unicode, ascii);

        buf[0] = 0x100;
        result = "%0100";
        unicode = new String(buf);
        ascii = File.unicodeToAsciiFilename(unicode);
        assertEquals(result, ascii);
        result = File.asciiFilenameToUnicode(ascii);
        assertEquals(unicode, result);
    }
     
    /*
     * - Delete "B" just in case it then
     *   rename file "A" to "B" and
     *   check exists("B") and check non-exists("A")
     * - rename file "B" to EMPTY name
     *   and expects a IOException
     */
    public void testRename() {

	// Make sure B does not exist
        try {
            fs.delete(TEST_FILE_B);
        } catch (IOException e) {
            // This will happen most of the time since "B" should not exist
        }

	// Rename A to B
	try {
	    fs.rename(TEST_FILE_A, TEST_FILE_B);
	    assertTrue("A does not exist", !fs.exists(TEST_FILE_A));
	    assertTrue("B does exist",      fs.exists(TEST_FILE_B));
        
	    try {
		fs.rename(TEST_FILE_B, EMPTY_STR);
		fail("rename to empty string");
	    } catch (IOException ioe) {
		assertTrue(true);
	    }
	} catch (Throwable t) {
	    fail("Exception when renaming: ");
	    t.printStackTrace();

	    // Clean-up by deleting A
	    try {
		fs.delete(TEST_FILE_A);
	    } catch (IOException ioe) {
		// ignore
	    }
	}
    }

    /**
     * Asserts that two objects are equal according to the equals() method.
     *
     * @param message the message to be emitted if the assertion fails
     * @param expected an object containing the expected value
     * @param actual an object containing the actual value
     */
    public void assertNotEquals(Object expected, Object actual) {
	if (expected != actual) {
            if (!expected.equals(actual)) {
                return;
            }

            fail("expected NOT equals actual");
	    return;
        }
    }

    /**
     * exists(EMPTY) return false.
     */
    void testExists() {
	assertTrue(!fs.exists(EMPTY_STR));
    }

    /**
     * - delete("B") and check for non-exists("B")
     * - delete("DontExist") expects IOException
     * - delete(EMPTY) expects IllegalArgumentException
     */
    void testDelete() {
	try {
	    // Create B first
	    createTestFile(TEST_FILE_B, 1024);

	    // Delete
	    fs.delete(TEST_FILE_B);

	    // Test for existence
	    assertTrue(fs.exists(TEST_FILE_B) == false);

	} catch (Throwable t) {
	    fail("Unexpected Exception while deleting:");
	    t.printStackTrace();
	}

	// Delete a non-existing file
	boolean ioeThrowed = false;
	try {
	    fs.delete("DontExist");
	} catch (IOException ioe) {
	    ioeThrowed = true;
	}

	assertTrue(ioeThrowed);
    }

    /**
     * - Call and store the size as initialSpace, expect > 0
     * - Create a file "A" and write 1KB data, and call size again
     *   as lessSpace
     *   expect: 0 < diff - 1KB < 512B
     * - Delete file "A" and call size again as moreSpace
         expect moreSpace - lessSpace > 1KB
         expect abs(moreSpace - lessSpace) < 512B
     * - Repeat from step 2 for 50 times
     */
    void testAvailable() {

	int SIZE = 2048; // bytes
	int OVERHEAD = 512; // bytes

	long initialSpace = fs.getBytesAvailableForFiles(Constants.INTERNAL_STORAGE_ID);
	assertTrue("initialSpace="+initialSpace, initialSpace > 0);

	try {
	    // Loop many times to ensure no space leak
	    for (int i = 0; i < 50; i++) {
		// Create a file with certain size
		// expect overhead be less than allowed number 
		createTestFile(TEST_FILE_A, SIZE);
		long lessSpace = fs.getBytesAvailableForFiles(Constants.INTERNAL_STORAGE_ID);
		
		long delta = initialSpace - lessSpace;
		
		assertTrue("space used="+delta, delta >= SIZE);

		assertTrue("create overhead="+(delta - SIZE),
			   delta < SIZE + OVERHEAD);
	     
		// expect delete a file will give back some space
		// expect less than overhead bytes will be kept
		fs.delete(TEST_FILE_A);
		long moreSpace = fs.getBytesAvailableForFiles(Constants.INTERNAL_STORAGE_ID);

		delta = moreSpace - lessSpace;

		assertTrue("freeup space="+delta, delta >= SIZE);

		if (moreSpace < initialSpace) {
		    assertTrue("delete overhead="+(initialSpace - moreSpace),
			       initialSpace - moreSpace < OVERHEAD);
		} else {
		    assertTrue("delete overhead="+(moreSpace - initialSpace),
				moreSpace - initialSpace < OVERHEAD);
		}
	    }
	} catch (IOException ioe) {
	    ioe.printStackTrace();
	}
    }

    void tearDown() {
    }

    void createTestFile(String name, int size) throws IOException {
	ras.connect(name, Connector.WRITE);
	ras.writeBytes(new byte[size], 0, size);
        ras.commitWrite();
	ras.disconnect();
    }

    public void runTests() {
	setUp();

        declare("testGetStorageRoot");
        testGetStorageRoot();

        declare("testGetConfigRoot");
        testGetConfigRoot();

        declare("testUnicodeToAsciiFilenameAndReverse");
        testUnicodeToAsciiFilenameAndReverse();

        declare("testRename");
        testRename();

        declare("testExists");
	testExists();

	declare("testDelete");
	testDelete();

	declare("testGetBytesAvailableForFiles");
	testAvailable();

	tearDown();
    }
}
