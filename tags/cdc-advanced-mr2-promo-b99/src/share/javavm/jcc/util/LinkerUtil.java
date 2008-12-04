/*
 * @(#)LinkerUtil.java	1.7 06/10/10
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
/*
 * Some things I find convenient for parsing strings and
 * options containing signatures.
 */
public class LinkerUtil{

    /*
     * Read the named file.
     * Tokenize the input.
     * Returns an array of Strings which are the tokens.
     * Propegates FileNotFound if the named file cannot be opened.
     * In the input stream, the character '#' is understood
     * to start a comment which continues to end-of-line.
     */
    public static String[]
    parseOptionFile( String fname ) throws java.io.IOException {
	java.util.Vector v = new java.util.Vector();
	java.io.StreamTokenizer in;
	in = new java.io.StreamTokenizer(
	    new java.io.BufferedInputStream(
		new java.io.FileInputStream( fname ) ) );
	in.resetSyntax();
	in.eolIsSignificant( false );
	in.whitespaceChars( 0, 0x20 );
	in.wordChars( '!', '~' );
	in.commentChar('#');

	while ( in.nextToken() != java.io.StreamTokenizer.TT_EOF ){
	    v.addElement( in.sval );
	}

	int n = v.size();
	String olist[] = new String[ n ];
	v.copyInto( olist );
	return olist;
    }

    /*
     * A few constants we frequently use.
     */
    public final static String mainName = "main";
    public final static String mainSig  = "([Ljava/lang/String;)V";
    public final static String constructorName = "<init>";
    public final static String constructorSig  = "()V";
    public final static String staticInitializerName = "<clinit>";
    public final static String staticInitializerSig  = "()V";


    /*
     * Classes are often written with . as component separator.
     * But in classfiles, as internally in our programs, we use /.
     * So we often want to find all instances of . and change them
     * into /.
     */
    public static String 
    sanitizeClassname( String classname ){
	return classname.replace('.', '/').intern();
    }

    /*
     * When writing a fully-qualified method name,
     * the type signature starts with (. Exploit this
     * fact when looking for that signature.
     */
    public static int
    sigOff( String n ){
	return n.indexOf( '(' );
    }

    /*
     * When writing a fully-qualified method name, 
     * the name of the method is separated from the name
     * of the containing class by a ., or sometimes by a /
     * find the offset of that character.
     */
    public static int
    methodOff( String n ){
	int moff = n.lastIndexOf( '.' );
	if ( moff >= 0 ) return moff;
	// curses. Must work harder.
	// Cannot just say lastIndexOf('/'), as that
	// may get us into the signature.
	int ending = n.indexOf( '(' );
	if ( ending < 0 ) ending = n.length();
	moff = n.lastIndexOf( '/', ending );
	return moff;
    }


}
