/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
package com.sun.mmedia.rtsp.sdp;

import java.io.*;
import java.util.*;


public class Parser {
    private static Vector buffer;

    public void init() {
        buffer = new Vector();
    }

    public String getTag( ByteArrayInputStream bin) {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();

        skipWhitespace(bin);

        if (bin.available() > 0) {
            int ch = readChar(bin);

            while (ch != '=' && ch != '\n' && ch != '\r' && ch != -1) {
                bout.write(ch);

                ch = readChar(bin);
            }

            bout.write(ch);
        }

        String tag = new String(bout.toByteArray());

	return tag;
    }

    public void ungetTag(String tokenStr) {
        byte token[] = tokenStr.getBytes();

        for (int i = 0; i < token.length; i++) {
            buffer.insertElementAt(
                    new Integer(token[token.length - i - 1]), 0);
        }
    }
    
    public String getLine(ByteArrayInputStream bin) {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();

        if (bin.available() > 0) {
            int ch = readChar(bin);

            while (ch != '\n' && ch != '\r' && ch != -1) {
                bout.write(ch);

                ch = readChar(bin);
            }
        }

        String line = new String(bout.toByteArray());

        return line;
    }

    private void skipWhitespace(ByteArrayInputStream bin) {
        int ch = readChar(bin);

        while (ch == ' ' || ch == '\n' || ch == '\r') {
            ch = readChar(bin);
        }

        buffer.insertElementAt(new Integer(ch), 0);
    }

    public int readChar(ByteArrayInputStream bin) {
        int ch;

        if (buffer.size() > 0) {
            ch = ((Integer) buffer.elementAt(0)).intValue();

            buffer.removeElementAt(0);
        } else {
            ch = bin.read();
        }

        return ch;
    }

    private boolean print_debug= false;
    
    public void debug( String str) {
	if( print_debug) {
	    System.out.println( str);
	}
    }
}
