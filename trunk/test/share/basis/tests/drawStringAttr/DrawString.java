/*
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

import java.awt.*;
import java.awt.font.*;
import java.text.*;
import java.util.*;


public class DrawString extends Frame
{
	public static void main(String[] args)
	{
		DrawString ds = new DrawString();
		ds.setSize(640, 480);
		ds.setVisible(true);

		// This string isn't really used ... its legacy.
		String bigStr = "A really cool string that is always having things happen to it!";
		char[] ca = new char[bigStr.length()];

		for(char i=0;i<bigStr.length();i++)
			ca[i] = (char)((i%10) + '0');

		String idxNums = new String(ca);	

		Graphics g = ds.getGraphics();

		Random rm = new Random();

		int maxRange = bigStr.length() - 1;
		String[] fnFamily = {"Dialog", "Monospaced", "DialogInput", "Serif", "SansSerif"};

		for(int counter=0;counter<1000;counter++)
		{
			// The info string may have to be split up in order
			// to be more readable...
			String info = "";
			int idxFrom, idxTo, fnSize;

//			AttributedString ats = new AttributedString(bigStr);
			AttributedString ats = new AttributedString(idxNums);

			idxFrom = rm.nextInt(maxRange-2);
			idxTo = rm.nextInt(maxRange-idxFrom) + 1 + idxFrom;
			String na = fnFamily[rm.nextInt(fnFamily.length)];
			info += "NA=(" + idxFrom + "," + idxTo + ")" + na;
			ats.addAttribute(TextAttribute.FAMILY, na, idxFrom, idxTo);

			idxFrom = rm.nextInt(maxRange-2);
			idxTo = rm.nextInt(maxRange-idxFrom) + 1 + idxFrom;
			Color cl = new Color(rm.nextInt(0xFFFFFF));
			info += " FG=(" + idxFrom + "," + idxTo + ")" + cl;
			ats.addAttribute(TextAttribute.FOREGROUND, cl, idxFrom, idxTo);

			idxFrom = rm.nextInt(maxRange-2);
			idxTo = rm.nextInt(maxRange-idxFrom) + 1 + idxFrom;
			Float fl = new Float(rm.nextFloat()*128.0);
			info += " SZ=(" + idxFrom + "," + idxTo + ")" + fl;
			fnSize = (int)(fl.floatValue()+0.5);
			ats.addAttribute(TextAttribute.SIZE, fl, idxFrom, idxTo);

			idxFrom = rm.nextInt(maxRange-2);
			idxTo = rm.nextInt(maxRange-idxFrom) + 1 + idxFrom;
			Integer in = rm.nextBoolean()?TextAttribute.UNDERLINE_ON:new Integer(-1);
			info += " UL=(" + idxFrom + "," + idxTo + ")" + in;
			ats.addAttribute(TextAttribute.UNDERLINE, in, idxFrom, idxTo);

			idxFrom = rm.nextInt(maxRange-2);
			idxTo = rm.nextInt(maxRange-idxFrom) + 1 + idxFrom;
			Boolean bl = new Boolean(rm.nextBoolean());
			info += " ST=(" + idxFrom + "," + idxTo + ")" + bl;
			ats.addAttribute(TextAttribute.STRIKETHROUGH, bl, idxFrom, idxTo);

			idxFrom = rm.nextInt(maxRange-2);
			idxTo = rm.nextInt(maxRange-idxFrom) + 1 + idxFrom;
			fl = rm.nextBoolean()?TextAttribute.WEIGHT_BOLD:TextAttribute.WEIGHT_REGULAR;
			info += " WI=(" + idxFrom + "," + idxTo + ")" + fl;
			ats.addAttribute(TextAttribute.WEIGHT, fl, idxFrom, idxTo);

			idxFrom = rm.nextInt(maxRange-2);
			idxTo = rm.nextInt(maxRange-idxFrom) + 1 + idxFrom;
			fl = rm.nextBoolean()?TextAttribute.POSTURE_OBLIQUE:TextAttribute.POSTURE_REGULAR;
			info += " PO=(" + idxFrom + "," + idxTo + ")" + fl;
			ats.addAttribute(TextAttribute.POSTURE, fl, idxFrom, idxTo);

			g.setColor(Color.black);
			g.fillRect(0, 0, 640, 480);

			g.setColor(Color.red);
			g.setFont(new Font("Dialog", Font.PLAIN, 12));
			g.drawString(info, 20, 50);

			g.drawString(ats.getIterator(), 20, 400);

			// This could be changed to asked some a yes/no verification
			// Or simply just wait longer and have the user remember
			// whether it passed or not.
			try{Thread.sleep(250);}catch(Exception e) { }

		}

/*   This is a simpler version and not used.
		AttributedString ats = new AttributedString("Red String with big blue underline");
		g.drawString(ats.getIterator(), 20, 40);

		ats.addAttribute(TextAttribute.FOREGROUND, Color.blue, 3, 7);
		g.drawString(ats.getIterator(), 20, 70);

		ats.addAttribute(TextAttribute.SIZE, new Float(64), 8, 10);
		g.drawString(ats.getIterator(), 20, 170);

		ats.addAttribute(TextAttribute.SIZE, new Float(64), 6, 10);
		g.drawString(ats.getIterator(), 20, 270);

		ats.addAttribute(TextAttribute.UNDERLINE, TextAttring.UNDERLINE_ON, 7, 11);
		g.drawString(ats.getIterator(), 20, 370);
*/
	}

}
