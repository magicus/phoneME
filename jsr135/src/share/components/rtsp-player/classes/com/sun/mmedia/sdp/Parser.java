/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
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
