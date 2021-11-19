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

package com.sun.satsa.util;

import java.io.IOException;

/**
 * This class provides interface to card file system.
 */
abstract public class FileSystemAbstract {

    /** Expected maximum path depth. */
    static final int MAX_PATH_DEPTH  = 10;
    /** APDU INS byte. */
    protected static final byte INS_SELECT    = (byte) 0xa4;
    /** APDU INS byte. */
    protected static final byte INS_READ      = (byte) 0xb0;
    /** APDU INS byte. */
    protected static final byte INS_READ_REC  = (byte) 0xb2;       
    /** APDU INS byte. */
    protected static final byte INS_UPDATE    = (byte) 0xd6;
    /** Root path for this application. */
    short[] root;
    /** The length of path to current DF. */
    int pathLength;
    /**
     * Path to the current DF. Element at index pathLength contains EF
     * identifier if EF is selected.
     */
    short[] currentPath;
    /** Connection used by this file system. */
    protected Connection apdu;
    /** Size of currently selected EF or -1 if DF is selected. */
    protected int currentFileSize;
    /** True if currently selected file is EF. */
    protected boolean isEFSelected;

    /**
     * Constructs new FileSystem object.
     * @param apdu connection to be used by this object.
     */
    public FileSystemAbstract(Connection apdu) {
        this.apdu = apdu;
    }

    /**
     * Sets the root directory for this file system.
     * @param root root directory path
     */
    public void setRoot(short[] root) {

        this.root = root;
        currentPath = new short[MAX_PATH_DEPTH];

        for (int j = 0; j < root.length; j++) {
            currentPath[j] = root[j];
        }
        pathLength = root.length;
    }

    /**
     * Sends the select DF command to the card.
     * @param root short[] directory path
     * @throws IOException if IO error occurs
     */
    public void selectRoot(short[] root) throws IOException {
        int P1P2 = (int) (0x0100 | Constants.P2);
        
        setRoot(root);
        for (int i = 0; i < root.length; i++) {
            byte[] data = apdu.resetCommand().
                          putShort(root[i]).
                          sendCommand(INS_SELECT, P1P2);
        }
    }

    /**
     * If necessary converts path into completely specified path.
     * @param t TLV object that contains octet string representing path
     * @return completely specified path
     * @throws IOException if path is incorrect
     */
    public short[] makePath(TLV t) throws IOException {

        short[] path;
        short s = Utils.getShort(t.data, t.valueOffset);
        if (root == null) {     // only file IDs are used
            if (t.length == 2) {
                path = new short[1];
                path[0] = s;
                return path;
            }
            throw new IOException("Invalid path - ID expected");
        }

        // absolute path
        if (s == 0x3f00) {
            path = new short[t.length / 2];
            for (int i = 0; i < path.length; i++) {
                path[i] = Utils.getShort(t.data, t.valueOffset + i * 2);
            }
            return path;
        }

        // it must be relative path
        int len = root.length + t.length / 2;
        int offset = t.valueOffset;

        if (s == 0x3fff) {
            len--;
            offset += 2;
        } else {
            if (t.length != 2) {
                throw new IOException(
                        "Invalid path - file ID must be used");
            }
        }

        path = new short[len];
        System.arraycopy(root, 0, path, 0, root.length);
        for (int i = root.length; i < path.length; i++) {
            path[i] = Utils.getShort(t.data, offset);
            offset += 2;
        }
        return path;
    }

    /**
     * Selects file using only SELECT APDU command with file identifier.
     * According to WIM specification support of this selection method
     * is mandatory.
     * @param path file path
     * @throws IOException if IO error occurs
     */
    public void select(short[] path) throws IOException {

        if (root == null) {
            if (path.length == 1) {
                select(path[0]);
                return;
            }
            throw new IOException(
                    "Invalid path - file ID must be used");
        }

        int c = 0;      // index of first path element that differs
        while (c < pathLength && c < path.length) {
            if (currentPath[c] != path[c]) {
                break;
            }
            c++;
        }

        if (c == 0) {
            throw new IOException("Invalid path - wrong root directory");
        }

        if (c == pathLength &&
            pathLength == (path.length - 1) &&
            isEFSelected &&
            currentPath[pathLength] == path[pathLength]) {
            return;
        }

        while (c < pathLength) {
            pathLength--;
            select(currentPath[pathLength]);
        }

        while (pathLength < path.length) {
            select(path[pathLength]);
            currentPath[pathLength] = path[pathLength];
            if (isEFSelected) {
                if (pathLength == path.length - 1) {
                    break;
                }
                throw new IOException("Invalid path - EF in path");
            }
            pathLength++;
        }
    }

    /**
     * Selects file by ID.
     * @param id file ID
     * @throws IOException if IOError occurs
     */
    abstract public void select(short id) throws IOException;

    /**
     * Reads the current EF.
     * @return array that contains EF body.
     * @throws IOException if IO error occurs
     */
    public byte[] readFile() throws IOException {
        byte[] data = readData(0, currentFileSize, 0);
        return data;
    }


    /**
     * Reads part of selected file.
     * @param offset the offset into data buffer where data should be
     * placed
     * @param length data length
     * @param fileOffset file data offset
     * @throws IOException if IO error occurs
     * @return data byte array of the data
     */
    abstract public byte[] readData(int offset, int length, int fileOffset)
            throws IOException;


    /**
     * Loads and parses one DER encoded object.
     * @param l location of the object.
     * @return the object.
     * @throws IOException if I/O error occurs
     * @throws TLVException if parsing error occurs
     */
    public TLV loadObject(Location l) throws IOException, TLVException {
        select(l.path);
        byte[] tmp = readData(0, l.length, l.offset);
        return new TLV(tmp, 0);
    }

    /**
     * Converts PKCS#15 path into location.
     * @param path TLV structure that represents path
     * @return Location object
     */
    public Location pathToLocation(TLV path) throws IOException {

        try {
            if (path.type == TLV.SEQUENCE_TYPE) {
                path = path.child;
            }

            short[] p = makePath(path);
            int offset = 0;
            int length = -1;

            if (path.next != null) {
                offset = path.next.getInteger();
                length = path.next.next.getInteger();
            }
            return new Location(p, offset, length);
        } catch (IOException e) {
            throw new IOException("Invalid path structure");
        } catch (NullPointerException npe) {
            throw new IOException("Invalid path structure");
        } catch (IndexOutOfBoundsException iobe) {
            throw new IOException("Invalid path structure");
        }
    }

    /**
     * Returns the size of currently selected EF or -1 if DF is
     * selected.
     * @return file size
     */
    public int getCurrrentFileSize() {
        return currentFileSize;
    }
}
