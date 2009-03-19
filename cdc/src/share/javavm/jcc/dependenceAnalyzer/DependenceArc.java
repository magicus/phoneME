/*
 * @(#)DependenceArc.java	1.8 06/10/10
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

/*
 * A DependenceArc connects two DependenceNode structures.
 * The "from" node has a dependence on the "to" node.
 * The "type" of dependence is not defined by us here, but depends
 * on the type of analysis being done. See the DependenceAnalyzer
 * classes. An alternative design would be to use subclassing of
 * DependenceArc to indicate dependence type. Let me know.
 *
 * Generally, we will know either the "from" or the "to" from 
 * context (as we're traversing a dependsOn or dependedOn enumeration)
 * and will only need to look at the other.
 */
public class DependenceArc {
    public DependenceNode from(){ return fromNode; }
    public DependenceNode to()  { return toNode; }
    public int		  type(){ return arcType; }

    public DependenceArc( DependenceNode isFrom, DependenceNode isTo, int dependenceType ){
	fromNode = isFrom;
	toNode   = isTo;
	arcType  = dependenceType;
    }

    public boolean equals( Object o ){
	try {
	    DependenceArc other = (DependenceArc)o;
	    return ( (this.fromNode==other.fromNode) && (this.toNode==other.toNode) && (this.arcType==other.arcType) );
	} catch (Throwable t ){
	    return false;
	}
    }

    // state is all private
    // object is immutable.
    private DependenceNode fromNode;
    private DependenceNode toNode;
    private int		   arcType;
}
