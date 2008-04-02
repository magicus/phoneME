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


package com.sun.cldchi.tools.memoryprofiler.jdwp;

import java.util.Vector;
import java.util.StringTokenizer;

/**
 * This class contains some auxillary functions that are used by
 * classes of <code>jdwp</code> package.
 */
class Tools {

    /**
     * Returns a hex representation of <code>long</code> value with
     * leading zeros.
     *
     * @param b an integer value
     * @param length a size of the integer value (2 for <code>short</code>,
     * 4 for <code>int</code>, 8 for <code>long</code>)
     * @return a hex representation of <code>long</code> value with
     * leading zeros
     */
    public static String Hex(long b, int length) {
	return Right(Long.toHexString(b), length).replace(' ', '0');
    }

    /**
     * Addes the specified substring to the beginning of the specified string
     * a few times until the length of the string becomes not less than the
     * specified length. If the length of the string is already more than
     * the specified length the resulting value if a string of asterisks with
     * the specified length.
     *
     * @param source a source string to align
     * @param length a desired length of the result string
     * @param what a substring that must be appended to the beginning of the
     * string
     * @return an aligned string
     */
    public static String PadL(String source, int length, String what) {

        if (length <= 0)
            return "";

        if (source.length() > length)
            return PadL("", length, "*");

	while (source.length() < length) 
		source = what + source;

	return source;
    }

    /**
     * Addes spaces to the beginning of the specified string
     * a few times until the length of the string becomes not less than the
     * specified length. If the length of the string is already more than
     * the specified length the resulting value if a string of asterisks with
     * the specified length.
     *
     * @param source a source string to align
     * @param length a desired length of the result string
     * @return an aligned string
     */
    public static String PadL(String source, int length) {
    	return PadL(source, length, " ");
    }

    /**
     * Appends the specified substring to the end of the specified string
     * a few times until the length of the string becomes not less than the
     * specified length. If the length of the string is already more than
     * the specified length the resulting value if a string of asterisks with
     * the specified length.
     *
     * @param source a source string to align
     * @param length a desired length of the result string
     * @param what a substring that must be appended to the end of the
     * string
     * @return an aligned string
     */
    public static String PadR(String source, int length, String what) {

        if (length <= 0)
        	return "";

        if (source.length() > length)
			return PadR("", length, "*");

		while (source.length() < length) 
			source = source + what;

		return source;
    }

    /**
     * Appends spaces to the end of the specified string
     * a few times until the length of the string becomes not less than the
     * specified length. If the length of the string is already more than
     * the specified length the resulting value if a string of asterisks with
     * the specified length.
     *
     * @param source a source string to align
     * @param length a desired length of the result string
     * @return an aligned string
     */
    public static String PadR(String source, int length) {
    	return PadR(source, length, " ");
    }

    /**
     * Truncates or increases the specified string to the specified
     * length. If the source string is less that the desired length the
     * necessary number of spaces is added to the end of the string.
     * If the source string is more
     * than the desired length the necessary number of last characters is
     * removed.
     *
     * @param source a sring to be aligned
     * @param length a desired length of the string
     * @return an aligned string
     */
    public static String Left(String source, int length) {

        if (length <= 0)
        	return "";

        if (length <= source.length())
			return source.substring(0, length);
		else
			return PadR(source, length);
    }

    /**
     * Truncates or increases the specified string to the specified
     * length. If the source string is less that the desired length the
     * necessary number of spaces is added to the beginning of the string.
     * If the source string is more
     * than the desired length the necessary number of first characters is
     * removed.
     *
     * @param source a sring to be aligned
     * @param length a desired length of the string
     * @return an aligned string
     */
    public static String Right(String source, int length) {

        if (length <= 0)
        	return "";

		if (length <= source.length())
			return source.substring(source.length() - length, source.length());
		else
			return PadL(source, length);
    }

    /**
     * Returns a content of the vector as a string where elements of the vector
     * are separated by tabs.
     *
     * @param v a vector that should be represented as a string
     * @return a string representation of the vector
     */
    public static String listVector(Vector v) {
		String s = "";
		for (int i = 0; i < v.size(); i++)
			s = s + (i + 1) + "\t" + v.elementAt(i) + "\n";
		return s;
    }

    /**
     * Returns a number of tokens in the string. Token delimiters are
     * spaces, colons, commas and tab characters.
     *
     * @param a string that consist on tokens and delimiteres.
     * @return a number of tokens in the string
     */
    public static int countBytes(String s) {
        return (new StringTokenizer(s, " :,\t")).countTokens();
    }

    /**
     * Converts a string that is a list of bytes separated by
     * spaces, colons, commas and tab characters
     * (for example, <code>01,15,44,135</code>) to byte array.
     *
     * @param s a string to be converted
     * @return an array of bytes that corresponds to the source string
     */
    public static byte[] str2bytes(String s) throws Exception {
    	
        StringTokenizer t = new StringTokenizer(s, " :,\t");

		byte[] buf = new byte[t.countTokens()];

		int k = 0;
		while (t.hasMoreTokens())
			buf[k++] = Integer.decode(t.nextToken()).byteValue();

    	return buf;
    }

    /**
     * Suspends the current thread for the specified number of
     * milliseconds.
     *
     * @param delay suspend time in milliseconds
     */
    public static void wait(int delay) {
		try {
			Thread.sleep(delay);
		}
		catch (Exception e) {}
    }
}
