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

/*
 * Represents a CONSTANT_Double or CONSTANT_Long
 * Should be read or written one whole word after another.
 */
package components;
import java.io.DataInput;
import java.io.DataOutput;
import java.io.IOException;
import jcc.Const;

public
class DoubleValueConstant extends ConstantObject
{
    // The two words of the constant
    public int highVal;
    public int lowVal;

    private DoubleValueConstant( int t, int h, int l ){
	tag = t;
	highVal = h;
	lowVal = l;
	nSlots = 2;
    }

    public static ConstantObject read( int t, DataInput i ) throws IOException{
	return new DoubleValueConstant( t, i.readInt(), i.readInt() );
    }

    public void write( DataOutput o ) throws IOException{
	o.writeByte( tag );
	o.writeInt( highVal );
	o.writeInt( lowVal );
    }

    public String toString(){
	String t = (tag == Const.CONSTANT_DOUBLE)?"Double [ " : "Long [ ";
	return t + Integer.toHexString( highVal ) + " " + Integer.toHexString( lowVal ) +" ]";
    }

    public int hashCode() {
	return tag + highVal + lowVal;
    }

    public boolean equals(Object o) {
	if (o instanceof DoubleValueConstant) {
	    DoubleValueConstant d = (DoubleValueConstant) o;
	    return tag == d.tag && highVal == d.highVal && lowVal == d.lowVal;
	} else {
	    return false;
	}
    }
    
    public boolean isResolved(){ return true; }
}
