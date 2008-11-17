/*
 * @(#)FieldConstant.java	1.9 06/10/10
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
import consts.Const;

/**
 * Represents a CONSTANT_Fieldref stored in a constant pool.
 */
public
class FieldConstant extends FMIrefConstant
{
    // Reference to the actual field (need it for index into the
    // fieldblock for the field reference).
    boolean	didLookup;
    FieldInfo	theField;

    private FieldConstant() { super(Const.CONSTANT_FIELD); }

    public FieldConstant(ClassConstant c, NameAndTypeConstant s) {
        super(Const.CONSTANT_FIELD, c, s);
    }

    /**
     * Factory method to construct a FieldConstant instance from the
     * constant pool data stream.  This method is only called from the
     * ConstantObject.readObject() factory.
     */
    static ConstantObject read(DataInput in) throws IOException {
	FieldConstant f = new FieldConstant();
	f.readIndexes(in);
	return f;
    }

    public FieldInfo find(){
	if ( ! didLookup ){
	    theField = (FieldInfo)super.find(tag);
	    didLookup = true;
	}
	return theField;
    }

}
