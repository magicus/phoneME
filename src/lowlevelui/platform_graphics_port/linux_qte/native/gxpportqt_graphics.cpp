/*
 *   
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
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
 * 
 * This source file is specific for Qt-based configurations.
 */
#include <midp_logging.h>

#include <qimage.h>
#include <qbitmap.h>
#include <qteapp_export.h>

#include <gxpport_graphics.h>
#include <gxpportqt_image.h>

/**
 * @file
 *
 * Graphics primitive functions for Linux QTE.
 *
 *  contains all the implementation of primitive
 * graphics for Qt. 
 */

/**
 * Draw triangle
 *
 * @param pixel The packed pixel value
 */
extern "C" void
gxpport_fill_triangle(int pixel, const jshort *clip, 
		      gxpport_mutableimage_native_handle dst, int dotted, 
		      int x1, int y1, int x2, int y2, int x3, int y3) {

    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    REPORT_INFO6(LC_LOWUI, 
		 "LF:gxpport_fill_triangle(%d, %d, %d, %d, %d, %d)\n",
		 x1, y1, x2, y2, x3, y3);

    MScreen * mscreen = qteapp_get_mscreen();
    QPointArray pts;
    QPainter *gc = mscreen->setupGC(-1, pixel, clip, 
				    (QPaintDevice*)qpixmap, dotted);
    pts.setPoints(3, x1, y1, x2, y2, x3, y3);
    gc->drawPolygon(pts); 
}

/**
 * Copy from a specify region to other region
 */
extern "C" void
gxpport_copy_area(const jshort *clip, 
              gxpport_mutableimage_native_handle dst, int x_src, int y_src, 
	      int width, int height, int x_dest, int y_dest) {
    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    // IMPL_NOTE:copyarea does not use clipping? 
    (void)clip; // Work around the warning


    REPORT_INFO6(LC_LOWUI, 
		 "LF:gxpport_copy_area called (%d, %d) %dw %dh -> (%d, %d)\n",
		 x_src, y_src, width, height, x_dest, y_dest);

    bitBlt((QPaintDevice *)qpixmap, x_dest, y_dest, (QPaintDevice *)qpixmap, 
	   x_src, y_src, width, height);
}

/**
 * Draw image in RGB format
 */
extern "C" void
gxpport_draw_rgb(const jshort *clip, 
		 gxpport_mutableimage_native_handle dst, jint *rgbData, 
		 jint offset, jint scanlen, jint x, jint y, 
		 jint width, jint height, jboolean processAlpha) {

    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    /* (offset/scanlen) * 4 */
    int rowOffset = (offset/scanlen) * 4;
  
    QImage image((unsigned char*)rgbData,
		 scanlen, height, IMAGE_DEPTH, NULL, 0, QImage::IgnoreEndian);

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(-1, -1, clip, (QPaintDevice *)qpixmap, 0);

    image.setAlphaBuffer( (processAlpha == KNI_TRUE) ? TRUE : FALSE );

    if (!image.isNull()) {
      int ix = (offset - rowOffset);
      gc->drawImage(x, y, image, ix, rowOffset, 
		    width, height,
		    Qt::ThresholdDither
		    | Qt::ThresholdAlphaDither
		    | Qt::AvoidDither);
    }
}

/**
 * Obtain the color that will be final shown 
 * on the screen after the system processed it.
 *
 * Note that QT use 5-6-5 color format
 * that why we shift 11 below (5+6)
 * and shift left (3 or 2) to get back
 * the exact color Qt converted to.
 *
 * 0x3F = 111111 (6's 1)
 * 0x1F = 011111 (5's 1)
 */
extern "C" int
gxpport_get_displaycolor(int color) {

    unsigned int r,g,b;
    MScreen * mscreen = qteapp_get_mscreen();
    QColor qcolor = mscreen->getColor(color);
    uint result = qcolor.alloc();

    r = (result >> 11) << 3;
    g = ((result >> 5 ) & 0x3F ) << 2; 
    b = (result & 0x1F) << 3;
    return ((r << 16) | (g << 8) | (b & 0xFF)) ;

}


/**
 * Draw a line between two points (x1,y1) and (x2,y2).
 */
extern "C" void
gxpport_draw_line(int pixel, const jshort *clip, 
              gxpport_mutableimage_native_handle dst, int dotted, 
              int x1, int y1, int x2, int y2)
{
  QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    REPORT_INFO4(LC_LOWUI, "gxpport_drawLine(%d, %d, %d, %d)\n",
		 x1, y1, x2, y2);

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(pixel, -1,
                                    clip, (QPaintDevice*)qpixmap, dotted);
    gc->drawLine(x1, y1, x2, y2);
}

/**
 * Draw a rectangle at (x,y) with the given width and height.
 *
 * @note x, y sure to be >=0
 *       since x,y is quan. to be positive (>=0), we don't
 *       need to test for special case anymore.
 */
extern "C" void 
gxpport_draw_rect(int pixel, const jshort *clip, 
              gxpport_mutableimage_native_handle dst, int dotted, 
              int x, int y, int width, int height)
{
    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    REPORT_INFO4(LC_LOWUI, "gxpport_draw_rect(%d, %d, %d, %d)\n",
		 x, y, width, height);

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(pixel, -1,
                                    clip, (QPaintDevice*)qpixmap, dotted);

    gc->drawRect(x, y, width+1, height+1);
}

/**
 * Fill a rectangle at (x,y) with the given width and height.
 */
extern "C" void 
gxpport_fill_rect(int pixel, const jshort *clip, 
              gxpport_mutableimage_native_handle dst, int dotted, 
              int x, int y, int width, int height) {
    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    REPORT_INFO4(LC_LOWUI, "LF:gxpport_fill_rect(%d, %d, %d, %d)\n",
		 x, y, width, height);


#if defined(OPTIMZATION) && defined(FILLRECT)
    if (!dst) {
	// fill/clear the entire backbuffer
	// \Need revisit check for clip
        MScreen * mscreen = qteapp_get_mscreen();
	QPixmap* buffer = mscreen->getBackBuffer();
	if (buffer && 
	    (width >= buffer->width()) &&
	    (height >= buffer->height()) &&
	    (x == 0) && (y == 0)) {

	    buffer->fill(mscreen->getColor(pixel));
	    return;
	}
    }
#endif

    // WORKAROUND: A feature in Qt, even when clip is (x,y,x,y)
    // it will still draw something!
    if (clip && (clip[0]==clip[2] || clip[1]==clip[3])) return;

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(-1, pixel, 
                                    clip, (QPaintDevice*)qpixmap, dotted);

    gc->fillRect(x, y, width, height, gc->brush());
}

/**
 * Draw a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
extern "C" void 
gxpport_draw_roundrect(int pixel, const jshort *clip, 
                   gxpport_mutableimage_native_handle dst, int dotted, 
                   int x, int y, int width, int height,
                   int arcWidth, int arcHeight)
{
    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    REPORT_INFO6(LC_LOWUI, 
		 "LF:gxpport_draw_roundrect(%d, %d, %d, %d, %d, %d) approx\n",
		 x, y, width, height, arcWidth, arcHeight);

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(pixel, -1, 
                                    clip, (QPaintDevice*)qpixmap, dotted);

    // Unlucky, w=1,h=1 will not draw anything by Qt...
    if ((width < 2) || (height < 2)) {
        gc->drawRect(x, y, width+1, height+1);
    } else {

        if (arcWidth < 0)       arcWidth = 0;
        if (arcWidth > width)   arcWidth = width;
        if (arcHeight < 0)      arcHeight = 0;
        if (arcHeight > height) arcHeight = height;

        // QT uses a range of 0..99 for round so normalize width and height
        arcWidth  = (width == 0) ? 0 : (99 * arcWidth) / width ;
        arcHeight = (height == 0) ? 0 : (99 * arcHeight) / height ;
        gc->drawRoundRect(x, y, width+1, height+1, arcWidth, arcHeight);

    }
}

/**
 * Fill a rectangle at (x,y) with the given width and height. arcWidth and
 * arcHeight, if nonzero, indicate how much of the corners to round off.
 */
extern "C" void 
gxpport_fill_roundrect(int pixel, const jshort *clip, 
                   gxpport_mutableimage_native_handle dst, int dotted, 
                   int x, int y, int width, int height,
                   int arcWidth, int arcHeight)
{
    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    REPORT_INFO6(LC_LOWUI, 
		 "LF:gxpport_fill_roundrect(%d, %d, %d, %d, %d, %d)\n",
		 x, y, width, height, arcWidth, arcHeight);


    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(-1, pixel, clip, (QPaintDevice*)qpixmap, dotted);
    QBrush brush = gc->brush();

    if ((width <= 2) || (height <= 2)) {
        if ((width>0) && (height>0)) {
            gc->drawRect(x, y, 1, 1);
	}
	return;
    }

    if (arcWidth < 0) arcWidth = 0;
    if (arcHeight < 0) arcHeight = 0;
    if (arcWidth > width) arcWidth = width;
    if (arcHeight > height) arcHeight = height;

    if ((arcWidth > 0) || (arcHeight > 0)) {

        /*
	 * Implementation note: I wish we could 
	 * use drawRoundRect() but we cannot 
	 * since MIDP spec. requires the round
	 * conner to be very exact.
	 */
        int tx1 = x + (arcWidth >> 1);
        int tx2 = x + width - (arcWidth >> 1);
        int ty1 = y + (arcHeight >> 1);
        int ty2 = y + height - (arcHeight >> 1);
        int txw = x + width;
        int tyh = y + height;

        gc->fillRect(tx1,   y, tx2 - tx1,   tyh - y, brush);
        gc->fillRect(  x, ty1,     width, ty2 - ty1, brush);

        gc->drawPie(x+1, y, arcWidth, arcHeight,
                    90<<4, 90<<4);

        gc->drawPie(txw-arcWidth, y, arcWidth, arcHeight,
                    0 /*<<4*/, 90<<4);

        gc->drawPie(txw-arcWidth, tyh - arcHeight, arcWidth, arcHeight,
                    270<<4, 90<<4);

        gc->drawPie(x+1, tyh - arcHeight , arcWidth, arcHeight,
                    180<<4, 90<<4);
    } else {
        gc->fillRect(x, y, width, height, brush);
    }
}

/**
 *
 * Draw an elliptical arc centered in the given rectangle. The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 *
 * @note: check for width, height <0 is done in share layer
 */
extern "C" void 
gxpport_draw_arc(int pixel, const jshort *clip, 
             gxpport_mutableimage_native_handle dst, int dotted, 
             int x, int y, int width, int height,
             int startAngle, int arcAngle)
{
    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    REPORT_INFO7(LC_LOWUI, 
		 "LF:gxpport_draw_arc(%d, %d, %d, %d, %d, %d) dot=%i\n",
		 x, y, width, height, startAngle, arcAngle, dotted);


    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(pixel, -1, clip, (QPaintDevice*)qpixmap, dotted);

    if ((width < 2) || (height < 2)) {
        if (width == 0 && height == 0) {
            /* Spec: The resulting arc covers and area width +1
             * pixels wide by height + 1 pixels tall.
             */
	    gc->drawPoint(x, y);
        } else {
            gc->drawLine(x, y, x + width, y + height);
        }
    } else {      
        if ((arcAngle <= -360 ) || (arcAngle >= 360)) {
            gc->drawEllipse(x, y, width, height);
	} else {
            gc->drawArc(x, y, width, height,
                    (startAngle*16), (arcAngle*16));
        }
    }
}

/**
 * Fill an elliptical arc centered in the given rectangle. The
 * portion of the arc to be drawn starts at startAngle (with 0 at the
 * 3 o'clock position) and proceeds counterclockwise by <arcAngle> 
 * degrees.  arcAngle may not be negative.
 */
extern "C" void 
gxpport_fill_arc(int pixel, const jshort *clip, 
             gxpport_mutableimage_native_handle dst, int dotted, 
             int x, int y, int width, int height,
             int startAngle, int arcAngle)
{
    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);

    // IMPL_NOTE:fillArc does not use clipping? 
    (void)clip; // Work around the warning

    // Dotted is not used here, instead, it is setting up
    // in graphics context call.
    (void)dotted; // Work around the warning


    REPORT_INFO7(LC_LOWUI, 
		 "LF:gxpport_fill_arc(%d, %d, %d, %d, %d, %d) dotted=%i\n",
		 x, y, width, height, startAngle, arcAngle, dotted);

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(-1, pixel,
                                    clip, (QPaintDevice*)qpixmap, 0);

    /*
     * @note Since Qt draws NOTHING when w==1,h==1
     * but the spec. required to draw a dot, we have to
     * draw it specially. Also, can't use drawPoint
     * since setupGC up there use -1.
     */
    if ((width < 2) || (height < 2)) {
       gc->drawRect(x, y, width, height);
       return;
    } else if ((arcAngle <= -360 ) || (arcAngle >= 360)) {
        // Share level quanteen arcAngle is > 0 ONLY IF
        // PLATFORM_SUPPORT_CCW_ARC_ONLY is on
        gc->drawEllipse(x, y, width, height);
    } else { 
        gc->drawPie(x, y, width, height, 
                (startAngle*16), (arcAngle*16));
    }
}

/**
 * Return the pixel value
 * for the SL-5000 using QT the colors are left intact
 * and QT later maps them to the display.
 */
extern "C"
int gxpport_get_pixel(int rgb, int gray, int isGray) {
  (void)gray; /* Suppress unused parameter warning */
  (void)isGray; /* Suppress unused parameter warning */

  return rgb;
}
