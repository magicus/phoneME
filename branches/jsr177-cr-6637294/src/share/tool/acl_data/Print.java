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
 * Print class implementation
 */
public class Print {


    /**
     * Main function
     * @param args program argument - input file name.
     */
    public static void main(String[] args) {
        new Print().run(args);
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
            FileInputStream f;
            ByteArrayOutputStream b;
            
            for (int i = 0; i < args.length; i++) {
                int ch;
                
                System.out.println("File: " + args[i]);
                f = new FileInputStream(args[i]);
                b = new ByteArrayOutputStream();
                while ((ch = f.read()) != -1) {
                    b.write(ch);
                }
                f.close();
                byte[] cont = b.toByteArray();
                if (cont.length == 0) {
                    System.out.println("    empty");
                } else {
                    int index = 0;
                    while (index < cont.length) {
                        if (index != 0) {
                            System.out.println("---");
                        }
                        TLV tlv = new TLV(cont, index);
                        tlv.print();
                        index = tlv.valueOffset + tlv.length;
                    }
                }
                System.out.println("======================================");
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}
