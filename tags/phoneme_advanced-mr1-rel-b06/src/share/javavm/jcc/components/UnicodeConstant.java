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

package components;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import consts.Const;
import util.ValidationException;


// Class that represents a CONSTANT_UTF8 in a classfile's
// constant pool

public
class UnicodeConstant extends ConstantObject
{
    public String string;
    public String UTFstring;
    public int	  stringTableOffset;	// used by in-core output writers
    public boolean isSuffix = false;    // used by in-core output writers

    private UnicodeConstant( int t, String s ){
	tag = t;
	string = s;
	nSlots = 1;
    }

    public UnicodeConstant( String s ){
	this( Const.CONSTANT_UTF8, s );
    }

    public static ConstantObject read( int t, DataInput i ) throws IOException{
	return new UnicodeConstant( t, i.readUTF() );
    }

    public void write( DataOutput o ) throws IOException{
	o.writeByte( tag );
	o.writeUTF( string );
    }


    public int hashCode() {
	return string.hashCode();
    }

    public void incReference() {
	Throwable t;
	references++;
	t = new Error();
	System.err.println("UnicodeConstant.incReference() at ");
	t.printStackTrace();
	System.err.println();
    }

    public boolean equals(Object o) {
	if (o instanceof UnicodeConstant) {
	    UnicodeConstant a = (UnicodeConstant) o;
	    return string.equals(a.string);
	} else {
	    return false;
	}
    }

    /*
     * This method can only be called at the very end of processing,
     * just before output writing. Unlike all other constants,
     * UnicodeConstants still exist, but are not expected to be found
     * in any constant pool. Their reference counts must all be 0.
     * Their index must be -1, and they must not appear in a shared ConstantPool.
     * Because of these different requirements, we cannot call super.validate() here.
     * (Note: even if the above is wrong, their ldcReferences must always
     * be zero, as no instructions are allowed to refer to them directly!)
     * If any constraint is violated, throw a ValidationException.
     * This should stop processing and print a backtrace to the offender.
     */
    public void validate(){
	if ((references != 0) || (ldcReferences != 0)){
	    throw new ValidationException("Referenced UnicodeConstant", this);
	}
	if (index != -1){
	    throw new ValidationException("UnicodeConstant with index not -1", this);
	}
	if (shared){
	    throw new ValidationException("UnicodeConstant marked shared", this);
	}
    }

    public String toString() {
	return string;
    }

    public boolean isResolved(){ return true; }
}
