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

import java.io.Reader;

/**
 * A parsed entity input state.
 *
 * This class represents a parsed entity input state. The parser uses 
 * an instance of this class to manage input.
 */

public class Input
{
	/** The entity public identifier or null. */
	public String  pubid;

	/** The entity system identifier or null. */
	public String  sysid;

	/** The encoding from XML declaration or null */
	public String  xmlenc;

	/** The XML version from XML declaration or 0x0000 */
	public char    xmlver;

	/** The entity reader. */
	public Reader  src;

	/** The character buffer. */
	public char[]  chars;

	/** The length of the character buffer. */
	public int     chLen;

	/** The index of the next character to read. */
	public int     chIdx;

	/** The next input in a chain. */
	public Input   next;

	/**
	 * Constructor.
	 *
	 * @param buffsize The input buffer size.
	 */
	public Input(int buffsize)
	{
		chars = new char[buffsize];
		chLen = chars.length;
	}

	/**
	 * Constructor.
	 *
	 * @param buff The input buffer.
	 */
	public Input(char[] buff)
	{
		chars = buff;
		chLen = chars.length;
	}

	/**
	 * Constructor.
	 */
	public Input()
	{
	}
}
