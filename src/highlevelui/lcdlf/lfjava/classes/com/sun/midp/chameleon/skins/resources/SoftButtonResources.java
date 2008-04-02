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
import com.sun.midp.chameleon.skins.SoftButtonSkin;
import com.sun.midp.chameleon.skins.ScreenSkin;

import com.sun.midp.chameleon.layers.SoftButtonLayer;

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Font;

public class SoftButtonResources {
    private static boolean init;
    
    private SoftButtonResources() {
    }
    
    public static void load() {
        load(false);
    }
        
    public static void load(boolean reload) {
        
        if (init && !reload) {
            return;
        }

        SoftButtonSkin.HEIGHT = (SoftButtonLayer.isNativeSoftButtonLayerSupported0())?
                0:SkinResources.getInt(SkinPropertiesIDs.SOFTBTN_HEIGHT);

        SoftButtonSkin.NUM_BUTTONS = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_NUM_BUTTONS);
        
        SoftButtonSkin.BUTTON_ANCHOR_X = SkinResources.getNumbersSequence(
                SkinPropertiesIDs.SOFTBTN_BUTTON_ANCHOR_X);
        for (int i = 0; i < SoftButtonSkin.NUM_BUTTONS; i++) {
            if (SoftButtonSkin.BUTTON_ANCHOR_X[i] < 0) {
                SoftButtonSkin.BUTTON_ANCHOR_X[i] = ScreenSkin.WIDTH +
                    SoftButtonSkin.BUTTON_ANCHOR_X[i];
            }
        }
        
        SoftButtonSkin.BUTTON_ANCHOR_Y = SkinResources.getNumbersSequence(
                SkinPropertiesIDs.SOFTBTN_BUTTON_ANCHOR_Y);
               
        int[] alignX = SkinResources.getNumbersSequence(
                SkinPropertiesIDs.SOFTBTN_BUTTON_ALIGN_X);
        for (int i = 0; i < alignX.length; ++i) {
            alignX[i] = SkinResources.resourceConstantsToGraphics(alignX[i]); 
        }
        SoftButtonSkin.BUTTON_ALIGN_X = alignX;
                
        SoftButtonSkin.BUTTON_MAX_WIDTH = SkinResources.getNumbersSequence(
                SkinPropertiesIDs.SOFTBTN_BUTTON_MAX_WIDTH);
        
        int shdAlign = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_BUTTON_SHD_ALIGN);
        SoftButtonSkin.BUTTON_SHD_ALIGN = 
            SkinResources.resourceConstantsToGraphics(shdAlign);

        SoftButtonSkin.COLOR_FG = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_COLOR_FG);
        SoftButtonSkin.COLOR_FG_SHD = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_COLOR_FG_SHD);
        SoftButtonSkin.COLOR_BG = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_COLOR_BG);
        SoftButtonSkin.COLOR_MU_FG = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_COLOR_MU_FG);
        SoftButtonSkin.COLOR_MU_FG_SHD = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_COLOR_MU_FG_SHD);
        SoftButtonSkin.COLOR_MU_BG = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_COLOR_MU_BG);
        SoftButtonSkin.COLOR_AU_FG = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_COLOR_AU_FG);
        SoftButtonSkin.COLOR_AU_FG_SHD = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_COLOR_AU_FG_SHD);
        SoftButtonSkin.COLOR_AU_BG = SkinResources.getInt(
                SkinPropertiesIDs.SOFTBTN_COLOR_AU_BG);
        SoftButtonSkin.FONT = SkinResources.getFont(
                SkinPropertiesIDs.SOFTBTN_FONT);
        SoftButtonSkin.TEXT_MENUCMD = SkinResources.getString(
                SkinPropertiesIDs.SOFTBTN_TEXT_MENUCMD);
        SoftButtonSkin.TEXT_BACKCMD = SkinResources.getString(
                SkinPropertiesIDs.SOFTBTN_TEXT_BACKCMD);
        SoftButtonSkin.IMAGE_BG = SkinResources.getCompositeImage(
                SkinPropertiesIDs.SOFTBTN_IMAGE_BG, 3);
        SoftButtonSkin.IMAGE_MU_BG = SkinResources.getCompositeImage(
                SkinPropertiesIDs.SOFTBTN_IMAGE_MU_BG, 3);
        SoftButtonSkin.IMAGE_AU_BG = SkinResources.getCompositeImage(
                SkinPropertiesIDs.SOFTBTN_IMAGE_AU_BG, 3);

        init = true;
    }
}
