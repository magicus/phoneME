/*
 * @(#)HeaderDump.java	1.19 06/10/10
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

package runtime;
import consts.Const;
import jcc.Util;
import util.*;
import components.*;
import vm.*;
import java.io.PrintStream;
import java.util.Enumeration;
import java.util.Vector;
import java.util.Hashtable;

abstract
public class HeaderDump{
    char		CDelim;
    final static char	DIR_DELIM = '/';
    final static char	INNER_DELIM = '$';

    public HeaderDump( char cdelim ){
	CDelim = cdelim;
    }

    public String filename( String classname ){
	return Util.convertToClassName( classname );
    }

    // this data is really per-header.
    // it is guarded by synchronous method access
    PrintStream	o;
    String      className;

    static String
    strsub( String src, char substitute ){
	return src.replace( DIR_DELIM, substitute).replace(INNER_DELIM, substitute );
    }

    protected boolean
    generateConsts( String classname, FieldInfo fields[] ){
	if ( fields == null ) return false;
	boolean didWork = false;
	for ( int i = 0; i < fields.length; i++ ){
	    FieldInfo f = fields[i];
	    if ( f.value == null ) continue; // not a constant.
	    String constName = classname+CDelim+f.name.string;
	    o.println("#undef "+constName );
	    ConstantObject v = f.value;
	    DoubleValueConstant dvc;
	    switch ( v.tag ){
	    case Const.CONSTANT_FLOAT:
		float fval = Float.intBitsToFloat( ((SingleValueConstant)v).value );
		o.println("#define "+constName+" "+fval+"f");
		break;
	    case Const.CONSTANT_DOUBLE:
		dvc = (DoubleValueConstant)v;
		double dval = Double.longBitsToDouble((((long)(dvc.highVal))<<32) | ((long)dvc.lowVal&0xffffffffL));
		o.println("#define "+constName+" "+dval+"D");
		break;
		
	    case Const.CONSTANT_LONG:
		dvc = (DoubleValueConstant)v;
		long lval = (((long)(dvc.highVal))<<32) | ((long)dvc.lowVal&0xffffffffL);
		o.println("#define "+constName+" "+lval+"LL");
		break;
	    case Const.CONSTANT_INTEGER:
		int ival = ((SingleValueConstant)v).value;
		if (ival == -2147483648) {
		    o.println("#define "+constName+ " (-2147483647-1)");
		} else {
		    o.println("#define "+constName+" "+ival+"L");
		}
		break;
	    default:
	    }
	    didWork = true;
	}
	return didWork;
    }

    // returns true if anything worthwhile got written,
    // else false.
    abstract public boolean
    dumpHeader( ClassInfo c, PrintStream outfile );

}
