/*
 * @(#)XletRunner.java	1.26 06/10/10
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
 *
 */

/*
 * A sample class to introduce xlet to the system
 *
 * Usage: 
 * cvm com.sun.xlet.XletRunner -name <XletName> {-path <XletPath> | 
 *     -codebase <URL_path>} [-args <arg1> [<arg2>] [<arg3>] ...]
 *
 * cvm com.sun.xlet.XletRunner -filename <filename>
 *
 * The xlet should not be found in the classpath and
 * <XletPath> is relative to the current directory.
 */

package com.sun.xlet;

// To read the command line from a file.
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.StringTokenizer;

// Data structure to save the command line options.
import java.util.Vector;

public class XletRunner {
    static String[] flags = {"-name", "-path", "-codebase", "-args", "-filename" };
    static boolean isKey(String s) {
        for (int i = 0; i < flags.length; i++) 
            if (s.equals(flags[i])) return true;
        return false;
    }
       
    public static void main(String[] args) {
        if (args.length < 2) 
            printErrorAndExit();

        // Parse the command line options.
        if (args[0].equals("-filename")) {
            String filename = args[1];
            try {
                BufferedReader reader = new BufferedReader(
                        new FileReader(filename));
                Vector v = new Vector();
                String s;
                while ((s = reader.readLine()) != null) {
                    StringTokenizer tok = new StringTokenizer(s, " ");
                    while (tok.hasMoreTokens()) {
                        v.addElement(tok.nextToken());
                    }
                }
                args = new String[v.size()];
                for (int i = 0; i < v.size(); i++) {
                    args[i] = (String) v.elementAt(i);
                }
            } catch (FileNotFoundException fnf) { 
                System.out.println("Could not find file " + filename);
                System.exit(1);
            } catch (IOException ioe) { 
                System.out.println("IOException caught while reading file " 
                    + filename);
                System.exit(1);
            }
        } 

        String   name = null;
        String[] paths;
        String[] xletArgs = new String[]{};
        for (int i = 0; i < args.length;) {
            try {
                if (args[i].equals("-name")) {
                    if (i+1 == args.length){
                        System.err.println("Name not specified");
                        printErrorAndExit();
                    }
                    name = args[++i];
                    if (++i == args.length || 
                            !(args[i].equals("-path") || 
                            args[i].equals("-codebase"))) {
                        System.err.println("Missing path arguments");
                        printErrorAndExit();
                    }
                    Vector v = new Vector();
                    if (args[i].equals("-path")) {
                       if (++i == args.length){
                           System.err.println("Path not specified");
                           printErrorAndExit();
                       }
                       StringTokenizer tok = new StringTokenizer(args[i], File.pathSeparator);
                       while (tok.hasMoreTokens()) { 
                           v.addElement(tok.nextToken()); 
                       }
                    } else {
                       while ((i+1) < args.length && !args[i+1].equals("-args")) {
                           v.addElement(args[++i]);
                       } 
                    }
                    paths = (String[]) v.toArray(new String[v.size()]); 
                    if ((i + 1) < args.length && args[++i].equals("-args")) {
                        v = new Vector();
                        while ((i + 1) < args.length && !isKey(args[++i])) {
                            v.addElement(args[i]);
                        }
                        xletArgs = (String[]) v.toArray(new String[v.size()]);
                    }


                    // Parsing is finished. Now start the xlet by calling methods on
                    // the Xlet Manager.

                    System.out.println("@@XletRunner starting Xlet " + name);
                    try {
                       // Get an instance of XletLifecycle from the Xlet Manager,
                       // and post a request on the handler.
                       XletLifecycleHandler handler = 
                           XletManager.createXlet(name, paths, xletArgs);
                       // Call a method so the xlet is initialized. 
                       // Xlet.initXlet(XletContext) is invoked on the Xlet Manager.
                       handler.postInitXlet();
                       // Call a method so the xlet is started.
                       // Xlet.startXlet() is invoked on the Xlet Manager.
                       handler.postStartXlet();
                    } catch (Exception e) {
                        System.out.println("Error while loading xlet: " + name);
                        e.printStackTrace();  
                        System.exit(1);
                    }
                    xletArgs = new String[]{};
                } else {
                    i++;
                }
            } catch (Exception e) { 
                e.printStackTrace(); 
                printErrorAndExit();
            }
        }
        if (name == null){
            System.err.println("Missing name arguments");
            printErrorAndExit();
        }
    }
   
    // If there was a problem parsing the command line, call this method, which ultimately exits.
    static void printErrorAndExit() {
        System.err.println("XletRunner Usage: ");
        System.err.println("cvm com.sun.xlet.XletRunner " + 
            "-name <xletname> -path <xletpath> ");
        System.err.println("\nOptions");
        System.err.println("-filename <filename>                   Reads XletRunner arguments from a file");
        System.err.println("-args <arguments separated by space>   Xlet runtime arguments");
        System.err.println("-codebase <URLs separated by space>    Specifies class location in URL format, replaces \"-path\" option");
        System.err.println("\nRepeat arguments to run more than one xlets");
        System.exit(1);
    }
}
