/*
 * @(#)StringConstant.java	1.11 06/10/10
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

package components;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import util.Assert;
import util.DataFormatException;
import consts.Const;

// Represents a CONSTANT_String.  These constants are somewhat
// complicated in that we must fake string bodies for them.

public
class StringConstant extends ConstantObject
{
    // Filled in by Clas.resolveConstants()
    public UnicodeConstant str;

    public int strIndex;	// unresolved index of UnicodeConstant

    // used by core-image writer.
    public int unicodeIndex = -1;
    public int unicodeOffset= 0;
    public boolean dataWritten = false;
    public boolean handleWritten = false;


    private StringConstant(int index) {
	super(Const.CONSTANT_STRING);
	strIndex = index;
    }

    /**
     * Factory method to construct a StringConstant instance from the
     * constant pool data stream.  This method is only called from the
     * ConstantObject.readObject() factory.
     */
    static ConstantObject read(DataInput i) throws IOException {
	return new StringConstant(i.readUnsignedShort());
    }

    public void flatten(ConstantPool cp) {
	if (isFlat) return;
        Assert.disallowClassloading();
	str = (UnicodeConstant)cp.elementAt(strIndex);
	isFlat = true;
        Assert.allowClassloading();
    }

    public void write(DataOutput o) throws IOException {
	o.writeByte(tag);
	if (isFlat) {
	    o.writeShort(str.index);
	} else {
	    throw new DataFormatException("unresolved StringConstant");
	    //o.writeShort(strIndex);
	}
    }

    public String toString() {
	return "String: "+ (isFlat ? str.string : ("[ "+strIndex+" ]"));
    }

    public void incReference() {
	super.incReference();
	//str.incReference();
    }

    public void decReference() {
	super.decReference();
	//str.decReference();
    }

    /*
     * This method can only be called at the very end of processing,
     * just before output writing. Make sure the str constant
     * obeys the validation constraints as described in ConstantObject.validate.
     * Call ConstantObject.validate to make sure its fields do, too.
     */
    public void
    validate(){
	super.validate();
	str.validate();
    }
    public int hashCode() {
	return tag + str.string.hashCode();
    }


    public boolean equals(Object o) {
	if (o instanceof StringConstant) {
	    StringConstant s = (StringConstant) o;
	    return str.string.equals(s.str.string);
	} else {
	    return false;
	}
    }

    public boolean isResolved(){ return true; }
}
