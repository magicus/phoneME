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

package com.sun.ams.ui;

import javax.microedition.lcdui.Displayable;
import javax.microedition.lcdui.Display;
import javax.microedition.lcdui.Command;
import javax.microedition.lcdui.Spacer;
import javax.microedition.lcdui.Form;
import javax.microedition.lcdui.List;
import javax.microedition.lcdui.Item;


public abstract class Screen extends BaseScreen {
    private Displayable displayable;

    protected abstract Displayable createDisplayable();

    protected Command getSelectItemCommand() {
        //  TODO: i18n for OK string - bug 6781618
        return new Command("OK", Command.OK, 1);
    }

    protected static Form
    createForm() {
        return new Form(null) {
            protected void sizeChanged(int w, int h) {
                recalcSpacers(this);
            }
        };
    }

    protected static List
    createList() {
        return new List(null, List.EXCLUSIVE | List.IMPLICIT);
    }

    private static void
    recalcSpacers(Form f) {
        // ROW_EXTRA_SPACE constant addresses extra space form adds to
        // every row. The exact value is implemntation dependant, so use some
        // reasonable though big enough default value. As we deal with small
        // screens we can expect LCDUI implementation to be responsible
        // enough not to waste much screen space just to separate rows.
        final int ROW_EXTRA_SPACE = 5;

        // 1. calculate height of all non-spacers
        int itemsHeight = ROW_EXTRA_SPACE;
        int spacersCount = 0;
        Spacer spacers[] = new Spacer[f.size()];
        for (int i = 0, size = f.size(); i < size; ++i) {
            Item item = f.get(i);
            if (item instanceof Spacer) {
                Spacer s = (Spacer)item;
                spacers[spacersCount++] = s;
            }
            else {
                // Item preferred height depends on its current preferred
                // width, so set it to be minimum between screen width and
                // default item's preferred width.
                item.setPreferredSize(
                    Math.min(
                        item.getPreferredWidth(),
                        Math.max(0, f.getWidth() - 2 * ROW_EXTRA_SPACE)),
                    -1);
                itemsHeight += item.getPreferredHeight();
                item.setPreferredSize(-1, -1);

                // add extra space
                itemsHeight += ROW_EXTRA_SPACE;
            }
        }

        int dh = f.getHeight() - itemsHeight;
        if (dh <= 0 || 0 == spacersCount) {
            return;
        }

        // 2. redistribute "dh" between spacers
        int spacerHeight = dh / spacersCount; // TBD: round up issue
        for (int i = 0; i < spacersCount; ++i) {
            spacers[i].setPreferredSize(-1, spacerHeight);
        }
    }

    public Screen(ScreenProperties props) {
        super(props);
    }

    synchronized Displayable getDisplayable() {
        if (displayable == null) {
            displayable = createDisplayable();
        }
        return displayable;
    }
}
