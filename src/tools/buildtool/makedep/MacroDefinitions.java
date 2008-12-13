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

public class MacroDefinitions {
    private Hashtable macros;

    public MacroDefinitions() {
	macros = new Hashtable();
    }

    private String lookup(String name) throws NoSuchElementException {
        String s = (String)macros.get(name);
        if (s != null) {
            return s;
        }
	throw new NoSuchElementException(name);
    }
    
    public void addMacro(String name, String contents) {
      //System.out.println("adding macro: "+ name + " = \"" + contents + "\"");
        macros.put(name, contents);
    }

    private boolean lineIsEmpty(String s) {
	for (int i = 0; i < s.length(); i++) {
	    if (!Character.isWhitespace(s.charAt(i))) {
		return false;
	    }
	}
	return true;
    }

    public void readFrom(String fileName, boolean missingOk)
	throws FileNotFoundException, FileFormatException, IOException {
	BufferedReader reader = null;
	try {
	    reader = new BufferedReader(new FileReader(fileName));
	} catch (FileNotFoundException e) {
	    if (missingOk) {
		return;
	    } else {
		throw(e);
	    }
	}
	String line;
	do {
	    line = reader.readLine();
	    if (line != null) {
		if ((!line.startsWith("//")) &&
		    (!lineIsEmpty(line))) {
		    int nameBegin = -1;
		    int nameEnd = -1;
		    boolean gotEquals = false;
		    int contentsBegin = -1;
		    int contentsEnd = -1;

		    int i = 0;
		    // Scan forward for beginning of name
		    while (i < line.length()) {
			if (!Character.isWhitespace(line.charAt(i))) {
			    break;
			}
			i++;
		    }
		    nameBegin = i;

		    // Scan forward for end of name
		    while (i < line.length()) {
			if (Character.isWhitespace(line.charAt(i))) {
			    break;
			}
                        if (line.charAt(i) == '=') {
                            break;
                        }
			i++;
		    }
		    nameEnd = i;

		    // Scan forward for equals sign
		    while (i < line.length()) {
			if (line.charAt(i) == '=') {
			    gotEquals = true;
			    break;
			}
			i++;
		    }

		    // Scan forward for start of contents
		    i++;
		    while (i < line.length()) {
			if (!Character.isWhitespace(line.charAt(i))) {
			    break;
			}
			i++;
		    }
		    contentsBegin = i;

		    // Scan *backward* for end of contents
		    i = line.length() - 1;
		    while (i >= 0) {
			if (!Character.isWhitespace(line.charAt(i))) {
			    break;
			}
                        i--;
		    }
		    contentsEnd = i+1;

                    String name = null, contents = null;
                    if (nameBegin < nameEnd) {
                        name = line.substring(nameBegin, nameEnd);
                    }
                    if (gotEquals) {
                        if (contentsBegin < contentsEnd) {
                            contents = line.substring(contentsBegin,
                                                      contentsEnd);
                        } else {
                            contents = "";
                        }
                    }

		    if (name == null || contents == null) {
			throw new FileFormatException(
			    "Expected \"macroname = value\", " +
			    "but found: " + line
			);
		    }

		    addMacro(name, contents);
		}
	    }
	} while (line != null);

	reader.close();

        /*
         * iarch and carch are defined only for a few platforms that
         * have different interpreter and compiler CPU architectures
         * (such as AOT-enabled ROM generator). On most platforms,
         * iarch and carch are not defined and should have the same
         * value as arch.
         */
        String arch = lookup("arch");
        String iarch = getMacroContent("iarch");
        String carch = getMacroContent("carch");
        if (iarch == null || iarch.equals("")) {
            addMacro("iarch", arch);
        }
        if (carch == null || carch.equals("")) {
            addMacro("carch", arch);
        }
    }

    /** Throws IllegalArgumentException if passed token is illegally
        formatted */
    public String expand(String token)
	throws IllegalArgumentException {
	// the token may contain one or more <macroName>'s

	String out = "";

	// emacs lingo
	int mark = 0;
	int point = 0;
	
	int len = token.length();

	if (len == 0)
	    return out;

	do {
	    // Scan "point" forward until hitting either the end of
	    // the string or the beginning of a macro
	    if (token.charAt(point) == '<') {
		// Append (point - mark) to out
		if ((point - mark) != 0) {
		    out += token.substring(mark, point);
		}
		mark = point + 1;
		// Scan forward from point for right bracket
		point++;
		while ((point < len) &&
		       (token.charAt(point) != '>')) {
		    point++;
		}
		if (point == len) {
		    throw new IllegalArgumentException(
		        "Could not find right angle-bracket in token " + token
		    );
		}
		String name = token.substring(mark, point);
		if (name == null) {
		    throw new IllegalArgumentException(
		        "Empty macro in token " + token
		    );
		}
		try {
		    String contents = lookup(name);
		    out += contents;
		    point++;
		    mark = point;
		} catch (NoSuchElementException e) {
		    throw new IllegalArgumentException(
		        "Unknown macro " + name + " in token " + token
		    );
		}
	    } else {
		point++;
	    }
	} while (point != len);
	
	if (mark != point) {
	    out += token.substring(mark, point);
	}
	
	return out;
    }

    public MacroDefinitions copy() {
	MacroDefinitions ret = new MacroDefinitions();
	for (Enumeration e = macros.keys(); e.hasMoreElements(); ) {
	    String name = (String) e.nextElement();
            String value = (String) macros.get(name);            
	    ret.macros.put(name, value);
	}
	return ret;
    }

    public void setAllMacroBodiesTo(String s) {
	for (Enumeration e = macros.keys(); e.hasMoreElements(); ) {
	    String name = (String) e.nextElement();
	    macros.put(name, s);
	}
    }

    public String getMacroContent(String name) {
        try {
            return lookup(name);
        } catch (NoSuchElementException e) {
            return null;
        }
    }

    private void error(String text) throws FileFormatException {
	throw new FileFormatException(
	    "Expected \"macroname = value\", but found: " + text
	);
    }
}
