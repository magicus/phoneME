/*
 * @(#)ClassFile.java	1.19 06/10/10
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

import components.*;
import consts.Const;

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.IOException;
import java.io.PrintStream;
import java.io.InputStream;

import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Set;

import vm.VMConfig;

/**
 * Reading of class files.
 * Which is different from reading a class from a multi-class file.
 * But not much.
 * Much of the real work is done in components.ClassInfo.
 *
 * There would be stuff for writing class files here, too, but we've
 * never been called upon to do that operation.
 *
 * Also includes a small test program for reading and dumping
 * a class file.
 */

public
class ClassFile
{
    // Input file name
    public String 	fileName;
    public boolean 	didRead = false;
    public Exception 	failureMode; // only for didRead == false.
    public boolean	verbose = false;
    public ClassInfo	cinfo;
    PrintStream   	log = System.out;
    InputStream		istream;

    // Identifies the classfile format
    int			minorID,
			majorID;

    public ClassFile(String name, boolean v) {
	this(name, null, v);
    }

    public ClassFile(String name, InputStream i, boolean v) {
	fileName = name;
	verbose	 = v;
	cinfo    = new ClassInfo(v);
	istream  = i;
    }

    // Read in the entire class file
    public boolean
    readClassFile() {
	DataInputStream in = null;
	failureMode = null;
    done:
	try {
	    int magic;
	    if ( istream == null ){
		istream = new java.io.BufferedInputStream( new java.io.FileInputStream( fileName ) );
	    }
	    in = new DataInputStream( istream );
	    if ((magic = in.readInt()) != Const.JAVA_MAGIC) {
		failureMode = new IOException(Localizer.getString("classfile.bad_magic.ioexception"));
		break done; // bad magic -- bail immediately.
	    }

	    minorID = in.readShort();
	    majorID = in.readShort();
            if (majorID < VMConfig.getMinSupportedClassfileVersion()
                || majorID > VMConfig.getMaxSupportedClassfileVersion()) {
                failureMode = new IOException(Localizer.getString(
                    "classfile.bad_major_version_number.ioexception"));
                break done; // bad major version -- bail immediately. 
            }

            if (majorID == VMConfig.getMaxSupportedClassfileVersion()
                && minorID > VMConfig.getMaxSupportedClassfileMinorVersion()) {
                failureMode = new IOException(Localizer.getString(
                    "classfile.bad_minor_version_number.ioexception"));
                break done; // bad minor version -- bail immediately.
            }
	    if (verbose) {
		log.println(Localizer.getString("classfile._file", fileName)+
			    Localizer.getString("classfile._magic/major/minor",
		                                Integer.toHexString( magic ),
		                                Integer.toHexString( majorID ),
		                                Integer.toHexString( minorID )));
	    }
	    cinfo.setVersionInfo(majorID, minorID);
	    cinfo.read(in, true);
	    didRead = true;
	} catch ( Exception e ){
	    failureMode = e;
	}
	if (in != null){
	    try{
		in.close();
	    } catch ( IOException e ){
		if (didRead==true){
		    didRead=false;
		    failureMode=e;
		}
	    }
	}
	return didRead;
    }

    public void
    dump( PrintStream o ){
	o.println(Localizer.getString("classfile.file", fileName));
	if ( ! didRead ){
	    o.println(Localizer.getString(
			  "classfile.could_not_be_read_because_of", 
			  failureMode));
	    if ( failureMode != null ) 
		failureMode.printStackTrace( o );
	    return;
	}
	cinfo.dump( o );
    }

    /*
     * Simple test program.
     * Parameters: list of class files to read and process.
     */
    public static void main( String clist[] ){
	for( int i = 0; i < clist.length; i++ ){
	    ClassFile cfile = new ClassFile( clist[i], true );
	    if (cfile.readClassFile() != true ){
		System.out.println(Localizer.getString(
				        "classfile.read_of",
				        clist[i]));
		cfile.dump( System.out );
		continue;
	    }
	    cfile.dump( System.out );
	}
    }

}
