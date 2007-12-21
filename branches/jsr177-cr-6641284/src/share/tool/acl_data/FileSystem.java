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

import java.util.Vector;
import java.util.Hashtable;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;

/**
 * Represents WIM file system.
 */
public class FileSystem {

    /** File attribute constant. */
    static final int DIR        = 1;
    /** File attribute constant. */
    static final int PIN        = 3;
    /** File attribute constant. */
    static final int PrK        = 4;
    /** File attribute constant. */
    static final int WIM        = 9;
    /** File attribute constant. */
    static final int READ       = 16;
    /** File attribute constant. */
    static final int UPDATE     = 48;
    /** File attribute constant. */
    static final int EMPTY      = 64;

    /** Root directory for this file system. */
    CardFile root;
    /** Base directory for WIM files. */
    CardFile base;

    /** Hash table used to determine file offsets in resuting data. */
    Hashtable labels = new Hashtable(3);

    /**
     * Constructor.
     * @param path root directory for this file system
     */
    public FileSystem(short[] path) {

        root = new CardFile(path[0], DIR);
        base = root;

        for (int i = 1; i < path.length; i++) {

            CardFile f = new CardFile(path[i], DIR);
            base.addFile(f);
            base = f;
        }
        base.type = WIM;
    }

    /**
     * Adds a new file to the file system.
     * @param path file path
     * @param type file type
     * @param data file body
     */
    public void addFile(short[] path, int type, byte[] data) {

        if (path == null) {
            return;
        }
        addFile(path, type, data, null);
    }

    /**
     * Adds a new file to the file system.
     * @param path file path
     * @param type file type
     * @param data file body
     * @param label label that can be used to retrieve file offset
     */
    public void addFile(short[] path, int type, byte[] data, String label) {

        if (path == null) {
            return;
        }

        CardFile top;

        if (path.length > 1) {
            if (path[0] == 0x3fff) {
                top = base;
            } else
            if (path[0] == root.id) {
                top = root;
            } else {
                throw new IllegalArgumentException("Invalid path");
            }

            for (int i = 1; i < path.length - 1; i++) {

                CardFile f = top.getFile(path[i]);
                if (f == null) {
                    f = new CardFile(path[i], DIR);
                    top.addFile(f);
                }
                if (f.type != DIR) {
                    throw new IllegalArgumentException("Invalid path 1");
                }
                top = f;
            }
        } else {
            top = base;
        }

        CardFile f = new CardFile(path[path.length - 1], type);
        f.setData(data);
        top.addFile(f);

        if (label != null) {
            labels.put(label, f);
        }
    }

    /**
     * Returs file offset in resulting data array.
     * @param label file label
     * @return offset value
     */
    public int getOffset(String label) {
        CardFile c = (CardFile) labels.get(label);
        if (c == null) {
            System.out.println("Warning: label not found: " + label);
            return -32768;
        }
        return c.offset;
    }

    /**
     * Returns binary representation of this file system.
     * @return binary representation of this file system
     * @throws IOException if I/O error occurs
     */
    public byte[] getEncoded() throws IOException {

        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        DataOutputStream out = new DataOutputStream(baos);

        root.write(out);
        return baos.toByteArray();
    }


    /**
     * This class represents card file.
     */
    class CardFile {

        /** File identifier. */
        int id;

        /** File type. */
        int type;

        /** File body. */
        byte[] data;

        /** Files that this file contains (if any). */
        Vector files;

        /** File offset in binary representation of the file system. */
        int offset;

        /**
         * Constructor.
         * @param id file identifier
         * @param type file type
         */
        CardFile(int id, int type) {

            this.id = id;
            this.type = type;
            files = new Vector();
        }

        /**
         * Set file data.
         * @param data file data
         */
        void setData(byte[] data) {
            this.data = data;
        }

        /**
         * Add file to this DF.
         * @param f the file
         */
        void addFile(CardFile f) {
            files.add(f);
        }

        /**
         * Returns file with this ID in this DF or null.
         * @param id file identifier
         * @return file with this ID in this DF or null
         */
        CardFile getFile(int id) {

            CardFile f;

            for (int i = 0; i < files.size(); i++) {

                f = (CardFile) files.elementAt(i);
                if (f.id == id) {
                    return f;
                }
            }
            return null;
        }

        /**
         * Writes binary representation of this file into the stream.
         * @param out output stream
         * @throws IOException if I/O error occurs
         */
        void write(DataOutputStream out) throws IOException {

            out.writeShort(id);
            out.writeByte(type);
            if ((type & DIR) == 0) {
                out.writeShort(data.length);
                offset = out.size();

                if ((type & EMPTY) == 0) {
                    out.write(data);
                } else {
                    int i = data.length - 1;
                    while (i > -1 && data[i] == 0) {
                        i--;
                    }
                    i++;
                    out.writeShort(i);
                    out.write(data, 0, i);
                }
            } else {
                out.writeShort(files.size());
                for (int i = 0; i < files.size(); i++) {

                    CardFile f = (CardFile) files.elementAt(i);
                    f.write(out);
                }
            }
        }
    }
}
