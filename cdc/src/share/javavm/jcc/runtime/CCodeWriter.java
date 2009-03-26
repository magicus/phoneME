/*
 * @(#)CCodeWriter.java	1.14 06/10/10
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
 *
 */

package runtime;
import util.BufferedPrintStream;

/*
 * An I/O abstraction that adds a few often-used function
 * to the basic PrintStream. Implemented on top of the BufferedPrintStream.
 */
public class CCodeWriter extends BufferedPrintStream {
    
    public CCodeWriter( java.io.OutputStream file ){
	super( file );
    }

    private static byte hexDigit[] = {
	(byte)'0', (byte)'1', (byte)'2', (byte)'3', 
	(byte)'4', (byte)'5', (byte)'6', (byte)'7', 
	(byte)'8', (byte)'9', (byte)'a', (byte)'b', 
	(byte)'c', (byte)'d', (byte)'e', (byte)'f'
    };
    private static byte hexPrefix[] = { (byte)'0', (byte)'x' };

    private static byte hexString[][];
    public  static byte byteString[][];
    public  static byte charString[][];

    static {
	hexString = new byte[ 0x100 ][2];
	int  n = 0;
	for( int i = 0; i < 0x10; i++ ){
	    byte u = hexDigit[i];
	    for( int j = 0; j < 0x10; j++ ){
		byte s[] = hexString[n++];
		s[0] = u;
		s[1] = hexDigit[j];
	    }
	}
	byteString = new byte[ 0x100 ][3];
	n = 0;
    outerLoop:
	for ( int i = 0; i < 3; i++ ){
	    byte a = (byte)(( i==0 )?' ':('0'+i));
	    for ( int j = 0; j <= 9; j ++ ){
		byte b = (byte)(( i==0 && j==0 )? ' ' : ('0'+j) );
		for ( int k = 0; k <= 9; k++ ){
		    byte s[] = byteString[n++];
		    s[0] = a;
		    s[1] = b;
		    s[2] = (byte)('0'+k);
		    if ( n > 255 ) break outerLoop;
		}
	    }
	}
	charString = new byte[0x100][];
	for (int i = 1; i < 256; i++) { 
	    if (i >= ' ' && i <= 0x7e) {
	        charString[i] = new byte[] {(byte)'\'', (byte)i, (byte)'\''};
	    } else { 
		charString[i] = new byte[] {(byte)'0', 
					     (byte)(((i >> 6) & 07) + '0'),
					     (byte)(((i >> 3) & 07) + '0'),
					     (byte)(((i >> 0) & 07) + '0')
					   };
	    }
	}
	charString[(int)0]    = new byte[] { (byte)'\'', (byte)'\\',
					     (byte)'0', (byte) '\'' };
	charString[(int)'\\'] = new byte[] { (byte)'\'', (byte)'\\',
					     (byte)'\\', (byte)'\'' };
	charString[(int)'\n'] = new byte[] { (byte)'\'', (byte)'\\',
					     (byte)'n', (byte) '\'' };
	charString[(int)'\r'] = new byte[] { (byte)'\'', (byte)'\\',
					     (byte)'r', (byte) '\'' };
	charString[(int)'\t'] = new byte[] { (byte)'\'', (byte)'\\',
					     (byte)'t', (byte) '\'' };
	charString[(int)'\''] = new byte[] { (byte)'\'', (byte)'\\',
					     (byte)'\'', (byte)'\'' };
    }

    public
    final void printHexInt( int v ){
	int a, b, c, d;
	a = v>>>24;
	b = (v>>>16) & 0xff;
	c = (v>>>8) & 0xff;
	d = v & 0xff;
	write(hexPrefix,0,2);
	if ( a != 0 ){
	    write( hexString[ a ], 0, 2 );
	    write( hexString[ b ], 0, 2 );
	    write( hexString[ c ], 0, 2 );
	    write( hexString[ d ], 0, 2 );
	} else if ( b != 0 ){
	    write( hexString[ b ], 0, 2 );
	    write( hexString[ c ], 0, 2 );
	    write( hexString[ d ], 0, 2 );
	} else if ( c != 0 ){
	    write( hexString[ c ], 0, 2 );
	    write( hexString[ d ], 0, 2 );
	} else {
	    write( hexString[ d ], 0, 2 );
	}
    }

    public final static byte commentLeader[] = {
	(byte)'/',(byte)'*',(byte)' ' };
    public final static byte commentTrailer[] = {
	(byte)' ',(byte)'*',(byte)'/',(byte)'\n' };

    public final void
    comment( String c ){
	write( commentLeader, 0, commentLeader.length );
	print( c );
	write( commentTrailer, 0, commentTrailer.length );
    }

}
