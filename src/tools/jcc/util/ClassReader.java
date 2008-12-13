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

package util;

import components.*;

import java.io.DataInputStream;
import java.io.IOException;
import java.io.File;
import java.io.InputStream;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.util.Enumeration;
import java.util.Vector;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipEntry;
import jcc.Const;

/*
 * ClassReader reads classes from a variety of sources
 * and, in all cases, creates ClassInfo structures for them.
 * It can read single class files, mclass files, entire zip
 * files or a member of a zip file.
 */
public class ClassReader {

    int verbosity;
    ConstantPool t;

    public ClassReader( ConstantPool cp, int verb ){
	t = cp;
	verbosity = verb;
    }

    private int
    getMagic( InputStream file ) throws IOException {
	DataInputStream data = new DataInputStream( file );
	int n = 0;
	file.mark( 4 );
	try {
	    n = data.readInt();
	} finally {
	    file.reset();
	}
	return n;
    }

    /*
     * To read the contents of a single, named class or mclass file.
     * The two cases are distinguished by magic number.
     * The return value is the number of classes read.
     * ClassInfo classes for the newly read classes are added
     * to the argument Vector.
     */
    public int
    readFile (String fileName, Vector done) throws IOException
    {
	InputStream infile;
	infile = new BufferedInputStream(new FileInputStream( fileName ) );
	return readStream(fileName, infile, done);

    }

    /*
     * To read the contents of an entire named zip (or Jar) file.
     * Each element that is a class or mclass file is read.
     * Others are silently ignored.
     * The return value is the number of classes read.
     * ClassInfo classes for the newly read classes are added
     * to the argument Vector.
     */
    public int
    readZip (String fileName, Vector done) throws IOException
    { 
	ZipEntry ent;
	int i = 0;
	ZipInputStream zip = 
	    new ZipInputStream(new FileInputStream(fileName));
	while ((ent = zip.getNextEntry()) != null) { 
	    String name = ent.getName();
	    if (!ent.isDirectory() && 
		(name.endsWith(".class") || name.endsWith(".mclass"))) {
		try { 
                    byte buffer[] = readZipEntry(zip);
		    i += readStream(name, new ByteArrayInputStream(buffer), 
				    done);
		} catch (IOException e) { 
		    System.out.println(Localizer.getString("classreader.failed_on", name));
		}
	    }
	}
	return i;
    }

    byte[] readZipEntry(ZipInputStream zip) throws IOException {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        byte[] buff = new byte[1024];
        int len;

        while (true) {
            len = zip.read(buff, 0, buff.length);
            if (len == -1) {
                break;
            }
            baos.write(buff, 0, len);
        }

        return baos.toByteArray();
    }

    /*
     * To read the contents of a class or mclass file from
     * a given input stream.
     * The two cases are distinguished by magic number.
     * The return value is the number of classes read.
     * ClassInfo classes for the newly read classes are added
     * to the argument Vector.
     * The inputName argument is used only for error messages.
     * This can be used for reading a zip file element or
     * from single opened file.
     */
    public int
    readStream( String inputName, InputStream infile, Vector done ) throws IOException {
	int magicNumber = getMagic( infile );

	int ndone = 0;
	if ( magicNumber == Const.JAVA_MAGIC ){
	    /*
	     * We have a solo class.
	     */
	    ClassFile f = new ClassFile( inputName, infile, verbosity>=2 );
	    if ( verbosity != 0 )
		System.out.println(
		    Localizer.getString("classreader.reading_classfile", 
		                        inputName));
	    if (f.readClassFile( t ) ){
		f.clas.externalize( t );
		done.addElement( f.clas );
		ndone+=1;
	    } else {
		throw new DataFormatException(
		    Localizer.getString("classreader.read_of_class_file", 
		                         inputName));
	    }
	} else {
	    throw new DataFormatException(
		Localizer.getString("classreader.file_has_bad_magic_number", 
				     inputName, Integer.toString(magicNumber)));
	}
	try {
	    infile.close();
	} catch ( Throwable x ){ }
	return ndone;
    }

    /*
     * To read a single, named class.
     * The return value is the number of classes read.
     * ClassInfo classes for the newly read class is added
     * to the argument Vector.
     * The class to be read is found using the ClassFileFinder,
     * which, if successful, will give us an InputStream, either
     * of a file from its search path, or of a zip file element
     * from its search path.
     */
    public int readClass( String classname, ClassFileFinder finder, Vector done ) 
    {
	InputStream f = finder.findClassFile( classname );
	if ( f == null ) return 0;
	try {
	    return readStream( classname, f, done );
	} catch ( IOException e ){
	    e.printStackTrace();
	    return 0;
	}
    }

}
