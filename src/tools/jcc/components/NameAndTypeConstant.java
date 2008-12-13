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
// This class represents a CONSTANT_NameAndType

public
class NameAndTypeConstant extends ConstantObject {
    // Filled in by Clas.resolveConstants()
    public UnicodeConstant name;
    public UnicodeConstant type;

    // Read in from classfile
    int nameIndex;
    int typeIndex;

    // The unique ID for this name and type (from Std2ID)
    public int ID = 0;

    private NameAndTypeConstant( int t, int ni, int ti ){
	tag = t;
	nameIndex = ni;
	typeIndex = ti;
	nSlots = 1;
    }

    public NameAndTypeConstant( UnicodeConstant name, UnicodeConstant type ){
	tag = Const.CONSTANT_NAMEANDTYPE;
	this.name = name;
	this.type = type;
	nSlots = 1;
	resolved = true;
    }

    public static ConstantObject read( int t, DataInput i ) throws IOException{
	return new NameAndTypeConstant( t, i.readUnsignedShort(), i.readUnsignedShort() );
    }

    public void resolve( ConstantObject table[] ){
	if ( resolved ) return;
	name = (UnicodeConstant)table[nameIndex];
	type = (UnicodeConstant)table[typeIndex];
	resolved = true;
    }

    public void externalize( ConstantPool p ){
	name = (UnicodeConstant)p.add( name );
	type = (UnicodeConstant)p.add( type );
    }

    public void write( DataOutput o ) throws IOException{
	o.writeByte( tag );
	if ( resolved ){
	    o.writeShort( name.index );
	    o.writeShort( type.index );
	} else {
	    throw new DataFormatException("unresolved NameAndTypeConstant");
	    //o.writeShort( nameIndex );
	    //o.writeShort( typeIndex );
	}
    }

    public String toString(){
	if ( resolved ){
	    return "NameAndType: "+name.string+" : "+type.string;
	}else{
	    return "NameAndType[ "+nameIndex+" : "+typeIndex+" ]";
	}
    }

    public void incReference() {
	references++;
	name.incReference();
	type.incReference();
    }

    public void decReference() {
	references--;
	name.decReference();
	type.decReference();
    }

    public int hashCode() {
	return tag + name.string.hashCode() + type.string.hashCode();
    }
    

    public boolean equals(Object o) {
	if (o instanceof NameAndTypeConstant) {
	    NameAndTypeConstant n = (NameAndTypeConstant) o;
	    return name.string.equals(n.name.string) &&
		   type.string.equals(n.type.string);
	} else {
	    return false;
	}
    }

    public boolean isResolved(){ return true; }
}
