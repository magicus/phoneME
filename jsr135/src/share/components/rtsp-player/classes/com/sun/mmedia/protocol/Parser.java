/*
 * Copyright 2003 Sun Microsystems, Inc. All rights reserved.
 * SUN PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
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
