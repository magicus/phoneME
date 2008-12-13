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

import java.io.*;
import java.util.*;

public class WinCEGammaPlatform extends Platform {
    public String fileSeparator() {
      return "\\";
    }

    private static String[] suffixes = { ".cpp", ".c" };

    public String[] outerSuffixes() {
        return suffixes;
    }

    public String objFileSuffix() {
        return ".obj";
    }

    public String asmFileSuffix() {
        return ".asm";
    }

    public String dependentPrefix() {
        return "$(VM_PATH)";
    }

    public boolean fileNameStringEquality(String s1, String s2) {
        return s1.equalsIgnoreCase(s2);
    }

    private void usage() throws IllegalArgumentException {
        System.err.println("WinCEGammaPlatform platform-specific options:");
        System.err.println("  -sourceBase <path to directory (workspace) " +
                           "containing source files; no trailing slash>");
        System.err.println("  -vcpFileName <full pathname to which .vcp file " +
                           "will be written; all parent directories must " +
                           "already exist>");
        System.err.println("  -envVar <environment variable to be inserted " +
                           "into .vcp file, substituting for path given in " +
                           "-sourceBase. Example: HotSpotWorkSpace>");
        System.err.println("  -exeLoc <path to directory in which to put " +
                           "cldc_vm.exe and cldc_vm_g.exe; no trailing slash>");
        System.err.println("  If any of the above are specified, "+
                           "they must all be.");
        System.err.println("  Additional, optional arguments, which can be " +
                           "specified multiple times:");
        System.err.println("    -absoluteInclude <string containing absolute " +
                           "path to include directory>");
        System.err.println("    -relativeInclude <string containing include " +
                           "directory relative to -envVar>");
        System.err.println("    -define <preprocessor flag to be #defined " +
                           "(note: doesn't yet support " +
                           "#define (flag) (value))>");
        System.err.println("    -perFileLine <file> <line>");
        System.err.println("    -conditionalPerFileLine <file> <line for " +
                           "release build> <line for debug build>");
        System.err.println("  (NOTE: To work around a bug in nmake, where " +
                           "you can't have a '#' character in a quoted " +
                           "string, all of the lines outputted have \"#\"" +
                           "prepended)");
        System.err.println("    -startAt <subdir of sourceBase>");
        System.err.println("    -ignoreFile <file which won't be able to be " +
                           "found in the sourceBase because it's generated " +
                           "later>");
        System.err.println("    -additionalFile <file not in database but " +
                           "which should show up in .vcp file, like " +
                           "includeDB_core>");
        System.err.println("    -additionalGeneratedFile <environment variable of " +
                           "generated file's location> <relative path to " +
                           "directory containing file; no trailing slash> " +
                           "<name of file generated later in the build process>");
        System.err.println("    Default includes: \".\"");
        System.err.println("    Default defines: WINCE, _CONSOLE");
        throw new IllegalArgumentException();
    }

    private String getVcpName(String fullPath)
        throws IllegalArgumentException, IOException {
        File file = new File(fullPath).getCanonicalFile();
        fullPath = file.getCanonicalPath();
        String parent = file.getParent();
        String separator = System.getProperty("file.separator");
        String vcpString = ".vcp";

        if (!fullPath.endsWith(vcpString)) {
            throw new IllegalArgumentException(".vcp file name \"" +
                                               fullPath +
                                               "\" does not end in .vcp");
        }

        if ((parent != null) &&
            (!fullPath.startsWith(parent))) {
            throw new RuntimeException(
                "Internal error: parent of file name \"" + parent +
                "\" does not match file name \"" + fullPath + "\""
            );
        }

        int len = parent.length();
        if (!parent.endsWith(separator)) {
            len += separator.length();
        }
        
        int end = fullPath.length() - vcpString.length();

        if (len == end) {
            throw new RuntimeException(
                "Internal error: file name was empty"
            );
        }

        return fullPath.substring(len, end);
    }

    public void addPerFileLine(Hashtable table,
                               String fileName,
                               String line) {
        Vector v = (Vector) table.get(fileName);
        if (v != null) {
            v.add(line);
        } else {
            v = new Vector();
            v.add(line);
            table.put(fileName, v);
        }
    }
                               
    private static class PerFileCondData {
        public String releaseString;
        public String debugString;
    }

    private void addConditionalPerFileLine(Hashtable table,
                                           String fileName,
                                           String releaseLine,
                                           String debugLine) {
        PerFileCondData data = new PerFileCondData();
        data.releaseString = releaseLine;
        data.debugString = debugLine;
        Vector v = (Vector) table.get(fileName);
        if (v != null) {
            v.add(data);
        } else {
            v = new Vector();
            v.add(data);
            table.put(fileName, v);
        }
    }

    public boolean findString(Vector v, String s) {
        for (Iterator iter = v.iterator(); iter.hasNext(); ) {
            if (((String) iter.next()).equals(s)) {
                return true;
            }
        }

        return false;
    }

    /* This returns a String containing the full path to the passed
       file name, or null if an error occurred. If the file was not
       found or was a duplicate and couldn't be resolved using the
       preferred paths, the file name is added to the appropriate
       Vector of Strings. */
    private String findFileInDirectory(String fileName,
                                       DirectoryTree directory,
                                       Vector preferredPaths,
                                       Vector filesNotFound,
                                       Vector filesDuplicate) {
        List locationsInTree = directory.findFile(fileName);
        String name = null;
        if ((locationsInTree == null) ||
            (locationsInTree.size() == 0)) {
            filesNotFound.add(fileName);
        } else if (locationsInTree.size() > 1) {
            // Iterate through them, trying to find one with a
            // preferred path
        search:
            {
                for (Iterator locIter = locationsInTree.iterator();
                     locIter.hasNext(); ) {
                    DirectoryTreeNode node =
                        (DirectoryTreeNode) locIter.next();
                    String tmpName = node.getName();
                    for (Iterator prefIter = preferredPaths.iterator();
                         prefIter.hasNext(); ) {
                        if (tmpName.indexOf((String) prefIter.next()) != -1) {
                            name = tmpName;
                            break search;
                        }
                    }
                }
            }
            
            if (name == null) {
                filesDuplicate.add(fileName);
            }
        } else {
            name = ((DirectoryTreeNode) locationsInTree.get(0)).getName();
        }

        return name;
    }

    private void outputSourceFiles(Vector outputStrings,
                                   Vector fileNames,
                                   Hashtable perFileLines,
                                   Hashtable conditionalPerFileLines,
                                   PrintWriter vcpFile,
                                   String vcpName) {
        Iterator outputIter = outputStrings.iterator();
        Iterator fileIter = fileNames.iterator();
        while (outputIter.hasNext()) {
            String outStr = (String) outputIter.next();
            String fileStr = (String) fileIter.next();

            vcpFile.println("# Begin Source File");
            vcpFile.println("");
            if (fileStr.endsWith(asmFileSuffix())) {
              vcpFile.println("SOURCE=.\\" + fileStr );
            } else {
              vcpFile.println("SOURCE=\"" + outStr + "\"");
            }
            Vector v = null;

            v = (Vector) perFileLines.get(fileStr);
            if (v != null) {
                for (Iterator lineIter = v.iterator(); lineIter.hasNext(); ) {
                    String str = (String) lineIter.next();
                    vcpFile.println("# " + str);
                }
            }
            v = (Vector) conditionalPerFileLines.get(fileStr);
            if (v != null) {
                vcpFile.println("!IF  \"$(CFG)\" == \"" + vcpName +
                                " - WinCE Release\"");
                vcpFile.println("");
                for (Iterator lineIter = v.iterator(); lineIter.hasNext(); ) {
                    PerFileCondData data = (PerFileCondData) lineIter.next();
                    vcpFile.println("#" + data.releaseString);
                }
                vcpFile.println("!ELSEIF  \"$(CFG)\" == \"" + vcpName +
                                " - WinCE Debug\"");
                vcpFile.println("");
                for (Iterator lineIter = v.iterator(); lineIter.hasNext(); ) {
                    PerFileCondData data = (PerFileCondData) lineIter.next();
                    vcpFile.println("#" + data.debugString);
                }
                vcpFile.println("!ENDIF");
            }                
            vcpFile.println("# End Source File");
        }
    }

    private void outputInterpreterDirectives(PrintWriter vcpFile, String vcpName) {
      String fileStr = "Interpreter_arm";
      String projectName = "Win32 (WCE ARM)";
      String assemblyCommand = "\"$(ASMDIR_ARM)\\armasm.exe\" -o $(OutDir)\\$(InputName).obj $(InputPath)";
 
	  vcpFile.println("");
      vcpFile.println("# Begin Source File");
      vcpFile.println("");
      vcpFile.println("SOURCE=\"..\\target\\product\\Interpreter_arm.asm\"");
      vcpFile.println("");
      vcpFile.println("!IF  \"$(CFG)\" == \"" + vcpName + " - " + projectName + " Product\"");
      vcpFile.println("");
      vcpFile.println("# Begin Custom Build - Performing Custom Build Step on $(InputName).asm");
      vcpFile.println("OutDir=.\\ARMPrd");
      vcpFile.println("InputPath=\".\\" + fileStr + "\"");
      vcpFile.println("InputName=" + fileStr);
      vcpFile.println("");
      vcpFile.println("\"$(OutDir)\\$(InputName).obj\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"");
      vcpFile.println("    " + assemblyCommand);
      vcpFile.println("# End Custom Build");
      vcpFile.println("");
      vcpFile.println("!ELSEIF  \"$(CFG)\" == \"" + vcpName + " - " + projectName + " Release\"");
      vcpFile.println("");
      vcpFile.println("# PROP Exclude_From_Build 1");
      vcpFile.println("");
      vcpFile.println("!ELSEIF  \"$(CFG)\" == \"" + vcpName + " - " + projectName + " Debug\"");
      vcpFile.println("");
      vcpFile.println("# PROP Exclude_From_Build 1");
      vcpFile.println("");
      vcpFile.println("!ENDIF");
      vcpFile.println("");
      vcpFile.println("# End Source File");

	  vcpFile.println("");
      vcpFile.println("# Begin Source File");
      vcpFile.println("");
      vcpFile.println("SOURCE=\"..\\target\\release\\Interpreter_arm.asm\"");
      vcpFile.println("");
      vcpFile.println("!IF  \"$(CFG)\" == \"" + vcpName + " - " + projectName + " Product\"");
      vcpFile.println("");
      vcpFile.println("# PROP Exclude_From_Build 1");
      vcpFile.println("");
      vcpFile.println("!ELSEIF  \"$(CFG)\" == \"" + vcpName + " - " + projectName + " Release\"");
      vcpFile.println("");
      vcpFile.println("# Begin Custom Build - Performing Custom Build Step on $(InputName).asm");
      vcpFile.println("OutDir=.\\ARMRel");
      vcpFile.println("InputPath=\".\\" + fileStr + "\"");
      vcpFile.println("InputName=" + fileStr);
      vcpFile.println("");
      vcpFile.println("\"$(OutDir)\\$(InputName).obj\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"");
      vcpFile.println("    " + assemblyCommand);
      vcpFile.println("# End Custom Build");
      vcpFile.println("");
      vcpFile.println("!ELSEIF  \"$(CFG)\" == \"" + vcpName + " - " + projectName + " Debug\"");
      vcpFile.println("");
      vcpFile.println("# PROP Exclude_From_Build 1");
      vcpFile.println("");
      vcpFile.println("!ENDIF");
      vcpFile.println("");
      vcpFile.println("# End Source File");

	  vcpFile.println("");
      vcpFile.println("# Begin Source File");
      vcpFile.println("");
      vcpFile.println("SOURCE=\"..\\target\\debug\\Interpreter_arm.asm\"");
      vcpFile.println("");
      vcpFile.println("!IF  \"$(CFG)\" == \"" + vcpName + " - " + projectName + " Product\"");
      vcpFile.println("");
      vcpFile.println("# PROP Exclude_From_Build 1");
      vcpFile.println("");
      vcpFile.println("!ELSEIF  \"$(CFG)\" == \"" + vcpName + " - " + projectName + " Release\"");
      vcpFile.println("");
      vcpFile.println("# PROP Exclude_From_Build 1");
      vcpFile.println("");
      vcpFile.println("!ELSEIF  \"$(CFG)\" == \"" + vcpName + " - " + projectName + " Debug\"");
      vcpFile.println("");
      vcpFile.println("# Begin Custom Build - Performing Custom Build Step on $(InputName).asm");
      vcpFile.println("OutDir=.\\ARMDbg");
      vcpFile.println("InputPath=\".\\" + fileStr + "\"");
      vcpFile.println("InputName=" + fileStr);
      vcpFile.println("");
      vcpFile.println("\"$(OutDir)\\$(InputName).obj\" : $(SOURCE) \"$(INTDIR)\" \"$(OUTDIR)\"");
      vcpFile.println("    " + assemblyCommand);
      vcpFile.println("# End Custom Build");
      vcpFile.println("");
      vcpFile.println("!ENDIF");
      vcpFile.println("");
      vcpFile.println("# End Source File");
    }
                                   
    private boolean databaseAllFilesEqual(Database previousDB,
                                          Database currentDB) {
        Iterator i1 = previousDB.getAllFiles().iterator();
        Iterator i2 = currentDB.getAllFiles().iterator();
        
        while (i1.hasNext() && i2.hasNext()) {
            FileList fl1 = (FileList) i1.next();
            FileList fl2 = (FileList) i2.next();
            if (!fl1.getName().equals(fl2.getName())) {
                return false;
            }
        }

        if (i1.hasNext() != i2.hasNext()) {
            // Different lengths
            return false;
        }

        return true;
    }

    private String envVarPrefixedFileName(String fileName,
                                          int sourceBaseLen,
                                          String envVar,
                                          DirectoryTree tree,
                                          Vector preferredPaths,
                                          Vector filesNotFound,
                                          Vector filesDuplicate) {
      String fullName = findFileInDirectory(fileName,
                                            tree,
                                            preferredPaths,
                                            filesNotFound,
                                            filesDuplicate);
      if (fullName == null) {
        return null;
      }
      return "$(" + envVar + ")" +
        fullName.substring(sourceBaseLen);
    }

    public void writePlatformSpecificFiles(Database previousDB,
                                           Database currentDB, String[] args)
        throws IllegalArgumentException, IOException {
        if (args.length == 0) {
            return;
        }
        
        String sourceBase = null;
        String startAt = null;
        String vcpFileName = null;
        String envVar = null;
        String exeLoc = null;
        // This contains Strings: list of all file names (database +
        // additional files)
        Vector allFileNames = new Vector();
        // This contains Strings: any extra includes with absolute paths
        Vector absoluteIncludes = new Vector();
        // This contains Strings: any extra includes with paths
        // relative to -envVar
        Vector relativeIncludes = new Vector();
        // This contains Strings: preprocessor flags to #define
        Vector defines = new Vector();
        // This maps Strings containing file names to Vectors of
        // Strings
        Hashtable perFileLines = new Hashtable();
        // This maps Strings containing file names to Vectors of
        // PerFileCondData objects
        Hashtable conditionalPerFileLines = new Hashtable();
        // This contains Strings: additional files to add to the
        // project file (i.e., includeDB, for convenience)
        Vector additionalFiles = new Vector();
        // This contains Strings: files in the database to be ignored
        // because they're autogenerated later (compiler2 files
        // ad_<arch>.[ch]pp, dfa_<arch>.cpp)
        Vector ignoredFiles = new Vector();
        // This contains Strings: lines to be added for the ignored
        // files
        Vector additionalGeneratedFileStrings = new Vector();
        // Just the file names of the above strings
        Vector additionalGeneratedFileNames = new Vector();

        int i = 0;

        System.err.println("WinCEGammaPlatform platform-specific arguments:");
        for (int j = 0; j < args.length; j++) {
            System.err.print(args[j] + " ");
        }
        System.err.println();

        try {
            while (i < args.length) {
                if (args[i].equals("-sourceBase")) {
                    sourceBase = args[i+1];
                    if (sourceBase.charAt(0) == '-') {
                        System.err.println("** Error: empty -sourceBase");
                        System.err.println(
                            "   (Did you set the HotSpotWorkSpace environment variable?)"
                        );
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-vcpFileName")) {
                    vcpFileName = args[i+1];
                    if (vcpFileName.charAt(0) == '-') {
                        System.err.println("** Error: empty -vcpFileName");
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-envVar")) {
                    envVar = args[i+1];
                    if (envVar.charAt(0) == '-') {
                        System.err.println("** Error: empty -envVar");
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-exeLoc")) {
                    exeLoc = args[i+1];
                    if (exeLoc.charAt(0) == '-') {
                        System.err.println("** Error: empty -exeLoc");
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-absoluteInclude")) {
                    absoluteIncludes.add(args[i+1]);
                    if (args[i+1].charAt(0) == '-') {
                        System.err.println("** Error: empty -absoluteInclude");
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-relativeInclude")) {
                    relativeIncludes.add(args[i+1]);
                    if (args[i+1].charAt(0) == '-') {
                        System.err.println("** Error: empty -relativeInclude");
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-define")) {
                    defines.add(args[i+1]);
                    if (args[i+1].charAt(0) == '-') {
                        System.err.println("** Error: empty -define");
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-perFileLine")) {
                    addPerFileLine(perFileLines, args[i+1], args[i+2]);
                    if (args[i+1].charAt(0) == '-' ||
                        args[i+2].charAt(0) == '-') {
                        System.err.println("** Error: wrong number of args to -perFileLine");
                        usage();
                    }
                    i += 3;
                } else if (args[i].equals("-conditionalPerFileLine")) {
                    addConditionalPerFileLine(conditionalPerFileLines,
                                              args[i+1], args[i+2],
                                              args[i+3]);
                    if (((args[i+1].length() > 0) && (args[i+1].charAt(0) == '-')) ||
                        ((args[i+2].length() > 0) && (args[i+2].charAt(0) == '-')) ||
                        ((args[i+3].length() > 0) && (args[i+3].charAt(0) == '-'))) {
                        System.err.println("** Error: wrong number of args to -conditionalPerFileLine");
                        usage();
                    }
                    i += 4;
                } else if (args[i].equals("-startAt")) {
                    if (startAt != null) {
                        System.err.println("** Error: multiple -startAt");
                        usage();
                    }
                    startAt = args[i+1];
                    if (args[i+1].charAt(0) == '-') {
                        System.err.println("** Error: empty -startAt");
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-ignoreFile")) {
                    ignoredFiles.add(args[i+1]);
                    if (args[i+1].charAt(0) == '-') {
                        System.err.println("** Error: empty -ignoreFile");
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-additionalFile")) {
                    additionalFiles.add(args[i+1]);
                    if (args[i+1].charAt(0) == '-') {
                        System.err.println("** Error: empty -additionalFile");
                        usage();
                    }
                    i += 2;
                } else if (args[i].equals("-additionalGeneratedFile")) {
                    String var = args[i+1];
                    String dir = args[i+2];
                    String fileName = args[i+3];
                    additionalGeneratedFileStrings.add("$(" + var + ")\\" +
                                                       dir + "\\" + fileName);
                    additionalGeneratedFileNames.add(fileName);
                    if (args[i+1].charAt(0) == '-' ||
                        args[i+2].charAt(0) == '-' ||
                        args[i+3].charAt(0) == '-') {
                        System.err.println("** Error: wrong number of args to -additionalGeneratedFile");
                        usage();
                    }
                    i += 4;
                } else {
                    System.err.println("Illegal argument: " + args[i]);
                    usage();
                }
            }
        }
        catch (ArrayIndexOutOfBoundsException e) {
            usage();
        }

        if ((sourceBase == null) ||
            (vcpFileName == null) ||
            (envVar == null) ||
            (exeLoc == null)) {
            usage();
        }
        
        // Compare contents of allFiles of previousDB and includeDB.
        // If these haven't changed, then skip writing the .vcp file.
        if (databaseAllFilesEqual(previousDB, currentDB) &&
            new File(vcpFileName).exists()) {
            System.out.println(
                "    Databases unchanged; skipping overwrite of .vcp file."
            );
            return;
        }

        String vcpName = getVcpName(vcpFileName);

        System.out.print("    Reading directory...");
        System.out.flush();
        DirectoryTree tree = new DirectoryTree();
        tree.addSubdirToIgnore("Codemgr_wsdata");
        tree.addSubdirToIgnore("deleted_files");
        tree.addSubdirToIgnore("SCCS");
        tree.setVerbose(true);
        if (startAt != null)
            tree.readDirectory(sourceBase + File.separator + startAt);
        else
            tree.readDirectory(sourceBase);
        int sourceBaseLen = sourceBase.length();
        // Contains Strings which are the source file names to be
        // written to the .vcp file
        Vector outputStrings = new Vector();
        Vector preferredPaths = new Vector();
        // In the case of multiple files with the same name in
        // different subdirectories, prefer the versions specified in
        // the platform file as the "os_family" and "arch" macros.
        String v;
        if ((v = currentDB.getMacroContent("os_family")) != null) {
            preferredPaths.add(v);
        }
        if ((v = currentDB.getMacroContent("arch")) != null) {
            preferredPaths.add(v);
        }
        // Also prefer "opto" over "adlc" for adlcVMDeps.hpp
        preferredPaths.add("opto");
        // Hold errors until end
        Vector filesNotFound = new Vector();
        Vector filesDuplicate = new Vector();

        System.out.println();
        System.out.print("    Looking up files in database...");
        System.out.flush();
        for (Iterator iter = currentDB.getAllFiles().iterator();
             iter.hasNext(); ) {
            FileList fl = (FileList) iter.next();
            String fileName = fl.getName();
            // Add to all files only if not ignored
            if (!findString(ignoredFiles, fileName)) {
                allFileNames.add(fileName);
                String prefixedName = envVarPrefixedFileName(fileName,
                                                             sourceBaseLen,
                                                             envVar,
                                                             tree,
                                                             preferredPaths,
                                                             filesNotFound,
                                                             filesDuplicate);
                if (prefixedName != null) {
                  outputStrings.add(prefixedName);
                }
                System.out.print(".");
                System.out.flush();
            }
        }

        for (Iterator iter = additionalFiles.iterator(); iter.hasNext(); ) {
            String fileName = (String) iter.next();
            allFileNames.add(fileName);
            String prefixedName = envVarPrefixedFileName(fileName,
                                                         sourceBaseLen,
                                                         envVar,
                                                         tree,
                                                         preferredPaths,
                                                         filesNotFound,
                                                         filesDuplicate);
            if (prefixedName != null) {
              outputStrings.add(prefixedName);
            }
        }

        if ((filesNotFound.size() != 0) ||
            (filesDuplicate.size() != 0)) {
            System.err.println("Error: some files were not found or " +
                               "appeared in multiple subdirectories of " +
                               "directory " + sourceBase + " and could not " +
                               "be resolved with the os_family and arch " +
                               "macros in the platform file.");
            if (filesNotFound.size() != 0) {
                System.err.println("Files not found:");
                for (Iterator iter = filesNotFound.iterator();
                     iter.hasNext(); ) {
                    System.err.println("  " + (String) iter.next());
                }
            }
            if (filesDuplicate.size() != 0) {
                System.err.println("Duplicate files:");
                for (Iterator iter = filesDuplicate.iterator();
                     iter.hasNext(); ) {
                    System.err.println("  " + (String) iter.next());
                }
            }
            throw new RuntimeException();
        }

        System.out.println();
        System.out.println("    Writing .vcp file...");
       // If we got this far without an error, we're safe to actually
        // write the .vcp file
        PrintWriter vcpFile = new PrintWriter(new FileWriter(vcpFileName));
        System.out.println("Writing " + vcpFileName);
        vcpFile.println("# Microsoft eMbedded Visual Tools Project File - Name=\"" + vcpName + "\" - Package Owner=<4>");
        vcpFile.println("# Microsoft eMbedded Visual Tools Generated Build File, Format Version 6.02");
        vcpFile.println("# ** DO NOT EDIT **");
        vcpFile.println("");
        vcpFile.println("# TARGTYPE \"Win32 (WCE ARM) Application\" 0x8501");
        vcpFile.println("");
        vcpFile.println("CFG=" + vcpName + " - Win32 (WCE ARM) Debug");

        vcpFile.println("!MESSAGE This is not a valid makefile. To build this project using NMAKE,");
        vcpFile.println("!MESSAGE use the Export Makefile command and run");
        vcpFile.println("!MESSAGE ");
        vcpFile.println("!MESSAGE NMAKE /f \"cldc_vm.vcn\".");
        vcpFile.println("!MESSAGE ");
        vcpFile.println("!MESSAGE You can specify a configuration when running NMAKE");
        vcpFile.println("!MESSAGE by defining the macro CFG on the command line. For example:");
        vcpFile.println("!MESSAGE ");
        vcpFile.println("!MESSAGE NMAKE /f \"cldc_vm.vcn\" CFG=\"cldc_vm - Win32 (WCE ARM) Debug\"");
        vcpFile.println("!MESSAGE ");
        vcpFile.println("!MESSAGE Possible choices for configuration are:");
        vcpFile.println("!MESSAGE ");
        vcpFile.println("!MESSAGE \"cldc_vm - Win32 (WCE ARM) Release\" (based on \"Win32 (WCE ARM) Application\")");
        vcpFile.println("!MESSAGE \"cldc_vm - Win32 (WCE ARM) Product\" (based on \"Win32 (WCE ARM) Application\")");
        vcpFile.println("!MESSAGE \"cldc_vm - Win32 (WCE ARM) Debug\" (based on \"Win32 (WCE ARM) Application\")");
        vcpFile.println("!MESSAGE ");
        vcpFile.println("");
        vcpFile.println("# Begin Project");
        vcpFile.println("# PROP AllowPerConfigDependencies 0");
        vcpFile.println("# PROP Scc_ProjName \"\"");
        vcpFile.println("# PROP Scc_LocalPath \"\"");
        vcpFile.println("# PROP ATL_Project 2");
        vcpFile.println("");
        
        vcpFile.println("!IF  \"$(CFG)\" == \"" + vcpName + " - Win32 (WCE ARM) Release\"");
        vcpFile.println("");

        vcpFile.println("# PROP BASE Use_MFC 0");
        vcpFile.println("# PROP BASE Use_Debug_Libraries 0");
        vcpFile.println("# PROP BASE Output_Dir \"ARMRel\"");
        vcpFile.println("# PROP BASE Intermediate_Dir \"ARMRel\"");
        vcpFile.println("# PROP BASE CPU_ID \"{D6518FFC-710F-11D3-99F2-00105A0DF099}\"");
        vcpFile.println("# PROP BASE Platform_ID \"{8A9A2F80-6887-11D3-842E-005004848CBA}\"");
        vcpFile.println("# PROP BASE Target_Dir \"\"");

        vcpFile.println("# PROP Use_MFC 0");
        vcpFile.println("# PROP Use_Debug_Libraries 0");
        vcpFile.println("# PROP Output_Dir \"ARMRel\"");
        vcpFile.println("# PROP Intermediate_Dir \"ARMRel\"");
        vcpFile.println("# PROP CPU_ID \"{D6518FFC-710F-11D3-99F2-00105A0DF099}\"");
        vcpFile.println("# PROP Platform_ID \"{8A9A2F80-6887-11D3-842E-005004848CBA}\"");
        vcpFile.println("# PROP Target_Dir \"\"");

        vcpFile.println("RSC=rc.exe");
        vcpFile.println("# ADD BASE RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d \"UNICODE\" /d \"_UNICODE\" /d \"NDEBUG\" /d \"$(CePlatform)\" /d \"ARM\" /d \"_ARM_\" /r");
        vcpFile.println("# ADD RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d \"UNICODE\" /d \"_UNICODE\" /d \"NDEBUG\" /d \"$(CePlatform)\" /d \"ARM\" /d \"_ARM_\" /r");
        vcpFile.println("CPP=clarm.exe");

        vcpFile.println("# ADD BASE CPP /nologo /W3 /D _WIN32_WCE=$(CEVersion) /D \"$(CePlatform)\" /D \"ARM\" /D \"_ARM_\" /D UNDER_CE=$(CEVersion) /D \"UNICODE\" /D \"_UNICODE\" /D \"NDEBUG\" /YX /Ox /Os /M$(CECrtMT) /c");
        vcpFile.print("# ADD CPP /nologo /W3 /I \".\" ");
        for (Iterator iter = absoluteIncludes.iterator(); iter.hasNext(); ) {
            String include = (String) iter.next();
            vcpFile.print(" /I \"" + include + "\"");
        }
        for (Iterator iter = relativeIncludes.iterator(); iter.hasNext(); ) {
            String include = (String) iter.next();
            vcpFile.print(" /I \"$(" + envVar + ")\\" + include + "\"");
        }
	for (Iterator iter = defines.iterator(); iter.hasNext(); ) {
	    String define = (String) iter.next();
	    vcpFile.print(" /D \"" + define + "\"");
	}
        vcpFile.println(" /Yu\"incls/_precompiled.incl\" /D _WIN32_WCE=$(CEVersion) /D \"$(CePlatform)\" /D \"ARM\" /D \"_ARM_\" /D UNDER_CE=$(CEVersion) /D \"UNICODE\" /D \"_UNICODE\" /D \"NDEBUG\" /YX /Ox /Os /M$(CECrtMT) /c");

        vcpFile.println("MTL=midl.exe");
        vcpFile.println("# ADD BASE MTL /nologo /D \"NDEBUG\" /mktyplib203 /o \"NUL\" /win32");
        vcpFile.println("# ADD MTL /nologo /D \"NDEBUG\" /mktyplib203 /o \"NUL\" /win32");
        vcpFile.println("BSC32=bscmake.exe");
        vcpFile.println("# ADD BASE BSC32 /nologo");
        vcpFile.println("# ADD BSC32 /nologo");
        vcpFile.println("LINK32=link.exe");
        vcpFile.println("# ADD BASE LINK32 commctrl.lib coredll.lib aygshell.lib gx.lib winsock.lib /nologo /base:\"0x00010000\" /stack:0x10000,0x1000 /entry:\"WinMainCRTStartup\" /nodefaultlib:\"$(CENoDefaultLib)\" /subsystem:$(CESubsystem) /align:\"4096\" /MACHINE:ARM");
        vcpFile.println("# ADD LINK32 commctrl.lib coredll.lib aygshell.lib gx.lib winsock.lib /nologo /base:\"0x00010000\" /stack:0x10000,0x1000 /entry:\"WinMainCRTStartup\" /nodefaultlib:\"$(CENoDefaultLib)\" /subsystem:$(CESubsystem) /align:\"4096\" /MACHINE:ARM");

        vcpFile.println("");

        vcpFile.println("!ELSEIF  \"$(CFG)\" == \"" + vcpName + " - Win32 (WCE ARM) Product\"");
        vcpFile.println("");

        vcpFile.println("# PROP BASE Use_MFC 0");
        vcpFile.println("# PROP BASE Use_Debug_Libraries 0");
        vcpFile.println("# PROP BASE Output_Dir \"ARMPrd\"");
        vcpFile.println("# PROP BASE Intermediate_Dir \"ARMPrd\"");
        vcpFile.println("# PROP BASE CPU_ID \"{D6518FFC-710F-11D3-99F2-00105A0DF099}\"");
        vcpFile.println("# PROP BASE Platform_ID \"{8A9A2F80-6887-11D3-842E-005004848CBA}\"");
        vcpFile.println("# PROP BASE Target_Dir \"\"");

        vcpFile.println("# PROP Use_MFC 0");
        vcpFile.println("# PROP Use_Debug_Libraries 0");
        vcpFile.println("# PROP Output_Dir \"ARMPrd\"");
        vcpFile.println("# PROP Intermediate_Dir \"ARMPrd\"");
        vcpFile.println("# PROP CPU_ID \"{D6518FFC-710F-11D3-99F2-00105A0DF099}\"");
        vcpFile.println("# PROP Platform_ID \"{8A9A2F80-6887-11D3-842E-005004848CBA}\"");
        vcpFile.println("# PROP Target_Dir \"\"");

        vcpFile.println("RSC=rc.exe");
        vcpFile.println("# ADD BASE RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d \"UNICODE\" /d \"_UNICODE\" /d \"NDEBUG\" /d \"$(CePlatform)\" /d \"ARM\" /d \"_ARM_\" /r");
        vcpFile.println("# ADD RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d \"UNICODE\" /d \"_UNICODE\" /d \"NDEBUG\" /d \"$(CePlatform)\" /d \"ARM\" /d \"_ARM_\" /r");
        vcpFile.println("CPP=clarm.exe");

        vcpFile.println("# ADD BASE CPP /nologo /W3 /D _WIN32_WCE=$(CEVersion) /D \"$(CePlatform)\" /D \"ARM\" /D \"_ARM_\" /D UNDER_CE=$(CEVersion) /D \"UNICODE\" /D \"_UNICODE\" /D \"NDEBUG\" /YX /Ox /Os /M$(CECrtMT) /c");
        vcpFile.print("# ADD CPP /nologo /W3 /D \"PRODUCT\" /I \".\" ");
        for (Iterator iter = absoluteIncludes.iterator(); iter.hasNext(); ) {
            String include = (String) iter.next();
            vcpFile.print(" /I \"" + include + "\"");
        }
        for (Iterator iter = relativeIncludes.iterator(); iter.hasNext(); ) {
            String include = (String) iter.next();
            vcpFile.print(" /I \"$(" + envVar + ")\\" + include + "\"");
        }
	for (Iterator iter = defines.iterator(); iter.hasNext(); ) {
	    String define = (String) iter.next();
	    vcpFile.print(" /D \"" + define + "\"");
	}
        vcpFile.println(" /Yu\"incls/_precompiled.incl\" /D _WIN32_WCE=$(CEVersion) /D \"$(CePlatform)\" /D \"ARM\" /D \"_ARM_\" /D UNDER_CE=$(CEVersion) /D \"UNICODE\" /D \"_UNICODE\" /D \"NDEBUG\" /YX /Ox /Os /M$(CECrtMT) /c");

        vcpFile.println("MTL=midl.exe");
        vcpFile.println("# ADD BASE MTL /nologo /D \"NDEBUG\" /mktyplib203 /o \"NUL\" /win32");
        vcpFile.println("# ADD MTL /nologo /D \"NDEBUG\" /mktyplib203 /o \"NUL\" /win32");
        vcpFile.println("BSC32=bscmake.exe");
        vcpFile.println("# ADD BASE BSC32 /nologo");
        vcpFile.println("# ADD BSC32 /nologo");
        vcpFile.println("LINK32=link.exe");
        vcpFile.println("# ADD BASE LINK32 commctrl.lib coredll.lib aygshell.lib gx.lib winsock.lib /nologo /base:\"0x00010000\" /stack:0x10000,0x1000 /entry:\"WinMainCRTStartup\" /nodefaultlib:\"$(CENoDefaultLib)\" /subsystem:$(CESubsystem) /align:\"4096\" /MACHINE:ARM");
        vcpFile.println("# ADD LINK32 commctrl.lib coredll.lib aygshell.lib gx.lib winsock.lib /nologo /base:\"0x00010000\" /stack:0x10000,0x1000 /entry:\"WinMainCRTStartup\" /nodefaultlib:\"$(CENoDefaultLib)\" /subsystem:$(CESubsystem) /align:\"4096\" /MACHINE:ARM");

        vcpFile.println("");

        vcpFile.println("!ELSEIF  \"$(CFG)\" == \"" + vcpName + " - Win32 (WCE ARM) Debug\"");

        vcpFile.println("");
        vcpFile.println("# PROP BASE Use_MFC 0");
        vcpFile.println("# PROP BASE Use_Debug_Libraries 1");
        vcpFile.println("# PROP BASE Output_Dir \"ARMDbg\"");
        vcpFile.println("# PROP BASE Intermediate_Dir \"ARMDbg\"");
        vcpFile.println("# PROP BASE CPU_ID \"{D6518FFC-710F-11D3-99F2-00105A0DF099}\"");
        vcpFile.println("# PROP BASE Platform_ID \"{8A9A2F80-6887-11D3-842E-005004848CBA}\"");
        vcpFile.println("# PROP BASE Target_Dir \"\"");

        vcpFile.println("# PROP Use_MFC 0");
        vcpFile.println("# PROP Use_Debug_Libraries 1");
        vcpFile.println("# PROP Output_Dir \"ARMDbg\"");
        vcpFile.println("# PROP Intermediate_Dir \"ARMDbg\"");
        vcpFile.println("# PROP CPU_ID \"{D6518FFC-710F-11D3-99F2-00105A0DF099}\"");
        vcpFile.println("# PROP Platform_ID \"{8A9A2F80-6887-11D3-842E-005004848CBA}\"");
        vcpFile.println("# PROP Target_Dir \"\"");

        vcpFile.println("RSC=rc.exe");
        vcpFile.println("# ADD BASE RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d \"UNICODE\" /d \"_UNICODE\" /d \"DEBUG\" /d \"$(CePlatform)\" /d \"ARM\" /d \"_ARM_\" /r");
        vcpFile.println("# ADD RSC /l 0x409 /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d \"UNICODE\" /d \"_UNICODE\" /d \"DEBUG\" /d \"$(CePlatform)\" /d \"ARM\" /d \"_ARM_\" /r");
        vcpFile.println("CPP=clarm.exe");
        vcpFile.println("# ADD BASE CPP /nologo /W3 /Zi /Od /D \"DEBUG\" /D \"ARM\" /D \"_ARM_\" /D UNDER_CE=$(CEVersion) /D _WIN32_WCE=$(CEVersion) /D \"$(CePlatform)\" /D \"UNICODE\" /D \"_UNICODE\" /YX /M$(CECrtMTDebug) /c");
        vcpFile.print("# ADD CPP /nologo /W3 /I \".\" ");

        for (Iterator iter = absoluteIncludes.iterator(); iter.hasNext(); ) {
            String include = (String) iter.next();
            vcpFile.print(" /I \"" + include + "\"");
        }
        for (Iterator iter = relativeIncludes.iterator(); iter.hasNext(); ) {
            String include = (String) iter.next();
            vcpFile.print(" /I \"$(" + envVar + ")\\" + include + "\"");
        }
	for (Iterator iter = defines.iterator(); iter.hasNext(); ) {
	    String define = (String) iter.next();
	    vcpFile.print(" /D \"" + define + "\"");
	}
        vcpFile.println(" /Yu\"incls/_precompiled.incl\" /Zi /Od /D \"_DEBUG\" /D \"AZZERT\" /D \"DEBUG\" /D \"ARM\" /D \"_ARM_\" /D UNDER_CE=$(CEVersion) /D _WIN32_WCE=$(CEVersion) /D \"$(CePlatform)\" /D \"UNICODE\" /D \"_UNICODE\" /YX /M$(CECrtMTDebug) /c");

        vcpFile.println("MTL=midl.exe");
        vcpFile.println("# ADD BASE MTL /nologo /D \"_DEBUG\" /mktyplib203 /o \"NUL\" /win32");
        vcpFile.println("# ADD MTL /nologo /D \"_DEBUG\" /mktyplib203 /o \"NUL\" /win32");
        vcpFile.println("BSC32=bscmake.exe");
        vcpFile.println("# ADD BASE BSC32 /nologo");
        vcpFile.println("# ADD BSC32 /nologo");
        vcpFile.println("LINK32=link.exe");
        vcpFile.println("# ADD BASE LINK32 commctrl.lib coredll.lib  aygshell.lib gx.lib winsock.lib /nologo /base:\"0x00010000\" /stack:0x10000,0x1000 /entry:\"WinMainCRTStartup\" /debug /nodefaultlib:\"$(CENoDefaultLib)\" /subsystem:$(CESubsystem) /align:\"4096\" /MACHINE:ARM");
        vcpFile.println("# ADD LINK32 commctrl.lib coredll.lib  aygshell.lib gx.lib winsock.lib /nologo /base:\"0x00010000\" /stack:0x10000,0x1000 /entry:\"WinMainCRTStartup\" /debug /nodefaultlib:\"$(CENoDefaultLib)\" /subsystem:$(CESubsystem) /align:\"4096\" /MACHINE:ARM");

        vcpFile.println("");

        vcpFile.println("!ENDIF ");
        vcpFile.println("");
        vcpFile.println("# Begin Target");
        vcpFile.println("");
        vcpFile.println("# Name \"" + vcpName + " - Win32 (WCE ARM) Release\"");
        vcpFile.println("# Name \"" + vcpName + " - Win32 (WCE ARM) Product\"");
        vcpFile.println("# Name \"" + vcpName + " - Win32 (WCE ARM) Debug\"");
        vcpFile.println("# Begin Group \"Source Files\"");
        vcpFile.println("");
        vcpFile.println("# PROP Default_Filter \"cpp;c;cxx;rc;def;r;odl;idl;hpj;bat\"");

        outputSourceFiles(outputStrings, allFileNames,
                          perFileLines, conditionalPerFileLines,
                          vcpFile, vcpName);
        
        outputSourceFiles(additionalGeneratedFileStrings,
                          additionalGeneratedFileNames,
                          perFileLines, conditionalPerFileLines,
                          vcpFile, vcpName);

        outputInterpreterDirectives(vcpFile, vcpName);

        vcpFile.println("# End Group");
        vcpFile.println("# Begin Group \"Header Files\"");
        vcpFile.println("");
        vcpFile.println("# PROP Default_Filter \"h;hpp;hxx;hm;inl\"");
        vcpFile.println("# End Group");
        vcpFile.println("# Begin Group \"Resource Files\"");
        vcpFile.println("");
        vcpFile.println("# PROP Default_Filter \"ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe\"");
        vcpFile.println("# End Group");
        vcpFile.println("# End Target");
        vcpFile.println("# End Project"); 

        vcpFile.close();
        System.out.println("    Done.");
    }
}
