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

public class Database {
    private MacroDefinitions macros;
    // allFiles is kept in lexicographically sorted order. See get().
    private FileList allFiles;
    // files that have implicit dependency on platform files
    // e.g. os.hpp: os_<os_family>.hpp os_<os_arch>.hpp but only
    // recorded if the platform file was seen.
    private FileList platformFiles;
    private FileList outerFiles;
    private FileList indivIncludes;
    private FileList grandInclude; // the results for the grand include file
    private long threshold;
    private int nOuterFiles;
    private int nPrecompiledFiles;
    private boolean missingOk;

    private Platform plat;
    /** These allow you to specify files not in the include database
      which are prepended and appended to the file list, allowing
      you to have well-known functions at the start and end of the
      text segment (allows us to find out in a portable fashion
      whether the current PC is in VM code or not upon a crash) */
    private String firstFile;
    private String lastFile;
    private boolean resolveVpath;
    private int sourceMergerLimit = 0;
    private String workspace;
    private String genDir;
    private Vector vpaths = null;
    private Hashtable resolvedFileNames;
    private Hashtable grandIncludeEntries = new Hashtable();
    static private String outputDir = ".";

    public Database(Platform plat, long t) {
      this.plat = plat;
      macros          = new MacroDefinitions();
      allFiles        = new FileList("allFiles", plat);
      platformFiles   = new FileList("platformFiles", plat);
      outerFiles      = new FileList("outerFiles", plat);
      indivIncludes   = new FileList("IndivIncludes", plat);
      grandInclude    = new FileList(plat.getGIFileTemplate().nameOfList(), plat);

      threshold = t;
      nOuterFiles = 0;
      nPrecompiledFiles = 0;
      missingOk = false;
      firstFile = null;
      lastFile = null;
      resolveVpath = false;
    };

    public FileList getAllFiles() {
      return allFiles;
    }
    public FileList getOuterFiles() {
        return outerFiles;
    }

    public String getMacroContent(String name) {
      return macros.getMacroContent(name);
    }

    public void canBeMissing() {
      missingOk = true;
    }

    public boolean hfileIsInGrandInclude(FileList hfile, FileList cfile) {
      return ((hfile.getCount() >= threshold) && (cfile.getUseGrandInclude()));
    }

    /** These allow you to specify files not in the include database
      which are prepended and appended to the file list, allowing
      you to have well-known functions at the start and end of the
      text segment (allows us to find out in a portable fashion
      whether the current PC is in VM code or not upon a crash) */
    public void setFirstFile(String fileName) {
      firstFile = fileName;
    }

    public void setLastFile(String fileName) {
      lastFile = fileName;
    }

    public void setWorkspace(String fileName) {
      workspace = fileName;
    }
    public String getWorkspace() {
      return workspace;
    }

    public void setGenDir(String path) {
      genDir = path;
    }
    public String getGenDir() {
      return genDir;
    }

    public void setOutputDir(String path) {
        outputDir = path;
    }

    private void initVpath() {
      if (vpaths != null) {
          return;
      }
      resolvedFileNames = new Hashtable();
      vpaths = new Vector();
      if (genDir != null) {
        addVpath(genDir);
        addVpath(genDir + "/incls");
      }
      String cpu         = macros.getMacroContent("arch");
      String cpu_variant = macros.getMacroContent("cpu_variant");

      addVpath(workspace + "/src/vm/cpu/arm");
      addVpath(workspace + "/src/vm/cpu/arm/jazelle");
      addVpath(workspace + "/src/vm/cpu/c");
      addVpath(workspace + "/src/vm/cpu/i386");
      addVpath(workspace + "/src/vm/cpu/sh");
      addVpath(workspace + "/src/vm/cpu/thumb");
      addVpath(workspace + "/src/vm/cpu/thumb2");
      if ((cpu_variant != null) && (!cpu_variant.equals(""))) {
          addVpath(workspace + "/src/vm/cpu/" + cpu + "/" + cpu_variant);
      }

      addVpath(workspace + "/src/vm/os/"+ macros.getMacroContent("os_family"));
      addVpath(workspace + "/src/vm/os/utilities");
      addVpath(workspace + "/src/vm/share/compiler");
      addVpath(workspace + "/src/vm/share/float");
      addVpath(workspace + "/src/vm/share/handles");
      addVpath(workspace + "/src/vm/share/interpreter");
      addVpath(workspace + "/src/vm/share/memory");
      addVpath(workspace + "/src/vm/share/natives");
      addVpath(workspace + "/src/vm/share/ROM");
      addVpath(workspace + "/src/vm/share/reflection");
      addVpath(workspace + "/src/vm/share/runtime");
      addVpath(workspace + "/src/vm/share/utilities");
      addVpath(workspace + "/src/vm/share/verifier");
      addVpath(workspace + "/src/vm/share/debugger");
      addVpath(workspace + "/src/vm/share/isolate");
      addVpath(workspace + "/src/vm/share/dynupdate");
      addVpath(workspace + "/src/vm/share/memoryprofiler");
    }

    private void addVpath(String path) {
      // System.out.println("adding = " + path);
      File file = new File(path);
      if (file.isDirectory()) {
        vpaths.addElement(path);
      }
    }

    public void setResolveVpath(boolean r) {
      resolveVpath = r;
    }

    /**
     * Reads an entire includeDB file and stores its contents into
     * internal representations.
     */
    public void get(String platFileName, String dbFileName,
                    Properties globalProps)
      throws FileFormatException, IOException, FileNotFoundException {
      macros.readFrom(platFileName, missingOk);

      BufferedReader reader = null;
      try {
          reader = new BufferedReader(new FileReader(dbFileName));
      } catch (FileNotFoundException e) {
          if (missingOk) {
            return;
          } else {
            throw(e);
          }
      }
      System.out.println("\treading database: " + dbFileName);
      String line;
      int lineNo = 0;
      Stack ifdef_stack = new Stack();
      do {
          line = reader.readLine();
          lineNo++;
          if (line != null) {
            StreamTokenizer tokenizer =
                new StreamTokenizer(new StringReader(line));
            tokenizer.slashSlashComments(true);
            tokenizer.wordChars('_', '_');
            tokenizer.wordChars('<', '>');
            tokenizer.wordChars('#', '#');
            tokenizer.wordChars('!', '!');

            // NOTE: if we didn't have to do this line by line,
            // we could trivially recognize C-style comments as
            // well.
            // tokenizer.slashStarComments(true);
            int numTok = 0;
            int res;
            String unexpandedIncluder = null;
            String unexpandedIncludee = null;
            String thirdToken = null;
            do {
                res = tokenizer.nextToken();
                if (res != StreamTokenizer.TT_EOF) {
                  if (numTok == 0) {
                      unexpandedIncluder = tokenizer.sval;
                  } else if (numTok == 1) {
                      unexpandedIncludee = tokenizer.sval;
                  } else if (numTok == 2) {
                      thirdToken = tokenizer.sval;
                  } else {
                      throw new FileFormatException(
                          "invalid line: \"" + line +
                        "\". Error position: line " + lineNo
                      );
                  }
                  numTok++;
                }
            } while (res != StreamTokenizer.TT_EOF);

            boolean valid = true;
            if (numTok == 0) {
                valid = true;
            } else if (unexpandedIncluder != null &&
                       unexpandedIncluder.charAt(0) == '#') {
                if (equalsIgnoreCase(unexpandedIncluder, "#if")) {
                    valid = (numTok == 2);
                } else if (equalsIgnoreCase(unexpandedIncluder, "#ifeq")) {
                    valid = (numTok == 3);
                } else if (equalsIgnoreCase(unexpandedIncluder, "#endif")) {
                    valid = (numTok == 1 || numTok == 2);
                } else {
                    System.out.println("Unknown preprocessor command: " +
                                       unexpandedIncluder + " at line " +
                                       lineNo);
                    System.exit(1);
                }
            } else {
                valid = (numTok == 2);
            }

            if (!valid) {
                throw new FileFormatException("invalid line: \"" + line +
                      "\". Error position: line " + lineNo);
            }

            if (numTok != 0) { // Non-empty line
                if (equalsIgnoreCase(unexpandedIncluder, "#if")) {
                    push(ifdef_stack, getIfdefValue(globalProps,
                                                    unexpandedIncludee));
                    continue;
                }
                else if (equalsIgnoreCase(unexpandedIncluder, "#ifeq")) {
                    push(ifdef_stack, getIfeqValue(globalProps,
                                                   unexpandedIncludee,
                                                   thirdToken));
                    continue;
                }
                else if (equalsIgnoreCase(unexpandedIncluder, "#endif")) {
                    if (ifdef_stack.empty()) {
                        throw new FileFormatException(
                              "#endif without #if at line: \"" + line);
                    }
                }

                if (!ifdef_stack.empty()) {
                    if (equalsIgnoreCase(unexpandedIncluder, "#endif")) {
                        ifdef_stack.pop();
                        continue;
                    }

                    Object obj = ifdef_stack.peek();
                    Boolean b = (Boolean)obj;
                    if (b.booleanValue() == false) {
                        continue;
                    }
                }

                String includer = macros.expand(unexpandedIncluder);
                String includee = macros.expand(unexpandedIncludee);

                if (includee.equals(plat.generatePlatformDependentInclude())) {
                    generatePlatformDependentInclude(unexpandedIncluder, 
                                                     includer);
                } else {
                  FileList p = allFiles.listForFile(includer);
                  if (isOuterFile(includer)) {
                      outerFiles.addIfAbsent(p);
                  }

                  if (includee.equals(plat.noGrandInclude())) {
                      p.setUseGrandInclude(false);
                  } else {
                      FileList q = allFiles.listForFile(includee);
                      p.addIfAbsent(q);
                  }
                }
            }
          }
      } while (line != null);

      if (!ifdef_stack.empty()) {
          throw new FileFormatException("#endif not found before end of file");
      }
      reader.close();

      if (sourceMergerLimit > 1) {
          createMergedOuterFiles();
      }

      // Keep allFiles in well-known order so we can easily determine
      // whether the known files are the same
      allFiles.sortByName();

      // Add first and last files differently to prevent a mistake
      // in ordering in the include databases from breaking the
      // error reporting in the VM.
      if (firstFile != null) {
        FileList p = allFiles.listForFile(firstFile);
        allFiles.setFirstFile(p);
        outerFiles.setFirstFile(p);
      }

      if (lastFile != null) {
        FileList p = allFiles.listForFile(lastFile);
        allFiles.setLastFile(p);
        outerFiles.setLastFile(p);
      }
    }

    static boolean equalsIgnoreCase(String a, String b) {
        return (a.compareToIgnoreCase(b) == 0);
    }

    /**
     * @param boolValue must be "true" or "false"
     */
    void push(Stack ifdef_stack, String boolValue) {
        if (!ifdef_stack.empty()) {
            Object obj = ifdef_stack.peek();
            Boolean b = (Boolean)obj;
            String prop;
            if (b.booleanValue() == false) {
                boolValue = "false";
            }
        }
        ifdef_stack.push(new Boolean(boolValue));
    }

    void generatePlatformDependentInclude(String unexpandedIncluder, 
                                          String includer) throws IOException {
        MacroDefinitions localExpander = macros.copy();
        MacroDefinitions localExpander2 = macros.copy();
        localExpander.setAllMacroBodiesTo("pd");
        localExpander2.setAllMacroBodiesTo("");

        // unexpanded_includer e.g. thread_<os_arch>.hpp
        // thread_solaris_i486.hpp -> _thread_pd.hpp.incl

        FileName pdName =
                      plat.getInclFileTemplate().copyStem(
                        localExpander.expand(unexpandedIncluder)
                      );

        // derive generic name from platform specific name
        // e.g. os_<arch_os>.hpp => os.hpp

        String newIncluder =
            localExpander2.expand(unexpandedIncluder);

        FileList p = allFiles.listForFile(includer);
        p.setPlatformDependentInclude(pdName.dirPreStemSuff());

        // Add an implicit dependency on platform
        // specific file for the generic file

        p = platformFiles.listForFile(newIncluder);

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        OutputStreamWriter writer = new OutputStreamWriter(baos);
        PrintWriter pdFile = new PrintWriter(writer);

        String incname = tryGetFullPath(includer);
        incname = plat.translateFileName(incname);
        pdFile.println("#include \"" + incname + "\"");
        pdFile.flush();
        updateFile(pdName.dirPreStemSuff(), baos);
        pdFile.close();

        // Add the platform specific file to the list
        // for this generic file.

        FileList q = allFiles.listForFile(includer);
        p.addIfAbsent(q);
    }

    static String getIfdefValue(Properties globalProps, String token) {
        boolean negate = false;
        if (token.startsWith("!")) {
            token = token.substring(1);
            negate = true;
        }
        String result = globalProps.getProperty(token, "false");
        if (negate) {
            if (result.equals("false")) {
                result = "true";
            } else {
                result = "false";
            }
        }
        return result;
    }

    String getIfeqValue(Properties globalProps, String name, String value) {
        String result = macros.getMacroContent(name);
        if (result == null) {
            result = globalProps.getProperty(name, null);
        }

        if (value.equals(result)) {
            return "true";
        } else {
            return "false";
        }
    }

    /**
     * We need to exclude some files from the merged source files. See
     * in-line comments for reasons.
     */
    boolean needToExcludeFromMerge(String file) {
        if (file.equals("ROMImage.cpp")) {
            // ROM image is not part of VM library.
            return true;
        }
        if (file.equals("NativesTable.cpp")) {
            // Natives Table is not part of VM library.
            return true;
        }
        if (file.equals("BSDSocket.cpp")) {
            // BSDSocket is not part of VM library.
            return true;
        }
        if (file.equals("PCSLSocket.cpp")) {
            // PCSLSocket is not part of VM library.
            return true;
        }
        if (file.equals("ReflectNatives.cpp")) {
            // ReflectNatives is not part of VM library.
            return true;
        }

        if (file.equals("HotRoutines0.cpp") ||
            file.equals("HotRoutines1.cpp") ) {
            // These could be placed in fast memory on low-end
            // ARM7TDMI devices, so don't merge them with other
            // files.
            return true;
        }
        if (file.equals("jvmspi.cpp")) {
            // SPI implementation is not part of VM library
            return true;
        }
        if (file.toLowerCase().startsWith("main_")) {
            // Main function is not part of VM library.
            return true;
        }
        if (file.toLowerCase().startsWith("ossocket_")) {
            // socket code is not part of VM library.
            return true;
        }
        if (file.toLowerCase().startsWith("floatsupport_")) {
            // On ARM we should build FloatSupport_arm.cpp in ARM
            // mode. If compiled in THUMB mode functions like jvm_fadd()
            // will be compiled into library calls instead of VFP instructions.
            return true;
        }
        if (file.toLowerCase().endsWith("uncommon.cpp")) {
            // This file is rarely used by optimized MIDP, so let's
            // leave in a separate .o file, which gcc could eliminate from
            // final executable image.
            return true;
        }
        if (file.toLowerCase().startsWith("os") && !file.equals("OS.cpp")) {
            // IMPORTANT: this has to be here for Linux and ADS:
            // -> leads to compilation problems (FIELD_OFFSET) if merged

            // IMPORTANT: this has to be here for GBA:
            // contains interrupt handler that HAVE to be in ARM, not Thumb
            return true;
        }
        return false;
    }

    /**
     * Merge multiple .cpp files into a single .cpp file to speed
     * up GCC compilation. This is done by creating a new 'outer' source
     * file that depends on several of the original outer source files.
     * E.g., if we merge Foo1.cpp, Foo2.cpp Foo3.cpp into _MergedSrc001.cpp,
     * we'd have the following files:
     *
     * _MergedSrc001.cpp:
     *   #include "incls/_precompiled.incl"
     *   #include "incls/__MergedSrc001.cpp.incl"
     *
     * incls/__MergedSrc001.cpp.incl:
     *   #include "/path/to/Foo1.cpp"
     *   #include "/path/to/Foo2.cpp"
     *   #include "/path/to/Foo3.cpp"
     *
     * The Makefile is directed to build _MergedSrc001.o, which will include
     * the contents of Foo1.cpp, Foo2.cpp and Foo3.cpp
     *
     * This speeds up GCC compilation time by 2x ~ 3x.
     */
    void createMergedOuterFiles() throws IOException {
        System.out.println("\tmerging source files, limit = " +
                           sourceMergerLimit);
        int nNewFiles = 1;
        int nMerged = 0;
        FileList oldOuterFiles = outerFiles;
        outerFiles = new FileList("outerFiles", plat);

        FileList newOuter = null;
        for (int i=0; i<oldOuterFiles.size(); i++) {
            FileList oldOuter = (FileList)oldOuterFiles.elementAt(i);

            if (needToExcludeFromMerge(oldOuter.getName())) {
                System.out.println("\t\texcluded from merging: " +
                                   oldOuter.getName());
                outerFiles.addIfAbsent(oldOuter);
            } else {
                if (newOuter == null) {
                    String name= "_MergedSrc"+ numToString(nNewFiles)+ ".cpp";
                    newOuter = new FileList(name, plat);
                    outerFiles.addIfAbsent(newOuter);
                    nNewFiles++;

                    ByteArrayOutputStream baos = new ByteArrayOutputStream();
                    OutputStreamWriter writer = new OutputStreamWriter(baos);
                    PrintWriter pw = new PrintWriter(writer);
                    pw.println("#include \"incls/_precompiled.incl\"");
                    pw.println("#include \"incls/_" + name + ".incl\"");
                    pw.flush();
                    updateFile(name, baos);
                    pw.close();
                }

                // Create blank incl file for the oldOuter
                String blankFile = "incls/_" + oldOuter.getName() + ".incl";
                ByteArrayOutputStream baos = new ByteArrayOutputStream();
                updateFile(blankFile, baos);
                baos.close();

                newOuter.addIfAbsent(oldOuter);
                nMerged ++;
                if (nMerged >= sourceMergerLimit) {
                    nMerged = 0;
                    newOuter = null;
                }
            }
        }
    }

    String numToString(int n) {
        String s = Integer.toString(n);
        while (s.length() < 3) {
            s = "0" + s;
        }
        return s;
    }
    public void compute() {
      // build both indiv and grand results
      for (Iterator iter = outerFiles.iterator(); iter.hasNext(); ) {
          indivIncludes.add(((FileList) iter.next()).doCFile());
          ++nOuterFiles;
      }

      if (!plat.haveGrandInclude())
          return; // nothing in grand include

      // count how many times each include is included & add em to grand
      for (Iterator iter = indivIncludes.iterator(); iter.hasNext(); ) {
          FileList indivInclude = (FileList) iter.next();
          if (!indivInclude.getUseGrandInclude()) {
            continue; // do not bump count if my files cannot be
                    // in grand include
          }
          indivInclude.doFiles(grandInclude); // put em on
                                    // grand_include list
          for (Iterator incListIter = indivInclude.iterator();
             incListIter.hasNext(); ) {
            ((FileList) incListIter.next()).incrementCount();
          }
      }
    }

    // Consider whether it is necessary in Java 
    public void verify() {
      for (Iterator iter = indivIncludes.iterator(); iter.hasNext(); ) {
          if (iter.next() == null) {
            plat.abort();
          }
      }
    }

    public void put() throws IOException {
      writeIndividualIncludes();
      writeGrandInclude();
      writeCompleteInclude();
      writeGrandUnixMakefile();
      writeDox();
    }

    // Write the include file for each individual .cpp file. E.g.,
    // create _BinaryAssembler_arm.cpp.incl for _BinaryAssembler_arm.cpp
    private void writeIndividualIncludes() throws IOException {
      System.out.print("\twriting individual include files ...");
        int count = 0;
      for (Iterator iter = indivIncludes.iterator(); iter.hasNext(); ) {
          FileList list = (FileList) iter.next();
          //System.out.println("\tcreating " + list.getName());
          list.putInclFile(this);
            ++ count;
      }
        System.out.println(" done (" + count + " files.)");
    }

    // Write the header file that contains all of the VM's
    // .hpp files. This file can be used for compiling C++ files
    // that are not part of the VM code (e.g.,
    // src/tests/natives/InternalNatives.cpp). This makes it possible
    // to link extra C++ files into the VM without editing includeDB
    private void writeCompleteInclude() throws IOException {
        Hashtable written = new Hashtable();
        String filename = outputDir + "/incls/_CompleteInclude.incl";
        PrintWriter inclFile = new PrintWriter(new FileWriter(filename));
        String filename2 = outputDir + "/incls/_CompleteInclude2.incl";
        PrintWriter inclFile2 = new PrintWriter(new FileWriter(filename2));

        inclFile2.println("/* This file contains all files that are in */");
        inclFile2.println("/* _CompleteInclude.incl but not in */");
        inclFile2.println("/* _precompiled.incl */");

        for (Iterator iter = indivIncludes.iterator(); iter.hasNext(); ) {
            FileList list = (FileList) iter.next();

            for (Iterator files = list.iterator(); files.hasNext(); ) {
                FileList file = (FileList) files.next();
		String name = file.getName();
		if (!name.endsWith(".hpp") && !name.endsWith(".h")) {
		    continue;
		}
                String incname = plat.getInclFileTemplate().getInvDir() +
                    tryGetFullPath(name);
                incname = plat.translateFileName(incname);
                if (written.get(incname) == null) {
                    written.put(incname, incname);
                    inclFile.println("#include \"" + incname + "\"");

                    if (grandIncludeEntries.get(incname) == null) {
                        inclFile2.println("#include \"" + incname + "\"");
                    }
                }
            }
        }
        inclFile.close();
        inclFile2.close();
    }

    // Write a file for controlling the Doxygen documentation
    // processor. See build/share/dox.make.
    private void writeDox() throws IOException {
        Hashtable written = new Hashtable();
        String filename = outputDir + "/Dox.tail";
        PrintWriter doxFile = new PrintWriter(new FileWriter(filename));

        doxFile.println("# Dox.tail -- this file is auto-generated.");
        doxFile.println("#             do not edit");
        doxFile.println("#");
        doxFile.print("INPUT =");
        for (Iterator iter = indivIncludes.iterator(); iter.hasNext(); ) {
            FileList list = (FileList) iter.next();

            for (Iterator files = list.iterator(); files.hasNext(); ) {
                FileList file = (FileList) files.next();
		String name = file.getName();
		if (!name.endsWith(".hpp") && !name.endsWith(".h")) {
		    continue;
		}
                String incname = plat.getInclFileTemplate().getInvDir() +
                    tryGetFullPath(name);
                incname = plat.translateFileName(incname);
                if (written.get(incname) == null) {
                    written.put(incname, incname);
                    //if (incname.indexOf("memory") >= 0) {
                        doxFile.print(" " + incname);
                    //}
                }
            }
        }
        doxFile.println("");
        doxFile.close();
    }

    // Write the _precompiled.incl file
    private void writeGrandInclude() throws IOException {
        System.out.println("\twriting grand include file\n");
        String filename = outputDir + "/" +
            plat.getGIFileTemplate().dirPreStemSuff();
        PrintWriter inclFile = new PrintWriter(new FileWriter(filename));

        // This file is always included first
        inclFile.println("#include \"jvmconfig.h\"");

        if (plat.haveGrandInclude()) {
            plat.writeGIPragma(inclFile);
            for (Iterator iter = grandInclude.iterator(); iter.hasNext(); ) {
                FileList list = (FileList) iter.next();
                if (list.getCount() >= threshold) {
                    String incname = tryGetFullPath(list.getName());
                    incname = plat.translateFileName(incname);
                    inclFile.println("#include \"" +
                                     plat.getGIFileTemplate().getInvDir() +
                                     incname +
                                     "\"");
                    nPrecompiledFiles += 1;
                    grandIncludeEntries.put(incname, incname);
                }
            }
        }
        inclFile.println();
        inclFile.close();
    }

    private void writeGrandUnixMakefile() throws IOException {
        if (resolveVpath) {
            initVpath();
        }

        if (!plat.writeDeps()) {
            return;
        }

      System.out.println("\twriting dependencies file\n");
      String fileName =
          outputDir + "/" + plat.getGDFileTemplate().dirPreStemSuff();
      PrintWriter gd = new PrintWriter(new FileWriter(fileName));

      gd.println("# generated by makeDeps");
      gd.println();

      {
          // write Obj_Files = ...
          String prefix = "Obj_Files = \\\n\t";
          outerFiles.sortByName();
          for (Iterator iter = outerFiles.iterator(); iter.hasNext(); ) {
              FileList anOuterFile = (FileList) iter.next();

              gd.print(prefix);
              String stemName = removeSuffixFrom(anOuterFile.getName());
              gd.print(stemName + plat.objFileSuffix());
              prefix = " \\\n\t";
          }
          gd.println();
          gd.println();
      }

      if (plat.haveGrandInclude()) {
          String prefix = "Precompiled_Headers = \\\n\t";
          for (Iterator iter = grandInclude.iterator(); iter.hasNext(); ) {
              FileList list = (FileList) iter.next();
              if (list.getCount() >= threshold) {
                  gd.print(prefix);
                  gd.print(tryGetFullPath(list.getName()));
                  prefix = " \\\n\t";
              }
          }
          gd.println();
          gd.println();
      }

      {
          // write each dependency

          for (Iterator iter = indivIncludes.iterator(); iter.hasNext(); ) {

            FileList anII = (FileList) iter.next();

            String stemName = removeSuffixFrom(anII.getName());
            gd.println(stemName + plat.objFileSuffix() + ": \\");
            gd.print("$(GEN_DIR)/jvmconfig.h ");

            printDependentOn(gd, anII.getName());

            for (Iterator iiIter = anII.iterator(); iiIter.hasNext(); ) {
                FileList hfile = (FileList) iiIter.next();
                if (!hfileIsInGrandInclude(hfile, anII)) {
                  printDependentOn(gd, hfile.getName());
                }
                if (platformFiles.hasListForFile(hfile.getName())) {
                  FileList p =
                      platformFiles.listForFile(hfile.getName());;
                  for (Iterator hiIter = p.iterator();
                       hiIter.hasNext(); ) {
                      FileList hi2 = (FileList) hiIter.next();
                      if (!hfileIsInGrandInclude(hi2, p)) {
                        printDependentOn(gd, hi2.getName());
                      }
                  }
                }
            }
            if (plat.haveGrandInclude()) {
                gd.println(" $(Precompiled_Headers)");
            }
            gd.println();
            gd.println();
          }
      }

      gd.close();
    }

    public void putDiffs(Database previous) throws IOException {
      System.out.println("\tupdating output files\n");

      if (!indivIncludes.compareLists(previous.indivIncludes)
           || !grandInclude.compareLists(previous.grandInclude)) {
          System.out.println("The order of .c or .s has changed, or " +
                         "the grand include file has changed.");
          put();
          return;
      }

      Iterator curIter = indivIncludes.iterator();
      Iterator prevIter = previous.indivIncludes.iterator();

      try {
          while (curIter.hasNext()) {
            FileList newCFileList = (FileList) curIter.next();
            FileList prevCFileList = (FileList) prevIter.next();
            if (!newCFileList.compareLists(prevCFileList)) {
                System.out.println("\tupdating " + newCFileList.getName());
                newCFileList.putInclFile(this);
            }
          }
      }
      catch (Exception e) {
          throw new InternalError("assertion failure: cur and prev " +
                            "database lists changed unexpectedly.");
      }

      writeGrandUnixMakefile();
    }

    private void printDependentOn(PrintWriter gd, String name) {
        gd.print(" ");
        gd.print(tryGetFullPath(plat.dependentPrefix(), name));
    }

    public String tryGetFullPath(String filename) {
        return tryGetFullPath("", filename);
    }

    private String tryGetFullPath(String prefix, String filename) {
        if (resolveVpath) {
            String file = getFullPath(prefix, filename);
            if (file != null) {
                return file;
            }
        }

        return prefix + filename;
    }

    public String getFullPath(String filename) {
        return getFullPath("", filename);
    }

    public String getFullPath(String prefix, String filename) {
        initVpath();
        String cached = (String)resolvedFileNames.get(filename);
        if (cached != null) {
            return cached;
        }

        String found = null;
        for (int i=0; i<vpaths.size(); i++) {
            String vpath = (String)vpaths.elementAt(i);
            File file = new File(vpath + "/" + filename);
            if (file.isFile()) {
                found = vpath + "/" + filename;
                break;
            }
        }
        if (found == null) {
            if (filename.equals("NativesTable.cpp") ||
                filename.equals("ROMImage.cpp") ||
                filename.startsWith("_MergedSrc")) {
                found = "../generated/" + filename;
            }
        }

        if (found != null) {
            resolvedFileNames.put(filename, found);
            return found;
        }
        System.err.println("couldn't find in vpath: " + filename);

        return null;
    }

    /**
     * Checks whether a file is an outer file - which means a
     * makefile rule has to be generated to compile this file
     * into an object file.
     *
     * This is done by checking if the suffix of the file is one
     * of plat.outerSuffixes(), which are typically .cpp and .c
     */
    private boolean isOuterFile(String s) {
      int len = s.length();
      String[] suffixes = plat.outerSuffixes();
      for (int i = 0; i < suffixes.length; i++) {
          String suffix = suffixes[i];
          int suffLen = suffix.length();
          if ((len >= suffLen) &&
            (plat.fileNameStringEquality(s.substring(len - suffLen),
                                   suffix))) {
            return true;
          }
      }
      return false;
    }

    private String removeSuffixFrom(String s) {
      int idx = s.lastIndexOf('.');
      if (idx <= 0)
          plat.abort();
      return s.substring(0, idx);
    }
    public void setSourceMergerLimit(int size) {
        sourceMergerLimit = size;
    }

    /**
     * Write the content of the baos to the given file, but only if the
     * change has been changed. We don't rewrite the file if the content is
     * the same. This avoids excessive rebuilding.
     */
    public static void updateFile(String filename, ByteArrayOutputStream baos)
        throws IOException
    {
        byte newContent[] = baos.toByteArray();
        byte oldContent[] = readFile(filename);

        // We won't write to the file if the content has not changed.
        // This will avoid excessive rebuilding when we change
        // includeDB, etc.
        if (oldContent != null && oldContent.length == newContent.length) {
            boolean same = true;

            for (int i=0; i<oldContent.length; i++) {
                if (oldContent[i] != newContent[i]) {
                    same = false;
                    break;
                }
            }
            if (same) {
                //System.out.println(filename + " same");
                return;
            }
        }

        filename = outputDir + "/" + filename;
        FileOutputStream out = new FileOutputStream(filename);
        out.write(newContent);
        out.close();
    }

    public static byte[] readFile(String filename) throws IOException {
        try {
            ByteArrayOutputStream baos = new ByteArrayOutputStream();
            FileInputStream in = new FileInputStream(filename);
            byte buffer[] = new byte[1024];
            int n;
            while ((n = in.read(buffer)) > 0) {
                baos.write(buffer, 0, n);
            }
            byte result[] = baos.toByteArray();
            baos.close();
            in.close();
            return result;
        } catch (IOException e) {
            return null;
        }
    }
}
