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

#include <qfont.h>
#include <qfontmetrics.h>
#include <qteapp_export.h>
#include <midp_logging.h>

#include <gxapi_constants.h>
#include <gxpport_graphics.h>
#include <gxpport_font.h>
#include <gxpportqt_image.h>
#include <font_constants.h>


/**
 * Locate the QT Font that matches the font parameters
 * for face, style, and size.
 *
 * Sets qfont and qfontInfo accordingly.
 */
static QFont qfont;
static QFontMetrics* qfontInfo; // = NULL;

static int last_style = -1;
static int last_face  = -1;
static int last_size  = -1;

static QFont
find_font(int face, int style, int size) {
    bool changed = false;

    if (last_face != face) { 
      if (face == FACE_MONOSPACE) {
        qfont.setStyleHint(QFont::TypeWriter);
        qfont.setFamily("fixed");
      } else {
        qfont.setStyleHint(QFont::Helvetica);
        qfont.setFamily("helvetica");
      }

      last_face = face;
      changed = true;
    }

    if (last_size != size) {
	int pointsize;

	switch (size) {
	default:
	case OEM_FONT_SIZE_SMALL:
	    pointsize = face == FACE_MONOSPACE ? 7: 11;
	    break;
	case OEM_FONT_SIZE_MEDIUM:
	    pointsize = 13;
	    break;
	case OEM_FONT_SIZE_LARGE:
	    pointsize = 17;
	    break;
	}

	qfont.setPointSize(pointsize);
	last_size = size;
	changed = true;
    }

    if (last_style != style) {
        qfont.setWeight((style & STYLE_BOLD) ? QFont::Bold : QFont::Normal);
	qfont.setItalic(style & STYLE_ITALIC);
	qfont.setUnderline(style & STYLE_UNDERLINED);
	last_style = style;
	changed = true;
    }

    if (changed) {
	if (qfontInfo) {
	    delete qfontInfo;
	    qfontInfo = NULL;
	}
	qfontInfo = new QFontMetrics(qfont);
    }

    return qfont;
}


/**
 * Make a QString from an array of unicode chars.
 */
static QString
make_string(const jchar *charArray, int len) {
    QChar chars[len];
    int i;

    for (i = 0; i < len; i++) {
	chars[i] = QChar(charArray[i]);
    }
    return QString(chars, len);
}


/**
 * Draw the first n characters in charArray, with the anchor point of the
 * entire (sub)string located at x, y.
 */
extern "C" void
gxpport_draw_chars(
  jint pixel, const jshort *clip, 
  gxpport_mutableimage_native_handle dst,
  int dotted,
  int face, int style, int size,
  int x, int y, int anchor, 
  const jchar *charArray, int n) {
    QPixmap* qpixmap = gxpportqt_get_mutableimage_pixmap(dst);
    QString s(make_string(charArray, n));

    REPORT_INFO4(LC_LOWUI, "gxpport_draw_chars(%d, %d, %x, [chars...], %d)", 
		 x, y, anchor, n); 

    find_font(face, style, size);

    switch (anchor & (LEFT | RIGHT | HCENTER)) {
    case LEFT:
        break;

    case RIGHT:
        x -= qfontInfo->width(s);
        break;

    case HCENTER:
        x -= (qfontInfo->width(s) >> 1);
        break;
    }

    switch (anchor & (TOP | BOTTOM | BASELINE)) {
    case BOTTOM:
      /* 1 pixel has to be added to account for baseline in Qt */
        y -= qfontInfo->descent()+1;

    case BASELINE:
        break;

    case TOP:
    default:
        y += qfontInfo->ascent();
        break;
    }

    MScreen * mscreen = qteapp_get_mscreen();
    QPainter *gc = mscreen->setupGC(pixel, -1, clip, (QPaintDevice*)qpixmap, 
				    dotted);
    gc->setFont(find_font(face, style, size));
    gc->drawText(x, y, s, n);
}

/*
 * Get the ascent, descent and leading info for the font indicated 
 * by face, style, size.
 */
extern "C" void
gxpport_get_fontinfo(int face, int style, int size, 
		     int *ascent, int *descent, int *leading) {

    find_font(face, style, size);

    *ascent  = qfontInfo->ascent();
    /* 1 pixel has to be added to account for baseline in Qt */
    *descent = qfontInfo->descent() + 1;
    *leading = 1;

    REPORT_INFO6(LC_LOWUI, "gxpport_get_fontinfo(%d, %d, %d) = %d, %d, %d\n", 
		 face, style, size, *ascent, *descent, *leading);
}

/*
 * Get the advance width for the first n characters in charArray if
 * they were to be drawn in the font indicated by face, style, size.
 */
extern "C" int
gxpport_get_charswidth(int face, int style, int size, 
		       const jchar *charArray, int n) {

    find_font(face, style, size);
    
    return qfontInfo->width(make_string(charArray, n)); 
}
