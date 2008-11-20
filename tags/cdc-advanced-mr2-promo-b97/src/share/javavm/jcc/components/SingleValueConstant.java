/*
 * @(#)SingleValueConstant.java	1.9 06/10/10
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

// Represents a CONSTANT_Integer or CONSTANT_Float
package components;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import consts.Const;

public
class SingleValueConstant extends ConstantObject
{
    public int value;

    private SingleValueConstant(int tag, int v) {
	super(tag);
	value = v;
    }

    public SingleValueConstant(int ival) {
	this( Const.CONSTANT_INTEGER, ival );
    }

    /**
     * Factory method to construct a SingleValueConstant instance from the
     * constant pool data stream.  This method is only called from the
     * ConstantObject.readObject() factory.
     */
    static ConstantObject read(int tag, DataInput i) throws IOException {
	return new SingleValueConstant(tag, i.readInt());
    }

    public void write( DataOutput o ) throws IOException{
	o.writeByte( tag );
	o.writeInt( value );
    }

    public String toString(){
	return (tag==Const.CONSTANT_FLOAT)? ("Float: "+Float.intBitsToFloat(value) ):
		("Int: "+Integer.toHexString( value ));
    }

    public int hashCode() {
	return tag + value;
    }


    public boolean equals(Object o) {
	if (o instanceof SingleValueConstant) {
	    SingleValueConstant s = (SingleValueConstant) o;
	    return tag == s.tag && value == s.value;
	} else {
	    return false;
	}
    }

    public boolean isResolved(){ return true; }
}
