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
import com.sun.midp.chameleon.skins.MenuSkin;

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Font;

public class MenuResources {
    private static boolean init;
    
    private MenuResources() {
    }
    
    public static void load() {
        load(false);
    }
        
    public static void load(boolean reload) {
        if (init && !reload) {
            return;
        }
        
        MenuSkin.WIDTH = SkinResources.getInt(
                SkinPropertiesIDs.MENU_WIDTH);
        MenuSkin.HEIGHT = SkinResources.getInt(
                SkinPropertiesIDs.MENU_HEIGHT);

        int alignX = SkinResources.getInt(
                SkinPropertiesIDs.MENU_ALIGN_X);
        MenuSkin.ALIGN_X = SkinResources.resourceConstantsToGraphics(alignX);

        int alignY = SkinResources.getInt(
                SkinPropertiesIDs.MENU_ALIGN_Y);
        MenuSkin.ALIGN_Y = SkinResources.resourceConstantsToGraphics(alignY);

        MenuSkin.TITLE_X = SkinResources.getInt(
                SkinPropertiesIDs.MENU_TITLE_X);
        MenuSkin.TITLE_Y = SkinResources.getInt(
                SkinPropertiesIDs.MENU_TITLE_Y);
        MenuSkin.TITLE_MAXWIDTH = SkinResources.getInt(
                SkinPropertiesIDs.MENU_TITLE_MAXWIDTH);

        int titleAlign = SkinResources.getInt(
                SkinPropertiesIDs.MENU_TITLE_ALIGN);
        MenuSkin.TITLE_ALIGN = SkinResources.resourceConstantsToGraphics(
                titleAlign);

        MenuSkin.MAX_ITEMS = SkinResources.getInt(
                SkinPropertiesIDs.MENU_MAX_ITEMS);
        MenuSkin.ITEM_HEIGHT = SkinResources.getInt(
                SkinPropertiesIDs.MENU_ITEM_HEIGHT);
        MenuSkin.ITEM_TOPOFFSET = SkinResources.getInt(
                SkinPropertiesIDs.MENU_ITEM_TOPOFFSET);
        MenuSkin.ITEM_INDEX_ANCHOR_X = SkinResources.getInt(
                SkinPropertiesIDs.MENU_ITEM_INDEX_ANCHOR_X);
        MenuSkin.ITEM_ANCHOR_X = SkinResources.getInt(
                SkinPropertiesIDs.MENU_ITEM_ANCHOR_X);
        MenuSkin.COLOR_BG = SkinResources.getInt(
                SkinPropertiesIDs.MENU_COLOR_BG);
        MenuSkin.COLOR_BG_SEL = SkinResources.getInt(
                SkinPropertiesIDs.MENU_COLOR_BG_SEL);
        MenuSkin.COLOR_TITLE = SkinResources.getInt(
                SkinPropertiesIDs.MENU_COLOR_TITLE);
        MenuSkin.COLOR_INDEX = SkinResources.getInt(
                SkinPropertiesIDs.MENU_COLOR_INDEX);
        MenuSkin.COLOR_INDEX_SEL = SkinResources.getInt(
                SkinPropertiesIDs.MENU_COLOR_INDEX_SEL);
        MenuSkin.COLOR_ITEM = SkinResources.getInt(
                SkinPropertiesIDs.MENU_COLOR_ITEM);
        MenuSkin.COLOR_ITEM_SEL = SkinResources.getInt(
                SkinPropertiesIDs.MENU_COLOR_ITEM_SEL);
        MenuSkin.TEXT_TITLE = SkinResources.getString(
                SkinPropertiesIDs.MENU_TEXT_TITLE);
        MenuSkin.FONT_TITLE = SkinResources.getFont(
                SkinPropertiesIDs.MENU_FONT_TITLE);
        MenuSkin.FONT_ITEM = SkinResources.getFont(
                SkinPropertiesIDs.MENU_FONT_ITEM);
        MenuSkin.FONT_ITEM_SEL = SkinResources.getFont(
                SkinPropertiesIDs.MENU_FONT_ITEM_SEL);
        MenuSkin.IMAGE_BG = SkinResources.getCompositeImage(
                SkinPropertiesIDs.MENU_IMAGE_BG, 9);
        /*
        Uncomment if background image for selected item is used
        MenuSkin.IMAGE_ITEM_SEL_BG = SkinResources.getCompositeImage(
                SkinPropertiesIDs.MENU_IMAGE_ITEM_SEL_BG, 3);
        */
        MenuSkin.IMAGE_SUBMENU_ARROW = SkinResources.getImage(
                SkinPropertiesIDs.MENU_IMAGE_SUBMENU);
        MenuSkin.IMAGE_SUBMENU_ARROW_HL = SkinResources.getImage(
                SkinPropertiesIDs.MENU_IMAGE_SUBMENU_HL);
            
        init = true;
    }

}
