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
 */

package acl_data;

import java.io.Reader;
import java.io.IOException;

/**
 * This class represents file reader for file that describes access control
 * information.
 */
public class ACLFileReader {

    /**
     * Input data reader.
     */
    Reader in;

    /**
     * Constructor.
     * @param r input data reader.
     */
    public ACLFileReader(Reader r) {

        lineBuffer = new char[128];
        in = r;
    }

    /**
     * Temporary data array.
     */
    char[] lineBuffer;

    /** Horizontal Tab - Unicode character 0x09. */
    protected static final int HT = 0x09;

    /** Line Feed - Unicode character 0x0A. */
    protected static final int LF = 0x0A;

    /** Carrage Return - Unicode character 0x0D. */
    protected static final int CR = 0x0D;

    /** End Of File - Unicode character 0x1A. */
    protected static final int EOF = 0x1A;

    /** SPace - Unicode character 0x20. */
    protected static final int SP = 0x20;

    /**
     * Current line number.
     */
    protected int lineNumber = 1;

    /**
     * Temporary variable used in parsing.
     */
    protected char savedChar = 0;

    /**
     * Read one word.
     * @return the word.
     * @throws IOException if I/O error occurs.
     */
    public String readWord() throws IOException {


        if (savedChar == '{') {
            savedChar = 0;
            return "{";
        }

        if (savedChar == '}') {
            savedChar = 0;
            return "}";
        }

        int room = lineBuffer.length;
        int offset = 0;
        boolean comment = false;
        int c;

        for (;;) {
            c = in.read();
            if (c == -1) {
                // LF or CR LF ends a line
                break;
            }

            if (c == '#') {
                comment = true;
            }

            if (c == LF) {
                lineNumber++;
                comment = false;
            }

            if (comment) {
                continue;
            }

            if (c == LF || c == CR || c == EOF || c == HT || c == SP) {
                if (offset == 0) {
                    continue;
                }
                break;
            }

            if (c == '{' || c == '}') {
                if (offset == 0) {
                    lineBuffer[offset++] = (char) c;
                } else {
                    savedChar = (char) c;
                }
                break;
            }

            if (--room < 0) {
                char[] temp = new char[offset + 128];
                room = temp.length - offset - 1;
                System.arraycopy(lineBuffer, 0, temp, 0, offset);
                lineBuffer = temp;
            }

            lineBuffer[offset++] = (char) c;
        }

        if ((c == -1) && (offset <= 0)) {
            return null;
        }

        return new String(lineBuffer, 0, offset).trim();
    }

    /**
     * Read current line without any modification.
     * @return the current line value.
     * @throws IOException if I/O error occurs.
     */
    public String readLine() throws IOException {

        int room;
        int offset = 0;
        int c;
        char[] temp;

        room = lineBuffer.length;

        for (;;) {
            c = in.read();

            if (c == LF) {
                lineNumber++;
            }

            if (c == -1 || c == LF) {
                // LF or CR LF ends a line
                break;
            }

            /*
            * throw away carrage returns and the end of file character.
            */
            if (c == CR || c == EOF) {
                continue;
            }

            if (--room < 0) {
                temp = new char[offset + 128];
                room = temp.length - offset - 1;
                System.arraycopy(lineBuffer, 0, temp, 0, offset);
                lineBuffer = temp;
            }

            lineBuffer[offset++] = (char) c;
        }

        if ((c == -1) && (offset <= 0)) {
            return null;
        }

        return new String(lineBuffer, 0, offset).trim();
    }

    /**
     * Reads one word, converts it into unsigned byte value.
     * @return unsigned byte value.
     * @throws IOException if I/O error occurs.
     */
    public int readByte() throws IOException {
        return Integer.parseInt(readWord(), 16) & 0xff;
    }

    /**
     * Check that the next word is equal to specified one.
     * @param s the word value.
     * @throws IOException if I/O error occurs.
     */
    public void checkWord(String s) throws IOException {

        if (! readWord().equals(s)) {
            throw new IOException();
        }
    }

    /**
     * Returns the current line number.
     * @return the current line number.
     */
    public int getLineNumber() {
        return lineNumber;
    }
}
