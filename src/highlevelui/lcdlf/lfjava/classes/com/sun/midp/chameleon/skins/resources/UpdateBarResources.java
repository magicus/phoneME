/*
 *   
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

package com.sun.midp.chameleon.skins.resources;

import com.sun.midp.chameleon.skins.SkinPropertiesIDs;
import com.sun.midp.chameleon.skins.UpdateBarSkin;
import com.sun.midp.chameleon.skins.ScreenSkin;

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Font;

public class UpdateBarResources {
    private static boolean init;
    
    // private constructor
    private UpdateBarResources() {
    }
    
    public static void load() {
        load(false);
    }
        
    public static void load(boolean reload) {
        if (init && !reload) {
            return;
        }
        
        UpdateBarSkin.WIDTH = SkinResources.getInt(
                SkinPropertiesIDs.UPDATEBAR_WIDTH);
        UpdateBarSkin.HEIGHT = SkinResources.getInt(
                SkinPropertiesIDs.UPDATEBAR_HEIGHT);
        UpdateBarSkin.NUM_FRAMES = SkinResources.getInt(
                SkinPropertiesIDs.UPDATEBAR_NUM_FRAMES);
        UpdateBarSkin.FRAME_X = SkinResources.getInt(
                SkinPropertiesIDs.UPDATEBAR_FRAME_X);
        UpdateBarSkin.FRAME_Y = SkinResources.getInt(
                SkinPropertiesIDs.UPDATEBAR_FRAME_Y);
        UpdateBarSkin.FRAME_SEQUENCE = SkinResources.getNumbersSequence(
                SkinPropertiesIDs.UPDATEBAR_FRAME_SEQU);
        UpdateBarSkin.IMAGE_BG = SkinResources.getImage(
                SkinPropertiesIDs.UPDATEBAR_IMAGE_BG);
        UpdateBarSkin.IMAGE_FRAME = SkinResources.getCompositeImage(
            SkinPropertiesIDs.UPDATEBAR_IMAGE_FRAME,
            UpdateBarSkin.NUM_FRAMES);

        init = true;
    }
    
}


