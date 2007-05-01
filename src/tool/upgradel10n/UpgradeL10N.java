/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

/**
 * This tool upgrades a LocalizedStrings<locale>.java file to the new XML file
 * format. An example file is
 * src/configuration/configurator/share/l10n/en-US.xml. <p>
 *
 * Usage:
 *    javac UpgradeL10N.java
 *    java -cp . UpgradeL10N [inencoding] [infile] \
 *                           [outencoding] [outfile] [outclass]
 *
 * You must specify an [inencoding] that's the same as the character 
 * encoding used by [infile].
 *
 * E.g., 
 *
 *    java -cp . UpgradeL10N ISO-8859-1 LocalizedStrings.java \
 *                           ISO-8859-1 en-US.xml LocalizedStringsBase
 */

import java.util.*;
import java.io.*;

public class UpgradeL10N {
    static BufferedReader reader;
    static PrintWriter writer;
    
    /**
     * Short-hand for printint a line into the output file
     */
    static void pl(String s) {
        writer.println(s);
    }

    /**
     * Run the tool
     */
    public static void main(String args[]) throws Throwable {
        String inEncoding  = args[0];
        String inFile      = args[1];
        String outEncoding = args[2];
        String outFile     = args[3];
        String outClass    = args[4];

        FileInputStream fin = new FileInputStream(inFile);
        InputStreamReader r = new InputStreamReader(fin, inEncoding);
        reader = new BufferedReader(r);

        FileOutputStream fout = new FileOutputStream(outFile);
        OutputStreamWriter w = new OutputStreamWriter(fout, outEncoding);
        writer = new PrintWriter(w);

        // Write the XML file prolog.
        pl("<?xml version=\"1.0\" encoding=\"" + outEncoding + "\"?>");
        pl("<!DOCTYPE configuration SYSTEM \"../configuration.dtd\">");
        pl("<!--");
        pl("         ");
        pl("");
        pl("  Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.");
        pl("  DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER");
        pl("  ");
        pl("  This program is free software; you can redistribute it and/or");
        pl("  modify it under the terms of the GNU General Public License version");
        pl("  2 only, as published by the Free Software Foundation.");
        pl("  ");
        pl("  This program is distributed in the hope that it will be useful, but");
        pl("  WITHOUT ANY WARRANTY; without even the implied warranty of");
        pl("  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU");
        pl("  General Public License version 2 for more details (a copy is");
        pl("  included at /legal/license.txt).");
        pl("  ");
        pl("  You should have received a copy of the GNU General Public License");
        pl("  version 2 along with this work; if not, write to the Free Software");
        pl("  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA");
        pl("  02110-1301 USA");
        pl("  ");
        pl("  Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa");
        pl("  Clara, CA 95054 or visit www.sun.com if you need additional");
        pl("  information or have any questions.");
        pl("-->");
        pl("<configuration>");
        pl("<localized_strings Package=\"com.sun.midp.l10n\" Name=\"" +
           outClass + "\">");

        StringBuffer sbuf = new StringBuffer();
        String line;
        while ((line = reader.readLine()) != null) {
            sbuf.append(line);
            sbuf.append("\n");
        }

        // Find all occurrences of 
        // new Integer(ResourceConstants.DONE), "\u5B8C\u6210"
        //                               ^^^^    ^^^^^^^^^^^^
        //                               key     value
        Parser p = new Parser(sbuf.toString());
        while (p.advance("Integer")) {
            p.mark();
            try {
                p.skipSpaces(); p.skip('(');
                p.skipSpaces(); p.skip("ResourceConstants");
                p.skipSpaces(); p.skip(".");
                p.skipSpaces();

                String key = p.readSymbol();

                p.skipSpaces(); p.skip(')');
                p.skipSpaces(); p.skip(',');
                p.skipSpaces();

                String value = p.readStringLiteral();

                while (true) {
                    p.mark();

                    try {
                        // Handle any "xxx" + "yyy" cases
                        p.skipSpaces(); p.skip('+');
                        p.skipSpaces();
                        String more = p.readStringLiteral();
                        value += more;
                        p.pop();
                    } catch (Error t) {
                        p.reset();
                        break;
                    }
                }
                pl("<localized_string Key=\"" + key + "\"");
                pl("                Value=\"" + quote(value) + "\"/>");
            } catch (UnsupportedOperationException t) {
                System.out.println("Error: " + t.getMessage());
                System.out.println("at line " + p.countLine());
                System.out.println("at character " + p.countPos());
                System.exit(1);
            } catch (Error t) {
                // This loop will eventually terminate, since we at least
                // consume ".*Integer" from each iteration.
                p.reset();
            }
        }
        pl("</localized_strings>");
        pl("</configuration>");
        writer.close();
    }

    /** Quote characters that must be escaped for valid XML documents
     */
    static String quote(String s) {
        if (s.indexOf('&') != -1 || 
            s.indexOf('<') != -1 || 
            s.indexOf('>') != -1 || 
            s.indexOf('\n') != -1 || 
            s.indexOf('"') != -1) {
            StringBuffer sbuf = new StringBuffer();
            for (int i=0; i<s.length(); i++) {
                char c = s.charAt(i);
                if (c == '"') {
                    sbuf.append("&quot;");
                } else if (c == '&') {
                    sbuf.append("&amp;");
                } else if (c == '>') {
                    sbuf.append("&gt;");
                } else if (c == '<') {
                    sbuf.append("&lt;");
                } else if (c == '\n') {
                    sbuf.append("&#010;");
                } else {
                    sbuf.append(c);
                }
            }
            return sbuf.toString();
        }
        return s;
    }
}

/**
 * A simple parser to parsing Java source code like this:
 * new Integer(ResourceConstants.DONE), "\u5B8C\u6210"
 *                               ^^^^    ^^^^^^^^^^^^
 *                               key     value
 */
class Parser {
    int index;
    String s;
    Stack markStack = new Stack();

    /**
     * Encounter an error when the parser tries to prefetch data. This is 
     * normal condition. The caller should call Parser.reset() and continue.
     *
     * In a source code error is found, a UnsupportedOperationException
     * is thrown instead.
     */
    void abortPrefetch() {
        throw new Error();
    }

    Parser(String s) {
        this.s = s;
        index = 0;
    }

    void mark() {
        markStack.push(new Integer(index));
    }

    void reset() throws Throwable {
        Integer i = (Integer)markStack.pop();
        index = i.intValue();
    }

    void pop() throws Throwable {
        markStack.pop();
    }

    boolean advance(String token) {
        int n = s.indexOf(token, index);
        if (n < 0) {
            return false;
        } else {
            index = n + token.length();
            return true;
        }
    }

    void skipSpaces() {
        while (index < s.length()) {
            char c = s.charAt(index);
            if (!Character.isSpaceChar(c)
                && c != '\n' && c != '\r' && c != '\t') {
                return;
            } else {
                index ++;
            }
        }
    }

    void skip(char c) {
        if (s.charAt(index) == c) {
            index ++;
            return;
        } else {
            abortPrefetch();
        }
    }

    void skip(String token) {
        for (int i=0; i<token.length(); i++) {
            skip(token.charAt(i));
        }
    }

    String readSymbol() {
        StringBuffer sbuf = new StringBuffer();
        while (index < s.length()) {
            char c = s.charAt(index);
            if (sbuf.length() == 0) {
                if (!Character.isJavaIdentifierStart(c)) {
                    abortPrefetch();
                }
            } else {
                if (!Character.isJavaIdentifierPart(c)) {
                    return sbuf.toString();
                }
            }

            index ++;
            sbuf.append(c);
        }
        if (sbuf.length() == 0) {
            abortPrefetch();
        }
        return sbuf.toString();
    }

    String readStringLiteral() {
        skip('"');
        StringBuffer sbuf = new StringBuffer();
        while (index < s.length()) {
            char c = next();
            switch (c) {
            case '\\':
                switch (next()) {
                case '\\':
                    sbuf.append("\\");
                    break;
                case '\'':
                    sbuf.append("\'");
                    break;
                case '\"':
                    sbuf.append("\"");
                    break;
                case 'n':
                    sbuf.append("\n");
                    break;
                case 'r':
                    sbuf.append("\r");
                    break;
                case 't':
                    sbuf.append("\t");
                    break;
                case 'x':
                    sbuf.append((char) ((nextHex() << 4) | nextHex()));
                    break;
                case 'u':
                    sbuf.append((char) ((nextHex() << 12) |
                                        (nextHex() <<  8) |
                                        (nextHex() <<  4) |
                                        (nextHex() <<  0)));
                    break;
                default:
                    throw new UnsupportedOperationException(
                         "Invalid escape sequence");
                }
                break;
            case '"':
                return sbuf.toString();
            default:
                sbuf.append(c);
            }
        }
        throw new UnsupportedOperationException("unterminated string");
    }

    int nextHex() {
        char c = next();
        if ('0' <= c && c <= '9') {
            return (int)(c - '0');
        }
        if ('A' <= c && c <= 'F') {
            return (int)(c - 'A');
        }
        if ('a' <= c && c <= 'f') {
            return (int)(c - 'a');
        }
        throw new
            UnsupportedOperationException("Expected hex number but got " + c);
    }

    char next() {
        // will throw if we get to EOF
        return s.charAt(index++);
    }


    int countLine() {
        int line = 1;
        for (int i=0; i<index && i < s.length(); i++) {
            if (s.charAt(i) == '\n') {
                ++ line;
            }
        }
        return line;
    }
    int countPos() {
        int pos = 1;
        for (int i=0; i<index && i < s.length(); i++) {
            if (s.charAt(i) == '\n') {
                pos = 0;
            } else {
                pos ++;
            }
        }
        return pos;
    }
}
