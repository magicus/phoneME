/*
 * @(#)DepgenUtil.java	1.21 06/10/10
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
import dependenceAnalyzer.*;
import java.util.Enumeration;
import java.util.Vector;

/*
 * Dependence-generation utility functions.
 * Specific to the interfaces defined in dependenceAnalyzer,
 * and to the external (String) representation of some of those things.
 * Actually, this the the framework for building any member-
 * dependence-analysis-based stuff.
 */

public class DepgenUtil extends LinkerUtil implements dependenceAnalyzer.MemberArcTypes {
    protected ClassFileFinder	finder = new ClassFileFinder();
    protected ClassnameFilter	filter = new ClassnameFilter();
    protected ClassnameFilter	wholeClass;
    protected ClassnameFilter	wholeData;

    protected MemberDependenceAnalyzer analyzer = new MemberDependenceAnalyzer( finder, filter );

    protected MemberName
    stringToName( String nameAndType ){
	int nameDivision = methodOff( nameAndType );
	String classname = nameAndType.substring( 0, nameDivision );
	String membername = nameAndType.substring( nameDivision+1 );
	return new MemberName(
	    analyzer.classByName( sanitizeClassname( classname ) ), 
	    sanitizeClassname( membername ) // may include signature.
	);
    }

    protected MemberDependenceNode
    stringToNode( String nameAndType ){
	return (MemberDependenceNode)(analyzer.addNodeByName( stringToName( nameAndType ) ));
    }

    /*
     * Similar to stringToName, but permits us
     * to name constructors (in context) without the bother of
     * the magic name. Thus
     * "java.lang.String([C)" is, in context, a synonym for
     * the more perfectly general "java.lang.String.<init>([C)"
     * We will also use the default signature if none is found.
     */
    protected MemberName
    stringToConstructorName( String classAndType ){
	int sigDivision  = sigOff( classAndType );
	String classname;
	String signature;
	if ( sigDivision == -1 ){
	    classname = classAndType;
	    signature = constructorSig;
	} else {
	    classname = classAndType.substring( 0, sigDivision );
	    signature = classAndType.substring( sigDivision );
	}
	return new MemberName(
	    analyzer.classByName( sanitizeClassname( classname ) ), 
	    constructorName+sanitizeClassname( signature )
	);
    }

    protected MemberDependenceNode
    stringToConstructorNode( String classAndType ){
	return (MemberDependenceNode)(analyzer.addNodeByName( stringToConstructorName( classAndType ) ));
    }

    /*
     * Start with a String, return a ClassEntry. Very simple.
     */
    protected ClassEntry
    stringToClass( String classname ){
	return analyzer.classByName( sanitizeClassname( classname ) );
    }

    /*
     * To exclude a class or package named by the given string.
     * If it ends in *, we treat it as a package name.
     * otherwise, it is a class.
     */
    protected void
    excludeClass( String xclassname ){
	String classname = sanitizeClassname( xclassname );
	filter.includeName( classname );
    }

    /*
     * This is the guts of dependence-based analysis.
     * It is only slightly parameterized to allow
     * for multiple target sets. In reality,
     * there will most likely be only one.
     *
     * processNode is called after we have determined that the given
     * node has not yet be added to our target set, but should be. 
     * It adds the node to targetList, then processes it,
     * looking for other things to be added to the worklist.
     * It is called by processNodeList, below.
     */

    protected void
    processNode(
	DependenceNode node,
	int inclusionFlag,
	int initIncludedFlag,
	int requiredMask,
	Vector targetlist,
	Vector worklist
    ){
	if ( node.state() == DependenceNode.UNANALYZED )
	    analyzer.analyzeDependences( node );
	if ( node.state() == DependenceNode.ERROR ) return; // cannot cope.
	if ( (node.flags&MemberDependenceNode.EXCLUDED) != 0 ) return; // not wanted.

	targetlist.addElement( node );
	node.flags |= inclusionFlag;

	if (  node instanceof ClassEntry ){
	    // if we require a class, what we really require is its <clinit>, if any.
	    ClassEntry classNode = (ClassEntry)node;
	    MemberDependenceNode clinit = classNode.lookupMember( staticInitializerName+staticInitializerSig );
	    if ( clinit != null ){
		if ( ( clinit.flags&requiredMask) == 0 )
		    worklist.addElement( clinit );
		    // if we were clever, we'd assign node=clinit and fall through...
	    }
	    // we also need its superclass
	    ClassEntry mySuper = classNode.superClass();
	    if ( mySuper!= null ){
		if ( (mySuper.flags&requiredMask) == 0 )
		    worklist.addElement( mySuper );
	    }
	    //
	    // we also believe that we need its interfaces.
	    // I personally believe this is an incorrect requirement
	    // by the current JVM implementation, but what can you do?
	    //
	    Enumeration ilist = classNode.interfaces();
	    while ( ilist.hasMoreElements() ){
		ClassEntry iface = (ClassEntry) (ilist.nextElement());
		if ( (iface.flags&requiredMask) == 0 )
		    worklist.addElement( iface );
	    }

	    //
	    // finally (?), if the class is named as one for which we
	    // want-all-members or want-all-data, then put all those
	    // things on the worklist, as appropriate.
	    String className = (String)( classNode.name() );
	    boolean wantAllMembers = wholeClass.accept( null, className );
	    boolean wantAllData = wantAllMembers || wholeData.accept( null, className );
	    if ( wantAllData ){ // ... which is the OR of two conditions...
		Enumeration members = classNode.members();
		if ( members != null ){
		    while ( members.hasMoreElements() ){
			MemberDependenceNode mnode = (MemberDependenceNode)(members.nextElement() );
			if ( wantAllMembers || ((mnode.flags&MemberDependenceNode.FIELD) != 0 ) ){
			    // either we want absolutely everything, or
			    // this is data ( and we implcitly want data )
			    // so, either way, we want mnode.
			    if ( (mnode.flags&requiredMask) == 0 ){
				worklist.addElement( mnode );
			    }
			}
		    }
		}
	    }

	    return;
	}
	MemberDependenceNode mNode = (MemberDependenceNode)node; // the only other kind we recognize.
	if (  (mNode.flags&MemberDependenceNode.OVERRIDDEN) != 0 ){
	    /*
	     * Any methods which override this one, and are members of classes having constructors
	     * loaded, must themselves be loaded.
	     */
	    Enumeration overriders = mNode.overriddenBy();
	    while ( overriders.hasMoreElements() ){
		MemberDependenceNode t = (MemberDependenceNode)(overriders.nextElement());
		if ( (t.flags&requiredMask) != 0 ) continue; // already loaded.
		if ( (((MemberName)(t.name())).classEntry.flags&initIncludedFlag) != 0 ){
		    // constructor is loaded but the overriding member is not.
		    // put it on the list.
		    worklist.addElement( t );
		}
	    }
	} else if ( (mNode.flags&MemberDependenceNode.INIT) != 0 ){
	    // this is a constructor.
	    // if its our first, check for necessary and overriding methods.
	    // then note that a constructor has been loaded
	    ClassEntry mClass = ((MemberName)(mNode.name())).classEntry;
	    if ( ((mClass.flags & initIncludedFlag ) == 0 ) && (mClass.flags&ClassEntry.HAS_OVERRIDING) != 0 ){
		Enumeration mMembers = mClass.members();
		while ( mMembers.hasMoreElements() ){
		    MemberDependenceNode t = (MemberDependenceNode)(mMembers.nextElement());
		    if ( (t.flags&(MemberDependenceNode.NO_OVERRIDING|MemberDependenceNode.EXCLUDED)) != 0 )
			continue; // not a candidate
		    Enumeration tover = t.overrides();
		overriders:
		    while ( tover.hasMoreElements() ){
			MemberDependenceNode u = (MemberDependenceNode)(tover.nextElement());
			if ( (u.flags&requiredMask) != 0 ){
			    worklist.addElement( t );
			    break overriders; // enough!
			}
		    }
		}
		mClass.flags |= initIncludedFlag;
	    }
	}

	Enumeration e = node.dependsOn();
	while ( e.hasMoreElements() ){
	    DependenceNode t = ((DependenceArc)(e.nextElement())).to();
	    if ( (t.flags&requiredMask) == 0 ){
		worklist.addElement( t );
	    }
	}
    }

    /*
     * The main entry point to dependence analysis.
     * The vector of dependence nodes is used as a starting point,
     * and they and everything they depend on are included in the
     * resulting vector. This is parameterized by flags used so
     * we can work with multiple sets at a time. In practice, this
     * will probably never happen.
     *
     * Parameters are:
     *    worklist   -- a Vector of DependenceNode's for which we need to
     *		      	build the dependence graphs
     *	  inclusionFlag -- a 1-bit mask. This gets ORed into the flag word
     *			of all nodes that are required (i.e. are in the
     *			dependence set we're
     *			calculating. (By using different bits, we allow you
     *			to calculate multiple sets within the same graph.)
     *			We use this to avoid infinitely recalculating dependences
     *			for nodes we've already visited.
     *    initIncludedFlag -- a 1-bit mask. This gets ORed into the flag
     *			word of all classes for which at least one constructor
     *			is required. Is used for part of the overriding
     *			induced implicit-dependence calculation.
     *	  requiredMask  -- a multi-bit mask, which will certainly include
     *			inclusionFlag. Will also include, for instance, a 
     *			bit noting that a node is part of an interface. This
     *			is used to test overridden members, to see if 
     *			overriders must be required.
     *	  verbosity     -- a small integer value. Currently, there is only
     *			one message, and it is only printed if (verbosity>1)
     *
     * Return value is:
     *	  a vector of all the nodes added to the required list by this
     * 	  calculation.
     *
     * Caveats:
     *	  Note that we keep a lot of state in each node's flag word.
     *    We let the caller determine which bits we use, so it is the caller's
     *	  responsibility to choose reasonable non-interfering sets.
     *    This algorithm might not work if some flag bits are
     *    unexpectedly set. If necessary, use clearUserFlags, below.
     */

    protected Vector
    processList(
	Vector worklist,
	int inclusionFlag,
	int initIncludedFlag,
	int requiredMask,
	int verbosity
    ){
	Vector target = new Vector();
	int targetsize = 0;
	while ( worklist.size() != 0 ){
	    Enumeration working = worklist.elements();
	    Vector newlist = new Vector();
	    while ( working.hasMoreElements() ){
		DependenceNode node = (DependenceNode)(working.nextElement());
		if ( (node.flags&requiredMask) == 0 ){
		    processNode( node, inclusionFlag, initIncludedFlag, requiredMask, target, newlist );
		}
	    }
	    int newsize = target.size();
	    if ( verbosity > 1 ){
		System.out.println(Localizer.getString("depgenutil.iteration_added_new_elements", new Integer(newsize-targetsize)));
	    }
	    targetsize = newsize;
	    worklist = newlist;
	}
	return target;
    }

    /*
     * If we are asking multiple, hypothetical questions about 
     * dependence sets, we might want to re-use the same membership
     * flags. This requires clearing them before re-use.
     * clearUserFlags will iterate through all classes and members,
     * clearing the flags given as input. 
     * This is NOT built into the operation of processList, as multiple
     * questions is NOT the expected normal behavior, and has some cost.
     */
    protected void
    clearUserFlags( int flagset ){
	int flagmask = ~ flagset;
	Enumeration classlist = analyzer.allClasses();
	while ( classlist.hasMoreElements() ){
	    ClassEntry c = (ClassEntry) (classlist.nextElement());
	    c.flags &= flagmask;
	    Enumeration memberlist = c.members();
	    if ( memberlist == null ) continue;
	    while ( memberlist.hasMoreElements() ){
		DependenceNode m = (DependenceNode)(memberlist.nextElement());
		m.flags &= flagmask;
	    }
	}
    }

}
