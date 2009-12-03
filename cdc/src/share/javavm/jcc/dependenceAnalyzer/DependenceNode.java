/*
 * @(#)DependenceNode.java	1.13 06/10/10
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
import java.util.Enumeration;
import util.Set;
/*
 * A DependenceNode represents an element in the dependence graph.
 * It will be extended by a more specific node type, depending on
 * whether we are doing class-level or class-member-level
 * dependence analysis.
 */
public abstract class DependenceNode{
    /*
     * A DependenceNode is in one of three states:
     *	UNANALYZED -- it has a name and may have a list of dependents.
     *		      it cannot have a dependence list. Intuitively,
     *		      we haven't looked at any class file yet, so don't
     *		      even know if it exists!
     *  ANALYZED   -- a class file has been found and analyzed. The 
     *		      dependence list is complete.
     *  ERROR	   -- tried to analyze. Could not. Generally, could not
     *		      find an appropriate class file.
     *  PRIMITIVE  -- primitive type 
     */
    public static final int UNANALYZED = 0;
    public static final int ANALYZED   = 1;
    public static final int ERROR      = 2;
    public static final int PRIMITIVE  = 3;
    public int		state(){return nodeState;}


    /*
     * The meaning of a node's name depends on the level of analysis
     * we're doing. It will be either a fully-qualified class name,
     * or it will be a member-name-with-signature using the usual
     * conventions.
     */
    public Object	name(){ return nodeName; }

    /*
     * The dependence lists are Enumerations yielding
     * DependenceArc's. See below.
     */
    public Enumeration	dependsOn() { return nodeDependsOn.elements(); }
    public Enumeration	dependedOn(){ return nodeDependedOn.elements(); }

    /*
     * The following word is for use of clients.
     * Flags such as "must load" or "must exclude" can be
     * put here.
     */
    public int		flags = 0;

    /*
     * Actual state. Package scope. Manipulated by dependenceAnalyzer.
     */
    int		nodeState = UNANALYZED;
    Object	nodeName;
    Set		nodeDependsOn;
    Set		nodeDependedOn;

    protected DependenceNode( Object myName ){
	nodeName = myName;
	nodeDependsOn = new Set();
	nodeDependedOn = new Set();
    }

}
