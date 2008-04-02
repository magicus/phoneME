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

package mjpp;

import java.io.*;
import java.util.*;
import util.*;

/**
 * Java preprocessor --
 * Preprocess java source files to reduce footprint.
 *
 * See $(JVMWorkSpace)/internal_doc/BuildSystem.html for more
 * information.
 */
public class Main {
    static void usage() {
        System.out.println("Usage: java -jar buildtool.jar " +
                           "mjpp -srcroot <dir> -outdir <dir> files ...");
    }

    static boolean do_strip = true;

    public static void main(String args[]) throws Throwable {
        Vector roots = new Vector();
        Vector files = new Vector();
        Vector output = new Vector();
        int i;

        String outdir = null;
        for (i=0; i<args.length; ) {
            if (args[i].equals("-srcroot")) {
                roots.addElement(args[i+1]); 
                i+=2;
            } else if (args[i].equals("-outdir")) {
                outdir = args[i+1];
                i+=2;
            } else if (args[i].equals("-n")) {
                do_strip = false;
                i++;
            } else {
                files.addElement(args[i]);
                i++;
            }
        }

        if (outdir == null) {
            System.err.println("-outdir not specified");
            System.exit(-1);
        }
        if (roots.size() == 0) {
            System.err.println("-srcroot not specified");
            System.exit(-1);
        }

        process(outdir, roots, files, output);
        for (i=0; i<output.size(); i++) {
            System.out.print(output.elementAt(i));
            System.out.print(" ");
        }
    }

    public static void process(String outdir, Vector roots, Vector files,
                               Vector output)
        throws Throwable
    {
        File file = new File(outdir);
        if (!file.exists()) {
            file.mkdirs();
        }
        for (int i=0; i<files.size(); i++) {
            String f = process(outdir, roots, (String)files.elementAt(i));
            output.addElement(f);
        }
    }

    static String process(String outdir, Vector roots, String file) 
        throws IOException
    {
        String subdir = findSubDir(roots, file);
        File dir = new File(outdir);
        dir = new File(dir, subdir);
        if (!dir.exists()) {
            dir.mkdirs();
        }

        File inf  = new File(file);
        File outf = new File(dir, inf.getName());
        process(inf, outf);

        return outf.toString().replace('\\', '/');
    }

    /*
     * IMPL_NOTE: currently the preprocessor doesn't look at the 
     * parameter of the #{if,ifdef,ifndef} directive. It just
     * blindly removes everything between #if ... #endif
     */
    static void process(File inf, File outf) throws IOException {
        FileInputStream in = new FileInputStream(inf);
        FileOutputStream out = new FileOutputStream(outf);
        BufferedReader reader = new BufferedReader(new InputStreamReader(in));
        PrintWriter writer = new PrintWriter(new OutputStreamWriter(out));
        String s;

        boolean skipping = false;
        String startPrefix = "/* #if";
        String endPrefix   = "/* #endif */";
        int lineno = 0;
        while ((s = reader.readLine()) != null) {
            lineno ++;
            if (do_strip && s.startsWith(startPrefix)) {
                if (skipping) {
                    System.err.println("Error in " + inf + ": " + lineno);
                    System.err.println("/* #ifxxx cannot be nested (yet)! */");
                    writer.close();
                    System.exit(1);
                } else {
                    skipping = true;
                }
                writer.println(s);
            } else if (do_strip && s.startsWith(endPrefix)) {
                if (!skipping) {
                    System.err.println("Error in " + inf + ": " + lineno);
                    System.err.println("unexpected /* #endif */");
                    writer.close();
                    System.exit(1);
                } else {
                    skipping = false;
                }
                writer.println(s);
            } else {
                if (skipping) {
                    writer.print("/// skipped ");
                }
                writer.println(s);
            }
        }
        if (skipping) {
            System.err.println("Error in " + inf + ": " + lineno);
            System.err.println("missing /* #endif */");
            writer.close();
            System.exit(1);
        }

        writer.close();
        reader.close();
    }

    static String findSubDir(Vector roots, String file) {
        for (int i=0; i<roots.size(); i++) {
            String root = (String)roots.elementAt(i);
            if (file.startsWith(root)) {
                file = file.substring(root.length());
                while (file.startsWith(File.separator)) {
                    file = file.substring(1);
                }
                File f = new File(file);
                return f.getParent();
            }
        }
        throw new RuntimeException("root not known: " + file);
    }
}
