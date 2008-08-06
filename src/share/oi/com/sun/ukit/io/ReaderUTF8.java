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

package com.sun.ukit.io;

import java.io.Reader;
import java.io.InputStream;
import java.io.IOException;
import java.io.EOFException;
import java.io.UTFDataFormatException;
import java.io.UnsupportedEncodingException;

/**
 * UTF-8 transformed UCS-2 character stream reader.
 *
 * This reader converts UTF-8 transformed UCS-2 characters to Java characters.
 * The UCS-2 subset of UTF-8 transformation is described in RFC-2279 #2 
 * "UTF-8 definition":
 *  0000 0000-0000 007F   0xxxxxxx
 *  0000 0080-0000 07FF   110xxxxx 10xxxxxx
 *  0000 0800-0000 FFFF   1110xxxx 10xxxxxx 10xxxxxx
 *
 * This reader will return incorrect last character on broken UTF-8 stream. 
 */
public class ReaderUTF8
	extends Reader
{
	private InputStream is;

	private byte[]  buff;
	private int     bidx = 0;
	private int     bcnt = 0;

	/**
	 * Constructor.
	 *
	 * @param is A byte input stream.
	 */
	public ReaderUTF8(InputStream is)
	{
		this.is = is;
		buff    = new byte[64];
	}

	/**
	 * Reads characters into a portion of an array. This method uses internal
	 * buffer. After the first call to this method, method read which returns 
	 * next character MUST NOT be called.
	 *
	 * @param cbuf Destination buffer.
	 * @param off Offset at which to start storing characters.
	 * @param len Maximum number of characters to read.
	 * @exception IOException If any IO errors occur.
	 * @exception UnsupportedEncodingException If UCS-4 character occur in the 
	 *  stream.
	 */
	public int read(char[] cbuf, int off, int len)
		throws IOException
	{
		int num = 0;
		while (num < len) {
			if (bidx >= bcnt) {
				if ((bcnt = is.read(buff, 0, buff.length)) < 0)
					return (num != 0)? num: -1;
				bidx = 0;
			}
			char val = (char)buff[bidx++];
			if (val <= 0x7f) {
				cbuf[off++] = val;
			} else {
				switch (val & 0xf0) {
				case 0xc0:
				case 0xd0:
					cbuf[off++] = (char)(((val & 0x1f) << 6) | (getch() & 0x3f));
					break;
	
				case 0xe0:
					cbuf[off++] = (char)(((val & 0x0f) << 12) | 
						((getch() & 0x3f) << 6) | (getch() & 0x3f));
					break;
	
				case 0xf0:	// UCS-4 character
					throw new UnsupportedEncodingException();
	
				default:
					throw new UTFDataFormatException();
				}
			}
			num++;
		}
		return num;
	}

	/**
	 * Reads a single character. This method does not use internal buffer. 
	 * This method MUST NOT be called once read method which fills an array 
	 * is called.
	 *
	 * @return The character read, as an integer in the range 0 to 65535 
	 *	(0x00-0xffff), or -1 if the end of the stream has been reached.
	 * @exception IOException If any IO errors occur.
	 * @exception UnsupportedEncodingException If UCS-4 character occur in the 
	 *  stream.
	 */
	public int read()
		throws IOException
	{
		int  val;
		if ((val = is.read()) < 0)
			return -1;
		if (val > 0x7f) {
			switch (val & 0xf0) {
			case 0xc0:
			case 0xd0:
				val = ((val & 0x1f) << 6) | (is.read() & 0x3f);
				break;
	
			case 0xe0:
				val = ((val & 0x0f) << 12) | 
					((is.read() & 0x3f) << 6) | (is.read() & 0x3f);
				break;
	
			case 0xf0:	// UCS-4 character
				throw new UnsupportedEncodingException();
	
			default:
				throw new UTFDataFormatException();
			}
		}
		return val;
	}

	/**
	 * Closes the stream.
	 *
	 * @exception IOException If any IO errors occur.
	 */
	public void close()
		throws IOException
	{
		is.close();
	}

	/**
	 * Reads next character from the stream buffer.
	 *
	 * @exception IOException If any IO errors occur.
	 */
	private char getch()
		throws IOException
	{
		if (bidx >= bcnt) {
			if ((bcnt = is.read(buff, 0, buff.length)) <= 0)
				throw new EOFException();
			bidx = 0;
		}
		return (char)buff[bidx++];
	}
}
