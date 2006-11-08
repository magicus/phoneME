/*
 * Copyright 1990-2006 Sun Microsystems, Inc. All Rights Reserved. 
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER 
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 only,
 * as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * version 2 for more details (a copy is included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa Clara,
 * CA 95054 or visit www.sun.com if you need additional information or have
 * any questions.
 */

package com.sun.appmanager.impl.presentation.AwtPDA;

import java.awt.*;
import java.awt.event.*;

public class InfoDialog
    extends Dialog {

    AwtPDAPresentationMode presentationMode = null;
    String str = null;
    TextArea textArea = null;

    public InfoDialog(AwtPDAPresentationMode presentationMode, String string) {
        super(presentationMode.getFrame(), true);

        setLayout(new BorderLayout());

        this.presentationMode = presentationMode;
        this.str = string;

        textArea = new TextArea(str, -1, -1, TextArea.SCROLLBARS_VERTICAL_ONLY);

        Font font = presentationMode.currentUserFont;

        if (font == null) {
            int fontSize = 0;
            String fontStr = null;
            fontSize = Integer.parseInt(presentationMode.preferences.
                                        getSystemPreference(
                "AwtPDAPresentationMode.loginFontSize"));
            fontStr = presentationMode.preferences.getSystemPreference(
                "AwtPDAPresentationMode.loginFont");
            font = new Font(fontStr, Font.PLAIN,
                            fontSize);
        }
        textArea.setFont(font);
        textArea.setEditable(false);

        add(textArea, BorderLayout.CENTER);

        ImageButton closeButton = new ImageButton(presentationMode.
                                                  mediumButtonImage) {
            public Dimension getPreferredSize() {
                return new Dimension(200, 50);
            }
        };

        closeButton.setLabel(presentationMode.getString(
            "AwtPDAPresentationMode.closeButton.label"), 97, 26);
        closeButton.setTextColor(Color.black);
        closeButton.setFont(font);
        closeButton.addActionListener(new HideAction());
        add(closeButton, BorderLayout.SOUTH);

        setBackground(Color.white);
        setForeground(Color.black);

        setLocation(100, 150);
    }

    public Dimension getPreferredSize() {
        return new Dimension(250, 200);
    }

    class HideAction
        implements ActionListener {

        public void actionPerformed(ActionEvent e) {
            dispose();
        }
    }
}
