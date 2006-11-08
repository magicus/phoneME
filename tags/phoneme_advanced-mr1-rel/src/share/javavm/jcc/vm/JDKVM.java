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

package vm;
import components.*;
import consts.Const;
import DataFormatException;

/*
 * This code knows information about the JDK1.x virtual machine
 * that is not really platform-specific. Extend this class to
 * make a core-image writer for your platform.
 */

public class JDKVM {

    public static boolean staticMember( ClassMemberInfo f ){
	return ( (f.access & Const.ACC_STATIC) != 0 );
    }

    public static boolean privateMember( ClassMemberInfo f ){
	return ( (f.access & Const.ACC_PRIVATE) != 0 );
    }

    public static boolean isClassType( FieldInfo f ){
	char t = f.type.string.charAt(0);
	return ( ( t == Const.SIGC_ARRAY ) || ( t == Const.SIGC_CLASS ));
    }

    public static boolean needsExtraStorage( FieldInfo f ){
        return true;
    }

    public static boolean isDataPure( FieldInfo f[] ){
	return true;
    }


}
