/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

import components.*;
import util.*;
import jcc.*;
import consts.*;

import java.io.PrintStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.BufferedInputStream;
import java.io.FileOutputStream;
import java.io.BufferedOutputStream;
import java.util.Vector;
import java.util.Hashtable;

class Strip {
    static Hashtable stripCodeAttrs;
    static Hashtable stripClassAttrs;
    
    static {
	stripCodeAttrs = new Hashtable();
	stripCodeAttrs.put(/*NOI18N*/"LineNumberTable", /*NOI18N*/"LineNumberTable");
	stripCodeAttrs.put(/*NOI18N*/"LocalVariableTable", /*NOI18N*/"LocalVariableTable");

	stripClassAttrs = new Hashtable();
	stripClassAttrs.put(/*NOI18N*/"SourceFile", /*NOI18N*/"SourceFile");
	stripClassAttrs.put(/*NOI18N*/"InnerClasses", /*NOI18N*/"InnerClasses");
    }

    private class StrippableMclass extends Strippable {
	ClassInfo[] classes;
	ConstantPool t;
	MultiClass m;

	StrippableMclass(String fName, ClassInfo[] classes, ConstantPool t) {
	    super(fName);
	    this.classes = classes;
	    this.t = t;
	    this.m = new MultiClass(t, classes, false);
	}

	void strip() {
	    for (int k = 0; k < classes.length; k++) {
		super.strip(classes[k]);
	    }
	}

	void writeOut(DataOutputStream out) throws IOException {
	    m.outputMultiClass(out);
	}
    }

    private class StrippableClass extends Strippable {
	ClassInfo clas;

	StrippableClass(String fName, ClassInfo clas) {
	    super(fName);
	    this.clas = clas;
	}

	void strip() {
	    super.strip(clas);
	}

	void writeOut(DataOutputStream out) throws IOException {
	    out.writeInt(Const.JAVA_MAGIC);
	    out.writeShort(Const.JAVA_MINOR_VERSION);
	    out.writeShort(Const.JAVA_VERSION);
	    clas.write(out);
	}
    }

    private abstract class Strippable {
	String fName;
	String outName;

	Strippable(String fName) {
	    this.fName = fName;
	    this.outName = /*NOI18N*/"." + fName + /*NOI18N*/".strip";
	}

	void output() throws IOException {
	    DataOutputStream out = new DataOutputStream
		(new BufferedOutputStream
		 (new FileOutputStream (outName)));
	    writeOut(out);
	    out.close();
	    File outf = new File(outName);
	    File orig = new File(fName);
	    if (!orig.delete() || !outf.renameTo(orig)) {
		throw new IOException(Localizer.getString(
				         "strip.could_not_modify.ioexception",
					 fName));
	    }
	}

	void strip(ClassInfo c) {
	    for (int k = 0; k < c.methods.length; k++) {
		MethodInfo m = c.methods[k];
		Attribute[] ca = m.codeAttributes;
		if (ca == null) {
		    continue;
		}
		int caLen = ca.length;
		Vector v = new Vector();
		int delLen = 0;
		//
		// Filter out the unwanted attributes
		//
		for (int i = 0; i < caLen; i++) {
		    Attribute a = ca[i];
		    if (stripCodeAttrs.get(a.name.string) != null) {
			delLen += a.length();
		    } else {
			v.addElement(a);
		    }
		}
		//
		// Now substitute the filtered attribute array
		// in all the right places
		//
		int filteredLen = v.size();
		if (filteredLen < caLen) {
		    Attribute[] newa = new Attribute[filteredLen];
		    v.copyInto(newa);
		    m.codeAttributes = newa;
		    m.codeAttribute.codeAttributes = newa;
		    m.codeAttribute.length -= delLen;
		}
	    }
	    
	    Attribute[] ca = c.classAttributes;
	    if (ca == null) {
		return;
	    }
	    int caLen = ca.length;
	    Vector v = new Vector();
	    //
	    // Filter out the unwanted attributes
	    //
	    for (int i = 0; i < caLen; i++) {
		Attribute a = ca[i];
		if (stripClassAttrs.get(a.name.string) == null) {
		    v.addElement(a);
		}
	    }
	    //
	    // Now substitute the filtered attribute array
	    // in all the right places
	    //
	    int filteredLen = v.size();
	    if (filteredLen < caLen) {
		if (filteredLen == 0) {
		    c.classAttributes = null;
		} else {
		    Attribute[] newa = new Attribute[filteredLen];
		    v.copyInto(newa);
		    c.classAttributes = newa;
		}
	    }
	}

	abstract void strip();
	abstract void writeOut(DataOutputStream out) throws IOException;
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

    private boolean
    readFile( String inputName, Strippable[] done, int k ){
	int magicNumber = 0;
	InputStream infile;
	try {
	    infile = new BufferedInputStream
		(new FileInputStream(inputName));
	    magicNumber = getMagic( infile );
	} catch ( IOException e ){
	    System.out.println(Localizer.getString(
			       "strip.could_not_read_class-or-mclass_file",
			       inputName));
	    return false;
	}
	if ( magicNumber == Const.JAVA_MAGIC ){
	    /*
	     * We have a solo class.
	     */
	    ClassFile f = new ClassFile(inputName, infile, false);
	    if (f.readClassFile(null) ){
		done[k] = new StrippableClass(inputName, f.clas);
	    } else {
		System.out.println(Localizer.getString(
				       "strip.read_of_class_file",
					inputName));
		f.dump( System.err );
		return false;
	    }
	} else if ( magicNumber == MultiClass.MULTICLASS_MAGIC ){
	    /*
	     * We have an mclass file.
	     */
	    MultiClass m = new MultiClass( inputName, infile, null, false );
	    if (m.readMultiClassFile() ){
		ClassInfo c[] = m.classes;
		int n = c.length;
		ConstantPool t = m.consts;

		done[k] = new StrippableMclass(inputName, c, t);
	    } else {
		System.out.println(Localizer.getString(
				   "strip.read_of_multi-class_file",
				   inputName));
		return false;
	    }
	} else {
	    System.err.println(Localizer.getString(
				   "strip.file_has_bad_magic_number", 
				   inputName, Integer.toString(magicNumber)));
	    return false;
	}
	return true;
    }

    public boolean process( String clist[] ){
	Strippable[] modules = new Strippable[clist.length];
	for( int i = 0; i < clist.length; i++ ) {
	    if (!readFile(clist[i], modules, i)) {
		return false;
	    }
	}
	for (int i = 0; i < modules.length; i++) {
	    Strippable s = modules[i];
	    try {
		s.strip();
		s.output();
	    } catch (IOException e) {
		System.out.println(Localizer.getString(
				   "strip.could_not_strip_class-or-mclass_file",
				   s.fName));
		e.printStackTrace();
		return false;
	    }
	}
	return true;
    }

    public static void main( String clist[] ){
	boolean success = new Strip().process( clist );
	if ( !success ){
	    System.out.flush();
	    System.err.flush();
	    System.exit(1);
	}
	return;
    }
}

