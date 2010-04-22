/*
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.  
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

package sun.misc;
import java.io.PrintStream;

/*
 * This is just a helper class that allows for easily dealing with both the CDC
 * and J2SE versions of sun.misc.Version. The CDC version takes a "longVersion"
 * argument and the J2SE version does not. When building cvm for J2SE, we
 * override VersionHelper with a copy that does not pass on the longVersion
 * argument to sun.misc.Version.
 */
public class VersionHelper {
    public static void print(boolean longVersion) {
	sun.misc.Version.print(longVersion);
    }
}
