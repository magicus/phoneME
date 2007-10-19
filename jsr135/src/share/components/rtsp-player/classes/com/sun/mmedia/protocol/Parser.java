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
package com.sun.mmedia.rtsp.protocol;

import java.io.*;
import java.util.*;

public class Parser {
    private Vector buffer;

    public Parser() {
        init();
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

    public String getToken(ByteArrayInputStream bin) {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();

        skipWhitespace(bin);

        if (bin.available() > 0) {
            int ch = readChar(bin);

            while (ch != ' ' && ch != '\n' && ch != '\r' && ch != -1) {
                bout.write(ch);

                ch = readChar(bin);
            }

            ungetChar(ch);
        }

        String token = new String(bout.toByteArray());

        return token;
    }

    public void ungetChar(int ch) {
        buffer.insertElementAt(new Integer(ch), 0);
    }

    public String getLine(ByteArrayInputStream bin) {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();

        int ch = readChar(bin);

        while (ch != '\n' && ch != '\r' && ch != -1) {
            bout.write(ch);

            ch = readChar(bin);
        }

        ch = readChar(bin);

        if (ch != '\n') {
            ungetChar(ch);
        }

        String line = new String(bout.toByteArray());

        return line;
    }

    public String getStringToken(ByteArrayInputStream bin) {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();

        skipWhitespace(bin);

        int ch = readChar(bin);

        while (ch != '\n' && ch != '\r' && ch != -1) {
            bout.write(ch);

            ch = readChar(bin);
        }

        String token = new String(bout.toByteArray());

        return token;
    }

    public byte[] getContent(ByteArrayInputStream bin) {
        ByteArrayOutputStream bout = new ByteArrayOutputStream();

        skipWhitespace(bin);

        int ch = readChar(bin);

        while (ch != -1) {
            bout.write(ch);

            ch = readChar(bin);
        }

        return bout.toByteArray();
    }

    private void skipWhitespace(ByteArrayInputStream bin) {
        int ch = readChar(bin);

        while (ch == ' ' || ch == '\n' || ch == '\r') {
            ch = readChar(bin);
        }

        ungetChar(ch);
    }

    private void init() {
        buffer = new Vector();
    }
}
