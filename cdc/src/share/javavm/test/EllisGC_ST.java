/*
 * @(#)EllisGC_ST.java	1.7 06/10/10
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
/*
 * The single threaded version of EllisGC
 */
class Node {
    Node left, right;
}

public class EllisGC_ST {

	public static final int kNumIterations      = 50;

	/* 15 levels = 1.5M
	 */
	public static final int kStretchTreeDepthX    = 19;	// about 24Mb
	public static final int kLongLivedTreeDepthX  = 18;  // about 12Mb
	public static final int kShortLivedTreeDepthX = 13;	// about 0.4Mb

	public static final int kStretchTreeDepth    = 19;	// about 24Mb
	public static final int kLongLivedTreeDepth  = 18;  // about 12Mb
	public static final int kShortLivedTreeDepth = 13;	// about 0.4Mb


	static void Populate(int iDepth, Node thisNode) {
		if (iDepth<=0)
			return;
		else {
			iDepth--;
			thisNode.left  = new Node();
			thisNode.right = new Node();
			Populate (iDepth, thisNode.left);
			Populate (iDepth, thisNode.right);
		}
	}


	public static void main(String args[]) {
		Node	root;
		Node	longLivedTree;
		Node	tempTree;

		System.out.println("Garbage Collector Test");
		System.out.println(" Stretching memory with a binary tree of depth "
+ kStretchTreeDepth);
		System.out.println(" Creating a long-lived binary tree of depth " +
kLongLivedTreeDepth);
		System.out.println(" Creating " + kNumIterations + " binary trees of depth " + kShortLivedTreeDepth);

		// Stretch the memory space quickly
		tempTree = new Node();
		Populate(kStretchTreeDepth, tempTree);
		tempTree = null;

		// Create a long lived object
		longLivedTree = new Node();
		Populate(kLongLivedTreeDepth, longLivedTree);

		for (int c=0; c<kNumIterations; c++) {
		    root = new Node();
		    Populate(kShortLivedTreeDepth, root);
		}

		root = longLivedTree;	// fake reference to LongLivedTree to keep it from being optimized away
	}
}
