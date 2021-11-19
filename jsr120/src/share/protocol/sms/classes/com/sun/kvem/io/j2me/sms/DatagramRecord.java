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

package com.sun.kvem.io.j2me.sms;
import java.util.Vector;
import java.io.ByteArrayOutputStream;
import java.io.IOException;

import com.sun.midp.io.j2me.sms.*;

/**
 * Encodes and decodes a single formatted datagram message
 * used to communicate an SMS request to the datagram
 * message server.
 */
public class DatagramRecord {
    /** An appropriate initial size for storage vectors (10). */
    private static int INITIAL_SIZE = 10;

    /** A vector of property keys. */
    private Vector keys = null;
    /** A vector of property values. */
    private Vector vals = null;

    /** Raw buffer of data. */
    private byte[] data = null;

    /** Initializes the record structures. */
    public DatagramRecord() {
    keys = new Vector(INITIAL_SIZE);
    vals = new Vector(INITIAL_SIZE);
    }

    /**
     * Gets a header value.
     * @param key field name to look up
     * @return value for the field as a <tt>String</tt> or <tt>null</tt> if
     *  the key is not found
     * @see #setHeader
     */
    public String getHeader(String key) {
        int idx;
        if (key == null) {
            throw new NullPointerException();
        }
        for (idx = 0; idx < keys.size(); idx++) {
            if (key.equals(keys.elementAt(idx)))
                break;
        }
        String rv = null;
        if (idx != keys.size()) {
            rv = (String)vals.elementAt(idx);
        }
        return rv;
    }

    /**
     * Stores a single <tt>key</tt>:<tt>value</tt> pair. If a key
     * already exists in storage,
     * the value corresponding to that key will be replaced and returned.
     *
     * @param key the key to be placed into this property list
     * @param value the value corresponding to <tt>key</tt>
     * @return the old
     * value, if the new property value replaces an existing one.
     * Otherwise, <tt>null</tt> is returned.
     * @see #getHeader
     */
    public synchronized String setHeader(String key, String value) {
        int idx;
        for (idx = 0; idx < keys.size(); idx++) {
            if (key.equals(keys.elementAt(idx)))
                break;
        }

        String rv = null;
        if (idx == keys.size()) {
            // If I don't have this, add it and return null
            keys.addElement(key);
            vals.addElement(value);
        } else {
            // Else replace it and return the old one.
            rv = (String)vals.elementAt(idx);
            vals.setElementAt(value, idx);
        }
        return rv;
    }

    /**
     * Gets the raw data payload.
     * @return  the raw byte array contents
     * @see #setData
     */
    public byte[] getData() {
        return data;
    }

    /**
     * Sets the data part of the <tt>DatagramRecord</tt>.
     * @param buffer raw data to transport
     * @see #getData
     */
    public void setData(byte[] buffer) {
        data = buffer;
    }

    /**
     * Adds data to another <tt>DatagramRecord</tt>.
     * This interface is used to support reassembly
     * of fragmented messages.
     * @param rec received datagram record to assemble
     * @return true if transmission is complete
     * @exception IOException if packets are out of order,
     *            or if fragmented packets share the same send timestamp
     * @see #setData
     */
    public boolean addData(DatagramRecord rec) throws IOException {
        /*
         * Make sure the the segments are part of the same
         * transaction. (Same send time should be sufficient)
         */
        if (rec != null && !rec.getHeader("Date").equals(getHeader("Date"))) {
            throw new IOException("Bad fragmentation");
        }

        String total = (rec != null ? rec.getHeader("Total-Size") : null);
        String cl = getHeader("Content-Length");

        String fr = getHeader("Fragment-Size");

        int len = 0;
        int contentlength = 0;
        try {
            contentlength = Integer.parseInt(cl);
        } catch (NumberFormatException npe) {
            /* IMPL_NOTE - ignore npe errors */
        }
        if (total == null) {
            len = data.length;
            /*
             * Make the buffer large enough to handle a full set
             * of data.
             */
            byte[] newbuf = new byte[contentlength];
            if (data != null) {
                System.arraycopy(data, 0, newbuf, 0, data.length);
            }
            total = String.valueOf(data.length);
            setHeader("Total-Size", total);
            data = newbuf;

        } else {
            int offset = 0;
            int size = 0;
            try {
                offset = Integer.parseInt(getHeader("Fragment-Offset"));
            } catch (NumberFormatException npe) {
                /* IMPL_NOTE - ignore npe errors */
            }
            try {
                len = Integer.parseInt(getHeader("Fragment-Size"));
            } catch (NumberFormatException npe) {
                /* IMPL_NOTE - ignore npe errors */
            }

            /* Use the previous record's data buffer. */
            byte[] newbuf = rec.getData();

            /*
             * Copy the data into the expanded buffer
             * and update the cumulative size.
             */

            System.arraycopy(data, 0, newbuf, offset, len);
            data = newbuf;

            try {
                size = Integer.parseInt(total);
                size += len;
            } catch (NumberFormatException npe) {
                /* IMPL_NOTE - ignore npe errors */
            }
            total = String.valueOf(size);
            setHeader("Total-Size", total);
            len = size;
        }

        return len == contentlength;
    }

    /**
     * Parses an inbound message into headers and data
     * buffers.
     * @param buf raw data from the datagram transmission
     * @param length  size of valid data in the buffer
     * @return true if this is a multi-part transmission
     */
    public boolean parseData(byte[] buf, int length) {
        /*
         * Walk through the datagram message looking for
         * specific header values.
         */
        int colon = 0;
        int endofline = 0;
        int startofline = 0;
        String field = null;
        String value = null;
        int  segments = 0;
        boolean colon_found = false;

        for (int i = 0; i < length; i++) {
            if (!colon_found && buf[i] == ':') {
                colon = i;
                colon_found = true;
                field = new String(buf, startofline, colon - startofline);
            }
            if (buf[i] == '\n') {
                endofline = i;
                value = new String(buf, colon + 1,
                                   endofline - colon - 1).trim();

                if (field.startsWith("Buffer")) {
                    int startofdata = endofline + 1;
                    data = new byte[length - startofdata];

                    for (int j = startofdata, k = 0; j < length;
                         j++, k++, i++) {
                        data[k] = buf[j];
                    }

                } else {
                    setHeader(field, value);
                }
                startofline = endofline + 1;
                colon_found = false;
            }
        }
        try {
            segments = Integer.parseInt(getHeader("Segments"));
        } catch (NumberFormatException nfe) {
            /* IMPL_NOTE - ignore npe errors */
        }

        return segments > 1;
    }

    /**
     * Formats the headers and data for transmission as a raw
     * byte array.
     *
     * @return byte array of formatted data
     */
    public byte[] getFormattedData() {
        ByteArrayOutputStream bos = new ByteArrayOutputStream();

        try {
            /* Put the headers in the buffer. */
            for (int i = 0; i < keys.size(); i++) {
                bos.write(((String)keys.elementAt(i) + ": "
                           + (String)vals.elementAt(i)
                          + "\n").getBytes());
            }
            bos.write(("Buffer: \n").getBytes());
            bos.write(data);
        } catch (IOException ioe) {
            /* IMPL_NOTE - ignore buffer writing errors */
        }

        return bos.toByteArray();
    }

    /**
     * Outputs a debug version of the record.
     * @return formatted string of headers and values
     */
    public String toString() {
        StringBuffer sb = new StringBuffer();

        /* Add a line for each header. */
        for (int i = 0; i < keys.size(); i++) {
            sb.append(keys.elementAt(i)
                      + ": "
                      + (String)vals.elementAt(i)
                      + "\n");
        }

        /* Output a text buffer or placeholder to indicate binary data. */
        if (getHeader("Content-Type").equals("text")) {
            String textbuf;
            String te = getHeader("Text-Encoding");
            if (te == null || te.equals("ucs2")) {
                textbuf = TextEncoder.toString(data);
            } else {
                byte[] gsmbytes = TextEncoder.decode(data);
                textbuf = TextEncoder.toString(gsmbytes);
            }
            sb.append("Buffer: " + textbuf + "\n");
        } else {
            sb.append("Buffer: (binary)\n" + new String(data) + "\n");
        }
        return sb.toString();
    }
}
