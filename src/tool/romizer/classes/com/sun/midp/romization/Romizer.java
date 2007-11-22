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
public class Romizer {
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

    /** Character output file writer */
    PrintWriter writer = null;

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
            } else {
                throw new IllegalArgumentException("invalid option \""
                        + args[i] + "\"");
            }
        }
    }

    /**
     * Prints usage information
     */
    private static void showUsage() {
        /**
         * Following options are recognized:
         * -in:         input file name.
         * -outc:       Output C file. If empty, output will be to stdout.
         * -help:       Print usage information.
         */
        System.err.println("Usage: java -jar "
            + "com.sun.midp.romization.Romizer "
            + "-in <inputFile> "
            + "-outc <outputCFile> "
            + "[-help]");
    }

    /**
     *
     * @param filePath
     */
    private static String getResourceNameFromPath(String filePath) {
        int start = filePath.lastIndexOf(File.separatorChar);
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
     */
    private void doRomization() throws IOException, FileNotFoundException {
        // output generated C file with images
        OutputStream outForCFile = new FileOutputStream(outputFileName);
        writer =
            new PrintWriter(new OutputStreamWriter(outForCFile));

        writeCopyright();

        // output C representation of a binary array
        hashTableSizeInC = inputFileNames.size() * 2; //10;
        
        for (int i = 0; i < inputFileNames.size(); i++) {
            String fileName = (String)inputFileNames.elementAt(i);
            ByteArrayOutputStream out = new ByteArrayOutputStream(8192);

            readFileToStream(fileName, out);

            // IMPL_NOTE: CHANGE IT !!!
            BinaryOutputStream outputStream = new BinaryOutputStream(out,
                false);

            String resourceName = getResourceNameFromPath(fileName);
            Integer hashValue = new Integer(
                (resourceName.hashCode() & 0x7fffffff) % hashTableSizeInC);
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
     *
     */
    private void readFileToStream(String fileName, OutputStream out)
            throws IOException, FileNotFoundException {
        FileInputStream in = new FileInputStream(fileName);
        byte[] data = new byte[in.available()];
        in.read(data);
        out.write(data);
        in.close();
    }

    /************* IMPL_NOTE: to be moved into a shared package *************/

    /**
     *
     */
    void writeByteArrayAsCArray(byte[] data, String arrayName) {
        pl("/** Romized " + arrayName + " */");
        pl("static const unsigned char " + arrayName + "[] = {");
        if (data != null && data.length > 0) {
            new RomizedByteArray(data).printDataArray(writer, "        ", 11);
        } else {
            pl("    0");
        }
        pl("};");
    }

    /**
     *
     */
    void writeGetResourceMethod() {
        pl("");
        pl("#include <string.h>");
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

        resNum = resourceHashTable.size();
        keys = resourceHashTable.keys();

        for (i = 0; i < hashTableSizeInC; i++) {
            List l = (List)resourceHashTable.get(new Integer(i));
            if (l != null) {
                l = (List)resourceHashTable.get(keys.nextElement());
                pl("    &resource" + resNum + "_" + l.size() + ",");
                resNum--;
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
        pl("    int i, len, res = 0;");
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
        pl("    return res;");
        pl("};");

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
        pl("int ams_get_resource(const unsigned char* name, " +
           "const unsigned char **bufPtr) {");
        pl("    int hash = getHash(name);");
        pl("    const RESOURCES_LIST* pResource = resourceHashTable[hash];");
        pl("");
        pl("    while (pResource) {");
        pl("        if (!strcmp(pResource->pResourceName, name)) {");
        pl("            *bufPtr = pResource->pData;");
        pl("            return pResource->dataSize;");
        pl("        }");
        pl("        pResource = pResource->pNext;");
        pl("    }");
        pl("");
        pl("    return -1;");
        pl("}");
    }

    /**
     * Short-hand for printint a line into the output file
     *
     * @param s line to print
     */
    void pl(String s) {
        writer.println(s);
    }

    /**
     *  Writes copyright banner.
     */
    private void writeCopyright() {
        pl("/**");
        pl(" * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.");
        pl(" * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER");
        pl(" * ");
        pl(" * This program is free software; you can redistribute it and/or");
        pl(" * modify it under the terms of the GNU General Public License version");
        pl(" * 2 only, as published by the Free Software Foundation.");
        pl(" * ");
        pl(" * This program is distributed in the hope that it will be useful, but");
        pl(" * WITHOUT ANY WARRANTY; without even the implied warranty of");
        pl(" * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU");
        pl(" * General Public License version 2 for more details (a copy is");
        pl(" * included at /legal/license.txt).");
        pl(" * ");
        pl(" * You should have received a copy of the GNU General Public License");
        pl(" * version 2 along with this work; if not, write to the Free Software");
        pl(" * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA");
        pl(" * 02110-1301 USA");
        pl(" * ");
        pl(" * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa");
        pl(" * Clara, CA 95054 or visit www.sun.com if you need additional");
        pl(" * information or have any questions.");
        pl(" * ");
        pl(" * NOTE: DO NOT EDIT. THIS FILE IS GENERATED. If you want to ");
        pl(" * edit it, you need to modify the corresponding XML files.");
        pl(" */");
    }
}


/************* IMPL_NOTE: to be moved into a shared package *************/

/**
 * Represents romized byte array
 */
class RomizedByteArray {
    /** romized binary data */
    byte[] data;

    /**
     * Constructor
     *
     * @param dataBytes romized image data
     */
    RomizedByteArray(byte dataBytes[]) {
        data = dataBytes;
    }

    /**
     * Prints romized image data as C array
     *
     * @param writer where to print
     * @param indent indent string for each row
     * @param maxColumns max number of columns
     */
    void printDataArray(PrintWriter writer, String indent, int maxColumns) {
        int len = data.length;

        writer.print(indent);
        for (int i = 0; i < len; i++) {
            writer.print(toHex(data[i]));
            if (i != len - 1) {
                writer.print(", ");

                if ((i > 0) && ((i+1) % maxColumns == 0)) {
                    writer.println("");
                    writer.print(indent);
                }
            }
        }
    }

    /**
     * Converts byte to a hex string
     *
     * @param b byte value to convert
     * @return hex representation of byte
     */
    private static String toHex(byte b) {
        Integer I = new Integer((((int)b) << 24) >>> 24);
        int i = I.intValue();

        if (i < (byte)16) {
            return "0x0" + Integer.toString(i, 16);
        } else {
            return "0x" + Integer.toString(i, 16);
        }
    }
}

/**
 * Binary output stream capable of writing data
 * in big/little endian format.
 */
final class BinaryOutputStream {
    /** Underlying stream for writing bytes into */
    private DataOutputStream outputStream = null;

    /** true for big endian format, false for little */
    private boolean isBigEndian = false;

    /**
     * Constructor
     *
     * @param out underlying output stream for writing bytes into
     * @param bigEndian true for big endian format, false for little
     */
    BinaryOutputStream(OutputStream out, boolean bigEndian) {
        outputStream = new DataOutputStream(out);
        isBigEndian = bigEndian;
    }

    /**
     * Writes byte value into stream
     *
     * @param value byte value to write
     */
    public void writeByte(int value)
        throws java.io.IOException {

        outputStream.writeByte(value);
    }

    /**
     * Writes integer value into stream
     *
     * @param value integer value to write
     */
    public void writeInt(int value)
        throws java.io.IOException {

        if (isBigEndian) {
            outputStream.writeByte((value >> 24) & 0xFF);
            outputStream.writeByte((value >> 16) & 0xFF);
            outputStream.writeByte((value >> 8) & 0xFF);
            outputStream.writeByte(value & 0xFF);
        } else {
            outputStream.writeByte(value & 0xFF);
            outputStream.writeByte((value >> 8) & 0xFF);
            outputStream.writeByte((value >> 16) & 0xFF);
            outputStream.writeByte((value >> 24) & 0xFF);
        }
    }

    /**
     * Closes stream
     */
    public void close()
        throws java.io.IOException {

        outputStream.close();
    }
}
