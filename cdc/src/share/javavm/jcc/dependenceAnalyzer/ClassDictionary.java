/*
 * @(#)ClassDictionary.java	1.14 06/10/10
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

package dependenceAnalyzer;
import util.*;
import java.io.FilenameFilter;
import java.util.*;
import java.io.InputStream;
import components.ClassInfo;

/*
 * A ClassDictionary is used by the dependence analyser
 * to encapsulate all class name lookup operations. Thus
 * it contains both a hash table and a ClassFileFinder,
 * to lookup, respectively, classes we've already read, and
 * those not yet read. We have also bundled in a FilenameFilter,
 * to know when a class should be excluded rather than found
 * and read in.
 *
 * Note that this is not a public class: it is only for use
 * within the dependenceAnalyzer package.
 */

class ClassDictionary {
    /*
     * public interfaces.
     */
    
    public
    ClassDictionary( ClassFileFinder find, FilenameFilter filt ){
	finder = find;
	filter = filt;
	cdict  = new Hashtable();
    }

    public ClassEntry lookup( String cname ){
	return (ClassEntry)(cdict.get( cname ) );
    }

    public ClassEntry lookupAdd( String cname ){
	ClassEntry ce = (ClassEntry)(cdict.get( cname ) );
	if ( ce == null ){
	    ce = new ClassEntry( cname );
	    if ( filter.accept( null, cname ) ){
		ce.flags |= ClassEntry.EXCLUDED;
	    }
	    cdict.put( cname, ce );
	}
	return ce;
    }

    public Enumeration elements(){
	return cdict.elements();
    }

    /*
     * and as a public service to ClassEntry...
     */
    ClassInfo findClassInfo( String cname ){
	InputStream fin = finder.findClassFile( cname );
	if ( fin == null ) return null;
	ClassFile cfile = new ClassFile( cname, fin, false );
	if ( ! cfile.readClassFile()){
	    cfile.dump(System.err);
	    return null;
	}
	return cfile.cinfo;
    }

    /*
     * private state parts.
     */
    private ClassFileFinder finder;
    private FilenameFilter  filter;
    private Hashtable       cdict;
}
