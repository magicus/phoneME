/*
 * @(#)JavaClassDepend.java	1.10 06/10/10
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

import java.io.FilenameFilter;
import util.ClassFileFinder;
import util.Set;
import util.PathComponent;
import java.util.Enumeration;
import java.util.Dictionary;
import java.util.Hashtable;
import components.*;
import util.ClassFile;
import java.io.InputStream;
import java.io.File;

/*
 * Following is a simple driver program.
 * Here is an example of its use:
 *
 * java JavaClassDepend -classpath <your classes> -path java/lang/Object java/io/OptionalDataException
 *
 * And the example shows two dependence chains of equal length from java.lang.Object to
 * java.io.OptionalDataException:
 *
 * java/lang/Object => java/lang/Class => java/lang/ClassLoader => java/net/URL => java/io/ObjectInputStream => java/io/OptionalDataException
 * java/lang/Object => java/lang/Class => java/lang/ClassLoader => java/util/Hashtable => java/io/ObjectInputStream => java/io/OptionalDataException
 * example of  class dependence analysis
 */

import util.*;
import java.util.Enumeration;
import dependenceAnalyzer.*;
 
class JavaClassDepend {
     ClassFileFinder	    finder = new ClassFileFinder();
     ClassnameFilter	    filter = new DummyFilter();
     ClassDependenceAnalyzer cda = new ClassDependenceAnalyzer( finder, filter );
     boolean forward;
 
     private void addToSearchPath( String pathname ){
 	finder.addToSearchPath( pathname );
     }

     private void printNode( DependenceNode node, String classname ) { 
        if ( node == null ){ 
            System.out.println("Cannot find class "+classname);
            return; 
        }       
        cda.analyzeDependences( node ); 
        if (forward) { 
            printDependences( node ); 
        } else { 
            cda.analyzeAllDependences(); 
            printReverseDependences( node ); 
        } 
     }
 
     private void showOne( String classname ){
	DependenceNode node;
	int colon;
	int curbegin = 0;
	int pl = classname.length();

	classname = classname.replace('.', '/');
	char sepChar = File.pathSeparatorChar;
	while( (colon = classname.indexOf(sepChar,curbegin) ) != -1 ){
	    String thisclass =  classname.substring( curbegin, colon );
            node = cda.addNodeByName( thisclass );
	    printNode(node, thisclass);
            curbegin = colon+1;
	
        }
        if ( curbegin < pl ){
	    String thisclass =  classname.substring( curbegin, pl );
            node = cda.addNodeByName( thisclass );
	    printNode(node, thisclass);
        }
     }

     private void showAll(){
	cda.analyzeAllDependences();
 	Enumeration e = cda.allNodes();
 	int nClasses = 0;
 	while ( e.hasMoreElements()){
 	    DependenceNode node = (DependenceNode)(e.nextElement());
	    if (forward) 
 	        printDependences( node );
	    else 
		printReverseDependences( node );
 	    nClasses++;
 	}
 	System.out.println("TOTAL OF "+nClasses+" CLASSES");
     }

     private void showPath( String srcclassname, String destclassname ){
 	ClassDependenceNode src = (ClassDependenceNode)cda.addNodeByName( srcclassname );
 	ClassDependenceNode dest = (ClassDependenceNode)cda.addNodeByName( destclassname );
 	Set tipset = new Set();
 	PathComponent r = new PathComponent( null, src );
 	tipset.addElement( r );
 	for ( int i = 0; i < 50; i++ ){
 	    Set newtips = new Set();
 	    Enumeration newGrowth = tipset.elements();
 	    while ( newGrowth.hasMoreElements() ){
 		PathComponent t = (PathComponent)(newGrowth.nextElement() );
 		if ( t.link.state() == DependenceNode.UNANALYZED )
 		    cda.analyzeDependences( t.link );
 		t.grow( newtips );
 	    }
 	    if ( newtips.size() == 0 ){
 		 /* exhaused the space without finding it. */
 		System.out.println("No path from "+srcclassname+" to "+destclassname );
 		return;
 	    }
 	     /* now see if we got to our destination. */
 	    newGrowth = newtips.elements();
 	    boolean foundDest = false;
 	    while ( newGrowth.hasMoreElements() ){
 		PathComponent t = (PathComponent)(newGrowth.nextElement() );
 		if ( t.link == dest ){
 		    t.print( System.out );
 		    System.out.println();
 		    foundDest = true;
 		}
 		t.link.flags |= PathComponent.INSET;
 	    }
 	    if ( foundDest ){
 		return;
 	    }
 	     /* round and round. */
 	    tipset = newtips;
 	}
 	System.out.println("Path from "+srcclassname+" to "+destclassname+" is too long" );
     }

     private boolean checkArg(int index, int argLength, int nargNeed) {
	int r = argLength - index;
	if (r > nargNeed)
	    return true;	

	return false;	
     }

     private void printUsage() {
	System.out.println("Usage: java JavaClassDepend [-options] <classname>");
	System.out.println(" " );
	System.out.println("where options include : ");
	System.out.println("    -help             print out this message");
	System.out.println("    -classpath <directories separated by colons>");
	System.out.println("                      list directories in which to look for classes");
	System.out.println("    -showDirect <classname(s)> (if more than one class separated by colons)");
	System.out.println("                      show direct dependencies of this particular class");
	System.out.println("    -showAll          show all dependencies of the class in the graph");
	System.out.println("    -path <classname1> <classname2>");
	System.out.println("                      show two dependence chains of equal length from <Class 1> to <Class 2>");
	System.out.println("    -membersOf <classname>");
	System.out.println("                      show all member functions of a given class <classname>");
	System.out.println("    -reverse          put this option before -showDirect or -showAll to show the reverse graph of the reverse graph of specified <class> or all nodes");
	System.out.println("    -loadClass <classname>");
        System.out.println("                      load Class <classname> into database");


     }
 
     private void process( String args[] ){
	boolean details = false;
	forward = true;

 	for ( int i = 0; i < args.length; i++ ){
 	    String a = args[i];
 	    if ( a.equals("-classpath") ){
		if (checkArg(i, args.length, 1))
 		    addToSearchPath( args[++i] );
		else
		    printUsage();
 		continue;
 	    }
 	    if ( a.equals("-path") ){
		if (checkArg(i, args.length, 2))
 		    showPath( args[++i], args[++i] );
		else
		    printUsage();
 		continue;
 	    }
	    if ( a.equals("-reverse")) {
		forward = false;
		continue;

	    }
 	    if ( a.equals("-showDirect") ){
		if (checkArg(i, args.length, 1))
 		    showOne( args[++i] );
		else
		    printUsage();
 		continue;
 	    }
 	    if ( a.equals("-showAll") ){
		details = true;
 		continue;
 	    }
            if ( a.equals("-membersOf")) {
                if (checkArg(i, args.length, 1))
                    cda.printMembers(args[++i]);
                else
                    printUsage();
                continue;
            }
	    if ( a.equals("-help")) {
		printUsage();
		break;
	    }

            if ( a.equals("-loadClass")) {
                if (checkArg(i, args.length, 1))
                    cda.addNodeByName(args[++i].replace('.', '/'));
                else
                    printUsage();
                continue;
            }
	    printUsage();
 	}

	if (details)
	    showAll();
     }
 
     public static void main( String args[] ){
 	new JavaClassDepend().process( args );
     }
 
     static void printDependences( DependenceNode node ){
 	if ( node.state() != DependenceNode.ANALYZED ){
	    if (node.state() == DependenceNode.ERROR)
 	        System.out.println( node.name()+" unanalysed");
 	    return;
 	}
 	System.out.println( node.name()+" depends on:");
 	Enumeration e = node.dependsOn();
 	while ( e.hasMoreElements() ){
 	    DependenceArc a = (DependenceArc)(e.nextElement());
 	    System.out.println("	"+a.to().name() );
 	}
     }

     static void printReverseDependences( DependenceNode node ){
        if ( node.state() != DependenceNode.ANALYZED ){
	    if (node.state() == DependenceNode.ERROR)
                System.out.println( node.name()+" unanalysed");
            return;
        }
        System.out.println( node.name()+" depended on:");
        Enumeration e = node.dependedOn();
        while ( e.hasMoreElements() ){
            DependenceArc a = (DependenceArc)(e.nextElement());
            System.out.println("        "+a.from().name() );
        }
     }
 }
 
 class DummyFilter extends util.ClassnameFilter{
     public boolean accept( java.io.File dir, String className ){
 	return false;
     }
 }
