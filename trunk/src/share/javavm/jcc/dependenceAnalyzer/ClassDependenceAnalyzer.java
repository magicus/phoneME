/*
 * @(#)ClassDependenceAnalyzer.java	1.17 06/10/10
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
import java.io.FilenameFilter;
import util.ClassFileFinder;
import util.Set;
import java.util.Enumeration;
import java.util.Dictionary;
import java.util.Hashtable;
import components.*;
import util.ClassFile;
import java.io.InputStream;

/*
 * ClassDependenceAnalyzer is a concrete subclass of DependenceAnalyzer
 * that operates class-by-class. It is not used by JavaFilter or
 * by JavaCodeCompact. It is for use by other dependence analysis
 * tools, which might operate on a more course-grained basis.
 *
 * Following this class definition is an example main program,
 * in a comment, that shows a simple driver for this class.
 */

public class ClassDependenceAnalyzer extends DependenceAnalyzer {

    private FilenameFilter filter;
    private ClassFileFinder finder;
    private Dictionary	    nodeDictionary;

    public ClassDependenceAnalyzer( ClassFileFinder find, FilenameFilter filt ){
	super();
	filter = filt;
	finder = find;
	nodeDictionary = new Hashtable();
    }

    /*
     * This analyzer maintains a name-to-node dictionary of
     * its graph. This lets you lookup nodes by name. It will
     * return null if the named entity is not in the graph.
     */
    public DependenceNode nodeByName( Object name ){
	return (DependenceNode)(nodeDictionary.get( name ) );
    }

    /*
     * To conditionally add a node to the graph. It is not
     * an error to add something that's already in the graph.
     * The return value is a DependenceNode. It may be
     * in any state, but if it was not in the graph previous to
     * this call, it will certainly be DependenceNode.UNANALYZED.
     * In other words, this call doesn't cause analysis.
     */
    public DependenceNode addNodeByName( Object name ) {
	Object t;
	if ( (t = nodeDictionary.get( name ) ) != null )
	    return (DependenceNode)t;
	DependenceNode newNode = new ClassDependenceNode( name );
	nodeDictionary.put( name, newNode );
	return newNode;
    }

    public Enumeration allNodes() { return nodeDictionary.elements(); }

    public int analyzeDependences( DependenceNode n ){

	if ( n.state() != DependenceNode.UNANALYZED )
	    return n.state();
	if ( filter.accept( null, (String)(n.name()) ) ){
	    // oops. on our excluded list.
	    // mark as excluded and analyzed.
	    n.flags = ClassDependenceNode.EXCLUDED;
	    n.nodeState = DependenceNode.ANALYZED;
	    return DependenceNode.ANALYZED;
	}
	ClassDependenceNode classNode = (ClassDependenceNode) n;
	String className = (String)(classNode.name());

	/* Deal with array */
	if (className.startsWith("[")) {
	    int depth = 1;
	    int len = className.length();
	    while (className.substring(depth, len).startsWith("[")) 
		depth++;
	    
	    if ((depth+1) > (len-1)) {
	        /* This will be primitive type */
		classNode.nodeState = DependenceNode.PRIMITIVE;
		return DependenceNode.PRIMITIVE;
	    }
	    className = className.substring(depth+1, len-1);
	    n.nodeState = DependenceNode.ANALYZED;
            return DependenceNode.ANALYZED;
	}

	ClassInfo ci = readClassFile( className );
	if ( ci == null ){
	    // sorry. cannot find.
	    // System.out.println("ci = null and its classname " + className);
	    classNode.nodeState = DependenceNode.ERROR;
	    return DependenceNode.ERROR;
	}
	if ( classNode.nodeDependsOn == null ){
	    classNode.nodeDependsOn = new Set();
	}
	DependenceNode other;
	// we depend on our super, if any.
	if ( ci.superClass != null ){
	    other = this.addNodeByName( ci.superClass.name.string );
	    classNode.superClass = (ClassDependenceNode)other;
	}

	// we might depend on our interfaces.
	// what do you think?

	//
	// find all the constants mentioned in the constant
	// pool. Call them dependences.
	//
	ConstantObject ctable[] = ci.getConstantPool().getConstants();
	if ( ctable != null ){
	    int nconst = ctable.length;
	    for ( int i = 0; i < nconst ; i++ ){
		ConstantObject co = ctable[i];
		if ( co instanceof ClassConstant ){
		    other = this.addNodeByName( ((ClassConstant)co).name.string );
		    newDependence( n, other );
		}
	    }
	}
	n.nodeState = DependenceNode.ANALYZED;
	return DependenceNode.ANALYZED;
    }

    public void printMembers( String classname ) {
	ClassInfo ci = readClassFile(classname.replace('.', '/'));

	if (ci == null) {
	    System.out.println("Cannot find class " + classname);
	    return;
	}

	MethodInfo mi[] = ci.methods;
	for (int i = 0; i < mi.length; i++) 
	    System.out.println(ci.className + "." + mi[i].getMethodName());
	
    }

    private ClassInfo readClassFile( String className )
    {
	InputStream classFileStream = finder.findClassFile( className );
	if ( classFileStream == null ){
	    return null;
	}
	ClassFile cfile = new ClassFile( className, classFileStream, false );
	if ( ! cfile.readClassFile() ){
	    return null;
	}
	return cfile.cinfo;
    }

    private void newDependence( DependenceNode here, DependenceNode there )
    {
	if ( here == there ) return; // don't depend on self.
	DependenceArc a = new DependenceArc( here, there, 0 );
	here.nodeDependsOn.addElement( a );
	there.nodeDependedOn.addElement( a );
    }

}

/*
 * Following is a simple driver program.
 * Here is an example of its use:
 *
 * java ClassDependenceDemo -classpath <your classes> -path java/lang/Object java/io/OptionalDataException
 *
 * And the example output (on Personal Java 1.0), showing two
 * dependence chains of equal length from java.lang.Object to
 * java.io.OptionalDataException:
 *
 * java/lang/Object => java/lang/Class => java/lang/ClassLoader => java/net/URL => java/io/ObjectInputStream => java/io/OptionalDataException
 * java/lang/Object => java/lang/Class => java/lang/ClassLoader => java/util/Hashtable => java/io/ObjectInputStream => java/io/OptionalDataException
 *
 */
// example of  class dependence analysis
// import util.*;
// import java.util.Enumeration;
// import dependenceAnalyzer.*;
// 
// class ClassDependenceDemo {
//     ClassFileFinder	    finder = new ClassFileFinder();
//     ClassnameFilter	    filter = new DummyFilter();
//     ClassDependenceAnalyzer cda = new ClassDependenceAnalyzer( finder, filter );
// 
//     private void addToSearchPath( String pathname ){
// 	finder.addToSearchPath( pathname );
//     }
// 
//     private void showOne( String classname ){
// 	DependenceNode node = cda.addNodeByName( classname );
// 	if ( node == null ){
// 	    System.out.println("Cannot find class "+classname);
// 	    return;
// 	}
// 	cda.analyzeDependences( node );
// 	printDependences( node );
//     }
// 
//     private void showAll(){
// 	cda.analyzeAllDependences();
// 	Enumeration e = cda.allNodes();
// 	int nClasses = 0;
// 	while ( e.hasMoreElements()){
// 	    DependenceNode node = (DependenceNode)(e.nextElement());
// 	    printDependences( node );
// 	    nClasses++;
// 	}
// 	System.out.println("TOTAL OF "+nClasses+" CLASSES");
//     }
// 
//     private void showPath( String srcclassname, String destclassname ){
// 	ClassDependenceNode src = (ClassDependenceNode)cda.addNodeByName( srcclassname );
// 	ClassDependenceNode dest = (ClassDependenceNode)cda.addNodeByName( destclassname );
// 	Set tipset = new Set();
// 	PathComponent r = new PathComponent( null, src );
// 	tipset.addElement( r );
// 	for ( int i = 0; i < 50; i++ ){
// 	    Set newtips = new Set();
// 	    Enumeration newGrowth = tipset.elements();
// 	    while ( newGrowth.hasMoreElements() ){
// 		PathComponent t = (PathComponent)(newGrowth.nextElement() );
// 		if ( t.link.state() == DependenceNode.UNANALYZED )
// 		    cda.analyzeDependences( t.link );
// 		t.grow( newtips );
// 	    }
// 	    if ( newtips.size() == 0 ){
// 		// exhaused the space without finding it.
// 		System.out.println("No path from "+srcclassname+" to "+destclassname );
// 		return;
// 	    }
// 	    // now see if we got to our destination.
// 	    newGrowth = newtips.elements();
// 	    boolean foundDest = false;
// 	    while ( newGrowth.hasMoreElements() ){
// 		PathComponent t = (PathComponent)(newGrowth.nextElement() );
// 		if ( t.link == dest ){
// 		    t.print( System.out );
// 		    System.out.println();
// 		    foundDest = true;
// 		}
// 		t.link.flags |= PathComponent.INSET;
// 	    }
// 	    if ( foundDest ){
// 		return;
// 	    }
// 	    // round and round.
// 	    tipset = newtips;
// 	}
// 	System.out.println("Path from "+srcclassname+" to "+destclassname+" is too long" );
//     }
// 
//     private void process( String args[] ){
// 	for ( int i = 0; i < args.length; i++ ){
// 	    String a = args[i];
// 	    if ( a.equals("-classpath") ){
// 		addToSearchPath( args[++i] );
// 		continue;
// 	    }
// 	    if ( a.equals("-path") ){
// 		showPath( args[++i], args[++i] );
// 		continue;
// 	    }
// 	    if ( a.equals("-show") ){
// 		showOne( args[++i] );
// 		continue;
// 	    }
// 	    if ( a.equals("-showAll") ){
// 		showAll();
// 		continue;
// 	    }
// 	    // else must be a class name!
// 	    cda.addNodeByName( a );
// 	}
//     }
// 
//     public static void main( String args[] ){
// 	new ClassDependenceDemo().process( args );
//     }
// 
//     static void printDependences( DependenceNode node ){
// 	if ( node.state() != DependenceNode.ANALYZED ){
// 	    System.out.println( node.name()+" unanalysed");
// 	    return;
// 	}
// 	System.out.println( node.name()+" depends on:");
// 	Enumeration e = node.dependsOn();
// 	while ( e.hasMoreElements() ){
// 	    DependenceArc a = (DependenceArc)(e.nextElement());
// 	    System.out.println("	"+a.to().name() );
// 	}
//     }
// 
// }
// 
// class DummyFilter extends util.ClassnameFilter{
//     public boolean accept( java.io.File dir, String className ){
// 	return false;
//     }
// }
// 
// class PathComponent {
//     PathComponent	root;
//     ClassDependenceNode link;
// 
//     public static int INSET = 8;
// 
//     PathComponent( PathComponent r, ClassDependenceNode l ){
// 	root = r;
// 	link = l;
//     }
// 
//     void print( java.io.PrintStream o ){
// 	if ( root != null ){
// 	    root.print( o );
// 	    o.print(" => ");
// 	}
// 	o.print( link.name() );
//     }
// 
//     void grow( Set tipset ){
// 	Enumeration t = link.dependsOn();
// 	while ( t.hasMoreElements() ){
// 	    DependenceArc arc = (DependenceArc)(t.nextElement());
// 	    ClassDependenceNode to = (ClassDependenceNode) arc.to();
// 	    if ( (to.flags&INSET) != 0 )
// 		continue;
// 	    tipset.addElement( new PathComponent( this, to ) );
// 	}
//     }
// }
