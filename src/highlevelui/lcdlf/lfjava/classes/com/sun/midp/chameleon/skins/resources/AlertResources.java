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
import com.sun.midp.chameleon.skins.AlertSkin;
import com.sun.midp.chameleon.skins.ScreenSkin;

import javax.microedition.lcdui.Image;
import javax.microedition.lcdui.Font;

public class AlertResources {
    private static boolean init;
    
    private AlertResources() {
    }
    
    public static void load() {
        load(false);
    }
        
    public static void load(boolean reload) {
        if (init && !reload) {
            return;
        }
        
        AlertSkin.WIDTH = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_WIDTH);
        if (AlertSkin.WIDTH == -1) {
            AlertSkin.WIDTH = (int)(.95 * ScreenSkin.WIDTH);
        }
        AlertSkin.HEIGHT = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_HEIGHT);
        if (AlertSkin.HEIGHT == -1) {
            AlertSkin.HEIGHT = (int)(.75 * ScreenSkin.HEIGHT);
        }

        int alignX = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_ALIGN_X);
        AlertSkin.ALIGN_X = SkinResources.resourceConstantsToGraphics(alignX);

        int alignY = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_ALIGN_Y);
        AlertSkin.ALIGN_Y = SkinResources.resourceConstantsToGraphics(alignY);

        AlertSkin.MARGIN_H = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_MARGIN_H);
        AlertSkin.MARGIN_V = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_MARGIN_V);

        int titleAlign = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_TITLE_ALIGN);
        AlertSkin.TITLE_ALIGN = SkinResources.resourceConstantsToGraphics(
                titleAlign);

        AlertSkin.TITLE_HEIGHT = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_TITLE_HEIGHT);
        AlertSkin.TITLE_MARGIN = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_TITLE_MARGIN);
        AlertSkin.TEXT_TITLE_INFO = SkinResources.getString(
                SkinPropertiesIDs.ALERT_TEXT_TITLE_INFO);
        AlertSkin.TEXT_TITLE_WARN = SkinResources.getString(
                SkinPropertiesIDs.ALERT_TEXT_TITLE_WARN);
        AlertSkin.TEXT_TITLE_ERRR = SkinResources.getString(
                SkinPropertiesIDs.ALERT_TEXT_TITLE_ERRR);
        AlertSkin.TEXT_TITLE_ALRM = SkinResources.getString(
                SkinPropertiesIDs.ALERT_TEXT_TITLE_ALRM);
        AlertSkin.TEXT_TITLE_CNFM = SkinResources.getString(
                SkinPropertiesIDs.ALERT_TEXT_TITLE_CNFM);
        AlertSkin.PAD_HORIZ = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_PAD_HORIZ);
        AlertSkin.PAD_VERT = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_PAD_VERT); 
        AlertSkin.SCROLL_AMOUNT = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_SCROLL_AMOUNT);
        AlertSkin.TIMEOUT = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_TIMEOUT);
        AlertSkin.COLOR_BG = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_COLOR_BG);
        AlertSkin.COLOR_TITLE = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_COLOR_TITLE);
        AlertSkin.COLOR_FG = SkinResources.getInt(
                SkinPropertiesIDs.ALERT_COLOR_FG);
        AlertSkin.FONT_TITLE = SkinResources.getFont(
                SkinPropertiesIDs.ALERT_FONT_TITLE);
        AlertSkin.FONT_TEXT = SkinResources.getFont(
                SkinPropertiesIDs.ALERT_FONT_TEXT);
        AlertSkin.IMAGE_BG = SkinResources.getCompositeImage(
                SkinPropertiesIDs.ALERT_IMAGE_BG, 9);
        AlertSkin.IMAGE_ICON_INFO = SkinResources.getImage(
                SkinPropertiesIDs.ALERT_IMAGE_ICON_INFO); 
        AlertSkin.IMAGE_ICON_WARN = SkinResources.getImage(
                SkinPropertiesIDs.ALERT_IMAGE_ICON_WARN);
        AlertSkin.IMAGE_ICON_ERRR = SkinResources.getImage(
                SkinPropertiesIDs.ALERT_IMAGE_ICON_ERRR);
        AlertSkin.IMAGE_ICON_ALRM = SkinResources.getImage(
                SkinPropertiesIDs.ALERT_IMAGE_ICON_ALRM);
        AlertSkin.IMAGE_ICON_CNFM = SkinResources.getImage(
                SkinPropertiesIDs.ALERT_IMAGE_ICON_CNFM);
        
        init = true;
    }

   
}
    

