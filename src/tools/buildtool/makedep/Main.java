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

package makedep;

// This program reads an include file database.
// The database should cover each self .c and .h file,
//   but not files in /usr/include
// The database consists of pairs of nonblank words, where the first word is
//   the filename that needs to include the file named by the second word.
// For each .c file, this program generates a fooIncludes.h file that 
//  the .c file may include to include all the needed files in the right order.
// It also generates a foo.dep file to include in the makefile.
// Finally it detects cycles, and can work with two files, an old and a new one.
// To incrementally write out only needed files after a small change.
//
//  Added PREFIX, {DEP/INC}_DIR, smaller dep output  10/92  -Urs

// Add something for precompiled headers

// To handle different platforms, I am introducing a platform file.
// The platform file contains lines like:
// os = svr4
//
// Then, when processing the includeDB file, a token such as <os>
// gets replaced by svr4. -- dmu 3/25/97

// Modified to centralize Dependencies to speed up make -- dmu 5/97

import java.util.*;
import util.*;

public class Main {

    static void usage() {
        System.out.print(usage_string);
    }

static final String usage_string =
  "Usage:\n"
+ "makeDeps platform-name platform-file database-file [MakeDeps args]\n"
+ "    [platform args]\n"
+ "\n"
+ "Supported values for platform-name: \n"
+ "    WinGammaPlatform, WinCEGammaPlatform, UnixPlatform\n"
+ "\n"
+ "MakeDeps args:\n"
+ "  -firstFile [filename]: Specify the first file in link order (i.e.,\n"
+ "   to have a well-known function at the start of the output file)\n"
+ "  -lastFile [filename]: Specify the last file in link order (i.e.,\n"
+ "   to have a well-known function at the end of the output file)\n"
+ "  -checkIncludeDB: Disable precompiled headers and/or source file merging\n"
+ "   to check if includeDB has missing entries\n"
;
    public static void main(String[] args) throws Throwable {
      try {
          if (args.length < 3) {
            usage();
            System.exit(1);
          }
      
          int argc = 0;

          String platformName = args[argc++];
          Class platformClass = Class.forName("makedep." + platformName);

          String plat1 = null;
          String db1 = null;
          String plat2 = null;
          String db2 = null;

          String firstFile = null;
          String lastFile = null;
          boolean resolveVpath = false;
          int sourceMergerLimit = 0;
          boolean checkIncludeDB = false;
          String workspace = null;
          String genDir = null;
          Properties globalProps = new Properties();
          getEnableFlags(globalProps);

          int numOptionalArgs = (args.length - 3);
          if (numOptionalArgs < 0) {
            usage();
            System.exit(1);
          }

          plat1 = args[argc++];
          db1   = args[argc++];

          // argc now points at start of optional arguments, if any

          try {
            boolean gotOne = true;
            while (gotOne && (argc <= args.length - 1)) {
              String arg = args[argc];
              if (arg.equals("-firstFile")) {
                firstFile = args[argc + 1];
                argc += 2;
              } else if (arg.equals("-lastFile")) {
                lastFile = args[argc + 1];
                argc += 2;
              } else if (arg.equals("-resolveVpath")) {
                resolveVpath = args[argc + 1].equals("true");
                argc += 2;
              } else if (arg.equals("-checkIncludeDB")) {
                checkIncludeDB = true;
                argc += 1;
              } else if (arg.equals("-sourceMergerLimit")) {
                // Merge multiple .cpp files into a single .cpp file to speed
                // up GCC compilation. For more info, see 
                // Database.createMergedOuterFiles()
                try {
                  sourceMergerLimit = Integer.parseInt(args[argc + 1]);
                } catch (Throwable t) {
                  System.err.println("invalid integer value \"" + args[argc + 1]
                                     + "\" for -sourceMergerLimit");
                  System.exit(-1);
                }      
                argc += 2;
              } else if (arg.equals("-workspace")) {
                workspace = args[argc + 1];
                argc += 2;
              } else if (arg.equals("-gendir")) {
                genDir = args[argc + 1];
                argc += 2;
              } else if (arg.indexOf('=') != -1) {
                String propName = arg.substring(0, arg.indexOf('='));
                String propValue = arg.substring(arg.indexOf('=') + 1);
                globalProps.setProperty(propName, propValue);
                argc++;
              } else {
                gotOne = false;
              }
            }
          }
          catch (Exception e) {
            e.printStackTrace();
            usage();
            System.exit(1);
          }

          Platform platform = (Platform) platformClass.newInstance();
          if (checkIncludeDB) {
            System.out.println("\n***\n");
            System.out.println("checking include DB -- precompiled headers/" +
                               "merged sources disabled");
            System.out.println("\n***");
            platform.setUsePrecompiledHeader(false);
          } else {
            platform.setUsePrecompiledHeader(true);
          }
          platform.setupFileTemplates();
          long t = platform.defaultGrandIncludeThreshold();
          
          String[] platformArgs = null;
          int numPlatformArgs = args.length - argc;
          if (numPlatformArgs > 0) {
            platformArgs = new String[numPlatformArgs];
            int offset = argc;
            while (argc < args.length) {
              platformArgs[argc - offset] = args[argc];
              ++argc;
            }
          }

          // If you want to change the threshold, change the default
          // "grand include" threshold in Platform.java, or override
          // it in the platform-specific file like UnixPlatform.java

          Database previous = new Database(platform, t);
          Database current = new Database(platform, t);

          previous.canBeMissing();
          
          if (firstFile != null) {
            previous.setFirstFile(firstFile);
            current.setFirstFile(firstFile);
          }
          if (lastFile != null) {
            previous.setLastFile(lastFile);
            current.setLastFile(lastFile);
          }
          previous.setResolveVpath(resolveVpath);
          current.setResolveVpath(resolveVpath);
          if (checkIncludeDB) {
            sourceMergerLimit = 0;
          }
          previous.setSourceMergerLimit(sourceMergerLimit);
          current.setSourceMergerLimit(sourceMergerLimit);

          if (workspace != null) {
            previous.setWorkspace(workspace);
            current.setWorkspace(workspace);
          }
          if (genDir != null) {
            previous.setGenDir(genDir);
            current.setGenDir(genDir);
          }

            if (resolveVpath) {
                if (workspace == null) {
                    System.out.println("-resolveVpath is set but " +
                                       "-workspace is not set");
                    usage();
                    System.exit(1);
                }
            }

          current.get(plat1, db1, globalProps);
          current.compute();
          current.put();

          if (platformArgs != null) {
            // Allow the platform to write platform-specific files
            platform.writePlatformSpecificFiles(previous, current,
                                        platformArgs);
          }
      }
      catch (Exception e) {
          e.printStackTrace();
      }
    }

    static void getEnableFlags(Properties globalProps) throws Throwable {
        Hashtable env = Util.getenv();
        boolean verbose = (env.get("VERBOSE") != null);

        for (Enumeration e = env.keys(); e.hasMoreElements() ;) {
            String key = (String)e.nextElement();
            if (key.startsWith("ENABLE_") && !key.endsWith("__BY")) {
                String propName = key;
                String propValue = (String)env.get(key);
                globalProps.setProperty(propName, propValue);
                if (verbose) {
                    System.out.println(propName + " = " + propValue);
                }
            }
        }
    }
}
