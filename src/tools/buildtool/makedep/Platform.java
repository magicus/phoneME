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

/** Defines what must be specified for each platform. This class must
    have a no-arg constructor. */

import java.io.*;

public abstract class Platform {
    // The next three must be instantiated in subclasses' constructors

    /** An incl file is produced per .c file and contains all the
      includes it needs */
    protected FileName inclFileTemplate;

    /** A GI (grand-include) file has any file used more than N times
      for precompiled headers */
    protected FileName giFileTemplate;

    /** A GD (grand-dependencies) file that tells Unix make all the
      .o's needed for linking and the include dependencies */
    protected FileName gdFileTemplate;

    // Accessors
    public FileName getInclFileTemplate() {
      return inclFileTemplate;
    }

    public FileName getGIFileTemplate() {
      return giFileTemplate;
    }

    public FileName getGDFileTemplate() {
      return gdFileTemplate;
    }

    // an incl file is the file included by each.c file that includes
    // all needed header files

    public void setupFileTemplates() {
        String inclsDir = "incls" + fileSeparator();
        inclFileTemplate = new FileName(this,
          inclsDir, "_", "", ".incl", "", "");
        giFileTemplate = new FileName(this,
              inclsDir, "",  "_precompiled", ".incl", "", "");
        gdFileTemplate = new FileName(this,
              "", "",  "Dependencies",         "",      "", "");
    }

    public abstract String[] outerSuffixes();
    public abstract String fileSeparator();

    /** empty file name -> no grand include file */
    public boolean haveGrandInclude() {
      return usePrecompiledHeader && 
          (giFileTemplate.nameOfList().length() > 0);
    }

    public boolean writeDeps() {
      return (gdFileTemplate.nameOfList().length() > 0);
    }

    /** <p> A gi file is the grand-include file. It includes in one
      file any file that is included more than a certain number of
      times. </p>

      <p> It is used for precompiled header files. </p>

      <p> It has a source name, that is the file that this program
      generates, and a compiled name; that is the file that is
      included by other files. </p>

      <p> Some platforms have this program actually explicitly
      include the preprocessed gi file-- see includeGIInEachIncl().
      </p>

      <p> Also, some platforms need a pragma in the GI file. </p> */
    public boolean includeGIInEachIncl() {
      return false;
    }

    /** For some platforms, e.g. Solaris, include the grand-include
      dependencies in the makefile. For others, e.g. Windows, do
      not. */
    public boolean includeGIDependencies() {
      return false;
    }

    /** Default implementation does nothing */
    public void writeGIPragma(PrintWriter out) {
    }

    /** A line with a filename and the noGrandInclude string means
      that this file cannot use the precompiled header. */
    public String noGrandInclude() {
      return "no_precompiled_headers";
    }

    /** A line with a filename and the
      generatePlatformDependentInclude means that an include file
      for the header file must be generated. Does not effect the
      dependency computation. */
    public String generatePlatformDependentInclude() {
      return "generate_platform_dependent_include";
    }

    /** Prefix and suffix strings for emitting Makefile rules */
    public abstract String objFileSuffix();
    public abstract String asmFileSuffix();
    public abstract String dependentPrefix();

    // Exit routines:

    /** Abort means an internal error */
    public void abort() {
      throw new RuntimeException("Internal error");
    }

    /** fatalError is used by clients to stop the system */
    public void fatalError(String msg) {
      System.err.println(msg);
      System.exit(1);
    }

    /** Default implementation performs case-sensitive comparison */
    public boolean fileNameStringEquality(String s1, String s2) {
      return s1.equals(s2);
    }

    public void fileNamePortabilityCheck(String name) {
      // Empty for now
    }

    public void fileNamePortabilityCheck(String name, String matchingName) {
      if (!name.equals(matchingName)) {
          fatalError("Error: file " + name + " also appears as " +
                   matchingName + ".  Case must be consistent for " +
                   "portability.");
      }
    }

    /** max is 31 on mac, so warn */
    public int fileNameLengthLimit() {
      return 40;
    }

    public int defaultGrandIncludeThreshold() {
      return 30;
    }

    /** Not very general, but this is a way to get platform-specific
        files to be written. Default implementation does nothing. */
    public void writePlatformSpecificFiles(Database previousDB,
                                 Database currentDB, String[] args)
      throws IllegalArgumentException, IOException {
    }

    /* A platform may use this to process the name of a file. E.g., Symbian
     * strips leading DOS drive names */
    public String translateFileName(String name) {
        return name;
    }

    boolean usePrecompiledHeader = true;

    public void setUsePrecompiledHeader(boolean value) {
        usePrecompiledHeader = value;
    }

    boolean getUsePrecompiledHeader() {
        return usePrecompiledHeader;
    }
}
