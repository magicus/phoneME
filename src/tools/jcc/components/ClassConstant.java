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

package components;

import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import util.DataFormatException;
import jcc.Const;

// ClassConstant represents a CONSTANT_Class stored in a constant pool.

public
class ClassConstant extends ConstantObject
{
    public UnicodeConstant name;
    
    private boolean	 didLookup;
    private ClassInfo	 theClass;

    public int nameIndex;

    ClassConstant( int t, int n ){ 
	tag = t;
	nameIndex = n;
	nSlots = 1;
    }

    public ClassConstant( UnicodeConstant n ){
	tag = Const.CONSTANT_CLASS;
	name = n;
	nSlots = 1;
	resolved = true;
    }

    // Read from constant pool.
    public static
    ClassConstant read( int tag, DataInput in ) throws IOException {
	return new ClassConstant( tag, in.readUnsignedShort() );
    }

    public void
    resolve( ConstantObject table[] ){
	if (resolved) return;
	name = (UnicodeConstant)table[nameIndex];
	resolved = true;
    }

    public void externalize( ConstantPool p ){
	name = (UnicodeConstant)p.add( name );
    }

    // Write out reference to the ClassClass data structure
    public void write( DataOutput o ) throws IOException{
	o.writeByte( tag );
	if ( ! resolved )
	    throw new DataFormatException("unresolved ClassConstant");
	int x = name.index;
	if ( x == 0 )
	    throw new DataFormatException("0 name index for "+name.string);
	o.writeShort( x );
    }

    public String toString(){
	if ( resolved )
	    return "Class: "+name.toString();
	else
	    return "Class: ["+nameIndex+"]";
    }

    public int hashCode() {
	return tag + name.hashCode();
    }

    public void incReference() {
	references++;
	name.incReference();
    }

    public void decReference() {
	references--;
	name.decReference();
    }

    public boolean equals(Object o) {
	if (o instanceof ClassConstant) {
	    ClassConstant c = (ClassConstant) o;
	    return name.string.equals(c.name.string);
	} else {
	    return false;
	}
    }

    public ClassInfo find(){
	if ( !didLookup ){
	    theClass = ClassInfo.lookupClass( name.string );
	    didLookup = true;
	}
	return theClass; // which may be null
    }

    public void forget(){
	didLookup = false;
	theClass = null;
    }

    public boolean isResolved(){ return find() != null; }
}
