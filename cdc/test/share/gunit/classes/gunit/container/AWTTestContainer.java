/*
 * @(#)AWTTestContainer.java	1.6 06/10/10
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
 */

package gunit.container ;

import java.awt.* ;
import java.awt.event.* ;

/**
 * <code>AWTTestContainer</code> is an implementation of 
 * <code>TestContainer</code> that uses AWT Frame as the container
 */
public class AWTTestContainer implements gunit.framework.TestContainer {
    Frame testContainer ;
    public AWTTestContainer() {
        this.testContainer = new Frame("GUnit:AWT Test Frame") ;
        showTestContainer() ;
    }

    private static final int DEFAULT_WIDTH = 640 ;
    private static final int DEFAULT_HEIGHT = 442 ;
    
    public void showTestContainer() {
        this.testContainer.setLayout(new BorderLayout());
        int width = DEFAULT_WIDTH ;
        int height = DEFAULT_HEIGHT ;
        String dim = System.getProperty("gunit.awt.frame.window.size",
                                        "640x442") ;
        if ( dim != null && dim.length() > 0 ) {
            int index = dim.indexOf("x") ;
            if ( index > 0 ) {
                try {
                    width = Integer.parseInt(dim.substring(0,index)) ;
                    height = Integer.parseInt(dim.substring(index+1)) ;
                }
                catch ( Exception ex ) {
                }
            }
        }
        
        this.testContainer.addWindowListener(new WindowAdapter() {
                public void windowClosing(WindowEvent e) {
                    System.exit(0) ;
                }
            }) ;

        // Dont setSize for PBP.
        if ( isJ2SE() )
            this.testContainer.setSize(width,height) ;
        this.testContainer.setVisible(true) ;
    }

    // gunit.framework.TestContainer
    public Container getContainer() {
        return this.testContainer ;
    }

    public static boolean isJ2SE() {
        try {
            // for now this class does not exist in PP/PBP
            Class.forName("javax.naming.Context") ;
            return true ;
        }
        catch ( Exception ex ) {
        }
        return false ;
    }
}
