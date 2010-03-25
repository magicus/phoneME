/*
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
 * A tool that reads and processes a series of rom.config 
 * files. For now we only care about the HiddenPackage and
 * RestrictedPackage.
 * 
 * Note this is not equivalent to the CLDC romizer.
 */

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintStream;
import java.io.PrintWriter;
import java.util.Vector;

public class RomConfProcessor {
    String outName = "MIDPPkgChecker";
    Vector pkgs = new Vector();

    public static void main(String args[]) {
        String dirs[] = null;
        String romfiles[] = null;
        RomConfProcessor processor = new RomConfProcessor();
        for (int i = 0; i < args.length; i++) {
            int j, idx;

            if (args[i].equals("-dirs")) {
                int firstdir = i + 1;
                int lastdir = firstdir;
                while (lastdir < args.length &&
                       !args[lastdir].startsWith("-")) {
                    lastdir++;
		}
                dirs = new String[lastdir - firstdir];
                idx = 0;
                for (j = firstdir; j < lastdir; j++) {
                    //DEBUG: System.out.println("dir="+args[j]);
                    dirs[idx++] = args[j];
		}
                i += (lastdir - firstdir);
	    } else if (args[i].equals("-romfiles")) {
                int firstfile = i + 1;
                int lastfile = firstfile;
                while (lastfile < args.length && 
                       !args[lastfile].startsWith("-")) {
                    lastfile++;
		}
                romfiles = new String[lastfile - firstfile];
                idx = 0;
                for (j = firstfile; j < lastfile; j++) {
                    //DEBUG: System.out.println("file="+args[j]);
                    romfiles[idx++] = args[j];
		}
                i += (lastfile - firstfile);
	    }
	}

        processor.processRomfiles(dirs, romfiles);
        processor.writeOutput();
    }

    void processRomfiles(String dirs[], String files[]) {
        int i;
        int numOfFiles = files.length;
        File romfiles[] = new File[numOfFiles];
        for (i = 0; i < numOfFiles; i++) {
            for (int j = 0; j < dirs.length; j++) {
                File f = new File(dirs[j], files[i]);
                if (f.exists()) {
		    System.out.println("Found file "+ f);
                    romfiles[i] = f;
                    break;
		}
	    }
            System.err.println("Could not find file " + files[i]);
        }

        for (i = 0; i < numOfFiles; i++) {
            File f = romfiles[i];
            if (f != null) {
                processRomfile(f);
	    }
        }
    }

    /* Process the rom.config file and collect HiddenPackage
       and RestrictedPackage. */
    void processRomfile(File f) {
        try {
            BufferedReader br = new BufferedReader(new FileReader(f));
            String s = br.readLine();
            while (s != null) {
                if (s.startsWith("HiddenPackage =") ||
                    s.startsWith("RestrictedPackage =")) {
		    String pkg = s.substring(s.indexOf('=')+1).trim();
                    // DEBUG: System.out.println(pkg);
                    pkgs.add(pkg);
	        }
                s = br.readLine();
   	    }
            br.close();
	} catch (FileNotFoundException e1) {
            System.err.println("Failed to open input file " + f);
            System.exit(-1);
        } catch (IOException e2) {
            System.err.println("Failed to read input file " + f);
            System.exit(-1);
	}
    }

    /* Generate the MIDPPkgChecker java file */
    void writeOutput() {
        int numOfPkgs = pkgs.size();
        int hashSize = (int)(numOfPkgs * 1.25);
        File outFile = new File(outName + ".java");
        PrintWriter output = null;

        try {
            output = new PrintWriter(new FileWriter( outFile ));
	} catch (IOException e) {
            System.out.println("Failed to open output file " + outFile);
            return;
	}
        output.println("package sun.misc;\n");
        output.println("import java.util.*;\n");
        output.println("public final class " + outName + " {");
        output.println("    static HashSet pkgs = new HashSet(" + hashSize + ");\n");
        output.println("    static {");
        for (int i = 0; i < numOfPkgs; i++) {
            String s = (String)pkgs.get(i);
            output.println("        pkgs.add(\"" + s + "\");");
	}
        output.println("    }\n");
        output.println("    public static boolean checkPackage(String pkg) {");
        output.println("        if (pkgs.contains(pkg)) {");
        output.println("            return true;");
        output.println("        } else { return false; }");
        output.println("    }\n");
        output.println("}");
        output.flush();
        output.close();
    }
}
