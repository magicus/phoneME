/*
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

package com.sun.ukit.xml;

/**
 * A name with value pair.
 *
 * This class keeps name with value pair with additional information and 
 * supports pair chaining.
 */

public class Pair
{
	/** The pair name. */
	public String name;

	/** The pair namespace. */
	public String ns;

	/** The pair value. */
	public String value;

	/** The pair numeric value. */
	public int    num;

	/** The characters of name. */
	public char[] chars;

	/** The pair identifier. */
	public int    id;

	/** The list of associated pairs. */
	public Pair   list;

	/** The next pair in a chain. */
	public Pair   next;

	/**
	 * Copies values of the original Pair object.
	 *
	 * @param orig The original object.
	 */
	public void copyof(Pair orig)
	{
		chars = orig.chars;
		name  = orig.name;
		ns    = orig.ns;
		value = orig.value;
		num   = orig.num;
		id    = orig.id;
	}

	/**
	 * Creates a qualified name string from qualified name.
	 *
	 * @return The qualified name string.
	 */
	public String qname()
	{
		return new String(chars, 1, chars.length - 1);
	}

	/**
	 * Creates a local name string from qualified name.
	 *
	 * @return The local name string.
	 */
	public String local()
	{
		if (chars[0] != 0) {
			return new String(chars, chars[0] + 1, chars.length - chars[0] - 1);
		}
		return new String(chars, 1, chars.length - 1);
	}

	/**
	 * Creates a prefix string from qualified name.
	 *
	 * @return The prefix string.
	 */
	public String pref()
	{
		if (chars[0] != 0) {
			return new String(chars, 1, chars[0] - 1);
		}
		return "";
	}

	/**
	 * Compares two qualified name prefixes.
	 *
	 * @param qname A qualified name.
	 * @return true if prefixes are equal.
	 */
	public boolean eqpref(char[] qname)
	{
		if (chars[0] == qname[0]) {
			char len = chars[0];
			for (char i = 1; i < len; i += 1) {
				if (chars[i] != qname[i])
					return false;
			}
			return true;
		}
		return false;
	}

	/**
	 * Compares two qualified names.
	 *
	 * @param qname A qualified name.
	 * @return true if qualified names are equal.
	 */
	public boolean eqname(char[] qname)
	{
		char len = (char)chars.length;
		if (len == qname.length) {
			for (char i = 0; i < len; i += 1) {
				if (chars[i] != qname[i])
					return false;
			}
			return true;
		}
		return false;
	}
}
