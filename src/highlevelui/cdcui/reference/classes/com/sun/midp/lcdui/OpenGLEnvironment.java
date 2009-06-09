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

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Graphics;

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
    
    public void createPbufferSurface(Image img) {
        //System.out.println("OpenGLEnvironment: createPbufferSurface");
        createPbufferSurface0(img);
    }
    
    public void flushPbufferSurface(Image offscreen_buffer,
                                    int ystart, int yend){
        //System.out.println("offscreen buffer is " + offscreen_buffer);
        flushPbufferSurface0(offscreen_buffer, ystart, yend);
        //System.out.println("back from flushPbufferSurface0");
    }
    
    public void createPixmapSurface(Graphics g, Image img) {
        createPixmapSurface0(g, img);
    }
    
    private native void flushOpenGL0(int[] regionArray,
                                     int numberOfRegions, int displayId);
    
    private native void createPbufferSurface0(Image img);
    private native void flushPbufferSurface0(Image src, int ystart, int yend);
    private native void createPixmapSurface0(Graphics g, Image img);
}
