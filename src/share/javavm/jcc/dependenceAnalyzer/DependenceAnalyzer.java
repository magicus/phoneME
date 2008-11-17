/*
 * @(#)DependenceAnalyzer.java	1.14 06/10/10
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
import util.ClassFileFinder;
import util.Set;
import java.util.Enumeration;
import java.util.Dictionary;
import java.util.Hashtable;

/*
 * A DependenceAnalyzer is the engine that actually does the work
 * and is the root of a dependence graph. You will subclass this,
 * depending on the level of analysis you want, but I believe
 * that the functions provided here are common.
 */
public abstract class DependenceAnalyzer {

    /*
     * This analyzer maintains a name-to-node dictionary of
     * its graph. This lets you lookup nodes by name. It will
     * return null if the named entity is not in the graph.
     */
    public abstract DependenceNode nodeByName( Object name );

    /*
     * Way(s) to access the whole graph.
     */
    public abstract Enumeration allNodes();
    // too hard for now.
    //public Enumeration unanalyzedNodes();
    //public Enumeration errorNodes();

    /*
     * To conditionally add a node to the graph. It is not
     * an error to add something that's already in the graph.
     * The return value is a DependenceNode. It may be
     * in any state, but if it was not in the graph previous to
     * this call, it will certainly be DependenceNode.UNANALYZED.
     * In other words, this call doesn't cause analysis.
     */
    public abstract DependenceNode addNodeByName( Object name );

    /*
     * To cause the state of a node to move from UNANALYZED,
     * one hopes to ANALYZED, but perhaps to ERROR.
     * Return value is the new state of the node.
     */
    public abstract int analyzeDependences( DependenceNode node );

    /*
     * To complete analysis of the whole graph. Return value
     * is true if all nodes in the graph end in state
     * ANALYZED, else false.
     * Any nodes not ANALYZED will be in state ERROR.
     */
    public boolean analyzeAllDependences(){
	Enumeration nodeList;
	boolean changed;
	boolean succeed = true;
	do {
	    changed = false;
	    nodeList = allNodes();
	    while ( nodeList.hasMoreElements() ){
		DependenceNode dep = (DependenceNode)(nodeList.nextElement());
		if ( dep.state() == DependenceNode.UNANALYZED ){
		    analyzeDependences( dep );
		    changed = true;
		}
		if ( dep.state() == DependenceNode.ERROR )
		    succeed = false;
	    }
	} while ( changed );
	return succeed;
    }
}
