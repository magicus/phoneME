/*
 * @(#)JVMDIParse.java	1.10 06/10/10
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

package com.sun.tools.jdwpgen;

import java.util.*;
import java.io.*;

class JVMDIParse {

    final StreamTokenizer izer;
    static final int DEFINE_TOKEN = 101010;
    static final int NAME_TOKEN = 101011;
    int tok;
    int prevTok = StreamTokenizer.TT_EOL;
    String origName;
    int value;

    JVMDIParse(Reader reader) throws IOException {
        izer = new StreamTokenizer(new BufferedReader(reader));
        izer.resetSyntax();
        izer.slashStarComments(true);
        izer.slashSlashComments(true);
        izer.wordChars((int)'a', (int)'z');
        izer.wordChars((int)'A', (int)'Z');
        izer.wordChars((int)'0', (int)'9');
        izer.wordChars((int)'_', (int)'_');
        izer.whitespaceChars(0, 32);
        izer.whitespaceChars((int)'(', (int)')');
        items();
    }

    void items() throws IOException {

        while ((tok = izer.nextToken()) != StreamTokenizer.TT_EOF) {
            prevTok = item();
        }
        if (Main.nameMap.size() == 0) {
            error("Empty name map");
        }
    }

    int item() throws IOException {
        switch (tok) {
            case StreamTokenizer.TT_EOF:
                error("Unexpected end-of-file");
                return tok;

            case StreamTokenizer.TT_WORD: {
                String word = izer.sval;
                if (Character.isDigit(word.charAt(0))) {
                    if (prevTok != NAME_TOKEN) {
                        return tok;
                    }
                    int num;
                    try {
                        if (word.startsWith("0x")) {
                            num = Integer.parseInt(word.substring(2), 16);
                        } else {
                            num = Integer.parseInt(word);
                        }
                    } catch (NumberFormatException exc) {
                        return tok;
                    }
                    NameNode nn = new NameValueNode(origName, num);
                    Main.nameMap.put(origName, nn);
                    return tok;
                }
                switch (prevTok) {
                    case '#':
                        if (word.equals("define")) {
                            return DEFINE_TOKEN;
                        } else {
                            return tok;
                        }
                    case DEFINE_TOKEN: {
                        if (word.startsWith("JVMDI_")) {
                            origName = word;
                            return NAME_TOKEN;
                        }
                        return tok;
                    }
                    case NAME_TOKEN:
                        if (word.equals("jint") ||
                            word.equals("jvmdiError")) {
                            return prevTok; // ignore
                        } else {
                            return tok;
                        }
                    default:
                        return tok;
                }
            }

            default:
                return tok;
        }
    }

    void error(String errmsg) {
        System.err.println("JVMDI:" + izer.lineno() + 
                           ": Internal Error - " + errmsg);
        System.exit(1);
    }
}
                    
                

                
