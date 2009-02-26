/*
 * @(#)ClassDependenceNode.java	1.10 06/10/10
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
import util.Set;

/*
 * A concrete subclass of DependenceNode for use by
 * the ClassDependenceAnalyzer. This is not used by JavaFilter or
 * by JavaCodeCompact. It is for use by other dependence analysis
 * tools, which might operate on a more course-grained basis.
 * See DependenceNodeAnalyzer for a full example of use.
 */

public
class ClassDependenceNode extends DependenceNode {
    // flag values
    public static int EXCLUDED = 1;
    public static int INCLUDED = 2;

    public ClassDependenceNode superClass;
    public Set		       interfacesImplemented;

    /**
     * The name of a ClassDependenceNode is simply the
     * fully-qualified class name as a String, using
     * / as name component separator.
     * (Other DependenceNode types may have more intricate names.)
     */
    public ClassDependenceNode( Object classname ){
	super( classname );
    }
}
