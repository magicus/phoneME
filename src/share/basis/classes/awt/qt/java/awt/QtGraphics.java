/*
 * @(#)QtGraphics.java	1.9 06/10/10
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

package java.awt;

import java.io.*;
import java.util.*;
import java.awt.image.BufferedImage;
import java.awt.image.ImageObserver;
import sun.awt.ConstrainableGraphics;
import java.text.AttributedCharacterIterator;
import java.awt.font.TextAttribute;

/**
 * QtGraphics is an object that encapsulates a graphics context for drawing with Qt.
 *
 * @version 1.12, 11/27/01
 */

final class QtGraphics extends Graphics2D implements ConstrainableGraphics {

	/** The default font to use when no font has been set or null is used. */

	private static final Font DEFAULT_FONT = new Font("Dialog", Font.PLAIN, 12);

	private AlphaComposite composite;

	/** The current foreground color. */

	private Color   foreground;

	/** The current background color. This is used by clearRect. */

	private Color   background;

	/** The current xor color. If null then we are in paint mode. */

	private Color   xorColor;

	/** The currently selected font for text operations. */

	private Font    font;

	/** The font metrics for the font. Cached for performance. */

	private QtFontMetrics fontMetrics;

	/** Translated X offset from native offset. */

	private int     originX;

	/** Translated Y offset from native offset. */

	private int     originY;

	/** The index of the native peer for this graphics object. */

	int             psd;

	/** The current user clip rectangle or null if no clip has been set. This is stored in the
	 native coordinate system and not the possibly translated Java coordinate system. */

	private Rectangle      clip;

	private Stroke		stroke;
	private int		lineWidth;

//	private int	clipWidth, clipHeight;

	/** The rectangle this graphics object has been constrained too. This is stored in the
	native coordinate system and not the (possibly) translated Java coordinate system.
	 If it is null then this graphics has not been constrained. The constrained rectangle
	 is another layer of clipping independant of the user clip. The actaul clip used is the
	 intersection */

	private Rectangle constrainedRect;

	/** The GraphicsConfiguration that this graphics uses. */

	private GraphicsConfiguration graphicsConfiguration;

	static native int pCloneGraphPSD(int Gpsd, boolean fresh);
	static native int pCloneImagePSD(int Ipsd, boolean fresh);

	static native void pDisposePSD(int psd);

	private static native void pSetColor(int psd, int argb);

	private static native void pSetFont(int psd, int font);

	private static native void pSetStroke(int psd, int width, int cap, int join, int style);

	private static native void pSetComposite(int psd, int rule, int extraAlpha);

	private static native void pSetPaintMode(int psd);

	private static native void pSetXORMode(int psd, int rgb);

	private static native void pChangeClipRect(int psd, int x, int y, int w, int h);

	private static native void pRemoveClip(int psd);

	private static native void pDrawString(int psd, String string, int x, int y);

	private static native void pFillRect(int psd, int x, int y, int width, int height);

	private static native void pClearRect(int psd, int x, int y, int width, int height, int rgb);

	private static native void pDrawRect(int psd, int x, int y, int width, int height);

	private static native void pCopyArea(int psd, int x, int y, int width, int height, int dx, int dy);

	private static native void pDrawLine(int psd, int x1, int y1, int x2, int y2);

	private static native void pDrawPolygon(int psd, int orinX, int originY, int xPoints[], int yPoints[], int nPoints, boolean close);

	private static native void pFillPolygon(int psd, int orinX, int originY, int xPoints[], int yPoints[], int nPoints);

	private static native void pDrawArc(int psd, int x, int y, int width, int height, int start, int end);

	private static native void pFillArc(int psd, int x, int y, int width, int height, int start, int end);

	private static native void pDrawOval(int psd, int x, int y, int width, int height);

	private static native void pFillOval(int psd, int x, int y, int width, int height);

	private static native void pDrawRoundRect(int psd, int x, int y, int width, int height, int arcWidth, int arcHeight);

	private static native void pFillRoundRect(int psd, int x, int y, int width, int height, int arcWidth, int arcHeight);

	private static native QtImage getBufferedImagePeer(BufferedImage bi);

	QtGraphics(Window window) {
		try{
			psd = pCloneGraphPSD(((QtGraphicsDevice) 
                window.getGraphicsConfiguration().getDevice()).psd, false);
		} catch(OutOfMemoryError e)
		{
			System.gc();
			psd = pCloneGraphPSD(((QtGraphicsDevice) window.getGraphicsConfiguration().getDevice()).psd, true);
		}

		foreground = window.foreground;

		if (foreground == null)
			foreground = Color.black;

//		clipWidth = window.getWidth()-1;
//		clipHeight = window.getHeight()-1;
//		clip = null;
//		setupClip();

		background = window.background;

		if (background == null)
			background = Color.black;

		composite = AlphaComposite.SrcOver;
		graphicsConfiguration = window.getGraphicsConfiguration();
		setFont(DEFAULT_FONT);
		setColorNative(); // 6241180
	}

	QtGraphics(QtGraphics g) {
		try{
			psd = pCloneGraphPSD(g.psd, false);
		} catch(OutOfMemoryError e)
		{
			System.gc();
			psd = pCloneGraphPSD(g.psd, true);
		}
		composite = g.composite;
		graphicsConfiguration = g.graphicsConfiguration;
		originX = g.originX;
		originY = g.originY;
//		clipWidth = g.clipWidth;
//		clipHeight = g.clipHeight;
//		clip = g.clip;
        if(g.clip!=null) {                    
            clip = new Rectangle(g.clip);
            setupClip();
        }
//		constrainedRect = g.constrainedRect;
		stroke = g.stroke;
//		setupClip();
		foreground = g.foreground;
		background = g.background;
		setColorNative(); // 6241180
		setFont(g.font);
	}

	QtGraphics(QtOffscreenImage image) {
		try{
			psd = pCloneImagePSD(image.psd, false); 
		} catch(OutOfMemoryError e)
		{
			System.gc();
			psd = pCloneImagePSD(image.psd, true);
		}

		foreground = image.foreground;

		if (foreground == null)
			foreground = Color.black;

		background = image.background;

		if (background == null)
			background = Color.black;

		composite = AlphaComposite.SrcOver;
		graphicsConfiguration = image.gc;

        // 5104620
        // If the image.font is null, use the default font. Fixes the 
        // crash in the native code
		setFont(((image.font == null) ? DEFAULT_FONT : image.font));

        // 6205694,6205693,6206487,6206315
        // Changed the width and height to the image's width and 
        // height. Got sidetracked by the "clipWidth" and "clipHeight"
		pChangeClipRect(psd, 0,0,image.getWidth(),image.getHeight());
        // 6205694,6205693,6206487,6206315,
	}

	QtGraphics(QtSubimage image, int x, int y) {
		try{
			psd = pCloneImagePSD(image.psd, false);
		} catch(OutOfMemoryError e)
		{
			System.gc();
			psd = pCloneImagePSD(image.psd, true);
		}

		composite = AlphaComposite.SrcOver;

		originX = x;
		originY = y;

//		clipWidth = image.getWidth() - 1 + originX;
//		clipHeight = image.getHeight() - 1 + originY;
		clip = null;
        pChangeClipRect(psd, x, y, 
                        image.getWidth()-1 + x, 
                        image.getHeight()-1 + y);	
	}

	/**
	 * Create a new QtGraphics Object based on this one.
	 */
	public Graphics create() {
		return new QtGraphics(this);
	}

	/**
	 * Translate
	 */
	public void     translate(int x, int y) {
		originX += x;
		originY += y;
	}

	public void finalize() {
		dispose();
		try { super.finalize(); } catch(Throwable t) { };
	}

	public final void dispose() {
		if (psd > 0)
		{
            pDisposePSD(psd);
			psd = -1;
		}
	}

	private void setFont(Font font, boolean backwardCompat)
	{
		this.font = font;
		fontMetrics = QtFontMetrics.getFontMetrics(font, backwardCompat);
	    pSetFont(psd, fontMetrics.nativeFont);
	}
	
	public void setFont(Font font) {
		if (font != null && !font.equals(this.font))
			setFont(font, true);			
	}

	public Font     getFont() {
		return font;
	}

	/**
	 * Gets the font metrics of the current font.
	 * @return    the font metrics of this graphics
	 *                    context's current font.
	 * @see       java.awt.Graphics#getFont
	 * @see       java.awt.FontMetrics
	 * @see       java.awt.Graphics#getFontMetrics(Font)
	 * @since     JDK1.0
	 */
	public FontMetrics getFontMetrics() {
		return fontMetrics;
	}

	/**
	 * Gets font metrics for the given font.
	 */
	public FontMetrics getFontMetrics(Font font) {
		return QtFontMetrics.getFontMetrics(font, true);
	}

	public void setStroke(Stroke stroke)
	{
		/* Don't know about this case !!! FIXME!!! */
		if(stroke == null) {
			this.stroke = null;
			return;
		}

		if(!(stroke instanceof BasicStroke))
            // 6189931: Throwing IAE as per the spec instead of
            // UnsupportedOperationException
			throw new IllegalArgumentException("Only BasicStroke Supported!");
            // 6189931

		this.stroke = stroke;

		BasicStroke bs = (BasicStroke)stroke;
		int cap, join;

		lineWidth = (int)bs.getLineWidth();
		cap = bs.getEndCap();
		join = bs.getLineJoin();

		pSetStroke(psd, lineWidth, cap, join, 0);
	}

	public Stroke getStroke() {
		return stroke;
	}


	/**
	 * Sets the foreground color.
	 */

	public void     setColor(Color c) {
		if ((c != null) && (c != foreground)) {
			foreground = c;
            setColorNative(); // 6241180
		}
	}

    // 6241180
    // Uses this.foreground and this.composite to figure out if a new
    // foreground color should be set for SRC rule and Extra Alpha 0.0
    private void setColorNative() {
         float[] rgbaSrc = foreground.getRGBComponents(null);
         // compute the actual alpha of the source color using the
         // composite's extraAlpha and source color alpha
         float alphasource = composite.getAlpha() * rgbaSrc[3];
         // if the colormodel does not have an alpha channel, and the
         // source has zero intensity alpha, then the source color
         // that should be copied as part of the SRC rule is forced
         // to black (the color channels multiplied by the alpha)
         Color fg = this.foreground;
         if ( this.composite.getRule() == AlphaComposite.SRC &&
              !this.graphicsConfiguration.getColorModel().hasAlpha() &&
              alphasource == 0.0f) {
             fg = new Color(0.0f, 0.0f, 0.0f, 1.0f);
         }
         pSetColor(psd, fg.value);
    }
    // 6241180

	public Color    getColor() {
		return foreground;
	}

	public Composite getComposite()
	{
		return composite;
	}

	public GraphicsConfiguration getDeviceConfiguration()
	{
		return graphicsConfiguration;
	}

	public void setComposite(Composite comp)
	{
        if ((comp != null) && (comp != composite)) {
		    if (!(comp instanceof AlphaComposite))
			    throw new IllegalArgumentException("Only AlphaComposite is supported");

		    composite = (AlphaComposite)comp;

			pSetComposite(psd, composite.rule, 
                          (int)(composite.extraAlpha*0xFF));
            setColorNative(); // 6241180
	    }
	}

	/**
	 * Sets the paint mode to overwrite the destination with the
	 * current color. This is the default paint mode.
	 */
	public void     setPaintMode() {
			xorColor = null;

			pSetPaintMode(psd);
	}

	/**
	 * Sets the paint mode to alternate between the current color
	 * and the given color.
	 */
	public void     setXORMode(Color color) {
			xorColor = color;

			pSetXORMode(psd, color.value);
	}

	/** Clears the rectangle indicated by x,y,w,h. */

	final public void     clearRect(int x, int y, final int w, final int h) {

		x += originX;
		y += originY;

	    pClearRect(psd, x, y, w, h, background.value);
	}

	/** Fills the given rectangle with the foreground color. */

	public void     fillRect(final int x, final int y, final int w, final int h) {
        pFillRect(psd, x + originX, y + originY, w, h);
	}

	/** Draws the given rectangle. */
	public void     drawRect(final int x, final int y, final int w, final int h) {
	    pDrawRect(psd, x + originX, y + originY, w, h);
	}

	/** Draws the given line. */

	public void     drawLine(final int x1, final int y1, final int x2, final int y2) {
	    pDrawLine(psd, x1 + originX, y1 + originY, x2 + originX, y2 + originY);
	}

	/**
	 * Copies an area of the canvas that this graphics context paints to.
	 * @param X the x-coordinate of the source.
	 * @param Y the y-coordinate of the source.
	 * @param W the width.
	 * @param H the height.
	 * @param dx the horizontal distance to copy the pixels.
	 * @param dy the vertical distance to copy the pixels.
	 */
	public void     copyArea(int X, int Y, final int W, final int H, final int dx, final int dy) {
        X += originX;
        Y += originY;
        pCopyArea(psd, X, Y, W, H, dx, dy);
	}

	/** Draws lines defined by an array of x points and y points */

	public void     drawPolyline(final int xPoints[], final int yPoints[], final int nPoints) {
	    pDrawPolygon(psd, originX, originY, xPoints, yPoints, nPoints, false);
	}

	/** Draws a polygon defined by an array of x points and y points */

	public void     drawPolygon(final int xPoints[], final int yPoints[], final int nPoints) {
	    pDrawPolygon(psd, originX, originY, xPoints, yPoints, nPoints, true);
	}

	/** Fills a polygon with the current fill mask */

	public void     fillPolygon(final int xPoints[], final int yPoints[], final int nPoints) {
	    pFillPolygon(psd, originX, originY, xPoints, yPoints, nPoints);
	}

	/** Draws an oval to fit in the given rectangle */
	public void     drawOval(final int x, final int y, final int w, final int h) {
	    pDrawOval(psd, x + originX, y + originY, w, h);
	}

	/** Fills an oval to fit in the given rectangle */
	public void     fillOval(final int x, final int y, final int w, final int h) {
	    pFillOval(psd, x + originX, y + originY, w, h);
	}

	/**
	 * Draws an arc bounded by the given rectangle from startAngle to
	 * endAngle. 0 degrees is a vertical line straight up from the
	 * center of the rectangle. Positive start angle indicate clockwise
	 * rotations, negative angle are counter-clockwise.
	 */
	public void     drawArc(final int x, final int y, final int w, final int h, final int startAngle, final int endAngle) {
	    pDrawArc(psd, x + originX, y + originY, w, h, startAngle, endAngle);
	}

	/** fills an arc. arguments are the same as drawArc. */
	public void     fillArc(final int x, final int y, final int w, final int h, final int startAngle, final int endAngle) {
	    pFillArc(psd, x + originX, y + originY, w, h, startAngle, endAngle);
	}

	/** Draws a rounded rectangle. */
	public void     drawRoundRect(final int x, final int y, final int w, final int h, final int arcWidth, final int arcHeight) {
	    pDrawRoundRect(psd, x + originX, y + originY, w, h, arcWidth, arcHeight);
	}

	/** Draws a filled rounded rectangle. */
	public void     fillRoundRect(final int x, final int y, final int w, final int h, final int arcWidth, final int arcHeight) {
	    pFillRoundRect(psd, x + originX, y + originY, w, h, arcWidth, arcHeight);
	}

	/** Draws the given string. */
	public void     drawString(final String string, final int x, final int y) {
        // 6259215
        // throw NPE instead of returning
		if (string == null)
            throw new NullPointerException("string is null");
        // 6259215
		
	    pDrawString(psd, string, x + originX, y + originY);
	}

	
	public void drawString(AttributedCharacterIterator iterator, int x, int y)
	{
		char c = iterator.first();
		Graphics ng = new QtGraphics(this);
		
		while(c != iterator.DONE)
		{
			Map attrs = iterator.getAttributes();
			Object obj;
			FontMetrics fm;
			Font fn;
						
            if ((obj = attrs.get(TextAttribute.FONT)) != null)
				fn = (Font)obj;
			else
				fn = new Font(attrs);
						
			((QtGraphics)ng).setFont(fn, false);
			fm = ng.getFontMetrics();
		
			// Might have to query the FONT first to see if it has a Colour set.
            if ((obj = attrs.get(TextAttribute.FOREGROUND)) != null)
				ng.setColor((Color)obj);
			else
				ng.setColor(this.foreground);

			int runIdx = iterator.getRunLimit();
			char[] str = new char[runIdx-iterator.getIndex()];

			for(int lc=0; iterator.getIndex() < runIdx; c = iterator.next())
				str[lc++] = c;
		
			String ns = new String(str);
			
			ng.drawString(ns, x, y);
			
			x += fm.stringWidth(ns);
		}

		ng.dispose();
	}
	
	/** Draws the given character array. */
	public void     drawChars(final char chars[], final int offset, final int length, final int x, final int y) {
	    pDrawString(psd, new String(chars, offset, length), x + originX, y + originY);
	}

	/**
	 * Gets the current clipping area
	 */
    public Rectangle getClipBounds() {
		if (clip != null)
			return new Rectangle (clip.x - originX, clip.y - originY, clip.width, clip.height);

		return null;
    }

	/** Returns a Shape object representing the MicroWindows clip. */

	public Shape    getClip() {
		return getClipBounds();
	}

	public void constrain(final int x, final int y, final int w, final int h) {
		originX += x;
		originY += y;
		Rectangle rect = new Rectangle(originX, originY, w, h);

		if (constrainedRect != null)
			constrainedRect = constrainedRect.intersection(rect);

		else constrainedRect = rect;

		setupClip();
	}

	/** Crops the clipping rectangle for this MicroWindows context. */

	public void clipRect(final int x, final int y, final int w, final int h) {
        Rectangle rect = new Rectangle(x + originX, y + originY, w, h);

        if (clip != null)
            clip = clip.intersection(rect);
        else
            clip = rect;
        
        setupClip();
	}

	/** Sets up the clip for this Graphics. The clip is the result of combining the user clip
	 with the constrainedRect. */

	private void setupClip() {
        int clipX, clipY, clipW, clipH;
		if (constrainedRect != null) {
            Rectangle c = this.constrainedRect ;
			if (clip != null) {
                c = constrainedRect.intersection(clip);
			}
		    clipX = c.x;
		    clipY = c.y;
		    clipW = c.width;
		    clipH = c.height;
		}
		else 
		if (clip != null) {
            clipX = clip.x;
            clipY = clip.y;
            clipW = clip.width;
            clipH = clip.height;
        }
        else {
            return; // this should not happen
        } 

	    pChangeClipRect(psd, clipX, clipY, clipW, clipH);
	}

	/** Sets the clipping rectangle for this X11Graphics context. */
	public void     setClip(final int x, final int y, final int w, final int h) {
		if(clip == null)
			clip = new Rectangle (x + originX, y + originY, w, h);
		else
		{
			clip.x = x+originX;
			clip.y = y+originY;
			clip.width = w;
			clip.height = h;
		}
		setupClip();
	}

	/** Sets the clip to a Shape (only Rectangle allowed). */

	public void  setClip(Shape clip) {
		if (clip == null) {
            this.clip = null;
		    pRemoveClip(psd);
		} else if (clip instanceof Rectangle) {
			Rectangle       rect = (Rectangle) clip;

			setClip(rect.x, rect.y, rect.width, rect.height);
		} else
			throw new IllegalArgumentException("setClip(Shape) only supports Rectangle objects");
	}

	/**
	 * Draws an image at x,y in nonblocking mode with a callback object.
	 */
	public boolean  drawImage(Image img, final int x, final int y, ImageObserver observer) {
		return drawImage(img, x, y, null, observer);
	}

	/**
	 * Draws an image at x,y in nonblocking mode with a solid background
	 * color and a callback object.
	 */
	public boolean  drawImage(Image img, final int x, final int y, Color bg,
		ImageObserver observer) {

		QtImage qtimg;

		if (img == null)
					throw new NullPointerException("Image can't be null");

		if (img instanceof BufferedImage)
			qtimg = getBufferedImagePeer((BufferedImage) img);
		else if(img instanceof QtVolatileImage)
			qtimg = ((QtVolatileImage)img).qtimage;
		else
			qtimg = (QtImage) img;
		
                boolean isComplete = qtimg.isComplete();
                if(!isComplete) {
                    qtimg.addObserver(observer);                
                    qtimg.startProduction();
                    isComplete = qtimg.isComplete();                    
                }

                int width = qtimg.getWidth();
                int height = qtimg.getHeight();

		if (width > 0 && height > 0) {		
            qtimg.drawImage(psd, x + originX, y + originY, bg);
        }

		return isComplete;
	}

	/**
	 * Draws an image scaled to x,y,w,h in nonblocking mode with a
	 * callback object.
	 */
	public boolean  drawImage(Image img, final int x, final int y, final int width, final int height,
		ImageObserver observer) {

		return drawImage(img, x, y, width, height, null, observer);
	}

	/**
	 * Draws an image scaled to x,y,w,h in nonblocking mode with a
	 * solid background color and a callback object.
	 */
	public boolean  drawImage(Image img, int x, int y, final int width, final int height,
		Color bg, ImageObserver observer) {
		QtImage qtimg;

		if (img == null)
					throw new NullPointerException("Image can't be null");

		if (img instanceof BufferedImage)
			qtimg = getBufferedImagePeer((BufferedImage) img);
		else if(img instanceof QtVolatileImage)
			qtimg = ((QtVolatileImage)img).qtimage;
		else
			qtimg = (QtImage) img;
                
                boolean isComplete = qtimg.isComplete();
                if(!isComplete) {
                    qtimg.addObserver(observer);                
					qtimg.startProduction();
                    isComplete = qtimg.isComplete();                    
                }
                
		int imgWidth = qtimg.getWidth();
		int imgHeight = qtimg.getHeight();

		if (imgWidth > 0 && imgHeight > 0) {		

                    x += originX;
                    y += originY;
	
			if(imgWidth==width&&imgHeight==height)
                            qtimg.drawImage(psd, x, y, bg); 
			else
           		    qtimg.drawImage(psd, x, y, x + width - 1, y + height - 1, 0, 0, imgWidth - 1, imgHeight - 1, bg);
        }

		return isComplete;
	}

	/**
	 * Draws a subrectangle of an image scaled to a destination rectangle
	 * in nonblocking mode with a callback object.
	 */
	public boolean  drawImage(Image img,
		final int dx1, final int dy1, final int dx2, final int dy2,
		final int sx1, final int sy1, final int sx2, final int sy2,
		ImageObserver observer) {

		return drawImage(img, dx1, dy1, dx2, dy2, sx1, sy1, sx2, sy2, null, observer);
	}

	/**
	 * Draws a subrectangle of an image scaled to a destination rectangle in
	 * nonblocking mode with a solid background color and a callback object.
	 */
	public boolean  drawImage(Image img,
		final int dx1, final int dy1, final int dx2, final int dy2,
		final int sx1, final int sy1, final int sx2, final int sy2,
		Color bg, ImageObserver observer) {

		QtImage qtimg;

		if (img == null)
					throw new NullPointerException("Image can't be null");

		if (img instanceof BufferedImage)
			qtimg = getBufferedImagePeer((BufferedImage) img);
		else if(img instanceof QtVolatileImage)
			qtimg = ((QtVolatileImage)img).qtimage;
		else
			qtimg = (QtImage) img;
			
                boolean isComplete = qtimg.isComplete();
                if(!isComplete) {
                    qtimg.addObserver(observer);                
                    qtimg.startProduction();
                    isComplete = qtimg.isComplete();                    
                }
                
		int imgWidth = qtimg.getWidth();
		int imgHeight = qtimg.getHeight();

		if (imgWidth > 0 || imgHeight > 0) {			
	        qtimg.drawImage(psd, dx1 + originX, dy1 + originY, dx2 + originX, dy2 + originY, sx1, sy1, sx2, sy2, bg);
		}

		return isComplete;
	}

	public String   toString() {
		return getClass().getName() + "[" + originX + "," + originY + "]";
	}

}






