/*
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

import java.io.*;

public class EncodingTest {
    static String[] s = { "US-ASCII", "ISO-8859-1", "UTF-8", "UTF-16BE",
                   "UTF-16LE", "UTF-16" };

    public static void main(String[] args) throws Exception {
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        //PrintStream p = new PrintStream(out, false, "US-ASCII");
        boolean failed = false;
        for (int i = 0; i < s.length; i++) {
           try {
              System.out.println("Testing " + s[i]);
              OutputStreamWriter o = new OutputStreamWriter(out, s[i]);
           } catch (Exception e) {
              failed = true;
              e.printStackTrace();
           }
        }
        if (!failed) 
           System.out.println("OK");
        else
           System.out.println("FAILED");
    }
}
