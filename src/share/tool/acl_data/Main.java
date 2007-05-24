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

package acl_data;

import java.util.Vector;
import java.io.*;
import java.security.*;
import java.security.cert.CertificateException;
/**
 * Main class implementation
 */
public class Main {

    /** ACL root DF. */
    short[] ACLDF         = {0x3f00, 0x5016}; // MUST be long name
    /** Output directory */
    private String outputDataDir = "./output/";
    private String outputFilesDir = "./output/files/";

    /** File system. */
    FileSystem fs;
    /** Stream for results. */
    PrintStream src;

    /**
     * Main function
     * @param args program argument - input file name.
     */
    public static void main(String[] args) {
        new Main().run(args);
    }

    /**
     * Check the program argument and start processing.
     * @param args program argument - input file name.
     */
    void run(String[] args) {
        if (args.length < 1) {
            System.err.println("No file");
        }
        try {
            if (args.length >= 2) {
                outputDataDir = args[1];
            }

            if (args.length >= 3) {
                outputFilesDir = args[2];
            }

            generateJavaData(generateFiles(args[0]));
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * Generates the data.
     * @param inl list of generated files
     * @throws KeyStoreException if this exception occurs
     * @throws CertificateException if this exception occurs
     * @throws IOException if this exception occurs
     * @throws NoSuchAlgorithmException if this exception occurs
     */
    public void generateJavaData(Vector inl)
            throws KeyStoreException, CertificateException,
            IOException, NoSuchAlgorithmException {
        String fname;
        short[] outname;
        fs = new FileSystem(ACLDF);

        src = new PrintStream(new FileOutputStream(outputDataDir + "Data.java"));

        src.print("/*\n" +
                  " *   \n" +
                  " *\n" +
                  " * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.\n" +
                  " * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER\n" +  
                  " *\n */\n\n");
        src.print("package com.sun.satsa.aclapplet;\n\n" +
                  "/**\n" +
                  " * This class contains file system.\n */\n" +
                  "public class Data {\n\n");

        for (int i = 0; i < inl.size(); i++) {
            fname = (String)inl.elementAt(i);
            System.out.println(" ... generating " + fname);
            if (fname == null) {
                break;
            }
            outname = Utils.stringToShorts(fname.substring(
                                                (outputFilesDir).length()));
            FileInputStream in = new FileInputStream(fname);
            byte[] b = new byte[in.available()];
            in.read(b);
            fs.addFile(outname, FileSystem.READ, b);
        }

        byte[] data = fs.getEncoded();

        src.println();

        src.print("    /** Files. */");
        Utils.writeDataArray(src, "Files", data);
        src.println("}");

        src.close();
    }

    /**
     * Returns list of generated files.
     * @param args input file.
     * @return vector of generated files.
     */
    Vector generateFiles(String args) {
        ACFile file = ACFile.load(args, outputFilesDir);
        try {
            file.createODF();
            file.createDODF();
            return file.files;
        } catch (IOException e) {
        }
        return null;
    }

}
