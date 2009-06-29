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

    private boolean midpIsRendering;
    /** 
     * Prepare openGL renderer to switch between lcdui and some exernal
     * API - can be either JSR226 or JSR239
     *
     */
    public void flushOpengGL(DisplayContainer container, Graphics bindTarget) {
        int regionArray[];
        System.out.println("in flushOpenGL - bindTarget is " + bindTarget);
        /*
        while (midpIsRendering) {
            try {
                System.out.println("waiting for midp to finish rendering");
                Thread.sleep(10);
            } catch (Exception e) {}
        }
         */
        if (!hasBackingSurface(bindTarget, bindTarget.getClipWidth(),
                               bindTarget.getClipHeight())) {
        System.out.println("now checking for normal flush");
        int displayId = NativeForegroundState.getState();
        DisplayAccess da = container.findDisplayById(displayId);
        if (da != null) {
            Object[] dirtyRegions = da.getDirtyRegions();
            if (dirtyRegions.length <= 0)
                return;
            regionArray = new int[dirtyRegions.length*4];
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
        }
    }
    
    public void createPbufferSurface(Image img) {
        createPbufferSurface0(img);
    }
    
    public void flushPbufferSurface(Image offscreen_buffer,
                                    int x, int y, int width, int height){
        System.out.println("offscreen buffer is " + offscreen_buffer);
        flushPbufferSurface0(offscreen_buffer, x, y, width, height);
        //System.out.println("back from flushPbufferSurface0");
    }
    
    public void createPixmapSurface(Graphics g, Image img) {
        createPixmapSurface0(g, img);
    }

    public void startMidpRendering() {
        System.out.println("*** startMidpRendering");
        midpIsRendering=true;
    }

    public void endMidpRendering() {
        System.out.println("*** endMidpRendering");
        midpIsRendering=false;
    }
    
    /** 
     * Return shared drawing surface for given Graphics targets
     */
    public int getDrawingSurface(Graphics bindTarget, int api) {
        int retval;
        retval = getDrawingSurface0(bindTarget, api);
        return retval;
    }
    
    public void enableOpenGL(int width, int height) {
        System.out.println("OpenGLEnvironmentProxy: enabling OpenGL");
        initMidpGL(width, height);
    }
    
    public void disableOpenGL() {
        System.out.println("OpenGLEnvironmentProxy: disabling OpenGL");
        disableOpenGL0();
    }
    
    public void raiseOpenGL() {
        raiseOpenGL0();
    }    
    public void lowerOpenGL() {
        lowerOpenGL0();;
    }
    
    private native void flushOpenGL0(int[] regionArray,
                                     int numberOfRegions, int displayId);
    
    private native void createPbufferSurface0(Image img);
    private native void flushPbufferSurface0(Image src, int x, int y,
                                             int width, int height);
    private native void createPixmapSurface0(Graphics g, Image img);
    private native boolean hasBackingSurface(Graphics bindTarget,
                                             int width, int height);
    private native int getDrawingSurface0(Graphics bindTarget, int api);
    private native void initMidpGL(int screenWidth, int screenHeight);
    private native void disableOpenGL0();
    private native void raiseOpenGL0();
    private native void lowerOpenGL0();
}
