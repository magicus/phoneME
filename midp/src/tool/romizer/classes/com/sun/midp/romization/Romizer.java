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
 *
 */


/**
 * This tool takes a binary file as an input and produces a C file
 * with the romized data.
 */

package com.sun.midp.romization;

import java.io.*;
import java.util.*;

/**
 * Main tool class
 */
public class Romizer extends RomUtil {
    /**
     * A number of times that the hash table in C will be larger
     * then the number of romized resources.
     */
    private static final int hashSizeCoefficient = 4;

    /** Format of images: big- or little-endian. */
    private static boolean isBigEndian = false;

    /** Print usage info and exit */
    private static boolean printHelp = false;

    /** Input file names. */
    private static Vector inputFileNames;

    /**
     * Pairs of resource names generated from the names of the input files
     * and their hash values.
     */
    private static Hashtable resourceHashTable;

    /** Output file name. */
    private static String outputFileName;

    /** Size of the resource hash table in C file. */
    private static int hashTableSizeInC;

    /**
     * Main method
     *
     * @param args Command line arguments
     */
    public static void main(String[] args) {
        inputFileNames = new Vector(args.length);
        resourceHashTable = new Hashtable();

        try {
            parseArgs(args);
            if (printHelp) {
                showUsage();
                return;
            }

            new Romizer().doRomization();
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    /**
     * Parses command line arguments.
     *
     * @param args command line arguments
     */
    private static void parseArgs(String[] args) {
        for (int i = 0; i < args.length; ++i) {
            String arg = args[i];
            if (arg.equals("-help")) {
                printHelp = true;
                break;
            } else if (arg.equals("-in")) {
                do {
                    inputFileNames.addElement(args[++i]);
                } while (i < args.length &&
                         !((i+1 == args.length) || args[i+1].startsWith("-")));
            } else if (arg.equals("-outc")) {
                outputFileName = args[++i];
            } else if (arg.equals("-endian")) {
                String endian = args[++i];
                if ("little".equals(endian)) {
                    isBigEndian = false;
                } else if ("big".equals(endian)) {
                    isBigEndian = true;
                } else {
                    throw new IllegalArgumentException("invalid endian: \"" +
                        endian + "\"");
                }
            } else {
                throw new IllegalArgumentException("invalid option \"" +
                        args[i] + "\"");
            }
        }
    }

    /**
     * Prints usage information.
     */
    private static void showUsage() {
        /**
         * Following options are recognized:
         * -in:         Input file name.
         * -outc:       Output C file. If empty, output will be to stdout.
         * -endian:     Format of images. Little-endian by default.
         * -help:       Print usage information.
         */
        System.err.println("Usage: java -jar "
            + "com.sun.midp.romization.Romizer "
            + "-in <inputFile> "
            + "-outc <outputCFile> "
            + "[-endian <big|little>] "
            + "[-help]");
    }

    /**
     * Makes a resource name from the given file name.
     *
     * @param filePath name of file containing the resource that will be romized
     */
    private static String getResourceNameFromPath(String filePath) {
        int start = filePath.lastIndexOf(File.separatorChar);

		// To be compatible with non-Cygwin win32 builds
		// that do not translate / to \.
        if (start < 0) {
            start = filePath.lastIndexOf('/');
        }

        if (start < 0) {
            start = 0;
        } else {
            start++;
        }
        
        String resourceName = filePath.substring(start, filePath.length());
        resourceName = resourceName.replace('.', '_'); 

        System.out.println("... romizing resource: '" + resourceName + "'");

        return resourceName;
    }

    /**
     * Romizing routine.
     *
     * @throws IOException if the input file can't be read
     * @throws FileNotFoundException if the input file was not found   
     */
    private void doRomization() throws IOException, FileNotFoundException {
        // output generated C file with images
        OutputStream outForCFile = new FileOutputStream(outputFileName);
        writer =
            new PrintWriter(new OutputStreamWriter(outForCFile));

        writeCopyright();

        // output C representation of a binary array
        hashTableSizeInC = inputFileNames.size() * hashSizeCoefficient;
        
        for (int i = 0; i < inputFileNames.size(); i++) {
            String fileName = (String)inputFileNames.elementAt(i);
            ByteArrayOutputStream out = new ByteArrayOutputStream(8192);

            readFileToStream(fileName, out);

            BinaryOutputStream outputStream = new BinaryOutputStream(out,
                isBigEndian);

            String resourceName = getResourceNameFromPath(fileName);
            Integer hashValue = new Integer(
                (resourceName.hashCode() & 0x7fffffff) % hashTableSizeInC);

            // System.out.println(">>> hash(" + resourceName + ") = " +
            //                    hashValue);

            List l = (List)resourceHashTable.get(hashValue);

            if (l == null) {
                l = new LinkedList();
                l.add(resourceName);
                resourceHashTable.put(hashValue, l);
            } else {
                l.add(resourceName);
            }

            writeByteArrayAsCArray(out.toByteArray(), resourceName);

            outputStream.close();
        }

        writeGetResourceMethod();

        writer.close();
    }

    /**
     * Reads the contents of the given file and writes it
     * to the given output stream.
     *
     * @param fileName name of the file to read
     * @param out output stream where to write the file's contents 
     */
    private void readFileToStream(String fileName, OutputStream out)
            throws IOException {
        FileInputStream in = new FileInputStream(fileName);
        byte[] data = new byte[in.available()];
        in.read(data);
        out.write(data);
        in.close();
    }

    /**
     * Generates a C code defining a method providing an access
     * to the romized resources by their names:
     *
     * int ams_get_resource(const unsigned char* pName,
     *                      const unsigned char **ppBufPtr);
     */
    private void writeGetResourceMethod() {
        pl("");
        pl("#include <string.h>");
        pl("#include <kni.h>");
        pl("");

        pl("typedef struct _RESOURCES_LIST {");
        pl("    const char* pResourceName;");
        pl("    const unsigned char* pData;");
        pl("    int dataSize;");
        pl("    const struct _RESOURCES_LIST* pNext;");
        pl("} RESOURCES_LIST;");
        pl("");

        pl("/** Hash table of the romized resources. */");

        int i, resNum = resourceHashTable.size();
        Enumeration keys = resourceHashTable.keys();

        while (keys.hasMoreElements()) {
            List l = (List)resourceHashTable.get(keys.nextElement());
            if (l != null) {
                ListIterator li = l.listIterator();
                int j = l.size();

                String name = (String)li.next();
                pl("static const RESOURCES_LIST resource" +
                   resNum + "_1" + " = {");
                pl("    \"" + name + "\", " + name +
                   ", sizeof(" + name + "), NULL");
                pl("};");

                while (j > 1) {
                    name = (String)li.next();
                    pl("static const RESOURCES_LIST resource" +
                       resNum + "_" + j + " = {");
                    pl("    \"" + name + "\", " + name + ", sizeof(" +
                       name + "), &resource" + resNum + "_" + (j - 1));
                    pl("};");
                    j--;
                }
            }
            resNum--;
        }

        pl("static const RESOURCES_LIST* resourceHashTable[" +
           hashTableSizeInC + "] = {");

        for (i = 0; i < hashTableSizeInC; i++) {
            List l = (List)resourceHashTable.get(new Integer(i));
            if (l != null) {
                // find the number of key with value i from the end
                // this code runs on desktop so it's not optimized
                resNum = resourceHashTable.size();
                keys = resourceHashTable.keys();
                while (keys.hasMoreElements()) {
                    if (((Integer)keys.nextElement()).intValue() == i) {
                        break;
                    }
                    resNum--;
                }

                pl("    &resource" + resNum + "_" + l.size() + ", " +
                   "/* " + l.get(l.size() - 1) + " */");
            } else {
                pl("    NULL,");
            }
        }

        pl("};");

        pl("");
        pl("/**");
        pl(" *");
        pl(" * Calculates the string's hash value as");
        pl(" * s[0]*31^(n-1) + s[1]*31^(n-2) + ... + s[n-1]");
        pl(" *");
        pl(" * @param str the string");
        pl(" *");
        pl(" * @return a hash value");
        pl(" */");
        pl("static int getHash(const char* str) {");
        pl("    int i, len;");
        pl("    jint res = 0;");
        pl("");
        pl("    if (str == NULL) {");
        pl("        return 0;");
        pl("    }");
        pl("");
        pl("    len = strlen(str);");
        pl("");
        pl("    for (i = 0; i < len; i++) {");
        pl("      int chr = (int)str[i];");
        pl("      res = 31 * res + chr;");
        pl("    }");
        pl("");
        pl("    return (int)((res & 0x7fffffff) % " + hashTableSizeInC + ");");
        pl("}");

        // pl("#include <stdio.h>");

        pl("");
        pl("/**");
        pl(" * Loads a ROMized resource data from ROM, if present.");
        pl(" *");
        pl(" * @param name name of the resource to load");
        pl(" * @param **bufPtr where to save pointer to the romized resource");
        pl(" *");
        pl(" * @return -1 if failed, otherwise the " +
           "size of the resource");
        pl(" */");
        pl("int ams_get_resource(const unsigned char* pName, " +
           "const unsigned char **ppBufPtr) {");
        pl("    int hash = getHash((const char*)pName);");
        pl("    const RESOURCES_LIST* pResource = resourceHashTable[hash];");

        // pl("    printf(\"hash(%s) = %d\\n\", pName, hash);");
        
        pl("");
        pl("    while (pResource) {");
        pl("        if (!strcmp(pResource->pResourceName, (const char*)pName)) {");
        pl("            *ppBufPtr = pResource->pData;");
        pl("            return pResource->dataSize;");
        pl("        }");
        pl("        pResource = pResource->pNext;");
        pl("    }");
        pl("");
        pl("    return -1;");
        pl("}");
    }
}
