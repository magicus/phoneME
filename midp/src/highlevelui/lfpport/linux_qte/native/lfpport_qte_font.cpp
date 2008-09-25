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

#include <lfpport_font.h>
#include <lfpport_error.h>
#include <midpMalloc.h>

#include <qfont.h>
#include <qlist.h>

typedef struct {
  int face;
  int style;
  int size;
  PlatformFontPtr fontPtr;
} MidpFont;

QList<MidpFont> fonts;

/**
 * Get Font type. Bits definition of each attribute is defined in MIDP spec.
 * Upon successful return, *fontPtr should point to the platform font.
 * Param face, style and size are defined as MIDP Spec.
 * This function is to be ported.
 */
extern "C"
MidpError lfpport_get_font(PlatformFontPtr* fontPtr, 
			   int face, int style, int size) {

    // look through allocated font to see if the font is there already
    MidpFont *f;
    for ( f = fonts.first(); f != 0; f=fonts.next() ) {
      if (f->face == face && f->style == style && f->size == size) {
	*fontPtr = f->fontPtr;    
	return KNI_OK;
      }
    }

    f = (MidpFont *)midpMalloc(sizeof(MidpFont));
    if (f != NULL) {

      f->size = size;
      f->style = style;
      f->face = face;

      int fontSize = 13; // for SIZE_MEDIUM
      
      if (size == SIZE_SMALL) {
	fontSize = face == FACE_MONOSPACE ? 7: 11;
      } else if (size == SIZE_LARGE) {
	fontSize = 17;
      }

      
      QFont *qFont = new QFont((face == FACE_MONOSPACE ? 
				"fixed" : "helvetica"),
			       fontSize, 
			       (style & STYLE_BOLD) == 0 ? QFont::Normal : 
			       QFont::Bold,
			       (style & STYLE_ITALIC) == 0 ? false : true);
      
      if (face == FACE_MONOSPACE) {
        qFont->setStyleHint(QFont::TypeWriter);
      } else {
        qFont->setStyleHint(QFont::Helvetica);
      }

      if ((style & STYLE_UNDERLINED) != 0) {
	qFont->setUnderline(true);
      }
      
      *fontPtr = f->fontPtr = qFont;
      
      fonts.append(f);

      return KNI_OK;
    }

    return KNI_ENOMEM;
  }

/**
 * Frees native resources used by the system for font registry
 */
extern "C"
void lfpport_font_finalize() {
    MidpFont *f;
    while((f = (MidpFont *)fonts.first()) != NULL) {
      delete ((QFont *)f->fontPtr);
      fonts.remove();
      midpFree(f);
    }
}
