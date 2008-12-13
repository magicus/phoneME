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

package util;

import java.io.*;
import java.util.*;

/**
 * Miscellaneous methods used by buildtool.jar
 */
public class Util {
    /* Heck there's no getenv, we have to roll one ourselves! */
    public static Hashtable getenv() throws Throwable {
        Process p;
        if (File.separator.equals("\\")) {
            p = Runtime.getRuntime().exec("cmd /c set");
        } else {
            p = Runtime.getRuntime().exec("printenv");
        }

        InputStream in = p.getInputStream();
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        String line;
        Hashtable table = new Hashtable();
        
        while ((line = reader.readLine()) != null) {
            int i = line.indexOf('=');
            if (i > 0) {
                String name = line.substring(0, i);
                String value = line.substring(i+1);
                table.put(name, value);
            }
        }

        in.close();
        p.waitFor();
        return table;
    }
}
