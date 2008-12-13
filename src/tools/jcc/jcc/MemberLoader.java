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

package jcc;

import java.io.PrintStream;
import java.io.InputStream;
import java.io.StreamTokenizer;
import java.io.IOException;
import java.util.Hashtable;
import java.util.Vector;
import java.util.Enumeration;
import util.*;
import components.*;
import util.ClassFile;

/*
 * MemberLoader is the Jld subsystem
 * which processes the -memberlist option.
 * It reads the named file,
 * it causes class files to be read if necessary,
 * it manipulates the ClassInfo structures
 * by marking members to be included.
 * and finally it expunges all references to unwanted members.
 */

public class MemberLoader {
    String fname;
    StreamTokenizer in;
    ClassFileFinder finder;
    ClassReader	    rdr;

    public MemberLoader( ClassReader r, ClassFileFinder f ){
	rdr = r;
	finder = f;
    }

    private ClassInfo enterClass( String classname, Vector v ){
	ClassInfo ce;
	classname = classname.intern();
	ce = components.ClassInfo.lookupClass( classname );
	if ( ce != null ) return ce;

	if ( rdr.readClass( classname, finder, v ) != 1 ){
	    //could not find anywhere.
	    System.err.println(Localizer.getString(
				   "memberloader.could_not_find_class", 
				    classname));
	    return null; // or worse...
	}
	ce = (ClassInfo)( v.lastElement() );
	//
	// we know that this was loaded on account of us.
	// unlike most classes, this one will start life
	// as empty, with no members assumed.
	ce.flags &= ~(ClassInfo.INCLUDE_ALL);
	ce.clearMemberFlags( ClassMemberInfo.INCLUDE );
	return ce;
    }

    private boolean
    enterMember( ClassInfo ce, String name ){
	if ( (ce.flags&ClassInfo.INCLUDE_ALL) != 0 ){
	    return true; // doesn't matter what we do anyway!
	}
	ClassMemberInfo t[];
	String sig;
	// determine if this is data or method by looking
	// at the signature, or lack thereof.
	int sigstart = name.indexOf( '(' );
	if ( sigstart == -1 ){
	    // this is data.
	    // too bad we cannot use the standard procedure
	    // that  components/FMIrefConstant.find() uses,
	    // but we cannot because the hash ID requires
	    // data type signature information, which we lack.
	    t = ce.fields;
	    sig = null;
	} else {
	    // this is code.
	    //
	    sig = name.substring( sigstart );
	    name = name.substring( 0, sigstart );
	    t = ce.methods;
	}
	int l = t.length;
	for ( int i = 0; i < l ; i++ ){
	    if ( t[i].name.string.equals( name ) ){
		if ( (sig == null) || sig.equals( t[i].type.string ) ){
		    t[i].flags |= ClassMemberInfo.INCLUDE;
		    return true; // done.
		}
	    }
	}
	System.err.println(Localizer.getString(
			       "memberloader.could_not_find_member",
			        ce.className, name));
	return false;
    }

    private void openStream( String name ) throws IOException {
	fname = name;
	in = new StreamTokenizer(
	    new java.io.BufferedInputStream(
		new java.io.FileInputStream( fname ) ) );
	in.resetSyntax();
	in.eolIsSignificant( true );
	in.whitespaceChars( 0, 0x20 );
	in.wordChars( '!', '~' );
	in.commentChar('#');
	in.ordinaryChar( '.' );

    }

    private void oops( String lmsg ) throws DataFormatException {
	String errmsg = Localizer.getString(
			    "memberloader.dependence_file_error_near_line", 
			     fname, Integer.toString(in.lineno()));
	if ( lmsg != null ){
	    errmsg += Localizer.getString(lmsg);
	}
	in = null; // when we bail, close the file.
	throw new DataFormatException( errmsg );
    }

    /*
     * Read output of JavaFilter
     */
    public void readFromFile( String fname, Vector classlist )throws DataFormatException, IOException{
	int t;
	openStream(fname);
	while ( (t = in.nextToken() ) != StreamTokenizer.TT_EOF ){
	    /*
	     * Process a line of input. It may be empty
	     * or contain only a comment.
	     * Else there must be a class name.
	     * If the class name is followed by a .,
	     * then a member name follows. The name
	     * will include a signature, if a method is named.
	     * No signature is needed for data members.
	     */
	    if ( t == StreamTokenizer.TT_EOL )
		continue;
	    String className;
	    String memberName;
	    String sig;
	    if ( t != StreamTokenizer.TT_WORD )
		oops("memberloader.missing_classname");
	    className = in.sval;
	    ClassInfo ce = enterClass( className, classlist );
	    if ( (t=in.nextToken()) == StreamTokenizer.TT_EOL ){
		continue; // class name only
	    }
	    if ( t != '.' ){
		oops( "memberloader.syntax_error" );
		continue;
	    }
	    if ( (t=in.nextToken()) != StreamTokenizer.TT_WORD ){
		oops("memberloader.malformed_member_name");
		continue;
	    }
	    memberName = in.sval;
	    /*
	     * Now that we've read the names, process it.
	     */
	    if ( ce != null )
		enterMember( ce, memberName );

	    if ( in.nextToken() != StreamTokenizer.TT_EOL )
		oops("memberloader.extra_material");
	}
	in = null; // close the file.
    }

    /*
     * Traverse the class list.
     * Delete unneeded methods!!
     */

    private int deleteMembers( ClassMemberInfo mlist[] ){
	if ( mlist == null || mlist.length == 0 )
	    return 0; // trivial case.
	int n = mlist.length;
	int ndeleted = 0;
	for ( int i = 0; i < n; i++ ){
	    if ( (mlist[i].flags&ClassMemberInfo.INCLUDE) == 0 ){
		mlist[i].index = -1; // obvious error value.
		mlist[i] = null;
		ndeleted+=1;
	    }
	}
	return ndeleted;
    }

    private static void copyNonNull( ClassMemberInfo from[], ClassMemberInfo to[] ){
	    int n = from.length;
	    int j = 0;
	    for ( int i = 0; i < n; i++ ){
		if ( from[i] != null ){
		    from[i].index = j;
		    to[j++] = from[i];
		}
	    }
    }

    private void loadMembers( ClassInfo ci ){
	int ndeleted = deleteMembers( ci.methods );
	if ( ndeleted != 0 ){
	    int n = ci.methods.length;
	    MethodInfo newlist[] = new MethodInfo[ n-ndeleted ];
	    copyNonNull( ci.methods, newlist );
	    ci.methods = newlist;
	}
	ndeleted = deleteMembers( ci.fields );
	if ( ndeleted != 0 ){
	    int n = ci.fields.length;
	    FieldInfo newlist[] = new FieldInfo[ n-ndeleted ];
	    copyNonNull( ci.fields, newlist );
	    ci.fields = newlist;
	}
    }

    /*
     * This is serious!
     * Iterate through all our classes.
     * Look at all the members
     * When I see one that doesn't have isLoaded set,
     * just delete it!
     */
    public void deleteUnwantedMembers( Vector classes )
    {
	Enumeration e = classes.elements();
	while ( e.hasMoreElements() ){
	    ClassInfo ci = (ClassInfo) e.nextElement();
	    if ( (ci.flags&ClassInfo.INCLUDE_ALL) != 0 )
		continue; // a fully-loaded class.
	    loadMembers( ci );
	}
    }

}
