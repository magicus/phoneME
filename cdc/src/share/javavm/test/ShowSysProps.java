/*
 * @(#)ShowSysProps.java	1.6 06/10/10
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

import java.io.PrintStream;
/*
 * Prints out system properties. See 'header[]' for additional info.
 */
public class ShowSysProps {
    private final static String header[] = {
	"---------------------------------------------------------------",
	" List of System Properties",
	"",
	"  Properties shown with leading '-' are not required by the spec. ",
	"  Specifications related to the system properties are:",
	"  - 'J2SE 1.3 API spec (System.getProperties())' at:",
	"    http://java.sun.com/j2se/1.3/docs/api/java/lang/System.html#getProperties()",
	"  - 'Java Product Versioning Specification' at:",
	"    http://java.sun.com/j2se/1.3/docs/guide/versioning/spec/VersioningTOC.html",
	"---------------------------------------------------------------",
    };
    private final static String footer[] = {
	"---------------------------------------------------------------",
    };

    /*
     * List of properties that this tool prints out.
     * Properties with '-' on top are not required by the spec.
     */
    private static final String bodySource[] = {
	"java.version",
	"java.vendor",
	"java.vendor.url",
	"-java.vendor.url.bug",
	"",
	"java.home",
	"",
	"java.vm.specification.version",
	"java.vm.specification.vendor",
	"java.vm.specification.name",
	"",
	"java.vm.version",
	"java.vm.vendor",
	"java.vm.name",
	"-java.vm.info",
	"",
	"java.specification.version",
	"java.specification.vendor",
	"java.specification.name",
	"",
	"java.class.version",
	"java.class.path",
	"java.ext.dirs",
	"-java.library.path",
	"-sun.boot.class.path",
	"-sun.boot.library.path",
	"",
	"-java.compiler",
	"",
	"os.name",
	"os.arch",
	"os.version",
	"file.separator",
	"path.separator",
	"line.separator",
	"file.encoding",
	"",
	"user.name",
	"user.home",
	"user.dir"
    };

    private static final int colPos = 28;
    private static void processBody(PrintStream ps, String src) {
	String  name = src;
	String  key = name;

	if (name.startsWith("-")) {
	    key = name.substring(1);
	}
	if (!name.equals("")) {
	    String n = name + " : ";
	    if (n.length() < colPos) {
		n += "                                                      ";
		n = n.substring(0, colPos);
	    }
	    ps.println("  " + n + "'" + System.getProperty(key) + "'");
	} else {
	    ps.println("\t----");
	}
    }

    public static void main(String[] args) {
	for (int i = 0; i < header.length; i++) {
	    System.out.println(header[i]);
	}
	for (int i = 0; i < bodySource.length; i++) {
	    processBody(System.out, bodySource[i]);
	}
	for (int i = 0; i < footer.length; i++) {
	    System.out.println(footer[i]);
	}
    }
}
