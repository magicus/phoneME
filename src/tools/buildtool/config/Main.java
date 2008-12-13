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

package config;

import java.io.*;
import java.util.*;
import util.*;

/**
 * Wrapper for the Configurator class.
 */
public class Main {
    static void usage() {
        System.out.println("Usage: java -jar buildtool.jar " +
                           "config <platformfile> <inputfile> <outputfile> " +
                           "<extraflags> ...");
    }

    public static void main(String args[]) throws Throwable {
        if (args.length < 3) {
            usage();
        } else {
            Configurator config = new Configurator();
            Hashtable env = Util.getenv();
            Vector extra = new Vector();
            for (int i=3; i<args.length; i++) {
                extra.addElement(args[i]);
            }

            config.readPlatformFile(args[0]);
            config.readInputFile(args[1]);
            config.write(args[2], env, extra);
        }
    }
}
