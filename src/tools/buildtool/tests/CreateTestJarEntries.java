/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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

package tests;

import java.io.*;
import java.util.*;
import java.util.zip.*;
import util.*;

/**
 * This class is used to generate JAR file entries for unit test cases.
 *
 * We add JAR entries of very long names into tests.jar
 */
public class CreateTestJarEntries {
    static void usage() {
        System.out.println("Usage: java -jar buildtool.jar " +
                           "testjarentries tests.jar");
    }

    public static void main(String args[]) throws Throwable {
        if (args.length != 1) {
            usage();
            System.exit(1);
        }
        File tests_jar  = new File(args[0]);
        InputStream in = readEntireFileInBuffer(tests_jar);
        FileOutputStream out = new FileOutputStream(tests_jar);
        ZipOutputStream zout = new ZipOutputStream(out);

        // Add Test JAR entries in front
        addTestEntry(zout, 1000, "Hello1.class");
        addTestEntry(zout, 1000, "Hello1.txt");

        // Copy all entries from tests.jar
        copyEntries(zout, in);

        // Add Test JAR entries at the back
        addTestEntry(zout, 1000, "Hello2.class");
        addTestEntry(zout, 1000, "Hello2.txt");

        // Add test entry for the test case 
        // vm.share.runtime.ClassPathAccess.read_completely1 
        addReadCompletely1Entry(zout);

        // Add test entries for the test case 
        // vm.share.runtime.ClassPathAccess.incremental_decompress
        addIncrementalDecomressEntries(zout);

        // Add entry for ResourceInput unit test case.
        // See src/tests/javaapi/com/sun/cldc/io/resource_input.java
        addResourceInputEntry(zout);

        zout.close();
    }

    static InputStream readEntireFileInBuffer(File file) throws Throwable {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        FileInputStream in = new FileInputStream(file);
        int n;
        byte buff[] = new byte[1024];

        while ((n = in.read(buff)) > 0) {
            baos.write(buff, 0, n);
        }
        in.close();

        byte data[] = baos.toByteArray();
        baos.close();
        return new ByteArrayInputStream(data);
    }

    /**
     * Copy the content of of the given zip file source 
     * to the ZIP file output stream
     */
    static void copyEntries(ZipOutputStream zout, InputStream in) 
        throws Throwable
    {
        ZipInputStream zin = new ZipInputStream(in);
        for (;;) {
            ZipEntry entry = zin.getNextEntry();
            if (entry == null) {
                break;
            }

            // Read the entry from input
            int size = (int)entry.getSize();
            byte data[] = new byte[size];
            zin.read(data, 0, size);
            zin.closeEntry();

            // Write entry to output
            zout.putNextEntry(entry);
            zout.write(data, 0, data.length);
            zout.closeEntry();
        }
        zin.close();
    }

    static String getLongPath(int numLevels, String sep) {
        String path = "long" + sep;
        for (int i=0; i<numLevels; i++) {
            path += "long" + sep;
        }
        return path;
    }

    /**
     * Create a JAR entry with a very long name. The JAR entry contains
     * with 100 bytes of zeros.
     */
    static void addTestEntry(ZipOutputStream zout, int numLevels, String name)
        throws Throwable
    {
        System.out.println("adding in very long path: \"" + name + "\"");
        String longName = getLongPath(numLevels, "/") + name;
        ZipEntry entry = new ZipEntry(longName);

        byte data[] = new byte[100];

        zout.putNextEntry(entry);
        zout.write(data, 0, data.length);
        zout.closeEntry();
    }

    static void addResourceInputEntry(ZipOutputStream zout)
        throws Throwable
    {
        ZipEntry entry = new ZipEntry("javaapi/com/sun/cldc/io/sampledata.txt");
        byte data[] = new byte[100];
        zout.putNextEntry(entry);
        zout.write(data, 0, data.length);
        zout.closeEntry();
    }

    /**
     * Create a JAR entry with non-compressible bytes (random data)
     * intermixed with compressible bytes (simple sequence of numbers)
     * as a stress test case for the JAR decoder, especially
     * the BTYPE_NO_COMPRESSION case.
     */
    static void addReadCompletely1Entry(ZipOutputStream zout)
        throws Throwable
    {
        String name = "vm/share/runtime/ClassPathAccess/read_completely1.dat";
        System.out.println("adding \"" + name + "\"");
        ZipEntry entry = new ZipEntry(name);
        ByteArrayOutputStream baos = new ByteArrayOutputStream();

        for (int i=0; i<10; i++) {
            // Simple sequence of bytes, compressible
            for (int j=0; j<256; j++) {
                baos.write(j);
            }

            /*
             * Random sequence, non-compressible. Will be stored as 
             * BTYPE_NO_COMPRESSION. See Inflate.cpp and search for 
             * BTYPE_NO_COMPRESSION
             */
            int count = 10240;
            for (int k=0; k<count; k++) {
                baos.write((local_rand() >> 24) & 0xff);
            }
        }

        byte data[] = baos.toByteArray();
        zout.putNextEntry(entry);
        zout.write(data, 0, data.length);
        zout.closeEntry();
    }

    /**
     * Create large JAR entries (>1Mb) which cannot be decompressed at once.
     * This tests Incremental Decompressing feature.
     */
    static void addIncrementalDecomressEntries(ZipOutputStream zout)
        throws Throwable
    {
        int count = 2048 * 1024;    // 2 Mb

        System.out.println("adding poorly-compressed large file");
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        for (int i = 0; i < count; i++) {
            baos.write((local_rand() >> 24) & 0xff);
        }
        writeToZip(zout, baos, "incremental_decompress1.dat");

        System.out.println("adding well-compressed large file");
        baos.reset();
        int written = 0;
        int groupLength = 1;
        while (written < count) { // written may exceed count - that's ok
            int b = (groupLength % 10) + '0';
            for (int i = 0; i < groupLength; i++) {
                baos.write(b);
            }
            written += groupLength;
            groupLength++;
        }
        writeToZip(zout, baos, "incremental_decompress2.dat");
    }

    static void writeToZip(ZipOutputStream zout,
                           ByteArrayOutputStream baos,
                           String name) throws Throwable
    {
        ZipEntry entry = new ZipEntry("javaapi/com/sun/cldc/io/" + name);
        zout.putNextEntry(entry);
        baos.writeTo(zout);
        zout.closeEntry();
    }

    static int state = 0x23451921;
    static int multiplier = 0xDEECE66D;
    static int addend = 0xB;

    static int local_rand() {
        state = state * multiplier + addend;
        return state;
    }

}
