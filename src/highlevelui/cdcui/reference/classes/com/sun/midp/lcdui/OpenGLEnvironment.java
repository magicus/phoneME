/*
 *   
 *
 * Copyright 2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.lcdui;

import com.sun.midp.main.NativeForegroundState;

/**
 * This class provides methods needed for JSRs to access OpenGL 
 * rendering capabilities in lcdui in order to properly synchronize 
 * rendering on certain platforms
 * 
 */
public class OpenGLEnvironment{
    
    /** 
     * Prepare openGL renderer to switch between lcdui and some exernal
     * API - can be either JSR226 or JSR239
     *
     */
    public void flushOpengGL(DisplayContainer container) {
        int displayId = NativeForegroundState.getState();
        DisplayAccess da = container.findDisplayById(displayId);
        Object[] dirtyRegions = da.getDirtyRegions();
        int regionArray[] = new int[dirtyRegions.length*4];
        int[] curRegion;
        for (int i=0; i<dirtyRegions.length; i++) {
            curRegion = (int[])dirtyRegions[i];
            regionArray[i]=curRegion[0];
            regionArray[i+1]=curRegion[1];
            regionArray[i+2]=curRegion[2];
            regionArray[i+3]=curRegion[3];
        }
        flushOpenGL0(regionArray, dirtyRegions.length, displayId);
    }
    
    private native void flushOpenGL0(int[] regionArray,
                                     int numberOfRegions, int displayId);
}
