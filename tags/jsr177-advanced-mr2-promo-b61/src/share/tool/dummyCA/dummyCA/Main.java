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

package dummyCA;

import java.io.*;

public class Main {

    public static void main(String[] args) throws Exception {

        if (args.length != 2) {
            System.out.println("Specify CSR and sertificate file names.");
            System.exit(1);
        }

        byte[] data = parseCSR(args[0]);

        Authority.init(null);
        Authority CA = new Authority();

        if (! CA.createCertificate(data)) {
            System.out.println(CA.getStatus());
            System.exit(1);
        }

//        writeBinary(args[1], CA.getCertificate());
        writeText(args[1], CA.getCertificate());

        System.out.println("Ok.");
    }

    private static byte[] parseCSR(String arg) {

        BufferedReader reader = null;

        try {
            reader = new BufferedReader(
                         new InputStreamReader(
                             new FileInputStream(new File(arg))));
        } catch (FileNotFoundException e) {
            System.out.println("Source file not found.");
            System.exit(1);
        }

        String s = "";

        String line = null;
        try {
            line = reader.readLine();
            while (line != null) {
                if (! line.startsWith("-")) {
                    s += line.trim();
                }
                line = reader.readLine();
            }
            reader.close();
        } catch (IOException e) {
            System.out.println("Error reading the CSR.");
            System.exit(1);
        }

        byte[] data = null;
        try {
            data = Base64.decode(s);
        } catch (IOException e) {
            System.out.println("Error decoding the CSR.");
            System.exit(1);
        }

        return data;
    }

    private static void writeBinary(String arg, byte[] data) {

        DataOutputStream writer = null;

        try {
            writer = new DataOutputStream(
                         new FileOutputStream(arg));
        } catch (IOException e) {
            System.out.println("Can't create certificate file.");
            System.exit(1);
        }

        try {
            writer.write(data, 0, data.length);
            writer.close();
        } catch (IOException e) {
            System.out.println("Error writing the certificate file.");
            System.exit(1);
        }
    }

    private static void writeText(String arg, byte[] data) {

        PrintWriter writer = null;

        try {
            writer = new PrintWriter(new FileOutputStream(arg));
        } catch (IOException e) {
            System.out.println("Can't create certificate file.");
            System.exit(1);
        }

        String s = Base64.encode(data, 0, data.length);

        writer.println("-----BEGIN CERTIFICATE-----");
        int i = 0;
        while (i < s.length()) {

            int j = s.length() - i;
            if (j > 64) {
                j = 64;
            }
            writer.println(s.substring(i, i + j));
            i += j;
        }
        writer.println("-----END CERTIFICATE-----");
        writer.close();
    }

    private static void writeArray(String arg, byte[] data) {

        PrintWriter writer = null;

        try {
            writer = new PrintWriter(new FileOutputStream(arg));
        } catch (IOException e) {
            System.out.println("Can't create output file.");
            System.exit(1);
        }

        for (int i = 0; i < data.length; i++) {

            writer.print("(byte) 0x" +
                    Integer.toHexString(data[i] & 0xff) + ", ");
            if ((i + 1) % 6 == 0) {
                writer.println();
            }
        }
        writer.close();
    }
}
