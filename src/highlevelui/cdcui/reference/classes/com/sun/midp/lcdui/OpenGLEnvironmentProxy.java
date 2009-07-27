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

import com.sun.midp.chameleon.skins.SoftButtonSkin;

/**
 * This class provides methods needed for JSRs to access OpenGL 
 * rendering capabilities in lcdui in order to properly synchronize 
 * rendering on certain platforms
 * 
 */
public class OpenGLEnvironmentProxy{
    
    private DisplayContainer container;
    private static OpenGLEnvironmentProxy instance = null;
    private OpenGLEnvironment env;
    private boolean openGLIsEnabled = false;
    
    public OpenGLEnvironmentProxy() {
        env = new OpenGLEnvironment();
    }

    /** 
     * Get instance of OpenGLEnvironment
     */
    public static synchronized OpenGLEnvironmentProxy getInstance() {
        if (instance == null)
            instance = new OpenGLEnvironmentProxy();
        return instance;
    }
    
    /**
     * Init the environment with the current DisplayContainer
     *
     * @param  displayContainer
     */
    public void init(DisplayContainer displayContainer) {
        this.container = displayContainer;
    }
    
    /**
     * Get DisplayContainer associated with this instance of OpenGLEnvironment
     *
     */
    public DisplayContainer getDisplayContainer() {
        return this.container;
    }
    
    /** 
     * Return shared drawing surface for given Graphics targets
     */
    public int getDrawingSurface(Object bindTarget, int api) {
        int retval;
        retval =  env.getDrawingSurface((Graphics)bindTarget, api);
        return retval;
    }
    
    /** 
     * Prepare openGL renderer to switch between lcdui and some exernal
     * API - can be either JSR226 or JSR239
     *
     */
    public void flushOpengGL(Object bindTarget) {
        env.flushOpengGL(container, (Graphics)bindTarget);
    }
    
    public void createPbufferSurface(Image img) {
        if (openGLIsEnabled)
            env.createPbufferSurface(img);
    }
    
    public void flushPbufferSurface(Image offscreen_buffer,
                                    int x, int y, int width, int height){
        env.flushPbufferSurface(offscreen_buffer, x, y, width, height);
    }

    public void startMidpRendering() {
        env.startMidpRendering();
    }

    public void endMidpRendering() {
        env.endMidpRendering();
    }
    
    public void enableOpenGL(int width, int height) {
        setSoftButtonHeight(SoftButtonSkin.HEIGHT);
        env.enableOpenGL(width, height);
        openGLIsEnabled = true;
    }
    
    public void disableOpenGL() {
        env.disableOpenGL();
        openGLIsEnabled = false;
    }
    
    public void raiseOpenGL() {
        env.raiseOpenGL();
    }    
    public void lowerOpenGL() {
        env.lowerOpenGL();
    }
    
    public void switchColorDepth(int param) {
        env.switchColorDepth(param);
    }

    public void setSoftButtonHeight(int height) {
        env.setSoftButtonHeight(height);
    }

}
