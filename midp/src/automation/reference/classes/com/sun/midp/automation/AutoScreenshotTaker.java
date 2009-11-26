/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.automation;

/**
 * Takes an screenshot. 
 * IMPL_NOTE: only implemented for putpixel based ports
 */
class AutoScreenshotTaker {
    /**
     * Takes screenshot.
     */
    void takeScreenshot() {
        takeScreenshot0();
    }

    /**
     * Gets screenshot data in RGB888 format after screenshot 
     * has been taken.
     *
     * @return screenshot data as byte array, or null if taking
     * screenshot is not implemented
     */
    byte[] getScreenshotRGB888() {
        return getScreenshotRGB8880();
    }

    /**
     * Gets screenshot width.
     *
     * @return screenshot width
     */
    int getScreenshotWidth() {
        return getScreenshotWidth0();
    }

    /**
     * Gets screenshot height.
     *
     * @return screenshot height
     */
    int getScreenshotHeight() {
        return getScreenshotHeight0();
    }
    
    /**
     * Native nethod that does all the real screenshot-taking work
     */
    private native void takeScreenshot0();

    /**
     * Native method that gets screenshot data in RGB888 format.
     *
     * @return screenshot data as byte array
     */
    private native byte[] getScreenshotRGB8880();

    /**
     * Native method that gets screenshot width.
     *
     * @return screenshot width
     */
    private native int getScreenshotWidth0();

    /**
     * Native method that gets screenshot height.
     *
     * @return screenshot width
     */
    private native int getScreenshotHeight0();
}
