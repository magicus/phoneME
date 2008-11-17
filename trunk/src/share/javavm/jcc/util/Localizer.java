/*
 * @(#)Localizer.java	1.9 06/10/10
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

package util;
import text.*;

import java.text.MessageFormat;

/*
 * The source of all localized messages for all the programs
 * and classes that used the JCCMessage bundle. Since there
 * are multiple programs involved, this facility is a separate class,
 * rather than part of any one of them. It is assumed that locale
 * is uniform over all classes of these programs, so does not need
 * to be determined per-class or per-object.
 */

public class Localizer {
    static      PI18n localizer = new PI18n("JCCMessage");

    public static String getString( String key ){
	return localizer.getString( key );
    }

    public static String getString (String key, Object[] values) { 
        return new MessageFormat(localizer.getString(key)).format(values);
    }

    public static String getString(String key, Object value) {
        return getString(key, new Object[]{value});
    }

    public static String getString(String key, Object value1, Object value2) {
        return getString(key, new Object[]{value1, value2});
    }

    public static String getString(String key, Object value1, 
				   Object value2, Object value3) {
        return getString(key, new Object[]{value1, value2, value3});
    }
}
