/*
 * @(#)PXletRunner.java	1.13 06/10/10
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
 * cvm sun.mtask.xlet.PXletRunner -name <XletName> {-path <XletPath> | 
 *     -codebase <URL_path>} [-args <arg1> [<arg2>] [<arg3>] ...]
 *
 * cvm sun.mtask.xlet.PXletRunner -filename <filename>
 *
 * The xlet should not be found in the classpath and
 * <XletPath> is relative to the current directory.
 */

package sun.mtask.xlet;

// To read the command line from a file.
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.StringTokenizer;

// Data structure to save the command line options.
import java.util.Vector;

import com.sun.xlet.XletLifecycleHandler;

public class PXletRunner {

    private static boolean verbose = (System.getProperty("cdcams.verbose") != null) &&
        (System.getProperty("cdcams.verbose").toLowerCase().equals("true"));

    static String[] flags = {"-name", "-path", "-codebase", "-args", "-filename", "-laf", "-lafTheme", "-loadOnly" };
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
                if (verbose) {
                    System.out.println("Could not find file " + filename);
                }
                System.exit(1);
            } catch (IOException ioe) { 
                if (verbose) {
                    System.out.println("IOException caught while reading file " 
                        + filename);
                }   
                System.exit(1);
            }
        } 

        String   name = null;
        String[] paths = null;
        String[] xletArgs = new String[]{};
	String   laf = null;        // L&F, if applicable
	String   lafTheme = null;   // Theme for L&F, if applicable
        boolean  isLoadOnly = false; // If true, don't init the xlet
        for (int i = 0; i < args.length;) {
            try {
                if (args[i].equals("-laf")) {
		    laf = args[++i];
		} else if (args[i].equals("-lafTheme")) {
		    lafTheme = args[++i];
		} else if (args[i].equals("-name")) {
                    name = args[++i];
                    if (!(args[++i].equals("-path") || args[i].equals("-codebase"))) {
                        printErrorAndExit();
                    }
                    Vector v = new Vector();
                    if (args[i].equals("-path")) {
                       StringTokenizer tok = new StringTokenizer(args[++i], File.pathSeparator);
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
                } else if (args[i].equals("-loadOnly")) {
                    isLoadOnly = true;
                    i++;
                } else {
                    i++;
                }
            } catch (Exception e) { 
                e.printStackTrace(); 
                printErrorAndExit();
            }
        }
	// Parsing is finished. Now start the xlet by calling methods on
	// the Xlet Manager.

	try {
            if (verbose) {
	        System.out.println("@@PXletRunner starting Xlet " + name);
            }

	    // Get an instance of XletLifecycle from the Xlet Manager,
	    // and post a request on the handler.
	    PXletManager handler = 
		PXletManager.createXlet(name, laf, lafTheme, paths, xletArgs);
	    
	    // First, tell the system to recognize us as the
	    // singleton xlet.
	    //handler.register();

            if (!isLoadOnly) {
	       // Call a method so the xlet is initialized. 
	       // Xlet.initXlet(XletContext) is invoked on the Xlet Manager.
	       handler.postInitXlet();
	       // Call a method so the xlet is started.
	       // Xlet.startXlet() is invoked on the Xlet Manager.
	       handler.postStartXlet();
            }

	} catch (Throwable e) {
	    // If we can't get this xlet started, there is no need to linger.
	    // So go away.
            if (verbose) {
	        System.out.println("Error while loading xlet: " + name);
            }
	    e.printStackTrace();
	    System.exit(1);
	}
	
    }
   
    // If there was a problem parsing the command line, call this method, which ultimately exits.
    static void printErrorAndExit() {
        if (verbose) {
            System.out.println("\n\nPXletRunner Usage: ");
            System.out.println("cvm sun.mtask.xlet.PXletRunner " + 
                "-name <xletname> -path <xletpath> ");
            System.out.println("\nOptions");
            System.out.println("-filename <filename>                   Reads PXletRunner arguments from a file");
            System.out.println("-args <arguments separated by space>   Xlet runtime arguments");
            System.out.println("-codebase <URLs separated by space>    Specifies class location in URL format, replaces \"-path\" option");
            System.out.println("\nRepeat arguments to run more than one xlets");
        }
        System.exit(1);
    }

}
