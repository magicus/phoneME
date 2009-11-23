/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.chameleon.skins;

import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

import com.sun.midp.chameleon.skins.resources.SkinLoader;
import com.sun.midp.chameleon.skins.SkinPropertiesIDs;
import com.sun.midp.i18n.Resource;
import com.sun.midp.i18n.ResourceConstants;

import com.sun.midp.lcdui.Text;

public class NoticePopupSkin {
    private static Font titleFont;
    private static Font appNameFont;
    private static Font labelFont;
    private static int NOTICE_POPUP_WIDTH;
    private static int NOTICE_POPUP_HEIGHT;
    private static int NOTICE_POPUP_MARGIN;
    private static int NOTICE_POPUP_BG_COLOR;
    private static int NOTICE_POPUP_PAD;
    private static int NOTICE_POPUP_IMG_WIDTH;
    private static int NOTICE_POPUP_IMG_HEIGHT;
    private static int NOTICE_POPUP_IMAGE_BG_COLOR;
    private static int NOTICE_POPUP_TITLE_COLOR;
    private static int NOTICE_POPUP_TITLE_ALIGN;
    private static int NOTICE_POPUP_APP_NAME_COLOR;
    private static int NOTICE_POPUP_LABEL_COLOR;
    private static String title;

    private String appName;
    private String label;
    private Image img;

    public NoticePopupSkin(String sAppName, String sLabel, Image image) {

        if (null == titleFont) {
            titleFont = SkinLoader.getFont(SkinPropertiesIDs.NOTICE_POPUP_TITLE_FONT);
            appNameFont = SkinLoader.getFont(SkinPropertiesIDs.NOTICE_POPUP_APP_NAME_FONT);
            labelFont = SkinLoader.getFont(SkinPropertiesIDs.NOTICE_POPUP_LABEL_FONT);
            title = Resource.getString(ResourceConstants.NOTIFICATION_POPUP_TITLE);
            NOTICE_POPUP_WIDTH = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_WIDTH);      
            NOTICE_POPUP_HEIGHT = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_HEIGHT);       
            NOTICE_POPUP_MARGIN = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_MARGIN);
            NOTICE_POPUP_BG_COLOR = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_BG_COLOR);
            NOTICE_POPUP_PAD = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_PAD);
            NOTICE_POPUP_IMG_WIDTH = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_IMG_WIDTH);
            NOTICE_POPUP_IMG_HEIGHT = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_IMG_HEIGHT);
            NOTICE_POPUP_IMAGE_BG_COLOR = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_IMAGE_BG_COLOR);
            NOTICE_POPUP_TITLE_COLOR = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_TITLE_COLOR);
            NOTICE_POPUP_TITLE_ALIGN = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_TITLE_ALIGN);
            NOTICE_POPUP_APP_NAME_COLOR = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_APP_NAME_COLOR);
            NOTICE_POPUP_LABEL_COLOR = SkinLoader.getInt(SkinPropertiesIDs.NOTICE_POPUP_LABEL_COLOR);
        }

        appName =  sAppName;
        label = sLabel;
        img = image;
    }

    public void paint(Graphics g, int x, int y, int w, int h) {
        x += (w - NOTICE_POPUP_WIDTH)/2;
        y += (h - NOTICE_POPUP_HEIGHT)/2;
        w = NOTICE_POPUP_WIDTH;
        h = NOTICE_POPUP_HEIGHT;

        // body drawing
        g.setColor(NOTICE_POPUP_BG_COLOR);
        g.fillRect(x, y, w, h);
        g.setColor(0);
        g.drawRect(x + NOTICE_POPUP_PAD, y + NOTICE_POPUP_PAD, w - NOTICE_POPUP_PAD * 2, h - NOTICE_POPUP_PAD * 2);

        // title drawing
        int tmpw = w - NOTICE_POPUP_MARGIN * 2;
        int tmph = h - NOTICE_POPUP_MARGIN * 2;
        int tmpy = y + NOTICE_POPUP_MARGIN;
        int tmpx = x + NOTICE_POPUP_MARGIN;
        g.setClip(tmpx, tmpy, tmpw, tmph);
        tmpx = x + w / 2;// HCENTER anchor
        g.setColor(NOTICE_POPUP_TITLE_COLOR);
        g.setFont(titleFont);
        g.drawString(title, tmpx, tmpy, NOTICE_POPUP_TITLE_ALIGN);
        
        // up-left side image
        tmpx = x + NOTICE_POPUP_MARGIN;
        tmpw = NOTICE_POPUP_IMG_WIDTH;
        tmpy = NOTICE_POPUP_MARGIN + titleFont.getHeight() + NOTICE_POPUP_PAD;
        tmph = NOTICE_POPUP_IMG_HEIGHT;
        g.setClip(tmpx, tmpy, tmpw, tmph);              
        if (null != img) {
            g.drawImage(img,
                        tmpx,
                        tmpy,
                        Graphics.LEFT|Graphics.TOP);
        } else {
            g.setColor(NOTICE_POPUP_IMAGE_BG_COLOR);
            g.fillRect(tmpx, tmpy, tmpw, tmph);
        }

        // up-right side app name
        tmpw = w - NOTICE_POPUP_MARGIN * 2 - NOTICE_POPUP_IMG_WIDTH - NOTICE_POPUP_PAD;
        tmph = NOTICE_POPUP_IMG_HEIGHT;
        // tmpy is the same
        tmpx = x + NOTICE_POPUP_MARGIN + NOTICE_POPUP_IMG_WIDTH + NOTICE_POPUP_PAD;
        g.setFont(appNameFont);
        g.setColor(NOTICE_POPUP_APP_NAME_COLOR);
        g.setClip(tmpx, tmpy, tmpw, tmph);
        g.translate(tmpx, tmpy);
        Text.paint(g, appName, appNameFont, 
                   NOTICE_POPUP_LABEL_COLOR, NOTICE_POPUP_LABEL_COLOR, 
                   tmpw, tmph, 0, Text.TRUNCATE, null); 
        g.translate(0, 0);
        int[] size = new int[Text.HEIGHT + 1];
        Text.getSizeForWidth(size, tmpw, appName, labelFont, 0);
        // blur right side if string length greater than drawing area
        if (size[Text.HEIGHT] > tmph) {
            tmpx += tmpw - tmpw / 10;// 10% to blur
            tmpw -= tmpw / 10;
            int[] rgb = new int[tmpw * tmph];
            for (int i = 0; i < tmpw; i++) {
                for (int j = 0; i < tmph; i++) {
                    rgb[i + j*tmpw] = (0xFF * i / tmpw)<<24;
                }
            }
            Image blur = Image.createRGBImage(rgb, tmpw, tmph, true);
            g.drawImage(blur, tmpx, tmpy, Graphics.TOP | Graphics.LEFT);
        }

        // bottom side app message
        tmpx = x + NOTICE_POPUP_MARGIN;
        tmpy +=  NOTICE_POPUP_IMG_HEIGHT + NOTICE_POPUP_PAD;
        tmpw = w - NOTICE_POPUP_MARGIN * 2;
        tmph = h - tmpy - NOTICE_POPUP_PAD;
        g.setClip(tmpx, tmpy, tmpw, tmph);
        g.setFont(labelFont);
        g.setColor(NOTICE_POPUP_LABEL_COLOR);
        g.translate(tmpx, tmpy);
        Text.paint(g, label, labelFont, 
                   NoticePopupSkin.NOTICE_POPUP_LABEL_COLOR, NoticePopupSkin.NOTICE_POPUP_LABEL_COLOR, 
                   tmpw, tmph, 0, Text.TRUNCATE, null); 
        g.translate(0, 0);

        Text.getSizeForWidth(size, tmpw, label, labelFont, 0);
        if (size[Text.HEIGHT] > tmph) {
            // blur bottom
            tmpy += tmph / 10;// 10%
            tmph -= tmph / 10;
            int[] rgb = new int[tmpw * tmph];
            for (int i = 0; i < tmpw; i++) {
                for (int j = 0; i < tmph; i++) {
                    rgb[i + j*tmpw] = (0xFF * j / tmph)<<24;
                }
            }
            Image blur = Image.createRGBImage(rgb, tmpw, tmph, true);
            g.drawImage(blur, tmpx, tmpy, Graphics.TOP | Graphics.LEFT);
        }
    }
}
