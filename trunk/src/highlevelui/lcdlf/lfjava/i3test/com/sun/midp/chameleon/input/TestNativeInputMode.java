/*
 *
 *
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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
 */

package com.sun.midp.chameleon.input;
import com.sun.midp.i3test.TestCase;


public class TestNativeInputMode extends TestCase {
    /**
     * Overridden from TestCase parent. This method will kick off each
     * individual test
     */
    public void testNativeInputMode() {
        /*
        String [] cn = InputModeFactory.getJavaInputModeClassNames();
        for (int i=0;i<cn.length;i++) {
            System.out.print("'"+cn[i]+"' ");
        }
        System.out.println(";");
        int [] ci = InputModeFactory.getNativeInputModeIds();
        for (int i=0;i<ci.length;i++) {
            System.out.print("'"+ci[i]+"' ");
        }
        System.out.println(";");
        */
        int [] ci = InputModeFactory.getInputModeIds();
        for (int i=0;i<ci.length;i++) {
            System.out.print("'"+ci[i]+"' ");
        }
        System.out.println(";");

        NativeInputMode nim = new NativeInputMode();
        nim.initialize(1);

        System.out.println("map:");
        boolean[][] ismap = nim.getIsConstraintsMap();
        for (int j=0; j<ismap.length; j++) {
            for (int i=0; i<ismap[j].length; i++) {
                System.out.print(" "+(ismap[j][i]?"t":"f"));
            }
            System.out.println(" ");
        }
        System.out.println(";");

        System.out.println("id "+nim.id);
        System.out.println("supportsConstraints "+nim.supportsConstraints(0));
        System.out.println("getName() "+nim.getName());
        System.out.println("getCommandName() "+nim.getCommandName());
        nim.beginInput(null,"",0);
        System.out.println("processKey "+nim.processKey(1,true));
        System.out.println("getPendingChar ["+nim.getPendingChar()+"]");
        System.out.println("getNextMatch "+nim.getNextMatch());
        System.out.println("hasMoreMatches "+nim.hasMoreMatches());
        String [] ml = nim.getMatchList();
            System.out.print("getMatchList() ");
        for (int i=0;i<nim.getMatchList().length;i++) {
            System.out.print("'"+ml[i]+"' ");
        }
        System.out.println(";");
        System.out.println("endInput");
        nim.endInput();
        System.out.println("endInput ok");

        InputMode[] ims = InputModeFactory.createInputModes();
        for (int i=0;i<ims.length;i++) {
            System.out.print(" "+ims[i]+" ");
        }
        System.out.println(";");
    }
    public void runTests() throws Throwable {

        try {
	
	System.out.println("======runtests====");
	    declare("test");
	    
	    
	    testNativeInputMode();
	    
	    
	    assertTrue(true);
        } finally {
//            LcduiTestMIDlet.cleanup();
        }
    }

}
