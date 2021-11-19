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

import java.io.ByteArrayInputStream;
import java.io.Reader;
import java.io.InputStream;
import java.io.IOException;
import java.io.EOFException;
import java.io.UTFDataFormatException;
import java.io.UnsupportedEncodingException;
import java.util.Vector;

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
public class ReaderUTF8 extends Reader
{
	private static final int maxBytesInUTF8Character = 3;

	private InputStream is;

	final private byte[]	buff = new byte[128];
	private int     		bidx = 0;
	private int     		bcnt = 0;

	/**
	 * Constructor.
	 *
	 * @param is A byte input stream.
	 */
	public ReaderUTF8(InputStream is)
	{
		this.is = is;
	}
	
	/**
	 * Fills buffer with data from the source InputStream
	 * @throws IOException 
	 */
	private void fillBuffer() throws IOException {
		bcnt -= bidx;
		if( bcnt > 0 )
			System.arraycopy(buff, bidx, buff, 0, bcnt);
		bidx = 0;
		while( buff.length - bcnt > 0 ){
			int count = is.read(buff, bcnt, buff.length - bcnt);
			if( count < 0 )
				break;
			bcnt += count;
		}
//System.out.print("fillBuffer[" + bcnt + "]: ");		
//for( int o = bidx; o < bcnt; o++) System.out.print( (char)buff[o] );
//System.out.println();
	}

	/**
	 * Reads characters into a portion of an array. This method uses internal
	 * buffer. 
	 *
	 * @param cbuf Destination buffer.
	 * @param off Offset at which to start storing characters.
	 * @param len Maximum number of characters to read.
	 * @exception IOException If any IO errors occur.
	 * @exception UnsupportedEncodingException If UCS-4 character occur in the 
	 *  stream.
	 */
	public int read(char[] cbuf, int off, int len) throws IOException
	{
		int num = 0;
		while (num < len) {
			if (bcnt - bidx < maxBytesInUTF8Character){
//System.out.println( "bcnt = " + bcnt + ", bidx = " + bidx + ", num = " + num );				
				if( bidx > bcnt )
					break;
				fillBuffer();
				if( bcnt - bidx == 0 ){
					if( num == 0 )
						return -1;
					break;
				}
			}
			char val = (char)buff[bidx++];
			if (val <= 0x7f) {
				cbuf[off++] = val;
			} else {
				switch (val & 0xf0) {
				case 0xc0:
				case 0xd0:
					cbuf[off++] = (char)(((val & 0x1f) << 6) | (buff[bidx++] & 0x3f));
					break;
	
				case 0xe0:
					cbuf[off++] = (char)(((val & 0x0f) << 12) | 
						((buff[bidx++] & 0x3f) << 6) | (buff[bidx++] & 0x3f));
					break;
	
				case 0xf0:	// UCS-4 character
					throw new UnsupportedEncodingException();
	
				default:
					throw new UTFDataFormatException();
				}
			}
			num++;
		}
		if( bidx > bcnt ){ // last input UTF-8 character is wrong 
			bidx = bcnt = 0;
			throw new EOFException();
		}
//System.out.print( "ReaderUTF8.read[" + num + "]:" );
//for( int o = off - num; o < off; o++) System.out.print( cbuf[o] );
//System.out.println();
		return num;
	}

	/**
	 * Reads a single character. This method does not use internal buffer. 
	 *
	 * @return The character read, as an integer in the range 0 to 65535 
	 *	(0x00-0xffff), or -1 if the end of the stream has been reached.
	 * @exception IOException If any IO errors occur.
	 * @exception UnsupportedEncodingException If UCS-4 character occur in the 
	 *  stream.
	 */
	public int read() throws IOException
	{
		if (bcnt - bidx < maxBytesInUTF8Character){
			fillBuffer();
			if( bcnt - bidx == 0 )
				return -1;
		}
		
		int val = buff[bidx++];
		if (val > 0x7f) {
			switch (val & 0xf0) {
			case 0xc0:
			case 0xd0:
				val = ((val & 0x1f) << 6) | (buff[bidx++] & 0x3f);
				break;
	
			case 0xe0:
				val = ((val & 0x0f) << 12) | 
						((buff[bidx++] & 0x3f) << 6) | (buff[bidx++] & 0x3f);
				break;
	
			case 0xf0:	// UCS-4 character
				throw new UnsupportedEncodingException();
	
			default:
				throw new UTFDataFormatException();
			}
		}
		if( bidx > bcnt ){ // last input UTF-8 character is wrong
			bidx = bcnt = 0;
			throw new EOFException();
		}
//System.out.println( "ReaderUTF8.read: " + (char)val );
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

	public InputStream getByteStream() {
		InputStream result = is;
		if( bidx < bcnt ){
			MultiInputStream r = new MultiInputStream();
			r.add( new ByteArrayInputStream( buff, bidx, bcnt - bidx ) );
			r.add(is);
			result = r;
		}
		is = null;
		return result;
	}
	
	static class MultiInputStream extends InputStream {
		protected Vector list = new Vector();
		
		public void add( InputStream is ){
			list.addElement(is);
		}
		
		public int read(byte[] b, int off, int len) throws IOException {
			int count = -1;
			while( list.size() != 0 && (count = ((InputStream)list.firstElement()).read(b, off, len)) < 0 )
				list.removeElementAt(0);
			return count;
		}

		final private byte[] buff = new byte[1];
		public int read() throws IOException {
			if( read( buff, 0, 1 ) < 0 )
				return -1;
			return buff[0] & 0xFF;
		}
		
	}
}
