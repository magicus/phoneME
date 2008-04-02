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
import jcc.Const;

// Represents a CONSTANT_Field

public
class FieldConstant extends FMIrefConstant
{
    // Reference to the actual field (need it for index into the
    // fieldblock for the field reference).
    boolean	didLookup;
    FieldInfo	theField;

    FieldConstant( int t ){ tag = t; }

    public FieldConstant( ClassConstant c, NameAndTypeConstant s ){
        super( Const.CONSTANT_FIELD, c, s );
    }

    public static ConstantObject
    read( int t, DataInput in ) throws IOException {
	FieldConstant f = new FieldConstant( t );
	f.read( in );
	return f;
    }

    public FieldInfo find(){
	if ( ! didLookup ){
	    theField = (FieldInfo)super.find(false);
	    didLookup = true;
	}
	return theField;
    }

}
