package com.sun.midp.chameleon;

import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.CommandListener;
import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Font;
import javax.microedition.lcdui.Graphics;
import javax.microedition.lcdui.Image;

import com.sun.midp.chameleon.CWindow;
import com.sun.midp.chameleon.layers.PopupLayer;
import com.sun.midp.lcdui.Notice;
import com.sun.midp.lcdui.NoticeVisualizer;

public class NoticePopup extends PopupLayer implements CommandListener {
   
    private String txt;
    private Image img;
    private Notice notice;
    private NoticeVisualizer parent;

    public NoticePopup(Notice n, NoticeVisualizer p) {
        super();
        txt = n.getLabel();;
        img = n.getImage();
        notice = n;
        visible = true;
        parent = p;
        transparent = false;

        Command[] cmds = new Command[] {
            new Command("Select", Command.OK, 1),
            new Command("Dissmiss", Command.CANCEL, 1)
        };

        setCommands(cmds);
        setCommandListener(this);
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
        int x = bounds[X] + bounds[W]/3;
        int y = bounds[Y] + bounds[H]/3;
        int w =  bounds[W]/3;
        int h =  bounds[H]/3;

        g.setColor(0xFFFFFF);
        g.fillRect(x, y, w, h);
        if (null != img) {
            g.setClip(x, y, w / 2, h / 2);
            g.drawImage(img, x, y, Graphics.LEFT|Graphics.TOP);
        }
        g.setColor(0);
        g.setClip(x + w/2, y, w/2, h/2);
        Font header = Font.getFont(Font.FACE_MONOSPACE, Font.STYLE_BOLD, Font.SIZE_SMALL);
        g.setFont(header);
        g.drawString("Notice from " + notice.getOriginatorID(), x + w/2, y, Graphics.LEFT|Graphics.TOP);
        Font body = Font.getFont(Font.FACE_PROPORTIONAL, Font.STYLE_PLAIN, Font.SIZE_SMALL);
        g.setFont(body);
        g.setClip(x, y + h/2, w, h/2);
        g.drawString(txt, x, y + h/2,  Graphics.LEFT|Graphics.TOP);
    }

    public void commandAction(Command c, Displayable d) {
        if (c.getCommandType() == Command.OK) {
                notice.select();
        } else {
                notice.dismiss();
        }
    }

    public void removeNotify(CWindow owner) {
        parent.removeNotify(this);
    }

}
