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
import com.sun.midp.chameleon.skins.TickerSkin;

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Font;

public class TickerResources {
    private static boolean init;
    
    private TickerResources() {
    }
    
    public static void load() {
        load(false);
    }
        
    public static void load(boolean reload) {
        if (init && !reload) {
            return;
        }
        
        TickerSkin.HEIGHT = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_HEIGHT);

        int align = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_ALIGN);
        TickerSkin.ALIGN = SkinResources.resourceConstantsToGraphics(align);

        int direction = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_DIRECTION);
        TickerSkin.DIRECTION = SkinResources.resourceConstantsToGraphics(
                direction);

        TickerSkin.RATE = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_RATE);
        TickerSkin.SPEED = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_SPEED);
        TickerSkin.TEXT_ANCHOR_Y = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_TEXT_ANCHOR_Y);

        int shdAlign = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_TEXT_SHD_ALIGN);
        TickerSkin.TEXT_SHD_ALIGN = SkinResources.resourceConstantsToGraphics(
                shdAlign);

        TickerSkin.COLOR_BG = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_COLOR_BG);
        TickerSkin.COLOR_FG = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_COLOR_FG);
        TickerSkin.COLOR_FG_SHD = SkinResources.getInt(
                SkinPropertiesIDs.TICKER_COLOR_FG_SHD);
        TickerSkin.FONT = SkinResources.getFont(
                SkinPropertiesIDs.TICKER_FONT);
        TickerSkin.IMAGE_BG = SkinResources.getCompositeImage(
                SkinPropertiesIDs.TICKER_IMAGE_BG, 3);
        TickerSkin.IMAGE_AU_BG = SkinResources.getCompositeImage(
                SkinPropertiesIDs.TICKER_IMAGE_AU_BG, 3);

        init = true;
    }
}

