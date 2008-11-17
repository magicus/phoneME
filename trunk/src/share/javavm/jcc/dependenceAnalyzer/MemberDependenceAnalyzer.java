/*
 * @(#)MemberDependenceAnalyzer.java	1.21 06/10/10
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
import util.*;
import java.io.FilenameFilter;
import java.util.Enumeration;
import java.util.Dictionary;
import java.util.Hashtable;

/*
 * This is the main class for member-level dependence analysis
 * of Java code. The most well-developed example of this is, of course,
 * JavaFilter. See also the class util.DepgenUtil for some
 * higher-level functions that operate on this class, without
 * constituting a main program.
 */

public class MemberDependenceAnalyzer extends DependenceAnalyzer {
    public boolean signatureFlag = false;

    public MemberDependenceAnalyzer( ClassFileFinder find, FilenameFilter filt ){
	super( );
	cdict = new ClassDictionary( find, filt );
    }

    public void useSignatureDependence(boolean f) {
	signatureFlag = f;
    }

    /*
     * This analyzer maintains a name-to-node dictionary of
     * its graph. This lets you lookup nodes by name. It will
     * return null if the named entity is not in the graph.
     * Actually, each class maintains such. Use them.
     */
    public DependenceNode nodeByName( Object name ){
	MemberName mname = (MemberName)name;
	return mname.classEntry.lookupMember( mname );
    }

    /*
     * To conditionally add a node to the graph. It is not
     * an error to add something that's already in the graph.
     * The return value is a DependenceNode. It may be
     * in any state, but if it was not in the graph previous to
     * this call, it will certainly be DependenceNode.UNANALYZED.
     * In other words, this call doesn't cause analysis.
     */
    public DependenceNode addNodeByName( Object name ){
	MemberName mname = (MemberName)name;
	return mname.classEntry.lookupAddMember( mname, 0 );
    }

    public Enumeration allNodes() {
	return new MemberEnumeration( allClasses() );
    }

    /*
     * To cause the state of a node to move from UNANALYZED,
     * one hopes to ANALYZED, but perhaps to ERROR.
     * Return value is the new state of the node.
     */
    public int analyzeDependences( DependenceNode n ){
	if ( n.state() != DependenceNode.UNANALYZED )
	    return n.state();
	ClassEntry c;
	if ( n instanceof ClassEntry ){
	    c = (ClassEntry)n;
	} else {
	    c = ((MemberName)(n.nodeName)).classEntry;
	}
	if ( c.state() == ClassEntry.UNANALYZED ){
	    c.analyzeClass( cdict, signatureFlag );
	}
	switch ( c.state() ){
	case ClassEntry.ERROR:
	    return ( n.nodeState = DependenceNode.ERROR );
	case ClassEntry.ANALYZED:
	    return n.nodeState;
	}
	// this cannot happen!
	throw new RuntimeException(
	    Localizer.getString("memberdependenceanalyzer.class_remains_unanalyzed", c.name())
	);
    }

    public Enumeration allClasses(){
	if ( cdict == null ) return EmptyEnumeration.instance;
	return cdict.elements();
    }

    public ClassEntry classByName( String cname ){
	return cdict.lookupAdd( cname );
    }

    public boolean makeInterface( ClassEntry c ){
	return c.makeInterface( cdict );
    }

    /*
     * Protection loophole.
     * This exists so that the calling program can add edges
     * to the dependence graph, in addition to the ones discovered
     * by the processing in class ClassEntry. JavaFilter, for 
     * example, allows the user to describe the dependences of
     * native code, which cannot be discovered automatically.
     * This is because native code might have a dependence on
     * a data member that is not referenced by any Java code!
     */
    public void addDependenceArc(
	DependenceNode fromNode,
	DependenceNode toNode,
	int	       arcType
    ) {
	DependenceArc a = new DependenceArc( fromNode, toNode, arcType );
	fromNode.nodeDependsOn.add( a );
	toNode.nodeDependedOn.add( a );
    }

    /*
     * State. Not public.
     */
    //
    ClassDictionary	cdict; // encapsulation of all class naming and lookup.
}

class MemberEnumeration implements Enumeration {
    private Enumeration curEnumeration;
    private Enumeration classes;

    MemberEnumeration( Enumeration c ){
	if ( c == null ){
	    curEnumeration = EmptyEnumeration.instance;
	} else {
	    classes = c;
	    nextEnumeration();
	}
    }

    public boolean hasMoreElements(){
	if ( curEnumeration.hasMoreElements() ) return true;
	nextEnumeration();
	return curEnumeration.hasMoreElements();
    }

    public Object nextElement(){
	while (true){
	    try {
		Object o = curEnumeration.nextElement();
		// got one. Return it.
		return o;
	    } catch ( java.util.NoSuchElementException e ){
		// user called nextElement without calling
		// hasMoreElements first. This is unusual,
		// but not strictly illegal. If this is the
		// empty enumeration, then there is no more
		// so we just re-throw the exception. Otherwise
		// we move on to the next class.
		if ( curEnumeration == EmptyEnumeration.instance )
		    throw e;
		nextEnumeration();
	    }
	}
    }

    private void nextEnumeration() {
	while ( classes.hasMoreElements() ){
	    ClassEntry v = (ClassEntry)(classes.nextElement());
	    Enumeration e = v.members();
	    if ( (e != null) && e.hasMoreElements() ){
		curEnumeration = e;
		return;
	    }
	}
	curEnumeration = EmptyEnumeration.instance; // bad news.
    }
}
