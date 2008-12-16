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

/**
 * @file
 * @ingroup highui_lfpport
 *
 * @brief Font-specific porting functions and data structures.
 */
#include <midpMalloc.h>
#include <lfpport_error.h>
#include <lfpport_font.h>
#include "lfpport_gtk.h"

#include <pango/pango.h>
#include <glib.h>

#define SMALL_FONT_SIZE  (7 * PANGO_SCALE)
#define MEDIUM_FONT_SIZE (9 * PANGO_SCALE)
#define LARGE_FONT_SIZE  (11 * PANGO_SCALE)

typedef struct {
    int face;
    int style;
    int size;
    PlatformFontPtr fontPtr;
} MidpFont;


GList *fonts_list = NULL;

#ifdef __cplusplus
extern "C" {
#endif

/* IMPL_NOTE: see
 * http://library.gnome.org/devel/pango/stable/pango-Text-Attributes.html#PangoAttrList
 */

/**
 * Gets the font type. The bit values of the font attributes are
 * defined in the <i>MIDP Specification</i>.
 * When this function returns successfully, *fontPtr will point to the
 * platform font.
 *
 * @param fontPtr pointer to the font's PlatformFontPtr structure.
 * @param face typeface of the font (not a particular typeface,
 *        but a typeface class, such as <tt>MONOSPACE</i>).
 * @param style any angle, weight, or underlining for the font.
 * @param size relative size of the font (not a particular size,
 *        but a general size, such as <tt>LARGE</tt>).
 *
 * @return an indication of success or the reason for failure
 */
MidpError lfpport_get_font(PlatformFontPtr* fontPtr,
			   int face, int style, int size){
    MidpFont *f;
    PangoFontDescription *font_descr;
    PangoStyle pango_style;
    gint font_size;
    GList *tmp;
    PangoWeight pango_weight;

    LIMO_TRACE(">>>%s face=%d style=%d size=%d\n", __FUNCTION__, face, style, size);

    // look through allocated font to see if the font is there already
    for (tmp = g_list_first(fonts_list); tmp != 0; tmp = g_list_next(fonts_list)) {
        if (((MidpFont*)tmp->data)->face == face &&
            ((MidpFont*)tmp->data)->style == style &&
            ((MidpFont*)tmp->data)->size == size) {
            *fontPtr = ((MidpFont*)tmp->data)->fontPtr;
            LIMO_TRACE("<<<%s found!\n", __FUNCTION__);
            return KNI_OK;
        }
    }

    f = (MidpFont *)midpMalloc(sizeof(MidpFont));
    font_descr = pango_font_description_new();

    if (f != NULL && font_descr != NULL) {
        f->size = size;
        f->style = style;
        f->face = face;

        /* set size */
        if (size == SIZE_SMALL) {
            font_size = SMALL_FONT_SIZE;
        } else if (size == SIZE_MEDIUM) {
            font_size = MEDIUM_FONT_SIZE;
        } else {
            font_size = LARGE_FONT_SIZE;
        }
        pango_font_description_set_size(font_descr, font_size);

        /* set style */
        pango_style = PANGO_STYLE_NORMAL;       /* STYLE_PLAIN */
        if (style & STYLE_ITALIC) {             /* STYLE_ITALIC */
            pango_style = PANGO_STYLE_ITALIC;
        }
        pango_font_description_set_style(font_descr, pango_style);
        pango_weight = PANGO_WEIGHT_NORMAL;     /* STYLE_BOLD */
        if (style & STYLE_BOLD) {
            pango_weight = PANGO_WEIGHT_BOLD;
        }
        pango_font_description_set_weight(font_descr, pango_weight);


         /* TODO:  set face */
//         if (face == FACE_SYSTEM) {
//             pango_font_description_set_family(font_descr, );
//        }
//         else if (face == FACE_MONOSPACE) {
//         }
//         else {  /* FACE_PROPORTIONAL */
//         }

        /* TODO:  set underline */
//       if ((style & STYLE_UNDERLINED) != 0) {
//       }


        /* insert font to the list */
        f->fontPtr = font_descr;
        fonts_list = g_list_prepend(fonts_list, f);

        /* return the font */
        *fontPtr = font_descr;
        LIMO_TRACE("<<<%s\n", __FUNCTION__);
        return KNI_OK;
    }

    LIMO_TRACE("<<<%s Returning error\n", __FUNCTION__);
    return -1;
}

/**
 * Frees native resources used by the system for font registry
 */
void lfpport_font_finalize(){
    GList *tmp;
    LIMO_TRACE(">>>%s\n", __FUNCTION__);

    /* free the allocated memory */
    for (tmp = g_list_first(fonts_list); tmp != 0; tmp = g_list_next(fonts_list)) {
        midpFree(tmp->data);
    }

    LIMO_TRACE("<<<%s\n", __FUNCTION__);
}

#ifdef __cplusplus
} /* extern C */
#endif
