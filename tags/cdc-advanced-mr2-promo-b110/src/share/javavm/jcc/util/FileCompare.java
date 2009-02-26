/*
 * @(#)FileCompare.java	1.7 06/10/10
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

package util;
import java.io.*;

public class FileCompare{

public static boolean debug = false;

private static void debugPrint( File a, File b, String msg ){
    System.err.print(a.getPath());
    System.err.print(", ");
    System.err.print(b.getPath());
    System.err.print(" ");
    System.err.println( msg );
}

public static boolean cmp( File a, File b ) {
    if ( !a.isFile() || !b.isFile()){
	if ( debug ){
	    debugPrint(a, b, "not both plain files");
	}
	return false; // NotAFile compares unequal.
    }
    if ( a.length() != b.length() ){
	if ( debug ){
	    debugPrint(a, b, "different lengths");
	}
	return false; // different lenghts cannot be the same.
    }
    try {
	FileInputStream astream = null;
	FileInputStream bstream = null;
	try {
	    astream = new FileInputStream( a );
	    bstream = new FileInputStream( b );
	    long flength = a.length(); // == b.length(), remember?
	    int bufsize = (int)Math.min( flength, 1024 );
	    byte abuf[] = new byte[ bufsize ];
	    byte bbuf[] = new byte[ bufsize ];
	    long n = 0;
	    while ( n < flength ){
		int naread = astream.read( abuf );
		int nbread = bstream.read( bbuf );
		if ( naread != nbread ) return false; // oops.
		for ( int i = 0; i < naread; i++ ){
		    if ( abuf[i] != bbuf[i] ){
			if ( debug ){
			    debugPrint(a, b, "differ at byte "+(n+i) );
			}
			return false;
		    }
		}
		n += naread;
	    }
	} finally {
	    if ( astream != null ) astream.close();
	    if ( bstream != null ) bstream.close();
	}
    } catch ( IOException e ){
	e.printStackTrace();
	return false;
    }
    if ( debug ){
	debugPrint(a, b, "are the same");
    }
    return true;
}

public static boolean cpy( File a, File b ) {
    try {
	FileInputStream astream  = null;
	FileOutputStream bstream = null;
	try {
	    astream = new FileInputStream( a );
	    bstream = new FileOutputStream( b );
	    long flength = a.length();
	    int bufsize = (int)Math.min( flength, 1024 );
	    byte buf[] = new byte[ bufsize ];
	    long n = 0;
	    while ( n < flength ){
		int naread = astream.read( buf );
		bstream.write( buf, 0, naread );
		n += naread;
	    }
	} finally {
	    if ( astream != null ) astream.close();
	    if ( bstream != null ) bstream.close();
	}
    } catch ( IOException e ){
	e.printStackTrace();
	return false;
    }
    return true;
}

public static void conditionalCopy( File fromFile, File toFile ){
    if ( ! cmp( fromFile, toFile ) )
	cpy( fromFile, toFile );
}

public static void main( String args[] ){
    debug = true;
    conditionalCopy( new File( args[0] ), new File( args[1] ) );
}

}
