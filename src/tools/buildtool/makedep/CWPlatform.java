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

/**
 * Metrowerks CodeWarrior for Windows
 */
public class CWPlatform extends Platform {

    /**
     * All C++ sources include the grand include file by the name of
     * "incls/_precompiled.incl".  For VisualC++ the compiler switches
     * do the magic of using a precompiled header instead of the grand
     * include file.  For CodeWarrior we need to include the
     * precompiled header explicitly, so "incls/_precompiled.incl" is
     * a "proxy" file that contains only the #include directive for
     * the precompiled header.  Hence we point giFileTemplate to a
     * file with a different name, and save in this variable the grand
     * include name that sources expect.
     *
     * @see #setupFileTemplates
     * @see #writePlatformSpecificFiles
     */
    private FileName giMchProxyIncl; // "incls/_precompiled.incl"


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
        return "";
    }


    public String fileSeparator() {
      return "\\";
    }


    public boolean fileNameStringEquality(String s1, String s2) {
      return s1.equalsIgnoreCase(s2);
    }


    public boolean includeGIInEachIncl() {
      return false;
    }


    public boolean includeGIDependencies() {
        return false;
    }


    /**
     * @see #giMchProxyIncl
     * @see #writePlatformSpecificFiles
     */
    public void setupFileTemplates() {
        super.setupFileTemplates();

        String stem = giFileTemplate.nameOfList();

        // a funky way to clone a FileName
        giMchProxyIncl = giFileTemplate.copyStem(stem);

	// write grand include into a different file that is be used
	// to generate the precompiled header, but is not used by the
	// rest of the sources
	// IMPL_NOTE: hardcoded name, must agree with the makefile
        giFileTemplate = giFileTemplate.copyStem(stem + "_pch");
    }


    /**
     * @see #giMchProxyIncl
     * @see #setupFileTemplates
     */
    public void writePlatformSpecificFiles(Database previousDB,
                                           Database currentDB,
                                           String[] args)
        throws IllegalArgumentException, IOException
    {
	// Generate "incls/_precompiled.incl" that is just a proxy
	// include for the precompiled header.
        System.out.println("\twriting MCH proxy file");

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        OutputStreamWriter writer = new OutputStreamWriter(baos);
        PrintWriter inclFile = new PrintWriter(writer);

        // IMPL_NOTE: hardcoded name, must agree with the makefile
        inclFile.println("#include \"cldcvm.mch\"");

        inclFile.flush();
        currentDB.updateFile(giMchProxyIncl.dirPreStemSuff(), baos);
        inclFile.close();
    }
}
