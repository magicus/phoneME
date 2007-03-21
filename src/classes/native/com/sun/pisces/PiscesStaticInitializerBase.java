/*
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


package com.sun.pisces;

public abstract class PiscesStaticInitializerBase {

    private static final int X_BIAS    = 0;
    private static final int Y_BIAS    = 1;
    private static final int BIAS_SIZE = 2;

    private static PiscesStaticInitializerBase instance = null;
    
    protected PiscesStaticInitializerBase() {
	int[] biases = new int[BIAS_SIZE];
	getStrokeBiases(biases);
	staticInitialize(biases[X_BIAS], biases[Y_BIAS]);
    }

    protected static PiscesStaticInitializerBase getInstance(Class clazz) {
	if (instance == null) {
	    try {
	        instance = (PiscesStaticInitializerBase)(clazz.newInstance());
	    } catch (InstantiationException e) {
		throw new RuntimeException("Internal error: " + e.getMessage());
	    } catch (IllegalAccessException e) {
		throw new RuntimeException("Internal error: " + e.getMessage());
	    }
	}
	return instance;
    }

    private static void getStrokeBiases(int[] strokeBiases) {
        String strValue;
        strokeBiases[X_BIAS] = 0; // default x bias
        strokeBiases[Y_BIAS] = 0; // default y bias

        strValue = Configuration.getProperty("pisces.stroke.xbias");
        if (strValue != null) {
            try {
                strokeBiases[X_BIAS] = Integer.parseInt(strValue);
            } catch (NumberFormatException e) {
            }
        }
        
        strValue = Configuration.getProperty("pisces.stroke.ybias");
        if (strValue != null) {
            try {
                strokeBiases[Y_BIAS] = Integer.parseInt(strValue);
            } catch (NumberFormatException e) {
            }
        }
    }
    
    private static native void staticInitialize(int strokeXBias, 
    	    int strokeYBias);

    private static native void staticFinalize();
}
