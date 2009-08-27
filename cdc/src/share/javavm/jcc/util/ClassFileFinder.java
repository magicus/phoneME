/*
 * @(#)ClassFileFinder.java	1.14 06/10/10
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
/*
 * Collect classpath components, usually from the command line.
 * Use this list to search for class files.
 * In addition to the classic, Unix-style directory names,
 * we also accept zip files.
 */

package util;
import java.util.Vector;
import java.io.File;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.io.InputStream;
import java.io.IOException;
import java.util.zip.*;

/*
 * This auxiliary class represents a component of the search
 * path, which can be either a directory, a zip file, or
 * neither of the above (in which case it is ignored).
 */
class searchPathComponent {
    public String		name;
    public boolean		isDirectory;
    public boolean		isZip;
    public ZipFile	 	zipfile; // only if isZip

    public searchPathComponent 	next; // chain.

    searchPathComponent( String name ){
	this.name = name;
	File thisFile = new File( name );
	if ( thisFile.exists() ){
	    if ( thisFile.isDirectory() ){
		this.isDirectory = true;
	    } else {
		// try to open it as a zip file.
		// if the open succeeds, it is a zip file.
		// else we are in error.
		try {
		    this.zipfile = new ZipFile( thisFile );
		    this.isZip   = true;
		}catch( java.io.IOException e ){
		    return;
		}
	    }
	}
	//
	// else it is none of the above.
	// this is an error.
	// we do not prevent putting erroneous components
	// on the path.
	//
    }

    InputStream find( String name ){
	try {
	    if ( isDirectory ){
		File thisFile = new File( this.name, name );
		if ( thisFile.exists() && thisFile.canRead() ){
		    return new BufferedInputStream( new FileInputStream( thisFile ) );
		}
	    } else if ( isZip ){
		ZipEntry ze = this.zipfile.getEntry( name );
		if ( ze != null )
		    return new BufferedInputStream( this.zipfile.getInputStream( ze ) );
	    }
	} catch ( IOException e ){
	    return null;
	}
	return null;
    }
    
    public String toString(){
	return
	    Localizer.getString(isDirectory?"classfilefinder.directory":isZip?"classfilefinder.zipfile":"classfilefinder.path_noop", this.name);
    }
    
}

public class ClassFileFinder {

    private searchPathComponent    searchPath;
    private searchPathComponent    searchPathEnd;

    public  boolean	verbose = false;


    /*
     * take a single place to look.
     * add it to the searchPath.
     */
    private void addSearchPathEntry( String pathComponent ){
	searchPathComponent t = new searchPathComponent( pathComponent );
	if ( searchPathEnd == null ){
	    searchPath = searchPathEnd = t;
	} else {
	    searchPathEnd.next = t;
	    searchPathEnd = t;
	}
    }

    /*
     * take a colon-separated list of places to look.
     * parse them out to an array of strings, which gets
     * added to any searchPath we already have.
     */
    public void addToSearchPath( String pathString ){
	int curbegin = 0;
	int pl = pathString.length();
	char sepChar = File.pathSeparatorChar;
	int colon;
	while( (colon = pathString.indexOf(sepChar,curbegin) ) != -1 ){
	    addSearchPathEntry( pathString.substring( curbegin, colon ) );
	    curbegin = colon+1;
	}
	if ( curbegin < pl ){
	    addSearchPathEntry( pathString.substring( curbegin, pl ) );
	}
    }

    public InputStream findClassFile( String cname ){
	cname = cname+".class";
	for ( searchPathComponent spc = searchPath; spc != null; spc = spc.next ){
	    InputStream s = spc.find( cname );
	    if ( s != null ){
		if ( verbose ){
		    System.out.print(Localizer.getString(
			      	         "classfilefinder.foundin", 
					  cname, spc));
		}
		return s;
	    }
	    if ( verbose ){
		System.out.print(Localizer.getString(
			             "classfilefinder.notfoundin",
				     cname, spc));
	    }
	}
	return null;
    }
}
