/*
 * @(#)MemberArcTypes.java	1.9 06/10/10
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

package dependenceAnalyzer;
public interface MemberArcTypes {
    // usual reference to a class name
    public final static int ARC_CLASS	= 1;

    // read of a field, static or instance
    public final static int ARC_READS	= 2;

    // write of a field, static or instance
    public final static int ARC_WRITES	= 3;

    // invocation of a method, static or instance
    public final static int ARC_CALLS	= 4;

    // a method either catches this class type
    // or declares to be throwing it.
    public final static int ARC_EXCEPTION = 5;

    // a class implements a Java interface
    public final static int ARC_INTERFACE = 6;
}
