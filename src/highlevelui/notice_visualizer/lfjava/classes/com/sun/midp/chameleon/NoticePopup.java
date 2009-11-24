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

package com.sun.midp.chameleon;

import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

import com.sun.midp.chameleon.CWindow;
import com.sun.midp.chameleon.layers.PopupLayer;
import com.sun.midp.chameleon.skins.NoticePopupSkin;
import com.sun.midp.lcdui.Notice;
import com.sun.midp.lcdui.NoticeVisualizer;

public class NoticePopup extends PopupLayer implements CommandListener {
   
    private String txt;
    private Image img;
    private Notice notice;
    private NoticeVisualizer parent;
    private NoticePopupSkin skin;

    public NoticePopup(Notice n, NoticeVisualizer p) {
        super();
        txt = n.getLabel();;
        img = n.getImage();
        notice = n;
        visible = true;
        parent = p;
        opaque = false;
        transparent = false;

        Command[] cmds = new Command[] {
            new Command("Select", Command.OK, 1),
            new Command("Dissmiss", Command.CANCEL, 1)
        };

        setCommands(cmds);
        setCommandListener(this);

        skin = new NoticePopupSkin(notice.getOriginator(), 
                                   notice.getLabel(), notice.getImage());
    }


    /**
     * 
     */
    public void dismiss() {
        owner.removeLayer(this);
    }

    /**
     * Update bounds of layer
     * @param layers - current layer can be dependant on this parameter
     */
    public void update(CLayer[] layers) {
        super.update(layers);
        if (owner == null) {
            return;
        }
        System.arraycopy(owner.bounds, 0, bounds, 0, owner.bounds.length);
        if (layers[MIDPWindow.BTN_LAYER].isVisible()) {
            bounds[H] -= layers[MIDPWindow.BTN_LAYER].bounds[H];
        }
    }

    /**
     * Semitransparent window
     * 
     */
    public void paintBackground(Graphics g) {
        int w = bounds[W] - bounds[X];
        int h = bounds[H] - bounds[Y];
        int[] rgb = new int[w * h];

        for (int i = 0; i < rgb.length; i ++) {
            rgb[i] = 0x80000000;
        }
        Image i1 = Image.createRGBImage(rgb, w, h, true);
        g.drawImage(i1, bounds[X], bounds[Y], Graphics.LEFT|Graphics.TOP);
    }

    public void paintBody(Graphics g) {
        skin.paint(g, bounds[X], bounds[Y], bounds[W], bounds[H]);
    }

    public void commandAction(Command c, Displayable d) {
        if (c.getCommandType() == Command.OK) {
                notice.select();
        } else {
                notice.dismiss();
        }
    }

    public void addNotify() {
 //        ((MIDPWindow) owner).paintWash(true);
    }

    public void removeNotify(CWindow owner) {
 //       ((MIDPWindow) owner).paintWash(false);
        parent.removeNotify(this);
    }
}
