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

package com.sun.midp.jadtool;

import java.io.*;

import com.sun.midp.util.Properties;

/**
 * A class for writing JAD's. for details see
 * {@link #write(Properties, OutputStream, String)}
 */
public class JadWriter {
    /** prevents anyone from instantiating this class */
    private JadWriter() {};

    /**
     * Writes this property list (key and element pairs) in this
     * <code>Properties</code> table to the output stream in a format suitable
     * for loading into a <code>Properties</code> table using the
     * <code>load</code> method.
     * The stream is written using the character encoding specified by
     * <code>enc</code>.
     * <p>
     * Then every entry in this <code>Properties</code> table is written out,
     * one per line. For each entry the key string is written, then an ASCII
     * <code>=</code>, then the associated element string. Each character of
     * the element string is examined to see whether it should be rendered as
     * an escape sequence. Characters less than <code>&#92;u0020</code> and
     * characters greater than <code>&#92;u007E</code> are written as
     * <code>&#92;u</code><i>xxxx</i> for
     * the appropriate hexadecimal value <i>xxxx</i>.
     * <p>
     * After the entries have been written, the output stream is flushed.  The
     * output stream remains open after this method returns.
     *
     * @param   out      an output stream.
     * @param   enc      character encoding used on input stream.
     * @exception  IOException if writing this property list to the specified
     *             output stream throws an <tt>IOException</tt>.
     * @exception  ClassCastException  if this <code>Properties</code> object
     *             contains any keys or values that are not 
     *             <code>Strings</code>.
     */
    public static void write(Properties props, OutputStream out,
                             String enc) throws IOException
    {
        OutputStreamWriter awriter;
        awriter = new OutputStreamWriter(out, enc);

	// output in order.
	for (int idx = 0; idx < props.size(); idx++) {
            String key = props.getKeyAt(idx);
	    String val = props.getValueAt(idx);

            if (enc.equals("ISO8859_1")) {
                key = saveConvert(key);	    
                val = saveConvert(val);
            }

            // don't forget the required space after the ":"
            writeln(awriter, key + ": " + val);
        }

        awriter.flush();
    }

    /**
     * Stores properties to the output stream using the ISO 8859-1
     * character encoding.
     *
     * @see #write(Properties, OutputStream, String)
     * @param   out      an output stream.
     * @exception  IOException if writing this property list to the specified
     *             output stream throws an <tt>IOException</tt>.
     * @exception  ClassCastException if this <code>Properties</code>
     *             object contains any keys or values that are not
     *             <code>Strings</code>.
     */
    public static void write(Properties props, OutputStream out) 
	throws IOException {
	write(props, out, "UTF-8");
    }

    private static void writeln(OutputStreamWriter ow, String s) 
	throws IOException {
        ow.write(s);
        ow.write("\n");
    }

    /*
     * Converts unicodes to encoded &#92;uxxxx
     * with a preceding slash
     */
    private static String saveConvert(String theString) {
        int len = theString.length();
        StringBuffer outBuffer = new StringBuffer(len*2);

        for (int x = 0; x < len; x++) {
            char aChar = theString.charAt(x);

            // don't for get to escape the '/'
            if ((aChar < 0x0020) || (aChar > 0x007e) || aChar == 0x005c) {
                outBuffer.append('\\');
                outBuffer.append('u');
                outBuffer.append(toHex((aChar >> 12) & 0xF));
                outBuffer.append(toHex((aChar >>  8) & 0xF));
                outBuffer.append(toHex((aChar >>  4) & 0xF));
                outBuffer.append(toHex(aChar         & 0xF));
                continue;
            }

            outBuffer.append(aChar);
        }

        return outBuffer.toString();
    }

    /**
     * Convert a nibble to a hex character
     * @param nibble the nibble to convert.
     * @return a hex digit
     */
    private static char toHex(int nibble) {
	return hexDigit[(nibble & 0xF)];
    }

    /** A table of hex digits */
    private static final char[] hexDigit = {
	'0', '1', '2', '3', '4', '5', '6', '7', 
	'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
    };
}
