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

package buildtimefontgenerator;

import java.awt.Color;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.image.BufferedImage;
import java.util.Vector;

public class Main {
    
    private static final String COPYRIGHT = 
"# Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.\n" +
"# DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER\n" +
"# \n" +
"# This program is free software; you can redistribute it and/or\n" +
"# modify it under the terms of the GNU General Public License version\n" +
"# 2 only, as published by the Free Software Foundation.\n" +
"# \n" +
"# This program is distributed in the hope that it will be useful, but\n" +
"# WITHOUT ANY WARRANTY; without even the implied warranty of\n" +
"# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU\n" +
"# General Public License version 2 for more details (a copy is\n" +
"# included at /legal/license.txt).\n" +
"# \n" +
"# You should have received a copy of the GNU General Public License\n" +
"# version 2 along with this work; if not, write to the Free Software\n" +
"# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA\n" +
"# 02110-1301 USA\n" +
"# \n" +
"# Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa\n" +
"# Clara, CA 95054 or visit www.sun.com if you need additional\n" +
"# information or have any questions.\n";
    
    private static final int WHITE_RGB = Color.WHITE.getRGB();
    private static final int BLACK_RGB = Color.BLACK.getRGB();

    private static boolean failed = false;
    private static String fontName;
    private static int fontStyle;
    private static int fontSize;
    private static int overrideWidth;
    private static Vector<Integer> ranges = new Vector<Integer>(2);

    private static void fail(String string) {
        System.err.println("Error: " + string);
        System.exit(1);
    }

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        if (args.length == 0) {
            System.err.println("Arguments are:\n" +
                    "font-name: <font-name> {font-style: italic|bold} font-size: <font-size> {range: <start> <end>} [override-width: <width>]");
            return;
        }
        
        for (int i = 0; i < args.length; i++) {
            String arg = args[i];
            if (i == args.length - 1) {
                fail("");
                break;
            }
            if (arg.equals("font-name:")) {
                fontName = args[++i].replace('+', ' ');
            } else if (arg.equals("font-style:")) {
                String style = args[++i];
                if (style.equals("bold"))
                    fontStyle |= Font.BOLD;
                else if (style.equals("italic"))
                    fontStyle |= Font.ITALIC;
                else
                    fail("Invalid value for font-style");
            } else if (arg.equals("font-size:")) {
                try {
                    fontSize = Integer.decode(args[++i]);
                }
                catch (NumberFormatException ex) {
                    fail("Invalid value provided for font-size");
                }
            } else if (arg.equals("override-width:")) {
                try {
                    overrideWidth = Integer.decode(args[++i]);
                }
                catch (NumberFormatException ex) {
                    fail("Invalid value provided for font-size");
                }
            } else if (arg.equals("range:")) {
                try {
                    int start, end;
                    start = Integer.decode(args[++i]);
                    if (i == args.length) {
                        fail("No value provided for range end");
                    } else {
                        end = Integer.decode(args[++i]);
                        if ((start & 0xff00) != (end & 0xff00))
                            fail("Chameleon does not support ranges rolling over code page boundary");
                        if (start > 0xffff || start < 0)
                            fail("Invalid range specified. Note that surrogates are not supported");
                        if ((start & 0xf800) == 0xd800)
                            fail("Invalid range specified. Note that surrogates are not supported");
                        ranges.add(start);
                        ranges.add(end);
                    }
                }
                catch (NumberFormatException ex) {
                    fail("Invalid value provided for range");
                }
            } else
                fail("Unrecognized option name " + arg);
        }
        
        if (fontName == null || ranges.size() < 2 || fontSize == 0) {
            fail("font name, size and range are mandatory arguments");
        }
        
        generate();
    }

    private static void generate() {

        Font font = new Font(fontName, fontStyle, fontSize);
        BufferedImage glyphBitmap = new BufferedImage(1, 1, BufferedImage.TYPE_BYTE_BINARY);
        FontMetrics fm = glyphBitmap.getGraphics().getFontMetrics(font);
        int bitmapWidth = overrideWidth == 0 ? fm.getMaxAdvance() : overrideWidth;
        int bitmapHeight = fm.getHeight();
        int baseline = fm.getAscent();
        char[] charToRender = new char[1];
        glyphBitmap = new BufferedImage(bitmapWidth, bitmapHeight, BufferedImage.TYPE_BYTE_BINARY);
        Graphics gr = glyphBitmap.getGraphics();
        gr.setFont(font);

        System.out.println(COPYRIGHT);
        System.out.println("# Font parameters:\n        \n# width height ascent descent leading");
        System.out.println("@ " + bitmapWidth + ' ' + bitmapHeight + ' ' + fm.getAscent() + ' ' + fm.getDescent() + ' ' + fm.getLeading());
        System.out.println("# high_byte first_code_low_byte last_code_low_byte");

        for (int i = 0; i < ranges.size(); i += 2) {
            int start = ranges.get(i);
            int end = ranges.get(i + 1);

            System.out.println("% " + Integer.toHexString(start >> 8) + ' ' + 
                    Integer.toHexString(start & 0xff) + ' ' + 
                    Integer.toHexString(end & 0xff));
            for (int ch = (char)start; ch <= end; ch++) {
                charToRender[0] = (char)ch;
                gr.setColor(Color.WHITE);
                gr.fillRect(0, 0, bitmapWidth, bitmapHeight);
                gr.setColor(Color.BLACK);
                gr.drawChars(charToRender, 0, 1, 0, baseline);
                dumpCharGlyph(ch, glyphBitmap);
            }
        }
    }

    private static void dumpCharGlyph(int ch, BufferedImage glyphBitmap) {
        int width = glyphBitmap.getWidth();
        int height = glyphBitmap.getHeight();
        drawHR(width);
        System.out.println(": " + Integer.toHexString(ch));
        drawHR(width);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                char code;
                int color = glyphBitmap.getRGB(x, y);
                if (color == WHITE_RGB)
                    code = ' ';
                else if (color == BLACK_RGB)
                    code = '*';
                else
                    code = 'x';
                System.out.print(code);
            }
            System.out.println(".");
        }
        drawHR(width);
    }

    private static void drawHR(int width) {
        System.out.print("#");
        for (int i = 0; i < width; i++) {
            System.out.print('-');
        }
        System.out.println();
    }

}
